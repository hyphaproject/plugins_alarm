// Copyright (c) 2015-2017 Hypha

#include "hypha/plugins/securitycamera/securitycamera.h"
#include "hypha/plugins/securitycamera/streamserver.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <Poco/ClassLibrary.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/LocalDateTime.h>
#include <Poco/Timezone.h>

#include <hypha/utils/logger.h>

using namespace hypha::utils;
using namespace hypha::plugin;
using namespace hypha::plugin::securitycamera;

void SecurityCamera::doWork() {
  std::this_thread::sleep_for(std::chrono::seconds(1) / fps);
  try {
    std::thread captureT = std::thread([this] { captureCameras(); });
    if (findPerson) findPersons();
    std::thread concatDrawT = std::thread([this] {
      concatImages();
      drawText();
    });
    std::thread writeVideoT = std::thread([this] { writeVideo(); });
    if (captureT.joinable()) captureT.join();
    if (concatDrawT.joinable()) concatDrawT.join();
    if (writeVideoT.joinable()) writeVideoT.join();
  } catch (cv::Exception &e) {
    Logger::error(e.what());
  } catch (std::exception e) {
    Logger::error(e.what());
  } catch (...) {
    Logger::error("Something terrible happened.");
  }
}

void SecurityCamera::captureCameras() {
  if (getFilmState() == FILM) {
    if (filmCounter++ >= 100) setFilmState(IDLE);
    try {
      std::thread t[MAXCAMERAS];
      for (int i = 0; i < MAXCAMERAS; ++i) {
        t[i] = std::thread([this, i] { captureCamera(i); });
      }
      for (int i = 0; i < MAXCAMERAS; ++i) {
        t[i].join();
      }
    } catch (std::exception &e) {
      Logger::warning("Capture Cameras Threads");
      Logger::warning(e.what());
    }
  }
}

void SecurityCamera::captureCamera(int camNumber) {
  if (capture_mutex[camNumber].try_lock()) {
    try {
      cv::Mat capt;
      if (capture[camNumber].isOpened()) {
        capture[camNumber].read(capt);
      }
      if (captureMat_mutex[camNumber].try_lock()) {
        captureMat[camNumber] = capt.clone();
        captureMat_mutex[camNumber].unlock();
      }
      capture_mutex[camNumber].unlock();
    } catch (cv::Exception &e) {
      Logger::error("Could not capture from camera");
      Logger::error(e.what());
    } catch (std::exception &e) {
      Logger::warning("Could not capture from camera");
    }
  }
}

cv::Mat SecurityCamera::rotateMat(cv::Mat mat) {
  if (mat.rows < 1) return mat;
  cv::Point2f src_center(mat.cols / 2.0F, mat.rows / 2.0F);
  cv::Mat rot_mat = cv::getRotationMatrix2D(src_center, 180, 1.0);
  cv::warpAffine(mat, mat, rot_mat, mat.size());
  return mat;
}

cv::Mat SecurityCamera::fillMat(cv::Mat mat) {
  if (mat.rows != height)
    cv::vconcat(mat, cv::Mat(height - mat.rows, mat.cols, mat.type()), mat);
  if (mat.cols != width)
    cv::hconcat(mat, cv::Mat(mat.rows, width - mat.cols, mat.type()), mat);
  return mat;
}

void SecurityCamera::concatImages() {
  cv::Mat frame;
  try {
    cv::Mat cam[MAXCAMERAS];
    for (int i = 0; i < MAXCAMERAS; ++i) {
      captureMat_mutex[i].lock();
      cam[i] = captureMat[i].clone();
      captureMat_mutex[i].unlock();

      if (upsideDown[i]) cam[i] = rotateMat(cam[i]);
    }

    if (cameras == 1) {
        fillMat(cam[0]);
      frame = cam[0];
    } else if (cameras == 2) {
      try {
        fillMat(cam[0]);
        fillMat(cam[1]);
        // concat images
        cv::hconcat(cam[0], cam[1], frame);
      } catch (cv::Exception &e) {
        Logger::error(e.what());
      }
    } else if (cameras == 3) {
      fillMat(cam[0]);
      fillMat(cam[1]);
      fillMat(cam[2]);
      // concat images
      cv::hconcat(cam[0], cam[1], frame);
      cv::hconcat(frame, cam[2], frame);
    } else if (cameras == 4) {
      fillMat(cam[0]);
      fillMat(cam[1]);
      fillMat(cam[2]);
      fillMat(cam[3]);
      // concat images
      cv::hconcat(cam[0], cam[1], frame);
      cv::hconcat(frame, cam[2], frame);
      cv::hconcat(frame, cam[3], frame);
    }
  } catch (std::exception e) {
    Logger::warning("Error in Image Concatenation: ");
  }
  currentImage_mutex.lock();
  currentImage = frame;
  currentImage_mutex.unlock();
}

