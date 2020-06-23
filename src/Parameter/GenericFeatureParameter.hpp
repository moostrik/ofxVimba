#pragma once

#include <atomic>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "Device.h"
#include "FeatureParameter.h"

namespace ofxVimba {
template <typename T>
class GenericFeatureParameter : public FeatureParameter {
 public:
  GenericFeatureParameter(const std::string& name) : FeatureParameter(name) {
    T initialValue;

    parameter = std::make_shared<ofParameter<T>>();
    parameter->set(name, initialValue);
    parameter->addListener(this, &GenericFeatureParameter<T>::onChange);

    setAbstractParameter(parameter);
  };

 protected:
  std::shared_ptr<ofParameter<T>> parameter;

  virtual void setup() override { pull(); }

  virtual void pull() override {
    T deviceValue;

    if (!getDeviceValue(deviceValue)) {
      logger.verbose("Failed to read current device value");
      return;
    }

    if (deviceValue != getParameterValue()) {
      setParameterValue(deviceValue);
    }
  }

  virtual void push() override {
    T currentValue;

    if (!getDeviceValue(currentValue)) {
      logger.verbose("Failed to read current device value");
      return;
    }

    // Device is up to date, don't update the device value
    if (currentValue == getParameterValue()) return;

    if (!setDeviceValue(getParameterValue())) {
      auto prev = getValueAsString();
      setParameterValue(currentValue);
      logger.warning("Failed to write value=" + prev +
                     ", using previous value=" + getValueAsString());
    }

    logger.verbose("Pushed change to device value=" + getValueAsString());
  }

  // Easy access to the parameter value
  const T& getParameterValue() const { return parameter->get(); }
  void setParameterValue(const T& value) {
    modify([&value](std::shared_ptr<ofParameter<T>>& param) {
      param->set(value);
    });
  }

  // Easy access to the device value
  virtual bool getDeviceValue(T& value) const {
    return isReadable() ? device->get(feature, value) : false;
  }

  virtual bool setDeviceValue(const T& value) {
    return isWritable() ? device->set(feature, value) : false;
  }

  virtual void modify(
      std::function<void(std::shared_ptr<ofParameter<T>>&)> modifier) {
    synchronize = false;
    modifier(parameter);
    synchronize = initialized;
  }

 private:
  // Whether the parameter's value is valid or not
  bool initialized = false;

  // Whether the parameter's value should be synchronize to the deve
  bool synchronize = false;

  // Whenever the parameter changes push it to the device
  void onChange(T&) {
    if (synchronize) {
      if (isWritable()) {
        push();
      } else if (isReadable()) {
        pull();
      }
    }

    initialized = true;
  }
};
};  // namespace ofxVimba
