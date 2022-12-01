#pragma once

#include <memory>

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Logger.h"

namespace OosVimba {

enum AccessMode {
  AccessModeNone = -1,
  AccessModeAuto = 0,
  AccessModeRead = 1,
  AccessModeMaster = 2
};

bool isAccessModeAvailable(const AccessMode &requested, const AccessMode &available);
bool isAccessModeAvailable(const AccessMode &requested, const VmbAccessModeType &available);

AccessMode translateAccessMode(const VmbAccessModeType &modes);
VmbAccessModeType translateAccessMode(const AccessMode &mode);

// Get features
template <typename ValueType>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, ValueType &value) {
  return false;
}

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, int64_t &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, uint64_t &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr& feature, int& value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr& feature, double& value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr& feature, float& value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr& feature, std::string& value);

// Set features
template <typename ValueType>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const ValueType &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value) == VmbErrorSuccess;
}

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const int64_t &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const uint64_t &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr& feature, const int& value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr& feature, const double& value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr& feature, const float& value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const std::string &value);

}  // namespace OosVimba
