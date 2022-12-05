#include "ofxVimbaGrabber.h"

using namespace ofxVimba;

ofxVimbaGrabber::ofxVimbaGrabber() :
  logger("ofxVimbaGrabber "),
  system(OosVimba::System::getInstance()),
  deviceID(OosVimba::DISCOVERY_ANY_ID), // equals to ""
  width(0), height(0),
  framerate(0), 
  desiredFrameRate(OosVimba::MAX_FRAMERATE), // equals to 1000
  pixelFormat(OF_PIXELS_UNKNOWN),
  desiredPixelFormat(OF_PIXELS_NATIVE),
  bMulticast(false), 
  bReadOnly(false), 
  userSet(-1),
  bNewFrame(false),
  bIsConnected(false),
  bResolutionChanged(false),
  bPixelFormatChanged(false)
{
  listCameras(false);
}

bool ofxVimbaGrabber::setup(int _w, int _h) {
  startDiscovery();
  pixels = std::make_shared<ofPixels>();
  return true;
}

void ofxVimbaGrabber::update() {
  bIsConnected = isConnected();
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
    height =h;
    bPixelFormatChanged = (pixelFormat != f);
    pixelFormat = f;
    pixels.swap(newPixels);
    bNewFrame = true;
  }

  std::unique_lock<std::mutex> lock(deviceMutex, std::try_to_lock);
  if (lock.owns_lock() && discoveredDevice) {
    auto next = discoveredDevice;
    // Reset our queued device and release the lock again
    discoveredDevice = nullptr;
    lock.unlock();

    if (activeDevice) {
      closeDevice();
      startDiscovery();
    }

    if (!activeDevice && !SP_ISNULL(next->getHandle())) openDevice(next);
    listCameras(false);
  }
}

void ofxVimbaGrabber::close() {
  stopDiscovery();
  closeDevice();
}

// -- DISCOVERY ----------------------------------------------------------------

void ofxVimbaGrabber::startDiscovery() {
  if (discovery) discovery->stop();
  if (!discovery) {
    discovery = std::make_shared<OosVimba::Discovery>();
    std::function<void(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger)> callback = std::bind(&ofxVimbaGrabber::discoveryCallback, this, std::placeholders::_1, std::placeholders::_2);
    discovery->setTriggerCallback(callback);
  }
  discovery->requestID(deviceID);
  discovery->start();
}

void ofxVimbaGrabber::stopDiscovery() {
  if (discovery) {
    discovery->setTriggerCallback();
    discovery->stop();
    discovery = nullptr;
  }
}

void ofxVimbaGrabber::discoveryCallback(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger trigger) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  switch (trigger) {
    case OosVimba::OOS_DISCOVERY_PLUGGED_IN:
      onDiscoveryFound(device);
      break;
    case OosVimba::OOS_DISCOVERY_PLUGGED_OUT:
      onDiscoveryLost(device);
      break;
    case OosVimba::OOS_DISCOVERY_STATE_CHANGED:
      onDiscoveryUpdate(device);
      break;
    default:
      break;
  }
}

void ofxVimbaGrabber::onDiscoveryFound(std::shared_ptr<OosVimba::Device> &device) {

  if (activeDevice && SP_ISEQUAL(activeDevice->getHandle(), device->getHandle())) {
    logger.warning("Discovered device is already active");
    // We might already be connected to this camera, do nothing
    return;
  }

  if (!filterDevice(device)) return;

  discoveredDevice = device;
  logger.verbose("Discovered Device " + device->getId());
}

void ofxVimbaGrabber::onDiscoveryUpdate(std::shared_ptr<OosVimba::Device> &device) {
  if (activeDevice) {
    if (SP_ISEQUAL(activeDevice->getHandle(), device->getHandle()) && !(bReadOnly || device->isAvailable())) {
      discoveredDevice = std::make_shared<OosVimba::Device>(AVT::VmbAPI::CameraPtr());
      logger.notice("Discovery Update : Our access mode dropped, schedule a disconnect for ", device->getId());
    }
  } else if (filterDevice(device)) {
    discoveredDevice = device;
    logger.verbose("Discovery Update: Device becomes available ", device->getId());
  }
}

