#pragma once

#include "ofMain.h"
#include "VimbaCPP/Include/VimbaCPP.h"

class ofxVimbaUtils {
public:

static inline string getVimbaPixelFormat(ofPixelFormat format) {
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

static inline ofPixelFormat getOfPixelFormat(VmbPixelFormatType format) {
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

static inline ofPixelFormat getOfPixelFormat(string format) {
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

};

