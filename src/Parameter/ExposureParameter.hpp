#pragma once

#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "NumericFeatureParameter.hpp"

namespace ofxVimba {
class ExposureParameter : public NumericFeatureParameter<float> {
 public:
  ExposureParameter() : NumericFeatureParameter<float>("ExposureTimeAbs") {
    parameter->setName("CustomExposure");
    category.push_back("Custom");
    descriptor = ("/Custom/CustomExposure");
  }

 protected:
  // Query for the feature on the device
  virtual bool attach(const std::shared_ptr<Device>& nextDevice) override {
    if (NumericFeatureParameter<float>::attach(nextDevice)) {
      exposure = this->feature;
      return nextDevice->locate("AcquisitionFrameRateAbs", framerate);
    }

    return false;
  }

  virtual bool detach() override {
    if (!SP_ISNULL(framerate)) SP_RESET(framerate);
    if (!SP_ISNULL(exposure)) SP_RESET(exposure);

    return NumericFeatureParameter<float>::detach();
  }

  // Start listening for feature changes...
  virtual bool observe() override {
    if (NumericFeatureParameter<float>::observe()) {
      if (SP_ISNULL(this->observer) || SP_ISNULL(framerate)) return false;
      return framerate->RegisterObserver(this->observer) == VmbErrorSuccess;
    }

    return false;
  }

  virtual bool unobserve() override {
    if (!SP_ISNULL(this->observer) && !SP_ISNULL(framerate)) {
      framerate->UnregisterObserver(this->observer);
    }

    return NumericFeatureParameter<float>::unobserve();
  }

  // Read our parameter limits
  virtual void setParameterLimits() override {
    if (SP_ISNULL(exposure) || SP_ISNULL(framerate)) return;

    float maxExposureForFramerate, value;
    if (device->get(framerate, value)) {
      maxExposureForFramerate = 1000000.0 / value;
    } else {
      maxExposureForFramerate = 0;
      logger.verbose("Failed to get current framerate");
    }

    float min, max;
    if (device->getRange(feature, min, max)) {
      if (maxExposureForFramerate > 0) {
        max = maxExposureForFramerate;
      }
      this->modify(
          [&min, &max](std::shared_ptr<ofParameter<float>>& parameter) {
            parameter->setMin(min);
            parameter->setMax(max);
            parameter->set(parameter->get());
          });
    } else {
      logger.verbose("Failed to retrieve value ranges");
      return;
    }
  }

  // Read our parameter limits and keep exposure within these limits
  virtual void pull() override {
    NumericFeatureParameter<float>::pull();
    if (parameter->get() > parameter->getMax()) {
      parameter->set(parameter->getMax());
    }
  }

 private:
  AVT::VmbAPI::FeaturePtr framerate;
  AVT::VmbAPI::FeaturePtr exposure;
};

};  // namespace ofxVimba