void ofxVimbaGrabber::onDiscoveryLost(std::shared_ptr<OosVimba::Device> &device) {
  if (!activeDevice || !SP_ISEQUAL(activeDevice->getHandle(), device->getHandle())) return;
  discoveredDevice = std::make_shared<OosVimba::Device>(AVT::VmbAPI::CameraPtr());
  logger.notice("Discovery Update: Device lost ", device->getId());
}

// -- DEVICE -------------------------------------------------------------------

bool ofxVimbaGrabber::filterDevice(std::shared_ptr<OosVimba::Device> &device) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  if (deviceID != OosVimba::DISCOVERY_ANY_ID && deviceID != device->getId()) {
    return false;
  }

  OosVimba::AccessMode requestedAccesMode = bReadOnly ? OosVimba::AccessModeRead : OosVimba::AccessModeMaster;

  if (device->getAvailableAccessMode() < requestedAccesMode) {
    logger.warning("filterDevice " + device->getId() + " acces mode " + ofToString(requestedAccesMode) +
      " is not available. Availability is " + ofToString(device->getAvailableAccessMode()));
    return false;
  }

  return true;
}

void ofxVimbaGrabber::openDevice(std::shared_ptr<OosVimba::Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);

  // We need to be disconnected, and have a next device
  if (activeDevice || !device || SP_ISNULL(device->getHandle())) return;
  if (deviceID != OosVimba::DISCOVERY_ANY_ID && deviceID != device->getId()) return;
  OosVimba::AccessMode requestedAccesMode = bReadOnly ? OosVimba::AccessModeRead : OosVimba::AccessModeMaster;

  if (!isAccessModeAvailable(requestedAccesMode, device->getAvailableAccessMode())) {
    logger.warning("openDevice " + device->getId() + " acces mode " + ofToString(requestedAccesMode) +
      " is not available. Availability is " + ofToString(device->getAvailableAccessMode()));
    return;
  }

  if (!device->open(requestedAccesMode)) {
    logger.warning("openDevice : unable to open" + device->getId() +" with acces mode " + ofToString(requestedAccesMode));
    return;
  }

  activeDevice = device;

  // retain the current id
  if (deviceID == OosVimba::DISCOVERY_ANY_ID) deviceID = activeDevice->getId();
  logger.setScope(activeDevice->getId());

  if(!bReadOnly) configureDevice(activeDevice);

  startStream();

  if (!bReadOnly) logger.verbose("Opened connection");
  else logger.notice("Opened read only connection");
}

void ofxVimbaGrabber::closeDevice() {
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (stream) stopStream();
  if (activeDevice) {

    activeDevice->close();
    activeDevice = nullptr;

    if (!bReadOnly) logger.verbose("Closed connection");
    else logger.notice("Closed read only connection");

    logger.clearScope();
  }
}

void ofxVimbaGrabber::configureDevice(std::shared_ptr<OosVimba::Device> &device) {

  activeDevice->run("GVSPAdjustPacketSize");
  VmbInt64_t GVSPPacketSize;
  activeDevice->get("GVSPPacketSize", GVSPPacketSize);
  logger.verbose("Packet size set to " + ofToString(GVSPPacketSize));

  device->set("MulticastEnable", bMulticast);

  if (userSet.load() >= 0) {
    device->set("UserSetSelector", getUserSetString(userSet));
    activeDevice->run("UserSetLoad");
  }
  
  //device->set("ChunkModeActive", true);
  
  if (desiredPixelFormat >= 0) device->set("PixelFormat", ofxVimbaUtils::getVimbaPixelFormat(desiredPixelFormat));  
  string vmbPixelFormat;
  device->get("PixelFormat", vmbPixelFormat);
  pixelFormat = ofxVimbaUtils::getOfPixelFormat(vmbPixelFormat);

  if (pixelFormat < 0) logger.warning("pixel format set to " + ofToString(pixelFormat));
  else if (pixelFormat != desiredPixelFormat) logger.notice("pixel format set to " + ofToString(pixelFormat));

  setFrameRate(desiredFrameRate.load());
  logger.notice("Device Configured");
}

