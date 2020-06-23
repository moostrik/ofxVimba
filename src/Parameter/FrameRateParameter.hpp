#pragma once

#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "NumericFeatureParameter.hpp"

namespace ofxVimba {
class FrameRateParameter : public NumericFeatureParameter<float> {
 public:
  FrameRateParameter()
      : NumericFeatureParameter<float>("AcquisitionFrameRateAbs") {
    parameter->setName("CustomFrameRate");
    category.push_back("Custom");
    descriptor = ("/Custom/CustomFrameRate");
  }

 protected:
  // Query for the feature on the device
  virtual bool attach(const std::shared_ptr<Device>& nextDevice) override {
    if (NumericFeatureParameter<float>::attach(nextDevice)) {
      frameRate = this->feature;
      nextDevice->locate("ExposureAutoMax", exposureAutoMax);
      nextDevice->locate("ExposureAutoMin", exposureAutoMin);
      return true;
    }
    return false;
  }

  virtual bool detach() override {
    if (!SP_ISNULL(exposureAutoMax)) SP_RESET(exposureAutoMax);
    if (!SP_ISNULL(exposureAutoMin)) SP_RESET(exposureAutoMin);
    if (!SP_ISNULL(frameRate)) SP_RESET(frameRate);

    return NumericFeatureParameter<float>::detach();
  }

  virtual void setup() override {
    NumericFeatureParameter<float>::setup();
    // initialize ExposureAutoMax based on framerate
    push();
  }

  virtual void push() override {
    NumericFeatureParameter<float>::push();
    // set ExposureAutoMax based on framerate
    if (SP_ISNULL(exposureAutoMax)) return;
    int maxExposureForFramerate, currentExposureAutoMax;
    maxExposureForFramerate = 1000000.0 / parameter->get();
    device->get(exposureAutoMax, currentExposureAutoMax);
    if (currentExposureAutoMax != maxExposureForFramerate) {
      device->set(exposureAutoMax, maxExposureForFramerate);
    }

    int64_t minValue, maxValue;
    device->getRange(exposureAutoMin, minValue, maxValue);
    device->set(exposureAutoMin, minValue);
  }

  virtual bool setDeviceValue(const float& value) override {
    double limitedValue = value;
    double minValue, maxValue;
    if (this->device->getRange(this->feature, minValue, maxValue)) {
      limitedValue = max(min(limitedValue, maxValue - 0.01), ceil(minValue));
    }
    if (limitedValue != value) {
      this->logger.verbose("limited value to" + ofToString(limitedValue));
    }

    return GenericFeatureParameter<float>::setDeviceValue(limitedValue);
  }

 private:
  AVT::VmbAPI::FeaturePtr frameRate;
  AVT::VmbAPI::FeaturePtr exposureAutoMax;
  AVT::VmbAPI::FeaturePtr exposureAutoMin;
};

};  // namespace ofxVimba
