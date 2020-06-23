#pragma once

#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "FeatureParameter.h"

namespace ofxVimba {
class CommandFeatureParameter : public FeatureParameter {
 public:
  CommandFeatureParameter(const std::string& name);

 protected:
  virtual void pull() override;
  virtual bool isSerializable() override { return false; }

 private:
  std::shared_ptr<ofParameter<bool>> parameter;
  void run(bool& nextValue);
};
}  // namespace ofxVimba
