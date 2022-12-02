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
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<long long>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, long &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<long>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned long long &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<unsigned long long>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned long &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<unsigned long>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, int &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<int>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned int &value) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<unsigned int>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, double &value) {
  if (SP_ISNULL(feature)) return false;
  double tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = tmp;
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, float &value) {
  if (SP_ISNULL(feature)) return false;
  double tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = static_cast<float>(tmp);
  return true;
}

template <>
bool OosVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, std::string &value) {
  if (SP_ISNULL(feature)) return false;
  std::string tmp;
  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
  value = tmp;
  return true;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const long long &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<VmbInt64_t>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned long long &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<VmbInt64_t>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const long &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<VmbInt64_t>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned long &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<VmbInt64_t>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const int &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<long long>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned int &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<VmbInt64_t>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const bool &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const double &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const float &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(static_cast<double>(value)) == VmbErrorSuccess;
}

template <>
bool OosVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature, const std::string &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value.c_str()) == VmbErrorSuccess;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, long long &min,  long long &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<long long>(vMin);
  max = static_cast<long long>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned long long &min, unsigned long long &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<unsigned long long>(vMin);
  max = static_cast<unsigned long long>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, long &min,  long &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<long>(vMin);
  max = static_cast<long>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned long &min, unsigned long &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<unsigned long>(vMin);
  max = static_cast<unsigned long>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, int &min, int &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<int>(vMin);
  max = static_cast<int>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned int &min, unsigned int &max) {
  if (SP_ISNULL(feature)) return false;
  VmbInt64_t vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<unsigned int>(vMin);
  max = static_cast<unsigned int>(vMax);;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, double &min,  double &max) {
  if (SP_ISNULL(feature)) return false;
  double vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = vMin;
  max = vMax;
  return true;
}

template <>
bool OosVimba::getRange(const AVT::VmbAPI::FeaturePtr &feature, float &min, float &max) {
  if (SP_ISNULL(feature)) return false;
  double vMin, vMax;
  auto error = feature->GetRange(vMin, vMax);
  if (error != VmbErrorSuccess) return false;
  min = static_cast<float>(vMin);
  max = static_cast<float>(vMax);;
  return true;
}

template <>
bool OosVimba::getOptions(const AVT::VmbAPI::FeaturePtr &feature, std::vector<std::string>& options) {
  if (feature->GetValues(options) != VmbErrorSuccess) return false;
  return true;
}

template <>
bool OosVimba::isOptionAvailable(const AVT::VmbAPI::FeaturePtr &feature, const std::string& option) {
  bool isAvailable = false;
  auto error = feature->IsValueAvailable(option.c_str(), isAvailable);
  if (error != VmbErrorSuccess) return false;
  return isAvailable;
}

