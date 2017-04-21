// Copyright (c) 2015-2017 Hypha

#ifndef SECURITYCAMERA_H
#define SECURITYCAMERA_H

#include <mutex>
#include <string>

#include <hypha/plugin/hyphahandler.h>
#include <hypha/plugin/hyphasensor.h>
#include <opencv2/opencv.hpp>

namespace hypha {
namespace plugin {
namespace securitycamera {
class StreamServer;
class SecurityCamera : public HyphaHandler, public HyphaSensor {
#define MAXCAMERAS 4

 public:
  void doWork() override;
  void setup() override;
  std::string communicate(std::string message) override;
  const std::string name() override { return "securitycamera"; }
  const std::string getTitle() override { return "Security Camera"; }
  const std::string getVersion() override { return "0.1"; }
  const std::string getDescription() override {
    return "Plugin to capture videos from camera.";
  }
  const std::string getConfigDescription() override {
    return "{"
           "\"confdesc\":["
           "{\"name\":\"fps\", "
           "\"type\":\"integer\",\"value\":\"10\",\"description\":\"frames per "
           "second\"}, "
           "{\"name\":\"width\", "
           "\"type\":\"integer\",\"value\":\"640\",\"description\":\"camera "
           "capture width\"}, "
           "{\"name\":\"height\", "
           "\"type\":\"integer\",\"value\":\"480\",\"description\":\"camera "
           "capture height\"}, "
           "{\"name\":\"port\", "
           "\"type\":\"integer\",\"value\":\"8080\",\"description\":\"port for "
           "camera web video\"}, "
           "{\"name\":\"findpersons\", "
           "\"type\":\"boolean\",\"value\":\"false\",\"description\":\"give "
           "alarm when person found in video\"}, "
           "{\"name\":\"user\", "
           "\"type\":\"string\",\"value\":\"\",\"description\":\"user to login "
           "for camera stream\"}, "
           "{\"name\":\"password\", "
           "\"type\":\"string\",\"value\":\"\",\"description\":\"password to "
           "login for camera stream\"}"

           "]}";
  }
  void loadConfig(std::string json) override;
  std::string getConfig() override;
  HyphaBasePlugin *getInstance(std::string id) override;

  void receiveMessage(std::string message) override;

  void startVideoCapture();
  void stopVideoCapture();
  cv::Mat getCurrentImage();
  std::string getUser();
  std::string getPassword();

 protected:
  enum State { IDLE, FILM, RECORDING };

  int cameras = 0;
  int fps = 5;
  int width = 320;
  int height = 240;
  int port = 8080;
  bool doCapture = false;
  bool findPerson = false;

  std::mutex user_mutex;
  std::string user = "";
  std::mutex password_mutex;
  std::string password = "";

  int frameCounter = 0;
  int filmCounter = 0;

  std::string device[MAXCAMERAS];
  bool upsideDown[MAXCAMERAS];

  std::mutex capture_mutex[MAXCAMERAS];
  cv::VideoCapture capture[MAXCAMERAS];

  std::mutex videoWriter_mutex;
  cv::VideoWriter *videoWriter = 0;

  std::mutex hog_mutex[MAXCAMERAS];
  cv::HOGDescriptor hog[MAXCAMERAS];

  std::mutex captureMat_mutex[MAXCAMERAS];
  cv::Mat captureMat[MAXCAMERAS];

  std::mutex currentImage_mutex;
  cv::Mat currentImage;

  StreamServer *srv = 0;

  void captureCameras();
  void captureCamera(int camNumber);
  void findPersons();
  std::mutex findPerson_mutex[MAXCAMERAS];
  void findPersonInCamera(int camNumber);

  cv::Mat rotateMat(cv::Mat mat);
  cv::Mat fillMat(cv::Mat mat);

  void concatImages();
  void drawText();
  void writeVideo();

  State currentState = IDLE;
  State filmState = IDLE;
  std::mutex state_mutex;

  State getState();
  void setState(State state);

  State getFilmState();
  void setFilmState(State state);

  std::string fileName;
  std::mutex fileName_mutex;
  std::string getFileName();
  void setFileName(std::string filename);
};
}
}
}
#endif  // SECURITYCAMERA_H
