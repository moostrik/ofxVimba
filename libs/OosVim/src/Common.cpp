#include "OosVim/Common.h"

using namespace OosVimba;

bool OosVimba::isAccessModeAvailable(const AccessMode &requested,
                                     const VmbAccessModeType &available) {
  auto mode = translateAccessMode(available);
  return isAccessModeAvailable(requested, mode);
};

bool OosVimba::isAccessModeAvailable(const AccessMode &requested,
                                     const AccessMode &available) {
  return available >= requested;
};

AccessMode OosVimba::translateAccessMode(const VmbAccessModeType &modes) {
  if (modes & VmbAccessModeFull) return AccessModeMaster;
  if (modes & VmbAccessModeRead) return AccessModeRead;

  return AccessModeNone;
};

VmbAccessModeType OosVimba::translateAccessMode(const AccessMode &mode) {
  if (mode == AccessModeMaster) return VmbAccessModeFull;
  if (mode == AccessModeRead) return VmbAccessModeRead;

  return VmbAccessModeNone;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, int &value) {
  long long tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<int>(tmp);
    return true;
  }

  return false;
}

#ifndef _MSC_VER
template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          int64_t &value) {
    long long tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<int64_t>(tmp);
    return true;
  }

  return false;
}
#endif

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          uint64_t &value) {
  long long tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<uint64_t>(tmp);
    return true;
  }

  return false;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          float &value) {
  double tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<float>(tmp);
    return true;
  }

  return false;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const int &value) {
  return setFeature(feature, static_cast<long long>(value));
}

#ifndef _MSC_VER
template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const int64_t &value) {
  return setFeature(feature, static_cast<long long>(value));
}
#endif

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const uint64_t &value) {
  return setFeature(feature, static_cast<long long>(value));
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const float &value) {
  return setFeature(feature, static_cast<double>(value));
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const std::string &value) {
  return setFeature(feature, value.c_str());
}