void ofxVimbaGrabber::setFrameRate(double value) {
  auto device = activeDevice;
  if (device == nullptr) return;

  double min, max;
  device->getRange("AcquisitionFrameRateAbs", min, max);
  framerate.store(ofClamp(value, min + 0.1, max - 0.1));
  device->set("AcquisitionFrameRateAbs", framerate);
  double fr;
  device->get("AcquisitionFrameRateAbs", fr);
  framerate.store(fr);

  if (framerate != value) {
    logger.notice("framerate set to " + ofToString(framerate));
  }

}

// -- STREAM -------------------------------------------------------------------

bool ofxVimbaGrabber::startStream() {
  if (stream) return true;

  if (activeDevice) {
    stream = std::make_shared<OosVimba::Stream>(activeDevice);
    std::function<void(const std::shared_ptr<OosVimba::Frame>)> callback = std::bind(&ofxVimbaGrabber::streamFrameCallBack, this, std::placeholders::_1);
    stream->setFrameCallback(callback);
    stream->start();
    return true;
  }
  return false;
}

void ofxVimbaGrabber::stopStream() {
  if (stream) {
    stream->setFrameCallback();
    stream->stop();
    stream = nullptr;
  }
}

void ofxVimbaGrabber::streamFrameCallBack(const std::shared_ptr<OosVimba::Frame> frame) {
  auto newPixels = std::make_shared<ofPixels>();
  auto format = ofxVimbaUtils::getOfPixelFormat(frame->getImageFormat());
  if (format == OF_PIXELS_UNKNOWN) {
    //ofLogWarning("streamFrameCallBack()") << "Received unknown pixel format";
    return;
  }

  // The data from the frame should NOT be used outside the scope of this function.
  // The setFromPixels method copies the pixel data.
  newPixels->setFromPixels(frame->getImageData(), frame->getWidth(), frame->getHeight(), format);
  
  std::lock_guard<std::mutex> lock(frameMutex);
  receivedPixels.swap(newPixels);
}

// -- SET ----------------------------------------------------------------------

void ofxVimbaGrabber::setVerbose(bool bTalkToMe) {
  if (bTalkToMe) logger.setLevel(OosVimba::VMB_LOG_VERBOSE);
  else logger.setLevel(OosVimba::VMB_LOG_NOTICE);
}

void ofxVimbaGrabber::setDeviceID(int simpleID) {
  setDeviceID(intIdToHexId(simpleID));
}

void ofxVimbaGrabber::setDeviceID(string ID) {
  if (ID == deviceID) return;
  if (!isConnected()) {
    std::lock_guard<std::mutex> lock(deviceMutex);
    deviceID = ID;
    return;
  }

  stopDiscovery();
  closeDevice();
  {
    std::lock_guard<std::mutex> lock(deviceMutex);
    deviceID = ID;
  }
  startDiscovery();
}

void ofxVimbaGrabber::setReadOnly(bool value) {
  if (value == bReadOnly.load()) return;
  if (!isConnected()) {
    bReadOnly.store(value);
    return;
  }
  stopDiscovery();
  closeDevice();
  bReadOnly.store(value);
  startDiscovery();
}

void ofxVimbaGrabber::setMulticast(bool value) {
  if (value == bMulticast.load()) return;
  if (!isConnected()) {
    bMulticast.store(value);
    return;
  }
  std::lock_guard<std::mutex> lock(deviceMutex);
  stopStream();
  bMulticast.store(value);
  configureDevice(activeDevice);
  startStream();
  logger.notice("multicast set to", ofToString(bMulticast));
}

