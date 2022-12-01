#include "ofxVimbaGrabber.h"

using namespace ofxVimba;

ofxVimbaGrabber::ofxVimbaGrabber() :
  logger("ofxVimbaGrabber"),
  system(OosVimba::System::getInstance()),
  deviceID(OosVimba::DISCOVERY_ANY_ID),
  width(0), height(0),
  xOffset(0), yOffset(0),
  framerate(0),
  pixelFormat (OF_PIXELS_RGB),
  bMulticast(false), bReadOnly(false), bUserSetLoad(false),
  bInited(false),
  bNewFrame(false),
  bIsConnected(false),
  bResolutionChange(false)
{
  listCameras(false);
}

bool ofxVimbaGrabber::setup(int _w, int _h) {
  startDiscovery();

  width = _w;
  height = _h;

  pixels = std::make_shared<ofPixels>();
  pixels->allocate(width, height, pixelFormat);

  bInited = true;
  return true;
}

void ofxVimbaGrabber::update() {
  bIsConnected = isConnected();
  bNewFrame = false;
  bResolutionChange = false;

  std::shared_ptr<ofPixels> newPixels = nullptr;
  {
    std::lock_guard<std::mutex> lock(frameMutex);
    newPixels.swap(receivedPixels);
  }

  if (newPixels) {
    if (newPixels->getPixelFormat() == OF_PIXELS_UNKNOWN) {
      ofLogWarning("onFrame") << "Received unknown pixel format";
      return;
    }

    bResolutionChange = (width != int(newPixels->getWidth()) || height != int(newPixels->getHeight()));
    width = newPixels->getWidth();
    height = newPixels->getHeight();
    pixelFormat = newPixels->getPixelFormat();

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
    }

    if (!activeDevice && !SP_ISNULL(next->getHandle())) {
      openDevice(next);
    }

    listCameras(false);
  }
}

void ofxVimbaGrabber::close() {
  stopDiscovery();
  closeDevice();
}

// -- DISCOVERY ----------------------------------------------------------------

void ofxVimbaGrabber::startDiscovery() {
  if (!discovery) {
    discovery = std::make_shared<OosVimba::Discovery>();
    std::function<void(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger)> callback = std::bind(&ofxVimbaGrabber::triggerCallback, this, std::placeholders::_1, std::placeholders::_2);
    discovery->setTriggerCallback(callback);
  }
  discovery->requestID(deviceID);
  discovery->start();
}

void ofxVimbaGrabber::stopDiscovery() {
  if (discovery) {
    discovery->setTriggerCallback();
    discovery = nullptr;
  }
}


void ofxVimbaGrabber::triggerCallback(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger trigger) {
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
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (activeDevice && SP_ISEQUAL(activeDevice->getHandle(), device->getHandle())) {
    // We might already be connected to this camera, do nothing
    return;
  }

  if (!filterDevice(device)) return;

  discoveredDevice = device;
}

void ofxVimbaGrabber::onDiscoveryUpdate(std::shared_ptr<OosVimba::Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (activeDevice) {
    if (SP_ISEQUAL(activeDevice->getHandle(), device->getHandle()) &&
        !device->isAvailable()) {
      // Our access mode dropped, schedule a disconnect
      discoveredDevice = std::make_shared<OosVimba::Device>(AVT::VmbAPI::CameraPtr());
    }
  } else if (filterDevice(device)) {
    discoveredDevice = device;
  }
}

void ofxVimbaGrabber::onDiscoveryLost(std::shared_ptr<OosVimba::Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (!activeDevice || !SP_ISEQUAL(activeDevice->getHandle(), device->getHandle()))
    return;
  discoveredDevice = std::make_shared<OosVimba::Device>(AVT::VmbAPI::CameraPtr());
}

// -- DEVICE -------------------------------------------------------------------

bool ofxVimbaGrabber::filterDevice(std::shared_ptr<OosVimba::Device> &device) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  if (deviceID != OosVimba::DISCOVERY_ANY_ID && deviceID != device->getId()) {
    return false;
  }

  OosVimba::AccessMode requestedAccesMode = bReadOnly ? OosVimba::AccessModeRead : OosVimba::AccessModeMaster;

  if (device->getAvailableAccessMode() < requestedAccesMode) {
    ofLogWarning("ofxVimbaGrabber::openDevice") << device->getId() << " acces mode " << requestedAccesMode <<
                                    " not available. Availability is " << device->getAvailableAccessMode();
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
    ofLogWarning("ofxVimbaGrabber::openDevice") << device->getId() << " acces mode " << requestedAccesMode <<
                                    " not available. Availability is " << device->getAvailableAccessMode();
    return;
  }

  if (!device->open(requestedAccesMode)) {
    ofLogWarning("ofxVimbaGrabber::openDevice") << "unable to open" << device->getId() <<
                                                   " with acces mode " << requestedAccesMode;
    return;
  }

  activeDevice = device;

  // retain the current id
  if (deviceID == OosVimba::DISCOVERY_ANY_ID) deviceID = activeDevice->getId();
  logger.setScope(activeDevice->getId());

  if(!bReadOnly) configureDevice(activeDevice);

  startStream();

  logger.notice("Opened camera connection");
  if (bReadOnly) {
    logger.notice("running in read only mode");
  }
}

