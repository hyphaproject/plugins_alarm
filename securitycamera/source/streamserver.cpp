// Copyright (c) 2015-2017 Hypha

#include "hypha/plugins/securitycamera/streamserver.h"
#include "hypha/plugins/securitycamera/requesthandlerfactory.h"

using namespace hypha::plugin::securitycamera;
StreamServer::StreamServer() {}

StreamServer::~StreamServer() {
  stop();
  delete srv;
}

void StreamServer::start() {
  if (!srv) {
    Poco::UInt16 port = this->port;
    Poco::Net::HTTPServerParams *params = new Poco::Net::HTTPServerParams;
    params->setMaxQueued(10);
    params->setMaxThreads(4);
    Poco::Net::ServerSocket socket(port);
    RequestHandlerFactory *factory = new RequestHandlerFactory();
    factory->setVideo(securitycamera);
    srv = new Poco::Net::HTTPServer(factory, socket, params);
  }
  srv->start();
}

void StreamServer::stop() { srv->stop(); }

void StreamServer::setPort(int port) { this->port = port; }

void StreamServer::setVideo(SecurityCamera *securitycamera) {
  this->securitycamera = securitycamera;
}
