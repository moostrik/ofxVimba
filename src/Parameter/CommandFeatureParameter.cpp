#include "CommandFeatureParameter.h"

using namespace ofxVimba;

CommandFeatureParameter::CommandFeatureParameter(const std::string& name)
    : FeatureParameter(name) {
  parameter = std::make_shared<ofParameter<bool>>();
  parameter->set(name, false);
  parameter->addListener(this, &CommandFeatureParameter::run);

  setAbstractParameter(parameter);
}

void CommandFeatureParameter::pull() {
  bool isDone = true;
  auto error = feature->IsCommandDone(isDone);

  if (error != VmbErrorSuccess) {
    logger.verbose("Failed to get run status of command", error);
  }

  parameter->set(!isDone);
}

void CommandFeatureParameter::run(bool& commandValue) {
  if (!commandValue) return;

  auto error = feature->RunCommand();
  if (error != VmbErrorSuccess) {
    logger.warning("Failed to run command", error);
    commandValue = false;
  } else {
    pull();
  }
}
