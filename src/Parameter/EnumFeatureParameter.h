#pragma once

#include <memory>
#include <string>

#include "ofMain.h"

#include "FeatureParameter.h"

namespace ofxVimba {
class EnumFeatureParameter : public FeatureParameter {
 public:
  EnumFeatureParameter(const std::string& name);

 protected:
  virtual void setup() override;
  virtual void pull() override;
  virtual void push() override;

 private:
  std::shared_ptr<ofParameterGroup> group;
  bool isUpdatingSelection = false;

  void select(const std::string& value);
  void onParameterChange(const void* sender, bool& value);
  void onParameterUnavailable(const void* sender, bool& value);
};
}  // namespace ofxVimba
