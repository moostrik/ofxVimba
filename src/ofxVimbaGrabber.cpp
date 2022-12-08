#include "ofxVimbaGrabber.h"

void ofxVimbaGrabber::updateFrame() {
  bNewFrame = false;
  bResolutionChanged = false;
  bPixelFormatChanged = false;

  std::shared_ptr<ofPixels> newPixels = nullptr;
  {
    std::lock_guard<std::mutex> lock(frameMutex);
    newPixels.swap(receivedPixels);
  }

  if (newPixels) {
    auto w = int(newPixels->getWidth());
    auto h = int(newPixels->getHeight());
    auto f = newPixels->getPixelFormat();
    bResolutionChanged = (width != w || height != h);
    width = w;
    height = h;
    bPixelFormatChanged = (pixelFormat != f);
    pixelFormat = f;
    pixels.swap(newPixels);
    bNewFrame = true;
  }
}

void ofxVimbaGrabber::streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) {
  auto newPixels = std::make_shared<ofPixels>();
  auto format = ofxVimbaUtils::getOfPixelFormat(frame->getImageFormat());
  if (format == OF_PIXELS_UNKNOWN) return;

  // The data from the frame should NOT be used outside the scope of this function.
  // The setFromPixels method copies the pixel data.
  newPixels->setFromPixels(frame->getImageData(), frame->getWidth(), frame->getHeight(), format);

  std::lock_guard<std::mutex> lock(frameMutex);
  receivedPixels.swap(newPixels);
}


void ofxVimbaGrabber::setDesiredPixelFormat(ofPixelFormat format) {
  Grabber::setDesiredPixelFormat(ofxVimbaUtils::getVimbaPixelFormat(format));
}


std::vector<ofVideoDevice> ofxVimbaGrabber::listDevices() const {
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
