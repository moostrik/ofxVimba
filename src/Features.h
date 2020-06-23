#pragma once

#include "ofMain.h"
#include "ofxXmlSettings.h"

#include "Device.h"
#include "Logger.h"

namespace ofxVimba {

class FeatureObserver;
class Feature {
  friend FeatureObserver;

 public:
  Feature(const std::string _name) : name(_name), logger("Feature") {
    logger.setScope(_name);
  }
  virtual bool bind(AVT::VmbAPI::FeaturePtr _VmbFeature);
  virtual void unbind();
  bool isBound() { return bound; }  // redundant?
  virtual bool push() { return false; }
  virtual bool pull() { return false; }
  virtual void setValueFromString(const std::string _value) {}
  virtual const std::string getValueAsString() { return ""; }

 protected:
  bool observe();
  void unobserve();

  std::string name;
  bool bound = false;
  AVT::VmbAPI::FeaturePtr VmbFeature;
  SP_DECL(FeatureObserver) observer;
  Logger logger;
};

template <typename T>
class GenericFeature : public Feature {
 public:
  GenericFeature(const std::string& name) : Feature(name) {}
  bool pull() override;
  bool push() override;
  void setValue(const T _value);
  const T getValue();
  void setValueFromString(const std::string _value) override;
  const std::string getValueAsString() override;

 protected:
  T value;
};

template <>
bool GenericFeature<string>::push();

class FeatureObserver : public AVT::VmbAPI::IFeatureObserver {
 public:
  FeatureObserver(Feature& _feature) : feature(_feature){};
  void FeatureChanged(const AVT::VmbAPI::FeaturePtr&) override;

 private:
  Feature& feature;
};

class Features {
 public:
  Features(Features const&) = delete;
  Features& operator=(Features const&) = delete;

  Features();
  ~Features();

  void setOffline(std::string _name, std::string _value);
  std::string getOffline(std::string _name);

  // Bind and unbind this instance to a specific device
  void bind(const std::shared_ptr<Device>& device);
  void unbind();

  bool isBound() const { return device != nullptr; }
  bool isUnbound() const { return device == nullptr; }

  bool load(const std::string& fileName = "");
  bool save(const std::string& fileName = "") const;

  template <typename ValueType>
  void set(const std::string& name, const ValueType& value) {
    if (device) {
      device->set(name, value);
    } else {
      if (initialized) {
        setOffline(name, ofToString(value));
      }
    }
  }

  template <typename ValueType>
  bool get(const std::string& name, ValueType& value) {
    return device ? device->get(name, value) : false;
  }

  float getFloat(const std::string& name) {
    float value = 0.0;
    get(name, value);
    return value;
  }

  int getInt(const std::string& name) {
    int value = 0;
    get(name, value);
    return value;
  }

  bool getBool(const std::string& name) {
    bool value = false;
    get(name, value);
    return value;
  }

  string getString(const std::string& name) {
    string value = "";
    get(name, value);
    return value;
  }

  template <typename ValueType>
  bool getRange(const std::string& name, ValueType& min, ValueType& max) {
    return device ? device->getRange(name, min, max) : false;
  }

  glm::vec2 getFloatRange(const std::string& name) {
    float minValue, maxValue;
    getRange(name, minValue, maxValue);
    return glm::vec2(minValue, maxValue);
  }

  glm::ivec2 getIntRange(const std::string& name) {
    int minValue, maxValue;
    getRange(name, minValue, maxValue);
    return glm::ivec2(minValue, maxValue);
  }

 private:
  bool initialized;
  Logger logger;

  std::shared_ptr<Device> device;
  std::map<const std::string, std::shared_ptr<Feature> > features;
  std::unordered_map<std::string, std::string> requests;

  // Start our feature observer
  bool observe();
  bool unobserve();

  void buildFeatures();
  void setFeatures();

  const std::shared_ptr<Feature> getFeature(const std::string& id) const;

  const std::shared_ptr<Feature> buildFeature(
      const std::string& name, const VmbFeatureDataType& dataType) const;
};
}  // namespace ofxVimba
