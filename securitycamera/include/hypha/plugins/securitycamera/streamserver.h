// Copyright (c) 2015-2017 Hypha

#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#include <Poco/Net/HTTPServer.h>
namespace hypha {
namespace plugin {
namespace securitycamera {
class SecurityCamera;
class StreamServer {
 public:
  StreamServer();
  ~StreamServer();

  void start();
  void stop();
  void setPort(int port);
  void setVideo(SecurityCamera *securitycamera);

 protected:
  Poco::Net::HTTPServer *srv = nullptr;
  int port;
  SecurityCamera *securitycamera = nullptr;
};
}
}
}
#endif  // STREAMSERVER_H
