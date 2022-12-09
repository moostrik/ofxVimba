// Copyright (C) 2022 Matthias Oostrik
//
// OpenFrameworks implementation of the OosVim Grabber
// Note that only 3 pixelformats are supported, others can be added

#pragma once

#include "ofMain.h"
#include "OosVim/Grabber.h"

namespace ofxVimba {

class Grabber : public OosVim::Grabber {
public:
  Grabber() : pixels(std::make_shared<ofPixels>()), bNewFrame(false) { }
  virtual ~Grabber() { OosVim::Grabber::stop(); }

  void setup() { OosVim::Grabber::start(); }
  void update() { bNewFrame = updateFrame(); }
  void close() { OosVim::Grabber::stop(); }

  bool isFrameNew() const { return bNewFrame; }

  const ofPixels& getPixels() const { return *pixels; }
  ofPixels& getPixels() { return *pixels; }

  void setDesiredPixelFormat(ofPixelFormat format);
  ofPixelFormat getDesiredPixelFormat();
  std::vector<ofVideoDevice> listDevices() const;

private:
  bool updateFrame() override;
  void streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) override;

  bool bNewFrame;
  std::mutex frameMutex;
  std::shared_ptr<ofPixels> pixels;
  std::shared_ptr<ofPixels> receivedPixels;
};

static inline string getVimbaPixelFormat(ofPixelFormat format) {
  switch (format) {
    case OF_PIXELS_GRAY:
      return "Mono8";
    case OF_PIXELS_RGB:
      return "RGB8Packed";
    case OF_PIXELS_BGR:
      return "BGR8Packed";
    default:
      return "unknown";
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
  }
  return OF_PIXELS_UNKNOWN;
}

}
