#pragma once

#include <atomic>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "Device.h"
#include "Parameter.h"

namespace ofxVimba {
class FeatureParameterObserver;
class FeatureParameter : public Parameter {
  friend FeatureParameterObserver;

 public:
  FeatureParameter(const std::string& _name) : Parameter(_name), dirty(false) {}

  // Locate the feature with "name" on the device and start listening
  virtual bool bind(const std::shared_ptr<Device>& nextDevice) override;

  // Unbind the located device feature
  virtual bool unbind() override;

  // Override the update methods
  virtual void update() override;

 protected:
  std::atomic<bool> dirty;
  AVT::VmbAPI::FeaturePtr feature;
  SP_DECL(FeatureParameterObserver) observer;

  // Start our feature observer
  virtual bool observe();
  virtual bool unobserve();

  // Locate our feature
  virtual bool attach(const std::shared_ptr<Device>& nextDevice);
  virtual bool detach();

  // Inspect the feature and limit the parameter whenever possible
  virtual void setup() {}

  // Pull the current device value to the parameter
  virtual void pull() {}

  // Push the current parameter value to the device
  virtual void push() {}

  // Mark the feature as dirty
  virtual void setDirty(bool isDirty) { dirty = isDirty; }
};

class FeatureParameterObserver : public AVT::VmbAPI::IFeatureObserver {
 public:
  FeatureParameterObserver(FeatureParameter& feature) : feature(feature){};

  void start() { active = true; }
  void stop() { active = false; }
  void FeatureChanged(const AVT::VmbAPI::FeaturePtr&) override;

 private:
  FeatureParameter& feature;
  bool active = false;
};
}  // namespace ofxVimba