void SecurityCamera::drawText() {
  try {
    cv::Mat frame;
    currentImage_mutex.lock();
    frame = currentImage.clone();
    currentImage_mutex.unlock();
    Poco::LocalDateTime now;
    std::string time(Poco::DateTimeFormatter::format(now, "%Y-%b-%d %H:%M:%S"));
    time += " " + Poco::Timezone::name();
    cv::putText(frame, id + " " + time, cv::Point(30, 30),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(20, 20, 250), 1,
                CV_AA);
    currentImage_mutex.lock();
    currentImage = frame.clone();
    currentImage_mutex.unlock();
  } catch (cv::Exception &e) {
    Logger::warning("Error in Draw Text: ");
    Logger::warning(e.what());
  } catch (std::exception e) {
    Logger::warning("Error in Draw Text: ");
    Logger::warning(e.what());
  }
}

void SecurityCamera::writeVideo() {
  switch (getState()) {
    case IDLE:
      if (videoWriter) stopVideoCapture();
      break;
    case RECORDING:
      if (!videoWriter) {
        startVideoCapture();
      }
      if (videoWriter_mutex.try_lock()) {
        try {
          if (videoWriter && videoWriter->isOpened()) {
            videoWriter->write(getCurrentImage());
          }

          if (frameCounter++ == fps * 2 /* seconds */) {
            cv::imwrite(getFileName() + ".jpg", getCurrentImage());
          }
        } catch (cv::Exception &e) {
          Logger::warning("Image write exception: ");
          Logger::warning(e.what());
          videoWriter_mutex.unlock();
        } catch (std::exception e) {
          Logger::warning("Image write exception: ");
          Logger::warning(e.what());
          videoWriter_mutex.unlock();
        }
        videoWriter_mutex.unlock();
      }
      break;
    default:
      break;
  };
}

void SecurityCamera::findPersons() {
  try {
    std::thread t[MAXCAMERAS];
    for (int i = 0; i < MAXCAMERAS; ++i) {
      t[i] = std::thread([this, i] { findPersonInCamera(i); });
    }
    for (int i = 0; i < MAXCAMERAS; ++i) {
      t[i].join();
    }
  } catch (std::exception e) {
    Logger::warning("Find Person exception: ");
    Logger::warning(e.what());
  }
}

void SecurityCamera::findPersonInCamera(int camNumber) {
  if (findPerson_mutex[camNumber].try_lock()) {
    try {
      captureMat_mutex[camNumber].lock();
      if (!captureMat[camNumber].data) {
        captureMat_mutex[camNumber].unlock();
        findPerson_mutex[camNumber].unlock();
        return;
      }
      captureMat_mutex[camNumber].unlock();
      if (hog_mutex[camNumber].try_lock()) {
        captureMat_mutex[camNumber].lock();
        cv::Mat image = captureMat[camNumber].clone();
        captureMat_mutex[camNumber].unlock();

        std::vector<cv::Rect> found, found_filtered;
        hog[camNumber].detectMultiScale(image, found, 0, cv::Size(8, 8),
                                        cv::Size(32, 32), 1.05, 2);

        size_t i, j;
        for (i = 0; i < found.size(); i++) {
          cv::Rect r = found[i];
          for (j = 0; j < found.size(); j++)
            if (j != i && (r & found[j]) == r) break;
          if (j == found.size()) found_filtered.push_back(r);
        }
        for (i = 0; i < found_filtered.size(); i++) {
          cv::Rect r = found_filtered[i];
          r.x += std::round(r.width * 0.1);
          r.width = std::round(r.width * 0.8);
          r.y += std::round(r.height * 0.06);
          r.height = std::round(r.height * 0.9);
          cv::rectangle(image, r.tl(), r.br(), cv::Scalar(20, 250, 10), 2);
        }
        if (found_filtered.size() > 0) {
          captureMat_mutex[camNumber].lock();
          captureMat[camNumber] = image.clone();
          captureMat_mutex[camNumber].unlock();

          boost::property_tree::ptree pt;
          pt.put("alarm", true);
          pt.put("id", this->id);
          pt.put("type", name());
          pt.put("movement", true);
          pt.put("value", true);
          std::stringstream ss;
          boost::property_tree::write_json(ss, pt);
          sendMessage(ss.str());
        } else {
          boost::property_tree::ptree pt;
          pt.put("alarm", true);
          pt.put("id", this->id);
          pt.put("type", name());
          pt.put("movement", false);
          pt.put("value", false);
          std::stringstream ss;
          boost::property_tree::write_json(ss, pt);
          sendMessage(ss.str());
        }
        hog_mutex[camNumber].unlock();
      }
    } catch (cv::Exception &e) {
      Logger::warning("Error in Find Person: ");
      Logger::warning(e.what());
      hog_mutex[camNumber].unlock();
    } catch (std::exception e) {
      Logger::warning("Error in Find Person: ");
      Logger::warning(e.what());
      hog_mutex[camNumber].unlock();
    }
    findPerson_mutex[camNumber].unlock();
  }
}

