#include <sstream>

#include "System.h"
using namespace ofxVimba;

System::System() : logger("System") { initialize(); }
System::~System() { shutdown(); }

bool System::initialize() {
  if (queryVersion()) {
    auto& sys = AVT::VmbAPI::VimbaSystem::GetInstance();
    auto error = sys.Startup();

    if (error == VmbErrorSuccess) {
      logger.verbose("Successfully initialized");
      api = &sys;
      return true;
    } else {
      logger.error("Failed to initialize system", error);
      api = nullptr;
      return false;
    }
  }

  logger.error("Failed to initialize, API not available");
  return false;
}

bool System::shutdown() {
  if (api == nullptr) return true;
  auto error = api->Shutdown();
  if (error == VmbErrorSuccess) {
    logger.verbose("Successfully shut down system");
    api = nullptr;
    return true;
  } else {
    logger.error("Failed to shut down system", error);
    return false;
  }
}

bool System::isVersionValid() {
  if (version.valid) return true;
  return queryVersion() && version.valid;
}

bool System::queryVersion() {
  if (version.valid) return true;

  VmbVersionInfo_t info;
  auto error = AVT::VmbAPI::VimbaSystem::GetInstance().QueryVersion(info);
  if (error == VmbErrorSuccess) {
    version.valid = true;
    version.major = info.major;
    version.minor = info.minor;
    version.patch = info.patch;

    // Construct an the API version string
    std::ostringstream version_string;
    version_string << info.major << "." << info.minor << "." << info.patch;
    version.string = version_string.str();
    logger.notice("Vimba API Version: " + version.string);

    return true;
  } else {
    logger.warning("Failed to get Vimba API Version", error);
    return false;
  }
}
