#include "Common.h"
#include "Logger.h"

using namespace ofxVimba;

bool ofxVimba::isAccessModeAvailable(const AccessMode &requested,
                                     const VmbAccessModeType &available) {
  auto mode = translateAccessMode(available);
  return isAccessModeAvailable(requested, mode);
};

bool ofxVimba::isAccessModeAvailable(const AccessMode &requested,
                                     const AccessMode &available) {
  return available >= requested;
};

AccessMode ofxVimba::translateAccessMode(const VmbAccessModeType &modes) {
  if (modes & VmbAccessModeFull) return AccessModeMaster;
  if (modes & VmbAccessModeRead) return AccessModeRead;

  return AccessModeNone;
};

VmbAccessModeType ofxVimba::translateAccessMode(const AccessMode &mode) {
  if (mode == AccessModeMaster) return VmbAccessModeFull;
  if (mode == AccessModeRead) return VmbAccessModeRead;

  return VmbAccessModeNone;
}


ofPixelFormat ofxVimba::toOfPixelFormat(string format) {
  if (format == "Mono8") {
    return OF_PIXELS_GRAY;
  } else if (format == "RGB8Packed") {
    return OF_PIXELS_RGB;
  } else if (format == "BGR8Packed") {
    return OF_PIXELS_BGR;
  } else if (format == "YUV422Packed") {
    return OF_PIXELS_YUY2;
  }
  ofLogWarning("ofxVimba::toOfPixelFormat") << "pixel format  "<< format << " not supported";
  return OF_PIXELS_UNKNOWN;
}

string ofxVimba::toVimbaPixelFormat(ofPixelFormat format) {
  switch (format) {
    case OF_PIXELS_GRAY:
      return "Mono8";
    case OF_PIXELS_RGB:
      return "RGB8Packed";
    case OF_PIXELS_BGR:
      return "BGR8Packed";
    case OF_PIXELS_YUY2:
      return "YUV422Packed";
    default:
    ofLogWarning("ofxVimba::toVimbaPixelFormat") << "pixel format  "<< format << " not supported";
      return "";
  }
}



template <>
bool ofxVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature, int &value) {
  long long tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<int>(tmp);
    return true;
  }

  return false;
}

#ifndef _MSC_VER
template <>
bool ofxVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
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
bool ofxVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          uint64_t &value) {
  long long tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<uint64_t>(tmp);
    return true;
  }

  return false;
}

template <>
bool ofxVimba::getFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          float &value) {
  double tmp;

  if (getFeature(feature, tmp)) {
    value = static_cast<float>(tmp);
    return true;
  }

  return false;
}

template <>
bool ofxVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const int &value) {
  return setFeature(feature, static_cast<long long>(value));
}

template <>
bool ofxVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const int64_t &value) {
  return setFeature(feature, static_cast<long long>(value));
}

template <>
bool ofxVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const uint64_t &value) {
  return setFeature(feature, static_cast<long long>(value));
}

template <>
bool ofxVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const float &value) {
  return setFeature(feature, static_cast<double>(value));
}

template <>
bool ofxVimba::setFeature(const AVT::VmbAPI::FeaturePtr &feature,
                          const std::string &value) {
  return setFeature(feature, value.c_str());
}
