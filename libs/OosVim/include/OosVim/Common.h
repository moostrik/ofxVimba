// Copyright (C) 2022 Matthias Oostrik

#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "VimbaCPP/Include/VimbaCPP.h"

namespace OosVim {

static const float MAX_FRAMERATE = 1000;

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

// Get Features
template <typename ValueType>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, ValueType &value) {
  std::cout << "OosVimba getFeature: Valuetype not recognized" << std::endl;
  return false;
//  if (SP_ISNULL(feature)) return false;
//  ValueType tmp;
//  if (feature->GetValue(tmp) != VmbErrorSuccess) return false;
//  value = (tmp);
//  return true;
}

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, long long &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned long long &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, long &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned long &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, int &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, unsigned int &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, bool &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, double &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, float &value);

template <>
bool getFeature(const AVT::VmbAPI::FeaturePtr &feature, std::string &value);


// Set Features
template <typename ValueType>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const ValueType &value) {
  if (SP_ISNULL(feature)) return false;
  return feature->SetValue(value) == VmbErrorSuccess;
}

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const long long &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned long long &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const long &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned long &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const int &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const unsigned int &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const bool &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const double &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const float &value);

template <>
bool setFeature(const AVT::VmbAPI::FeaturePtr &feature, const std::string &value);


// Get Feature Range
template <typename ValueType>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, ValueType &min, ValueType &max) {
  return false;
}

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, long long &min, long long &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned long long &min, unsigned long long &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, long &min, long &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned long &min, unsigned long &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, int &min, int &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, unsigned int &min, unsigned int &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, double &min, double &max);

template <>
bool getFeatureRange(const AVT::VmbAPI::FeaturePtr &feature, float &min, float &max);

// Options
template <typename ValueType>
bool getOptions(const AVT::VmbAPI::FeaturePtr &feature, std::vector<ValueType> &options) {
  return false;
}

template <typename ValueType>
bool isOptionAvailable(const AVT::VmbAPI::FeaturePtr &feature, const ValueType &option) {
  return false;
}

}  // namespace OosVimba
