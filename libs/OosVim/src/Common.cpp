#include "OosVim/Common.h"

using namespace OosVimba;

bool OosVimba::isAccessModeAvailable(const AccessMode &requested, const VmbAccessModeType &available) {
  auto mode = translateAccessMode(available);
  return isAccessModeAvailable(requested, mode);
};

bool OosVimba::isAccessModeAvailable(const AccessMode &requested, const AccessMode &available) {
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
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, long long &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<uint64_t>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, signed long &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<uint64_t>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, unsigned long long& value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<uint64_t>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, unsigned long& value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<uint64_t>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, int& value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<int>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, double& value) {
  if (SP_ISNULL(feature)) return false;
  double tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = tmp;
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, float& value) {
  if (SP_ISNULL(feature)) return false;
  double tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = static_cast<float>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr& feature, std::string& value) {
  if (SP_ISNULL(feature)) return false;
  std::string tmp;
  if (!(feature->GetValue(tmp) == VmbErrorSuccess)) return false;
  value = tmp;
  return true;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const int64_t &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<long long>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const uint64_t &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<long long>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr& feature, const int& value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<long long>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr& feature, const double& value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr& feature, const float& value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<double>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const std::string &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value.c_str()) == VmbErrorSuccess;
}
