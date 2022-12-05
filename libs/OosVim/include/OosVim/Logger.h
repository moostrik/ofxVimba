#pragma once

#include <memory>
#include <string>
#include <sstream>
#include <iostream>

#include "VimbaCPP/Include/VimbaCPP.h"


#define OFX_VMB_LOG_LEVEL(alias, level)                                   \
  void alias(const std::string &msg) const { log(level, identity, msg); } \
  void alias(const std::string &msg, VmbErrorType &err) const {           \
    log(level, identity, msg, err);                                       \
  }                                                                       \
  static void alias(const std::string &mod, const std::string &msg) {     \
    return log(level, mod, msg);                                          \
  }                                                                       \
  static void alias(const std::string &mod, const std::string &msg,       \
                    const VmbErrorType &err) {                            \
    return log(level, mod, msg, err);                                     \
  }

namespace OosVimba {

enum vmbLogLevel : short {
  VMB_LOG_VERBOSE,
  VMB_LOG_NOTICE,
  VMB_LOG_WARNING,
  VMB_LOG_ERROR,
  VMB_LOG_FATAL_ERROR,
  VMB_LOG_NONE
};

static vmbLogLevel currentLogLevel = VMB_LOG_NOTICE;

static const std::string LOGGER_PREFIX_START = "[OosVimba";
static const std::string LOGGER_PREFIX_SCOPE = "::";
static const std::string LOGGER_PREFIX_END = "] ";
static const std::string LOGGER_EMPTY = "";
static const VmbErrorType LOGGER_NO_ERROR = VmbErrorSuccess;

class Logger {
 public:
  Logger(std::string name, std::string scope = "") : module(name) {
    setScope(scope);
  };

  void clearScope();
  void setScope();
  void setScope(const std::string &nextScope);

  OFX_VMB_LOG_LEVEL(verbose,  VMB_LOG_VERBOSE)
  OFX_VMB_LOG_LEVEL(notice,   VMB_LOG_NOTICE)
  OFX_VMB_LOG_LEVEL(warning,  VMB_LOG_WARNING)
  OFX_VMB_LOG_LEVEL(error,    VMB_LOG_ERROR)
  OFX_VMB_LOG_LEVEL(fatal,    VMB_LOG_FATAL_ERROR)

  static void log(vmbLogLevel level, const std::string &message) {
    log(level, LOGGER_EMPTY, message, VmbErrorSuccess);
  };

  static void log(vmbLogLevel level, const std::string &message, VmbErrorType &error) {
    log(level, LOGGER_EMPTY, message, error);
  };

  static void log(vmbLogLevel level, const std::string &module, const std::string &message) {
    log(level, module, message, LOGGER_NO_ERROR);
  };

  static void log(vmbLogLevel level, const std::string &module,  const std::string &message, const VmbErrorType &error);

 private:
  static const char *messageFor(const VmbErrorType &error);
  static const char *levelFor(const vmbLogLevel& level);

  std::string module = "";
  std::string identity = "";
};
}  // namespace OosVimba