void SecurityCamera::setup() {
  for (int i = 0; i < MAXCAMERAS; ++i) {
    if (this->findPerson)
      hog[i].setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
    if (!device[i].empty()) {
      capture[i].open(std::stoi(device[i]));
      capture[i].set(CV_CAP_PROP_FRAME_WIDTH, width);
      capture[i].set(CV_CAP_PROP_FRAME_HEIGHT, height);
      capture[i].set(CV_CAP_PROP_FPS, fps);
      captureMat[i] = cv::Mat(height, width, CV_8UC3);
      cameras++;
    }
  }
  filmCounter = 0;
  filmState = FILM;
  srv = new StreamServer();
  srv->setVideo(this);
  srv->setPort(port);
  srv->start();
}

std::string SecurityCamera::communicate(std::string UNUSED(message)) {
  return "SUCCESS";
}

void SecurityCamera::loadConfig(std::string json) {
  boost::property_tree::ptree pt;
  std::stringstream ss(json);
  boost::property_tree::read_json(ss, pt);
  if (pt.get_optional<int>("fps")) {
    fps = pt.get<int>("fps");
  }
  if (pt.get_optional<int>("width")) {
    width = pt.get<int>("width");
  }
  if (pt.get_optional<int>("height")) {
    height = pt.get<int>("height");
  }
  if (pt.get_optional<int>("port")) {
    port = pt.get<int>("port");
  }
  if (pt.get_optional<bool>("findpersons")) {
    findPerson = pt.get<bool>("findpersons");
  }
  if (pt.get_optional<std::string>("user")) {
    user = pt.get<std::string>("user");
  }
  if (pt.get_optional<std::string>("password")) {
    password = pt.get<std::string>("password");
  }

  if (pt.get_child_optional("devices")) {
    int i = 0;
    BOOST_FOREACH (const boost::property_tree::ptree::value_type &child,
                   pt.get_child("devices")) {
      device[i] = child.second.get<std::string>("device", "");
      upsideDown[i] = child.second.get<bool>("upsidedown", false);
      i++;
    }
  } else {
    device[0] = "0";
    upsideDown[0] = false;
  }
}

std::string SecurityCamera::getConfig() {
  boost::property_tree::ptree config;
  config.put("fps", this->fps);
  config.put("width", this->width);
  config.put("height", this->height);
  config.put("port", this->port);
  config.put("findpersons", this->findPerson);
  config.put("user", this->user);
  config.put("password", this->password);
  std::stringstream sstream;
  boost::property_tree::write_json(sstream, config);
  return sstream.str();
}

HyphaBasePlugin *SecurityCamera::getInstance(std::string id) {
  SecurityCamera *instance = new SecurityCamera();
  instance->setId(id);
  return instance;
}

void SecurityCamera::receiveMessage(std::string message) {
  try {
    boost::property_tree::ptree pt;
    std::stringstream ss(message);
    boost::property_tree::read_json(ss, pt);
    if (pt.get_optional<bool>("videocapture")) {
      if (pt.get<bool>("videocapture")) {
        setFileName(pt.get<std::string>("savedir"));
        setState(RECORDING);
      } else {
        setState(IDLE);
      }
    }
  } catch (std::exception &e) {
    Logger::error(e.what());
  }
}

