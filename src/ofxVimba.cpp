// Copyright (C) 2022 Matthias Oostrik

#include "ofxVimba.h"

using namespace ofxVimba;

void Grabber::updateFrame() {
  bNewFrame = false;
  std::shared_ptr<ofPixels> newPixels = nullptr;
  {
    std::lock_guard<std::mutex> lock(frameMutex);
    newPixels.swap(receivedPixels);
  }
  if (newPixels) {
    pixels.swap(newPixels);
    bNewFrame = true;
  }
}

void Grabber::streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) {
  auto newPixels = std::make_shared<ofPixels>();
  auto format = getOfPixelFormat(frame->getImageFormat());
  if (format == OF_PIXELS_UNKNOWN) return;

  // The data from the frame should NOT be used outside the scope of this function.
  // The setFromPixels method copies the pixel data.
  newPixels->setFromPixels(frame->getImageData(), frame->getWidth(), frame->getHeight(), format);

  std::lock_guard<std::mutex> lock(frameMutex);
  receivedPixels.swap(newPixels);
}

void Grabber::setDesiredPixelFormat(ofPixelFormat format) {
  auto formatVMB = getVimbaPixelFormat(format);
  if (formatVMB == "unknown") {
    formatVMB = Grabber::getDesiredPixelFormat();
    logger->warning("setDesiredPixelFormat(): unknown pixel format, default to " + formatVMB);
  }
  OosVim::Grabber::setDesiredPixelFormat(formatVMB);
}

ofPixelFormat Grabber::getDesiredPixelFormat() {
  return getOfPixelFormat(OosVim::Grabber::getDesiredPixelFormat());
}


std::vector<ofVideoDevice> Grabber::listDevices() const {
  printDeviceList(deviceList);
  std::vector<ofVideoDevice> deviceListOF;
  for (auto& device : deviceList) {
    ofVideoDevice deviceOF;
    deviceOF.id = hexIdToIntId(device->getId());
    deviceOF.deviceName = device->getModel();
    deviceOF.hardwareName = "Allied Vision";
    deviceOF.serialID = device->getSerial();
    deviceOF.bAvailable = (device->getAvailableAccessMode() == OosVim::AccessModeMaster);
    deviceListOF.push_back(deviceOF);
  }
  return deviceListOF;
}
