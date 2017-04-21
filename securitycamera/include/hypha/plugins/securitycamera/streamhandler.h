// Copyright (c) 2015-2017 Hypha

#ifndef STREAMHANDLER_H
#define STREAMHANDLER_H

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace hypha {
namespace plugin {
namespace securitycamera {
class SecurityCamera;
class StreamHandler : public Poco::Net::HTTPRequestHandler {
 public:
  void handleRequest(Poco::Net::HTTPServerRequest &request,
                     Poco::Net::HTTPServerResponse &response);
  void setVideo(SecurityCamera *securitycamera);

 protected:
  SecurityCamera *securitycamera;
};
}
}
}
#endif  // STREAMHANDLER_H
