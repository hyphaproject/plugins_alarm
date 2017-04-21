// Copyright (c) 2015-2017 Hypha

#include "hypha/plugins/securitycamera/streamhandler.h"
#include "hypha/plugins/securitycamera/securitycamera.h"

#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include <Poco/Net/HTTPBasicCredentials.h>
#include <Poco/Net/MessageHeader.h>
#include <Poco/Net/MultipartWriter.h>

using namespace hypha::plugin::securitycamera;
void StreamHandler::handleRequest(Poco::Net::HTTPServerRequest &request,
                                  Poco::Net::HTTPServerResponse &response) {
  if (!securitycamera->getUser().empty()) {
    if (!request.hasCredentials()) {
      response.requireAuthentication(securitycamera->getId());
      response.setContentLength(0);
      response.send();
      return;
    } else {
      Poco::Net::HTTPBasicCredentials cred(request);
      if (securitycamera->getUser() != cred.getUsername() ||
          securitycamera->getPassword() != cred.getPassword()) {
        response.requireAuthentication(securitycamera->getId());
        response.setContentLength(0);
        response.send();
        return;
      }
    }
  }

  std::string boundary = "videostream";
  response.setChunkedTransferEncoding(false);
  response.set("Max-Age", "0");
  response.set("Expires", "0");
  response.set("Pragma", "no-cache");
  response.set("Cache-Control", "no-cache");
  response.setContentType("multipart/x-mixed-replace;boundary=--" + boundary);
  std::ostream &ostr = response.send();
  while (ostr.good()) {
    Poco::Net::MultipartWriter writer(ostr, boundary);
    Poco::Net::MessageHeader header;
    header.set("Content-Type", "image/jpeg");
    writer.nextPart(header);
    std::vector<uchar> buffer;
    cv::imencode(".jpg", securitycamera->getCurrentImage(), buffer);
    header.set("Content-Length", std::to_string(buffer.size()));
    ostr.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void StreamHandler::setVideo(SecurityCamera *securitycamera) {
  this->securitycamera = securitycamera;
}
