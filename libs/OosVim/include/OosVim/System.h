#pragma once

#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Logger.h"

namespace ofxVimba {

struct Version {
  bool valid = false;
  unsigned int major = 0;
  unsigned int minor = 0;
  unsigned int patch = 0;
  std::string string = "Unknown";
};

class System {
 public:
  // Disable copy and move
  System(System const&) = delete;             // Copy construct
  System(System&&) = delete;                  // Move construct
  System& operator=(System const&) = delete;  // Copy assign
  System& operator=(System&&) = delete;       // Move assign

  // Allow public destruction by the shared pointer
  ~System();

  static std::shared_ptr<System> getInstance() {
    static std::weak_ptr<System> _weakInstance;
    if (auto existingPtr = _weakInstance.lock()) return existingPtr;
    auto newPtr = std::shared_ptr<System>(new System());
    _weakInstance = newPtr;
    return newPtr;
  }

  // Flags
  bool isAvailable() { return api != nullptr && isVersionValid(); };

  // API information
  AVT::VmbAPI::VimbaSystem& getAPI() { return *api; }
  Version& getVersion() { return version; };
  std::string& getVersionString() { return version.string; };

 private:
  System();

  bool initialize();
  bool shutdown();
  bool isVersionValid();
  bool queryVersion();

  AVT::VmbAPI::VimbaSystem* api = nullptr;
  Logger logger;
  Version version;
};
}  // namespace ofxVimba
