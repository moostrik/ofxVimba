#include "Features.h"

using namespace ofxVimba;

// -- FEATURE ------------------------------------------------------------------
bool Feature::bind(AVT::VmbAPI::FeaturePtr _VmbFeature) {
  bool isReadWritable;
  auto error = _VmbFeature->IsWritable(isReadWritable);
  if (!isReadWritable || error != VmbErrorSuccess) return false;
  error = _VmbFeature->IsReadable(isReadWritable);
  if (!isReadWritable || error != VmbErrorSuccess) return false;
  VmbFeature = _VmbFeature;

  if (observe()) {
    bound = true;
    return true;
  } else {
    unbind();
    return false;
  }
}

void Feature::unbind() {
  bound = false;
  unobserve();
  SP_RESET(VmbFeature);
}

bool Feature::observe() {
  if (SP_ISNULL(VmbFeature)) return false;
  SP_SET(observer, new FeatureObserver(*this));
  return (VmbFeature->RegisterObserver(observer) == VmbErrorSuccess);
}

void Feature::unobserve() {
  if (!SP_ISNULL(observer)) {
    VmbFeature->UnregisterObserver(observer);
    SP_RESET(observer);
  }
}

// -- GENERIC FEATURE ----------------------------------------------------------
template <typename T>
bool GenericFeature<T>::pull() {
  auto error = VmbFeature->GetValue(value);
  if (error != VmbErrorSuccess) {
    ofLogVerbose("failed to get value of " + name);
    return false;
  }
  return true;
}

template <typename T>
bool GenericFeature<T>::push() {
  auto error = VmbFeature->SetValue(value);
  if (error == VmbErrorSuccess) {
    logger.verbose("set to " + ofToString(value));
    return true;
  } else {
    logger.warning("failed to set to " + ofToString(value), error);
    return false;
  }
}

template <>
bool GenericFeature<std::string>::push() {
  auto error = VmbFeature->SetValue(value.c_str());
  if (error == VmbErrorSuccess) {
    logger.verbose("set to " + ofToString(value));
    return true;
  } else {
    logger.verbose("failed to set to " + ofToString(value), error);
    return false;
  }
}

template <>
bool GenericFeature<double>::push() {
  double limitedValue, minValue, maxValue;
  VmbFeature->GetRange(minValue, maxValue);
  limitedValue = max(min(value, maxValue), minValue);
  if (value != limitedValue) {
    logger.notice(ofToString(value) + " out of range, clamped to " +
                  ofToString(limitedValue));
    value = limitedValue;
  }

  auto error = VmbFeature->SetValue(limitedValue);
  if (error == VmbErrorSuccess) {
    logger.verbose("set to " + ofToString(limitedValue));
    return true;
  } else {
    logger.verbose("failed to set to " + ofToString(limitedValue), error);
    return false;
  }
}

template <>
bool GenericFeature<VmbInt64_t>::push() {
  VmbInt64_t limitedValue, minValue, maxValue;
  VmbFeature->GetRange(minValue, maxValue);
  limitedValue = max(min(value, maxValue), minValue);
  if (value != limitedValue) {
    logger.notice(ofToString(value) + " out of range, clamped to " +
                  ofToString(limitedValue));
    value = limitedValue;
  }

  auto error = VmbFeature->SetValue(limitedValue);
  if (error == VmbErrorSuccess) {
    logger.verbose("set to " + ofToString(limitedValue));
    return true;
  } else {
    logger.verbose("failed to set to " + ofToString(limitedValue), error);
    return false;
  }
}

template <typename T>
void GenericFeature<T>::setValue(const T _value) {
  value = _value;
  push();
}

template <typename T>
const T GenericFeature<T>::getValue() {
  return value;
}

template <typename T>
void GenericFeature<T>::setValueFromString(const std::string _value) {
  setValue(ofTo<T>(_value));
}

template <typename T>
const std::string GenericFeature<T>::getValueAsString() {
  return ofToString(value);
}

void FeatureObserver::FeatureChanged(const AVT::VmbAPI::FeaturePtr&) {
  feature.pull();
}

// -- FEATURES -----------------------------------------------------------------
Features::Features() : logger("Features"), initialized(false) {}
Features::~Features() {
  if (isBound()) unbind();
}

void Features::setOffline(std::string _name, std::string _value) {
  auto it = requests.find(_name);
  if (it == requests.end())
    requests.insert({_name, _value});
  else
    requests[_name] = _value;

  if (device) {
    setFeatures();
  }
}

std::string Features::getOffline(std::string _name) {
  if (!initialized) {
    logger.warning("features are not initialized");
    return "";
  }
  std::shared_ptr<Feature> feature = getFeature(_name);
  if (!feature) {
    logger.warning("feature '" + _name + "' not found");
    return "";
  }
  return feature->getValueAsString();
}

void Features::setFeatures() {
  if (device) {
    for (auto& req : requests) {
      if (!features.count(req.first)) {
        logger.warning("feature " + req.first + " not found");
        continue;
      } else {
        auto it = features.find(req.first);
        it->second->setValueFromString(req.second);
      }
    }
    requests.clear();
  }
}

