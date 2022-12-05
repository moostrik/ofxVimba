#pragma once

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Common.h"
#include "Logger.h"
#include "System.h"

namespace OosVimba {

class Device {
 public:
  Device(Device const&) = delete;
  Device& operator=(Device const&) = delete;

  Device(AVT::VmbAPI::CameraPtr handle);
  ~Device();

  const std::string& getId() const { return id; }
  const std::string& getName() const { return name; }
  const std::string& getModel() const { return model; }
  const std::string& getSerial() const { return serial; }
  const AccessMode& getAvailableAccessMode() const { return availableMode; }
  const AccessMode& getCurrentAccessMode() const { return currentMode; }
  const AVT::VmbAPI::CameraPtr& getHandle() const { return handle; }

  // Flags
  bool isAvailable() const { return availableMode >= AccessModeRead; }
  bool isOpen() const { return currentMode >= AccessModeRead; }
  bool isMaster() const { return currentMode >= AccessModeMaster; }

  // Opening and closing the camera
  bool open(const AccessMode mode);
  bool close();

  // Access commands
  bool run(const std::string& name);

  // Locate features
  bool locate(const std::string& name, AVT::VmbAPI::FeaturePtr& feature);

  template <typename ValueType>
  bool get(const std::string& name, ValueType& value) const {
    if (!isOpen()) {
      logger.warning("Cannot read features from unopened device");
      return false;
    }

    AVT::VmbAPI::FeaturePtr feature;
    auto error = handle->GetFeatureByName(name.c_str(), feature);
    if (error != VmbErrorSuccess) {
      logger.warning("Failed to retrieve feature '" + name + "'", error);
      return false;
    }

    if (getFeature(feature, value)) return true;
    else logger.warning("Failed to get value for feature " + name);
    return false;
  }

  template <typename ValueType>
  bool set(const std::string& name, const ValueType& value) {
    if (!isOpen()) {
      logger.warning("Cannot read features from unopened device");
      return false;
    }

    AVT::VmbAPI::FeaturePtr feature;
    auto error = handle->GetFeatureByName(name.c_str(), feature);
    if (error != VmbErrorSuccess) {
      logger.warning("Failed to retrieve feature '" + name + "'", error);
      return false;
    }

    if (setFeature(feature, value)) return true;
    else logger.warning("Failed to set value for feature " + name);
    return false;
  }

  template <typename ValueType>
  bool getRange(const std::string& name, ValueType& min, ValueType& max) {
    if (!isOpen()) {
      logger.warning("Cannot read features from unopened device");
      return false;
    }

    AVT::VmbAPI::FeaturePtr feature;
    auto error = handle->GetFeatureByName(name.c_str(), feature);
    if (error != VmbErrorSuccess) {
      logger.warning("Failed to retrieve feature '" + name + "'", error);
      return false;
    }

    if (getFeatureRange(feature, min, max)) return true;
    else logger.warning("Failed to get range for feature " + name);
    return false;
  }

 private:
  Logger logger;
  std::shared_ptr<System> system;

  std::string id = "";
  std::string name = "";
  std::string model = "";
  std::string serial = "";
  AccessMode availableMode = AccessModeNone;
  AccessMode currentMode = AccessModeNone;
  AVT::VmbAPI::CameraPtr handle;

  bool inspect();
};
}  // namespace OosVimba
