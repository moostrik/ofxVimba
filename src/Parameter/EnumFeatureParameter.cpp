#include "EnumFeatureParameter.h"

using namespace ofxVimba;

EnumFeatureParameter::EnumFeatureParameter(const std::string& name)
    : FeatureParameter(name), isUpdatingSelection(false) {
  group = std::make_shared<ofParameterGroup>(name);
  setAbstractParameter(group);
}

void EnumFeatureParameter::setup() {
  std::vector<std::string> options;
  std::string currentValue;

  group->clear();

  if (device->get(feature, currentValue) &&
      device->getOptions(feature, options)) {
    for (auto& option : options) {
      ofParameter<bool> button(option, option == currentValue);

      if (device->isOptionAvailable(feature, option)) {
        button.addListener(this, &EnumFeatureParameter::onParameterChange);
      } else {
        button.addListener(this, &EnumFeatureParameter::onParameterUnavailable);
      }

      group->add(button);
    }
  } else {
    logger.verbose("Failed to setup enum feature");
  }
}

// Push the selected value to the device
void EnumFeatureParameter::push() {
  if (!group || !isWritable()) return;

  for (auto& param : *group) {
    auto& option = param->cast<bool>();
    if (!option.get()) continue;

    if (!device->set(feature, option.getName().c_str())) {
      isUpdatingSelection = true;
      option.set(false);
      isUpdatingSelection = false;
    }
  }
}

// Whenever the feature changes, we want to rerun the setup
void EnumFeatureParameter::pull() {
  std::string currentValue;
  if (device->get(feature, currentValue)) select(currentValue);
}

// Change the selection
void EnumFeatureParameter::onParameterChange(const void* sender,
                                             bool& selection) {
  if (isUpdatingSelection) return;

  if (isWritable()) {
    auto param = static_cast<const ofParameter<bool>*>(sender);
    device->set(feature, param->getName());
  } else {
    selection = false;
  }

  if (isReadable()) pull();
}

// Block selection change
void EnumFeatureParameter::onParameterUnavailable(const void* sender,
                                                  bool& selection) {
  if (isUpdatingSelection) return;

  auto param = static_cast<const ofParameter<bool>*>(sender);
  logger.warning("Selecting unavailable option: " + param->getName());
  selection = false;
}

// Select a given value
void EnumFeatureParameter::select(const std::string& value) {
  isUpdatingSelection = true;

  for (auto& param : *group) {
    param->cast<bool>().set(param->getName() == value);
  }

  isUpdatingSelection = false;
}
