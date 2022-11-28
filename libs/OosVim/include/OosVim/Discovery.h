#pragma once

#include <mutex>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Device.h"
#include "Logger.h"
#include "System.h"

namespace ofxVimba {
static const std::string DISCOVERY_ANY_ID = "";

class Discovery {
 public:
  Discovery(Discovery const &) = delete;
  Discovery &operator=(Discovery const &) = delete;

  Discovery();
  ~Discovery();

  // Trigger connection and disconnection
  void requestID(std::string id) { reqID = id; }

  bool start();
  bool stop();
  bool restart() { return stop() && start(); }

  // Status information
  bool isStarted() const { return !isStopped(); }
  bool isStopped() const { return SP_ISNULL(observer); }

  // Events
  ofEvent<std::shared_ptr<Device>> onFound;
  ofEvent<std::shared_ptr<Device>> onLost;
  ofEvent<std::shared_ptr<Device>> onUpdate;

 private:
  std::string reqID = DISCOVERY_ANY_ID;

  // Our nested observer is a private class
  class Observer : public AVT::VmbAPI::ICameraListObserver {
   public:
    Observer(Discovery &discovery, std::string requestID)
        : logger("Discovery::Observer"),
          system(System::getInstance()),
          discovery(discovery),
          reqID(requestID) {
      discover();
    };

    virtual void CameraListChanged(AVT::VmbAPI::CameraPtr camera, AVT::VmbAPI::UpdateTriggerType reason) {
      process(camera, reason);
    }

   private:
    Logger logger;
    std::shared_ptr<System> system;

    Discovery &discovery;
    std::string reqID;

    void discover();
    void process(AVT::VmbAPI::CameraPtr camera, AVT::VmbAPI::UpdateTriggerType reason);
  };

  // Create a logger for discovery
  Logger logger;

  // Keep an instance of the system around while discovery exists
  std::shared_ptr<System> system;

  // Observe for a next camera to connect to
  SP_DECL(Observer) observer;
};
}  // namespace ofxVimba
