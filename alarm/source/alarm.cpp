// Copyright (c) 2015-2017 Hypha

#include "hypha/plugins//alarm/alarm.h"

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <Poco/ClassLibrary.h>
#include <hypha/core/database/database.h>
#include <hypha/core/database/userdatabase.h>
#include <hypha/plugin/hyphabaseplugin.h>
#include <hypha/plugin/pluginloader.h>

using namespace hypha::plugin;
using namespace hypha::plugin::alarm;
using namespace hypha::settings;
using namespace hypha::database;

Alarm::Alarm() {}

Alarm::~Alarm() {}

void Alarm::doWork() { std::this_thread::sleep_for(std::chrono::seconds(1)); }

void Alarm::parse(std::string UNUSED(message)) {}

void Alarm::loadConfig(std::string UNUSED(config)) {}

std::string Alarm::getConfig() { return "{}"; }

HyphaBasePlugin *Alarm::getInstance(std::string id) {
  Alarm *instance = new Alarm();
  instance->setId(id);
  return instance;
}

void Alarm::receiveMessage(std::string message) {
  boost::property_tree::ptree ptjson;
  std::stringstream ssjson(message);
  boost::property_tree::read_json(ssjson, ptjson);

  boost::property_tree::ptree sendobject;

  if (ptjson.get_optional<bool>("movement")) {
    std::string mail = "movement";
    sendobject.put("mail", mail);
  }

  if (ptjson.get_optional<bool>("alarm")) {
    std::string id = ptjson.get<std::string>("id");
    bool isAlarm = ptjson.get<bool>("alarm");
    if (alarm.find(id) == alarm.end()) {
      alarm[id] = isAlarm;
    } else if (alarm[id] != isAlarm) {
      alarm[id] = isAlarm;
      std::string value;
      if (ptjson.get_optional<bool>("value")) {
        value = ptjson.get<bool>("value") ? "true" : "false";
      } else if (ptjson.get_optional<double>("value")) {
        std::stringstream tostring;
        tostring << ptjson.get<double>("value");
        value = tostring.str();
      } else {
        value = ptjson.get<std::string>("value");
      }
      sendobject.put("mail", ptjson.get<std::string>("id") + "(" +
                                 ptjson.get<std::string>("type") + ") " +
                                 value);

      std::stringstream ssso;
      boost::property_tree::write_json(ssso, sendobject);
      sendMessage(ssso.str());
    }
  }
}

std::string Alarm::communicate(std::string UNUSED(message)) { return ""; }

PLUGIN_API POCO_BEGIN_MANIFEST(HyphaBasePlugin)
    POCO_EXPORT_CLASS(Alarm) POCO_END_MANIFEST
