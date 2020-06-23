#include "FeatureParameter.h"

using namespace ofxVimba;

bool FeatureParameter::bind(const std::shared_ptr<Device>& nextDevice) {
  if (isBound()) unbind();

  if (attach(nextDevice) && observe()) {
    // Allow the value to be restored
    setup();

    // After restoring the value activate the observer
    observer->start();
    return true;
  }

  unbind();
  return false;
}

bool FeatureParameter::unbind() {
  unobserve();
  detach();

  return true;
}

bool FeatureParameter::attach(const std::shared_ptr<Device>& nextDevice) {
  if (nextDevice->locate(name, feature)) {
    auto error = feature->IsReadable(readable);
    if (error == VmbErrorSuccess) error = feature->IsWritable(writable);
    if (error == VmbErrorSuccess) {
      device = nextDevice;
      return true;
    }
  }

  return false;
}

bool FeatureParameter::detach() {
  if (!SP_ISNULL(feature)) SP_RESET(feature);

  readable = false;
  writable = false;
  device = nullptr;

  return true;
}

bool FeatureParameter::observe() {
  if (SP_ISNULL(feature)) return false;

  SP_SET(observer, new FeatureParameterObserver(*this));
  return feature->RegisterObserver(observer) == VmbErrorSuccess;
}

bool FeatureParameter::unobserve() {
  if (!SP_ISNULL(observer)) {
    auto error = feature->UnregisterObserver(observer);
    SP_RESET(observer);
    return error == VmbErrorSuccess;
  }

  return true;
}

void FeatureParameter::update() {
  if (!dirty) return;
  if (isBound()) pull();

  dirty = false;
}

void FeatureParameterObserver::FeatureChanged(const AVT::VmbAPI::FeaturePtr&) {
  if (active) feature.setDirty(true);
}
