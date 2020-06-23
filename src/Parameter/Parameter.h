#pragma once

#include <atomic>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "Common.h"
#include "Device.h"
#include "Logger.h"

namespace ofxVimba {
class Parameter {
 public:
  Parameter(Parameter const&) = delete;
  Parameter& operator=(Parameter const&) = delete;

  Parameter(const std::string& _name)
      : name(_name), descriptor("UNKNOWN/" + _name), logger("Parameter") {
    logger.setScope(_name);
  }

  // Parameter flags
  virtual bool isBound() const { return device && parameterReference; }
  virtual bool isReadable() const { return isBound() && readable; }
  virtual bool isWritable() const { return isBound() && writable; }
  virtual bool isSerializable() {
    return parameterReference && parameterReference->isSerializable();
  }

  // Category naming
  virtual void setCategory(const std::string& nextCategoryName);
  virtual void setCategory(const std::vector<std::string>& nextCategory) {
    category = nextCategory;
  }

  // Parameter detail accessors
  virtual std::string getType() const { return typeid(*this).name(); }
  // virtual const std::string& getId() const { return id; }
  virtual const std::string& getName() const { return name; }
  virtual const std::string& getDescriptor() const { return descriptor; }
  virtual const std::vector<std::string>& getCategory() const {
    return category;
  }

  virtual const std::shared_ptr<ofAbstractParameter>& getAbstractParameter()
      const {
    return parameterReference;
  }

  // Serialization
  virtual std::string getValueAsString() const {
    return parameterReference->toString();
  }

  virtual void setValueFromString(const std::string& value) const {
    return parameterReference->fromString(value);
  }

  virtual bool bind(const std::shared_ptr<Device>& nextDevice);
  virtual bool unbind();

  // Needs to be called every frame
  virtual void update(){};

 protected:
  Logger logger;

  // std::string id;
  std::string name;
  std::vector<std::string> category;
  std::string descriptor;

  std::shared_ptr<Device> device;
  std::shared_ptr<ofAbstractParameter> parameterReference;

  bool persisted = false;
  bool readable = false;
  bool writable = false;

  void setAbstractParameter(
      const std::shared_ptr<ofAbstractParameter>& parameter) {
    parameterReference = parameter;
  }
};
}  // namespace ofxVimba
