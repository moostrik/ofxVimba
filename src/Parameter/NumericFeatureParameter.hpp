#pragma once

#include <memory>
#include <string>

#include "ofMain.h"

#include "GenericFeatureParameter.hpp"

namespace ofxVimba {
template <typename T>
class NumericFeatureParameter : public GenericFeatureParameter<T> {
 public:
  NumericFeatureParameter(const std::string& name)
      : GenericFeatureParameter<T>(name) {}

 protected:
  virtual void setParameterLimits() {
    T min, max;

    if (this->device->getRange(this->feature, min, max)) {
      this->modify([&min, &max](std::shared_ptr<ofParameter<T>>& parameter) {
        parameter->setMin(min);
        parameter->setMax(max);
        parameter->set(parameter->get());
      });
    } else {
      this->logger.verbose("Failed to retrieve value ranges");
    }
  }

  virtual bool setDeviceValue(const T& value) override {
    T limitedValue, minValue, maxValue;
    if (this->device->getRange(this->feature, minValue, maxValue)) {
      limitedValue = max(min(value, maxValue), minValue);
    }
    if (limitedValue != value) {
      this->logger.warning("Failed to write value=" + ofToString(value) +
                           ", using limited value=" + ofToString(limitedValue));
    }

    return GenericFeatureParameter<T>::setDeviceValue(limitedValue);
  }

  virtual void setup() override {
    GenericFeatureParameter<T>::setup();
    setParameterLimits();
  }

  virtual void push() override {
    GenericFeatureParameter<T>::push();
    setParameterLimits();
  }

  virtual void pull() override {
    GenericFeatureParameter<T>::pull();
    setParameterLimits();
  }
};
}  // namespace ofxVimba
