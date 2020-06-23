#include "Frame.h"

using namespace ofxVimba;

Frame::~Frame() {
  if (!SP_ISNULL(ancilleryData)) ancilleryData->Close();
}

bool Frame::getAncillaryFeature(const std::string& name,
                                AVT::VmbAPI::FeaturePtr& feature) const {
  if (SP_ISNULL(ancilleryData)) return false;
  return ancilleryData->GetFeatureByName(name.c_str(), feature) ==
         VmbErrorSuccess;
}

bool Frame::load(const AVT::VmbAPI::FramePtr& framePtr) {
  auto error = framePtr->GetPixelFormat(format);
  if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract pixel format from frame",
                    error);
    return false;
  }

  VmbUint64_t value;
  error = framePtr->GetFrameID(value);
  if (error == VmbErrorSuccess) {
    id = static_cast<uint64_t>(value);
  } else if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract id from frame", error);
    return false;
  }

  error = framePtr->GetTimestamp(value);
  if (error == VmbErrorSuccess) {
    timestamp = static_cast<uint64_t>(value);
  } else if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract timestamp from frame", error);
    return false;
  }

  error = framePtr->GetWidth(width);
  if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract width from frame", error);
    return false;
  }

  error = framePtr->GetHeight(height);
  if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract height from frame", error);
    return false;
  }

  error = framePtr->GetImageSize(size);
  if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract size from frame", error);
    return false;
  }

  error = framePtr->GetImage(data);
  if (error != VmbErrorSuccess) {
    Logger::warning("Frame", "Failed to extract data from frame", error);
    return false;
  }

  // Attempt to extract ancillery data
  if (framePtr->GetAncillaryData(ancilleryData) == VmbErrorSuccess) {
    if (ancilleryData->Open() == VmbErrorSuccess) {
      ancilleryData->GetFeatures(ancilleryFeatures);
    } else {
      SP_RESET(ancilleryData);
    }
  }

  // Populate the current pixels
  pixels.setFromPixels(data, width, height, getPixelFormat());

  return true;
}

ofPixelFormat Frame::getPixelFormat() {
  switch (format) {
    case VmbPixelFormatMono8:
      return OF_PIXELS_GRAY;
    case VmbPixelFormatRgb8:
      return OF_PIXELS_RGB;
    case VmbPixelFormatBgr8:
      return OF_PIXELS_BGR;
    case VmbPixelFormatRgba8:
      return OF_PIXELS_RGBA;
    case VmbPixelFormatBgra8:
      return OF_PIXELS_BGRA;
    default:
      return OF_PIXELS_UNKNOWN;
  }
}