void ofxVimbaGrabber::closeDevice() {
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (stream) stopStream();
  if (activeDevice) {

    activeDevice->close();
    activeDevice = nullptr;

    logger.notice("Closed camera connection");
    logger.clearScope();
  }
}

void ofxVimbaGrabber::configureDevice(std::shared_ptr<OosVimba::Device> &device) {

  activeDevice->run("GVSPAdjustPacketSize");
  VmbInt64_t GVSPPacketSize;
  activeDevice->get("GVSPPacketSize", GVSPPacketSize);
  ofLogVerbose("ofxVimbaGrabber::openDevice") << "Packet size set to " << GVSPPacketSize;

  device->set("MulticastEnable", bMulticast);

  if (bUserSetLoad) {
    activeDevice->run("UserSetLoad");
  } else {
    device->set("TriggerSource", "FixedRate");
    device->set("AcquisitionMode", "Continuous");
    device->set("ChunkModeActive", "1");
    device->set("PixelFormat", ofxVimbaUtils::getVimbaPixelFormat(pixelFormat));
  }

  string vmbPixelFormat;
  device->get("PixelFormat", vmbPixelFormat);
  auto ofPixelFormat = ofxVimbaUtils::getOfPixelFormat(vmbPixelFormat);

  if (pixelFormat != ofPixelFormat) {
    logger.notice("pixel format set to " + ofToString(ofPixelFormat));
    pixelFormat = ofPixelFormat;
  }

  setWidth(width);
  setHeight(height);
  setFrameRate(framerate);
}

// -- STREAM -------------------------------------------------------------------

bool ofxVimbaGrabber::startStream() {
  if (stream) return true;

  if (activeDevice) {
    stream = std::make_shared<OosVimba::Stream>(activeDevice);
    std::function<void(const std::shared_ptr<OosVimba::Frame>)> callback = std::bind(&ofxVimbaGrabber::frameCallBack, this, std::placeholders::_1);
    stream->setFrameCallback(callback);
//    ofAddListener(stream->onFrame, this, &ofxVimbaGrabber::onFrame);

    stream->start();
    return true;
  }
  return false;
}

void ofxVimbaGrabber::stopStream() {
  if (stream) {
    stream->setFrameCallback();
//    ofRemoveListener(stream->onFrame, this, &ofxVimbaGrabber::onFrame);

    stream->stop();
    stream = nullptr;
  }
}

void ofxVimbaGrabber::frameCallBack(const std::shared_ptr<OosVimba::Frame> frame) {
  auto newPixels = std::make_shared<ofPixels>();

  // setFromPixels copies the pixel data, as the data from the frame should not be used outside of this scope;
  newPixels->setFromPixels(frame->getImageData(), frame->getWidth(), frame->getHeight(), ofxVimbaUtils::getOfPixelFormat(frame->getImageFormat()));

  std::lock_guard<std::mutex> lock(frameMutex);
  receivedPixels.swap(newPixels);
}

// -- SET BEFORE SETUP ---------------------------------------------------------

void ofxVimbaGrabber::setDeviceID(int _simpleDeviceID) {
  setDeviceID(intIdToHexId(_simpleDeviceID));
}

void ofxVimbaGrabber::setDeviceID(string _deviceID) {
  if (bInited) {
    logger.warning("Cannot set device ID when inizialized");
    return;
  }
  deviceID = _deviceID;
}

bool ofxVimbaGrabber::setPixelFormat(ofPixelFormat value) {
  if (bInited) {
    logger.warning("Cannot set pixelformat when inizialized");
    return false;
  }
  pixelFormat = value;
  return true;
}

void ofxVimbaGrabber::enableMulticast() {
  if (bInited) {
    logger.warning("Cannot enable multicast when inizialized");
    return;
  }

  if (bReadOnly) {
    logger.warning("Cannot enable multicast when in read only mode");
    return;
  }
  bMulticast = true;
}

void ofxVimbaGrabber::enableReadOnly() {
  if (bInited) {
    logger.warning("Cannot enable read only access mode when inizialized");
    return;
  }

  if (bMulticast) {
    logger.warning(
        "Cannot enable read only access mode when multicast is enabled");
    return;
  }

  bReadOnly = true;
}

void ofxVimbaGrabber::enableUserSetLoad() {
  if (bInited) {
    logger.warning("Cannot enable user set load when inizialized");
    return;
  }

  bUserSetLoad = true;
}

// -- SET ----------------------------------------------------------------------

void ofxVimbaGrabber::setWidth(int value) {
  if (isConnected()) {
    activeDevice->set("Width", value);
    int v;
    activeDevice->get("Width", v);
    if (v != value) logger.notice("Width set to " + ofToString(v));
  } else {
    width = value;
  }
}

void ofxVimbaGrabber::setHeight(int value) {
  if (isConnected()) {
    activeDevice->set("Height", value);
    int v;
    activeDevice->get("Height", v);
    if (v != value) logger.notice("Height set to " + ofToString(v));
  } else {
    height = value;
  }
}

void ofxVimbaGrabber::setFrameRate(int value) {
  if (isConnected()) {
    activeDevice->set("AcquisitionFrameRateAbs", value);
    float v;
    activeDevice->get("AcquisitionFrameRateAbs", v);
    if (v != value) logger.notice("Framerate set to " + ofToString(v));
    framerate = v;
  } else {
    framerate = value;
  }
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