void Features::bind(const std::shared_ptr<Device>& nextDevice) {
  if (isBound()) unbind();

  device = nextDevice;
  logger.setScope(device->getId());

  // Discover new features and bind them directly
  buildFeatures();
  setFeatures();

  initialized = true;
}

void Features::unbind() {
  if (!isBound()) return;

  for (auto& f : features) {
    f.second->unbind();
  }

  // Unbind the device
  device = nullptr;
  logger.verbose("Unbound from device");
  logger.clearScope();
}

void Features::buildFeatures() {
  logger.verbose("Discovering device features");

  AVT::VmbAPI::FeaturePtrVector vmbFeatures;
  auto error = device->getHandle()->GetFeatures(vmbFeatures);
  if (error != VmbErrorSuccess) return;

  // check existing features
  for (auto& existingFeature : features) {
    std::string existingFeatureName = existingFeature.first;
    AVT::VmbAPI::FeaturePtr existingFeaturePtr;
    auto error = device->getHandle()->GetFeatureByName(
        existingFeatureName.c_str(), existingFeaturePtr);
    if (error != VmbErrorSuccess) {
      features.erase(existingFeatureName);
      logger.warning("feature " + existingFeatureName +
                     " does not exist anymore");
    }
  }

  bool isWritable;
  bool isReadable;
  std::string featureName;
  VmbFeatureDataType featureType;

  for (const auto& ft : vmbFeatures) {
    error = ft->IsWritable(isWritable);
    if (!isWritable || error != VmbErrorSuccess) continue;
    error = ft->IsReadable(isReadable);
    if (!isReadable || error != VmbErrorSuccess)
      continue;  // ?? does a feature need to be readable
    ft->GetDataType(featureType);
    if (error != VmbErrorSuccess) continue;
    ft->GetName(featureName);
    if (error != VmbErrorSuccess) continue;

    auto previous = getFeature(featureName);
    if (previous) {
      previous->bind(ft);
      std::string previousValue = previous->getValueAsString();
      previous->pull();
      std::string currentValue = previous->getValueAsString();
      if (previousValue != currentValue) {
        logger.notice(featureName + " has changed to " + currentValue +
                      ". Reset to " + previousValue);
        previous->setValueFromString(previousValue);
        previous->push();
      }
    } else {
      auto feature = buildFeature(featureName, featureType);
      if (feature) {
        if (feature->bind(ft)) {
          feature->pull();
          features.insert({featureName, feature});
        }
      }
    }
  }
}

const std::shared_ptr<Feature> Features::buildFeature(
    const std::string& name, const VmbFeatureDataType& dataType) const {
  switch (dataType) {
    case VmbFeatureDataBool:
    case VmbFeatureDataCommand:
      return std::make_shared<GenericFeature<bool> >(name);
    case VmbFeatureDataInt:
      return std::make_shared<GenericFeature<VmbInt64_t> >(name);
    case VmbFeatureDataFloat:
      return std::make_shared<GenericFeature<double> >(name);
    case VmbFeatureDataString:
    case VmbFeatureDataEnum:
      return std::make_shared<GenericFeature<string> >(name);
    default:
      logger.warning("Unsupported data type: " + ofToString(dataType) +
                     " for feature: " + name);
      return nullptr;
  }
}

const std::shared_ptr<Feature> Features::getFeature(
    const std::string& _name) const {
  auto it = features.find(_name);
  if (it == features.end()) return nullptr;
  return it->second;
}

bool Features::save(const std::string& fileName) const {
  if (features.size() == 0) return false;

  string fn = fileName;
  if (fn == "") fn = ofToDataPath(device->getId() + ".xml");

  logger.verbose("Saving camera features to " + fn);

  ofxXmlSettings xml;

  xml.addTag("vimba");
  xml.pushTag("vimba");
  xml.addTag("features");
  xml.pushTag("features");

  for (const auto& entry : features) {
    auto name = entry.first;
    auto feature = entry.second;

    xml.setValue(name, feature->getValueAsString());
  }

  xml.popTag();
  xml.popTag();

  return xml.save(fn);
}

bool Features::load(const std::string& fileName) {
  if (!isBound()) return false;

  string fn = fileName;
  if (fn == "") fn = ofToDataPath(device->getId() + ".xml");

  logger.verbose("Loading camera features to " + fn);

  ofFile file;
  if (!file.doesFileExist(fn)) {
    logger.warning(fn + " does not exist");
    return false;
  }

  logger.verbose("Reading camera features from " + fn);
  ofxXmlSettings xml;

  if (!xml.loadFile(fn)) {
    logger.warning("Could not read features from " + fn);
    return false;
  }

  if (!xml.pushTag("vimba") || !xml.pushTag("features")) {
    logger.warning("Could not recognize format in " + fn);
    return false;
  }

  for (const auto& entry : features) {
    auto name = entry.first;
    auto feature = entry.second;

    if (xml.tagExists(name)) {
      auto currentValue = feature->getValueAsString();
      auto savedValue = xml.getValue(name, currentValue);

      if (savedValue != currentValue) {
        feature->setValueFromString(savedValue);
      }
    }
  }

  return true;
}
