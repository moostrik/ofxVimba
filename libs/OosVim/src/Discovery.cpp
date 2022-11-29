#include "OosVim/Discovery.h"

using namespace ofxVimba;

Discovery::Discovery() : logger("Discovery"), system(System::getInstance()){};
Discovery::~Discovery() { stop(); }

bool Discovery::start() {
  if (!SP_ISNULL(observer)) return true;
  if (!system->isAvailable()) {
    logger.error("Failed to set up discovery, system unavailable");
    return false;
  }
  logger.setScope(reqID);

  SP_SET(observer, new Observer(*this, reqID));
  auto error = system->getAPI().RegisterCameraListObserver(observer);

  if (error != VmbErrorSuccess) {
    logger.error("Failed to set up connection listener", error);
    SP_RESET(observer);
    return false;
  }

  logger.notice("Listening for camera to connect");
  return true;
}

bool Discovery::stop() {
  if (SP_ISNULL(observer)) return true;

  auto error = system->getAPI().UnregisterCameraListObserver(observer);
  if (error != VmbErrorSuccess) {
    logger.error("Failed to remove connection listener", error);
    return false;
  }

  logger.notice("Stopped listening for connection changes");
  logger.clearScope();
  SP_RESET(observer);
  return true;
}

void Discovery::Observer::discover() {
  if (!system->isAvailable()) {
    logger.error(
        "Failed to retrieve current camera list, system is unavailable");
    return;
  }

  AVT::VmbAPI::CameraPtrVector cameras;
  auto err = system->getAPI().GetCameras(cameras);

  if (err != VmbErrorSuccess) {
    logger.error("Failed to retrieve current camera list");
    return;
  }

  // Make sure we discover all the camera's at boot
  for (auto &camera : cameras) {
    process(camera, AVT::VmbAPI::UpdateTriggerPluggedIn);
  }
}

void Discovery::Observer::process(AVT::VmbAPI::CameraPtr camera,
                                  AVT::VmbAPI::UpdateTriggerType reason) {
  auto device = std::make_shared<Device>(camera);

  // Make sure the provided filter matched the camera
  if (reqID != DISCOVERY_ANY_ID && device->getId() != reqID) return;

  // Publish the event
  if (discovery.triggerCallbackFuction)
  switch (reason) {
    case AVT::VmbAPI::UpdateTriggerPluggedIn:
      discovery.triggerCallbackFuction(device, OOS_DISCOVERY_PLUGGED_IN);
      break;
    case AVT::VmbAPI::UpdateTriggerPluggedOut:
      discovery.triggerCallbackFuction(device, OOS_DISCOVERY_PLUGGED_OUT);
      break;
    case AVT::VmbAPI::UpdateTriggerOpenStateChanged:
      discovery.triggerCallbackFuction(device, OOS_DISCOVERY_STATE_CHANGED);
      break;
    default:
      break;
  }
};
