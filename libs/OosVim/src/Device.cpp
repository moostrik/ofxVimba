#include "OosVim/Device.h"

using namespace OosVim;

Device::Device(AVT::VmbAPI::CameraPtr handle)
    : logger("Device "), system(System::getInstance()), handle(handle) {
  logger.setScope("Unknown");
  inspect();
}

Device::~Device() {
  if (isOpen()) close();
}

bool Device::open(AccessMode mode) {
  if (!isAvailable()) {
    logger.warning("No access mode available to open camera");
    return false;
  }

  if (mode == AccessModeAuto) mode = getAvailableAccessMode();
  auto error = SP_ACCESS(getHandle())->Open(translateAccessMode(mode));
  if (error != VmbErrorSuccess) {
    logger.warning("Failed to open camera", error);
    currentMode = AccessModeNone;
    return false;
  }

  currentMode = mode;
  logger.notice("Open");
  return true;
}

bool Device::close() {
  if (!isOpen()) {
    logger.warning("Cannot close unopened device");
    return false;
  }

  auto error = SP_ACCESS(getHandle())->Close();
  if (error == VmbErrorInvalidCall) {
    logger.warning("Cannot close camera at this time", error);
    return false;
  } else if (error != VmbErrorSuccess) {
    logger.warning("Failed to close camera", error);
    return false;
  }

  currentMode = AccessModeNone;

  logger.notice("Closed");
  return true;
}

bool Device::run(const std::string& name) {
  if (!isOpen()) {
    logger.warning("Cannot run command on unopened device");
    return false;
  }

  AVT::VmbAPI::FeaturePtr feature;
  auto error = handle->GetFeatureByName(name.c_str(), feature);
  if (error == VmbErrorSuccess) error = feature->RunCommand();
  if (error != VmbErrorSuccess) {
    logger.error("Failed to run " + name, error);
    return false;
  }

  return true;
}

bool Device::locate(const std::string& name, AVT::VmbAPI::FeaturePtr& feature) {
  return handle->GetFeatureByName(name.c_str(), feature) == VmbErrorSuccess;
}

bool Device::inspect() {
  if (SP_ISNULL(handle)) return false;

  auto error = SP_ACCESS(handle)->GetID(id);
  if (error != VmbErrorSuccess) {
    logger.error("Failed to read camera ID", error);
    return false;
  }

  logger.setScope(id);

  error = SP_ACCESS(handle)->GetName(name);
  if (error != VmbErrorSuccess) {
    logger.error("Failed to read camera name", error);
    return false;
  }

  error = SP_ACCESS(handle)->GetModel(model);
  if (error != VmbErrorSuccess) {
    logger.error("Failed to read camera model", error);
    return false;
  }

  error = SP_ACCESS(handle)->GetSerialNumber(serial);
  if (error != VmbErrorSuccess) {
    logger.error("Failed to read camera serial", error);
    return false;
  }

  VmbAccessModeType modes;
  error = SP_ACCESS(handle)->GetPermittedAccess(modes);
  if (error == VmbErrorSuccess) {
    availableMode = translateAccessMode(modes);
  } else if (error != VmbErrorNotFound) {
    logger.verbose("Could not get available access modes assumed none", error);
    availableMode = AccessModeNone;
  }

  return true;
}
