// Copyright (c) 2015-2017 Hypha

#ifndef ROOTHANDLER_H
#define ROOTHANDLER_H

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

namespace hypha {
namespace plugin {
namespace securitycamera {

class RootHandler : public Poco::Net::HTTPRequestHandler {
 public:
  void handleRequest(Poco::Net::HTTPServerRequest &request,
                     Poco::Net::HTTPServerResponse &response);
};
}
}
}

#endif  // ROOTHANDLER_H
