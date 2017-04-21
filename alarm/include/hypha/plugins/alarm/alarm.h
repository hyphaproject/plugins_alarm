// Copyright (c) 2015-2017 Hypha

#pragma once

#include <map>
#include <string>

#include <hypha/plugin/hyphahandler.h>

namespace hypha {
namespace plugin {
namespace alarm {
class Alarm : public HyphaHandler {
 public:
  explicit Alarm();
  ~Alarm();
  static Alarm *instance();
  void doWork() override;
  const std::string name() override { return "alarm"; }
  const std::string getTitle() override { return "Alarm"; }
  const std::string getVersion() override { return "0.1"; }
  const std::string getDescription() override {
    return "Handler for Alarm Control.";
  }
  const std::string getConfigDescription() override {
    return "{"
           "\"confdesc\":["
           "{\"name\":\"savedir\", "
           "\"type\":\"string\",\"value\":\"\",\"description\":\"Save Path:\"}"
           "]}";
  }
  void loadConfig(std::string config) override;
  std::string getConfig() override;

  void parse(std::string message);
  HyphaBasePlugin *getInstance(std::string id) override;

  void receiveMessage(std::string message) override;
  std::string communicate(std::string message) override;

 protected:
  std::map<std::string, bool> alarm;
};
}
}
}