void SecurityCamera::startVideoCapture() {
  try {
    std::string filename = getFileName();
    videoWriter_mutex.lock();
    videoWriter = new cv::VideoWriter();
    cv::Mat frame;
    frame = getCurrentImage();
    // WMV2
    if (!videoWriter->open(filename + ".avi", CV_FOURCC('M', 'P', '4', '2'),
                           fps, cv::Size(frame.cols, frame.rows))) {
      // if(!videoWriter->open(filename+".wmv", CV_FOURCC('W','M','V','2'), fps,
      // cv::Size(frame.cols, frame.rows))){
      Logger::warning("VideoWriter could not been opened! Maybe wrong codec");
    }
    fileName = filename;
    frameCounter = 0;
    if (!videoWriter->isOpened()) {
      videoWriter_mutex.unlock();
      throw "Could not write to video file";
    }
    videoWriter_mutex.unlock();
  } catch (std::exception &e) {
    Logger::error(e.what());
    Logger::warning("Could not start video writing");
    if (videoWriter) videoWriter->release();
    videoWriter = 0;
    videoWriter_mutex.unlock();
  }
}

void SecurityCamera::stopVideoCapture() {
  videoWriter_mutex.lock();
  if (videoWriter) {
    Logger::info("stop video capture");
    videoWriter->release();
    delete videoWriter;
    videoWriter = 0;
  }
  videoWriter_mutex.unlock();
}

cv::Mat SecurityCamera::getCurrentImage() {
  setFilmState(FILM);
  cv::Mat frame;
  currentImage_mutex.lock();
  frame = currentImage.clone();
  currentImage_mutex.unlock();
  return frame;
}

std::string SecurityCamera::getUser() {
  std::string returnStr;
  user_mutex.lock();
  returnStr = user.c_str();
  user_mutex.unlock();
  return returnStr;
}

std::string SecurityCamera::getPassword() {
  std::string returnStr;
  password_mutex.lock();
  returnStr = password.c_str();
  password_mutex.unlock();
  return returnStr;
}

SecurityCamera::State SecurityCamera::getState() {
  State state = IDLE;
  state_mutex.lock();
  state = currentState;
  state_mutex.unlock();
  return state;
}

void SecurityCamera::setState(State state) {
  state_mutex.lock();
  currentState = state;
  state_mutex.unlock();
}

SecurityCamera::State SecurityCamera::getFilmState() {
  State state = IDLE;
  state_mutex.lock();
  state = filmState;
  state_mutex.unlock();
  return state;
}

void SecurityCamera::setFilmState(State state) {
  state_mutex.lock();
  if (filmState == IDLE && state == FILM) {
    filmState = FILM;
    cameras = 0;
    for (int i = 0; i < MAXCAMERAS; ++i) {
      if (!device[i].empty()) {
        capture[i].open(std::stoi(device[i]));
        capture[i].set(CV_CAP_PROP_FRAME_WIDTH, width);
        capture[i].set(CV_CAP_PROP_FRAME_HEIGHT, height);
        capture[i].set(CV_CAP_PROP_FPS, fps);
        cameras++;
      }
    }
  } else if (filmState == FILM && state == IDLE) {
    filmState = IDLE;
    for (int i = 0; i < MAXCAMERAS; ++i) {
      if (!device[i].empty()) {
        capture[i].release();
      }
      cameras = 0;
    }
  }
  filmCounter = 0;
  state_mutex.unlock();
}

std::string SecurityCamera::getFileName() {
  std::string filename;
  fileName_mutex.lock();
  filename = fileName;
  fileName_mutex.unlock();
  return filename;
}

void SecurityCamera::setFileName(std::string filename) {
  if (!filename.empty()) filename = filename + "/";
  Poco::DateTime now;
  std::string time(Poco::DateTimeFormatter::format(now, "%Y%m%d"));

  filename = filename + time + "/";
  boost::filesystem::path rootPath(filename);
  boost::system::error_code returnedError;
  boost::filesystem::create_directories(rootPath, returnedError);

  time = Poco::DateTimeFormatter::format(now, "%Y%m%d_%H%M%S");
  filename = filename + "capture_" + time;
  fileName_mutex.lock();
  fileName = filename;
  fileName_mutex.unlock();
}

PLUGIN_API POCO_BEGIN_MANIFEST(HyphaBasePlugin)
    POCO_EXPORT_CLASS(SecurityCamera) POCO_END_MANIFEST
