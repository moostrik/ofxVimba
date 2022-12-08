#pragma once

#include <mutex>
#include <string>
#include <functional>

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Device.h"
#include "Logger.h"
#include "System.h"

namespace OosVim {
static const std::string DISCOVERY_ANY_ID = "";

enum DiscoveryTrigger {
  OOS_DISCOVERY_PLUGGED_IN = 0,
  OOS_DISCOVERY_PLUGGED_OUT = 1,
  OOS_DISCOVERY_STATE_CHANGED = 3,
};

class Discovery {
 public:
  Discovery(Discovery const &) = delete;
  Discovery &operator=(Discovery const &) = delete;

  Discovery();
  ~Discovery();

  // Trigger connection and disconnection
  void requestID(std::string id);
  void updateTriggers();

  bool start();
  bool stop();
  bool restart() { return stop() && start(); }

  // Status information
  bool isStarted() const { return !isStopped(); }
  bool isStopped() const { return SP_ISNULL(observer); }


  // Callback
  std::function<void(std::shared_ptr<Device> device, const DiscoveryTrigger)> triggerCallbackFuction;
  void setTriggerCallback(std::function<void(std::shared_ptr<Device> device, const DiscoveryTrigger)> value) { triggerCallbackFuction = value; }
  void setTriggerCallback() { triggerCallbackFuction = std::function<void(std::shared_ptr<Device> device, const DiscoveryTrigger)>(); }

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

    void setId(std::string id);
    void discover();

   private:
    Logger logger;
    std::shared_ptr<System> system;

    Discovery &discovery;
    std::mutex reqIdMutex;
    std::string reqID;

    void process(AVT::VmbAPI::CameraPtr camera, AVT::VmbAPI::UpdateTriggerType reason);
  };

  Logger logger;
  std::shared_ptr<System> system;

  // Observe for a next camera to connect to
  SP_DECL(Observer) observer;
};
}  // namespace OosVimba