bool ofxVimbaGrabber::setPixelFormat(ofPixelFormat format) {
  if (format == desiredPixelFormat) return true;
  if (!isConnected()) {
    desiredPixelFormat = format;
    return false; // the format is not successfully changed YET
  }
  std::lock_guard<std::mutex> lock(deviceMutex);
  stopStream();
  desiredPixelFormat = format;
  configureDevice(activeDevice);
  startStream();
  return true;
}

void ofxVimbaGrabber::setLoadUserSet(int setToLoad) {
  userSet.store(setToLoad);
  if (!isConnected()) return;

  std::lock_guard<std::mutex> lock(deviceMutex);
  stopStream();
  configureDevice(activeDevice);
  startStream();
  //logger.notice("loaded user set", ofToString(userSet.load()));
}

void ofxVimbaGrabber::setDesiredFrameRate(int frameRate) {
  desiredFrameRate.store(frameRate);
  if (!isConnected()) return;
  setFrameRate(desiredFrameRate.load());
}

// -- LIST ---------------------------------------------------------------------

std::vector<ofVideoDevice> ofxVimbaGrabber::listDevices() const {
  printCameras();
  return ofDevices;
}

void ofxVimbaGrabber::listCameras(bool _verbose) {
  if (!system->isAvailable()) {
    logger.error(
        "Failed to retrieve current camera list, system is unavailable");
    return;
  }

  AVT::VmbAPI::CameraPtrVector cameras;
  auto err = system->getAPI().GetCameras(cameras);

  if (err != VmbErrorSuccess) {
    logger.error("Failed to retrieve current camera list");
    return;
  }

  ofDevices.clear();

  for (auto cam : cameras) {
    auto device = std::make_shared<OosVimba::Device>(cam);
    ofVideoDevice ofDevice;
    // convert to int to make compatable with ofVideoDevice
    ofDevice.id = hexIdToIntId(device->getId());
    ofDevice.deviceName = device->getModel();
    ofDevice.hardwareName = "Allied Vision";
    ofDevice.serialID = device->getSerial();
    ofDevice.bAvailable =
        (device->getAvailableAccessMode() == OosVimba::AccessModeMaster);
    ofDevices.push_back(ofDevice);
  }

  if (_verbose) {
    printCameras();
  }
}

void ofxVimbaGrabber::printCameras() const {
  std::ostringstream out;
  out << endl;
  out << "##########################################################################################";
  out << endl;
  out << "##             LISTING CAMERAS" << endl;
  for (int i = 0; i < (int)ofDevices.size(); i++) {
    auto dev = ofDevices[i];
    out << "##  " << i << "  name: " << dev.deviceName.c_str()
         << "  simple id: " << dev.id << "  id: " << intIdToHexId(dev.id)
         << "  available: " << dev.bAvailable << endl;
  }
  if (ofDevices.size() == 0) {
    out << "## no cameras found" << endl;
  }

  out << "##" << endl;
  out << "##########################################################################################";
  out << endl;
  out << endl;
  cout << out.str() << endl;
}

// -- TOOLS --------------------------------------------------------------------

int ofxVimbaGrabber::hexIdToIntId(string value) const {
  int intId;
  std::stringstream ss;
  // Last 6 hex values: the unique ID as used in the predecessor of Vimba, PvAPI
  string uniqueID = value.substr(10, 6);
  ss << std::hex << uniqueID;
  ss >> intId;
  return intId;
}

string ofxVimbaGrabber::intIdToHexId(int _intId) const {
  std::stringstream ss;
  ss << std::uppercase << std::hex << _intId;
  std::string stringId(ss.str());
  while (stringId.length() < 6) {
    stringId = "0" + stringId;
  }
  stringId = "DEV_000F31" + stringId;
  return stringId;
}

string ofxVimbaGrabber::getUserSetString(int userSet) const {
  std::string us = "Default";
  if (userSet >= 1 && userSet <= 3) {
    us = "UserSet" + ofToString(userSet);
  }
  return us;
}