// Copyright (c) 2015-2017 Hypha

#include "hypha/plugins/securitycamera/requesthandlerfactory.h"

using namespace hypha::plugin::securitycamera;
RequestHandlerFactory::RequestHandlerFactory() {}

Poco::Net::HTTPRequestHandler *RequestHandlerFactory::createRequestHandler(
    const Poco::Net::HTTPServerRequest &request) {
  if (boost::starts_with(request.getURI(), "/stream/")) {
    StreamHandler *sh = new StreamHandler();
    sh->setVideo(securitycamera);
    return sh;
  } else
    return new RootHandler();
}

void RequestHandlerFactory::setVideo(SecurityCamera *securitycamera) {
  this->securitycamera = securitycamera;
}
