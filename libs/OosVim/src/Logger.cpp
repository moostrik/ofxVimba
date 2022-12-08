#include "OosVim/Logger.h"

using namespace OosVim;

static vmbLogLevel currentLogLevel = VMB_LOG_NOTICE;

void Logger::clearScope() { identity = module; }
void Logger::setScope() { identity = module; }
void Logger::setScope(const std::string& nextScope) {
  identity = nextScope == LOGGER_EMPTY ? module : module + "#" + nextScope;
}

void Logger::setLevel(vmbLogLevel level) {
  currentLogLevel = level;
}

void Logger::log(vmbLogLevel level, const std::string& module, const std::string& message, const VmbErrorType& error) {
  std::ostringstream out;

  out << levelFor(level);

  out << LOGGER_PREFIX_START;

  // Add our module tag
  if (module != LOGGER_EMPTY) out << LOGGER_PREFIX_SCOPE << module;

  out << LOGGER_PREFIX_END << message;

  if (error != VmbErrorSuccess)
    out << " (error=\"" << std::string(messageFor(error)) << "\")";

  if (level >= currentLogLevel)
    std::cout << out.str() << std::endl;
}

const char* Logger::messageFor(const VmbErrorType& error) {
  switch (error) {
    case VmbErrorSuccess:
      return "Success";
    case VmbErrorInternalFault:
      return "Unexpected fault in VimbaC or driver";
    case VmbErrorApiNotStarted:
      return "VmbStartup() was not called before the current command";
    case VmbErrorNotFound:
      return "The designated instance (camera, feature etc.) cannot be "
             "found";
    case VmbErrorBadHandle:
      return "The given handle is not valid";
    case VmbErrorDeviceNotOpen:
      return "Device was not opened for usage";
    case VmbErrorInvalidAccess:
      return "Operation is invalid with the current access mode";
    case VmbErrorBadParameter:
      return "One of the parameters is invalid (usually an illegal "
             "pointer)";
    case VmbErrorStructSize:
      return "The given struct size is not valid for this version of the "
             "API";
    case VmbErrorMoreData:
      return "More data available in a string/list than space is provided";
    case VmbErrorWrongType:
      return "Wrong feature type for this access function";
    case VmbErrorInvalidValue:
      return "The value is not valid; either out of bounds or not an "
             "increment of the minimum";
    case VmbErrorTimeout:
      return "Timeout during wait";
    case VmbErrorOther:
      return "Other error";
    case VmbErrorResources:
      return "Resources not available (e.g. memory)";
    case VmbErrorInvalidCall:
      return "Call is invalid in the current context (e.g. callback)";
    case VmbErrorNoTL:
      return "No transport layers are found";
    case VmbErrorNotImplemented:
      return "API feature is not implemented";
    case VmbErrorNotSupported:
      return "API feature is not supported";
    case VmbErrorIncomplete:
      return "A multiple registers read or write is partially completed";
    case VmbErrorIO:
      return "Low level IO error in transport layer";
    default:
      return "Unknown error";
  }
}

const char* Logger::levelFor(const vmbLogLevel& level) {
    switch (level) {
    case VMB_LOG_VERBOSE:
        return "Verbose";
    case VMB_LOG_NOTICE:
        return "Notice";
    case VMB_LOG_WARNING:
        return "Warning";
    case VMB_LOG_ERROR:
        return "Error";
    case VMB_LOG_FATAL_ERROR:
        return "Fatal Error";
    default:
        return "Unknown";
    }
}
