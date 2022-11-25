#include "ofxVimbaGrabber.h"

using namespace ofxVimba;

ofxVimbaGrabber::ofxVimbaGrabber() :
  logger("ofxVimbaGrabber"), system(ofxVimba::System::getInstance()),
  deviceID (DISCOVERY_ANY_ID),
  width(0), height(0),
  xOffset(0), yOffset(0),
  framerate(0),
  pixelFormat (OF_PIXELS_RGB),
  bMulticast(false), bReadOnly(false), bUserSetLoad(false),
  bInited(false),
  bNewFrame(false),
  bIsConnected(false),
  bResolutionChange(false),
  frameCount(0)
{
  listCameras(false);
}

bool ofxVimbaGrabber::setup(int _w, int _h) {
  startDiscovery();

  width = _w;
  height = _h;

  bInited = true;
  return true;
}

void ofxVimbaGrabber::update() {
  bIsConnected = isConnected();
  bNewFrame = false;
  bResolutionChange = false;

  std::shared_ptr<ofxVimba::Frame> newFrame = nullptr;
  {
    std::lock_guard<std::mutex> lock(frameMutex);
    newFrame.swap(receivedFrame);
  }

  if (newFrame) {
    auto &source = newFrame->getPixels();
    if (source.getPixelFormat() == OF_PIXELS_UNKNOWN) {
      ofLogWarning("onFrame") << "Received unknown pixel format";
      return;
    }

    bResolutionChange = (width != int(source.getWidth()) || height != int(source.getHeight()));
    width = source.getWidth();
    height = source.getHeight();
    pixelFormat = source.getPixelFormat();

    // Allocate is applied only when resolution of pixelformat has changed
    pixels.allocate(width, height, pixelFormat);

    memcpy(pixels.getData(), source.getData(), source.getTotalBytes());

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
    discovery = std::make_shared<Discovery>();

    // Whenever a device is found or updated try to connect
    ofAddListener(discovery->onFound, this, &ofxVimbaGrabber::onDiscoveryFound);
    ofAddListener(discovery->onUpdate, this,
                  &ofxVimbaGrabber::onDiscoveryUpdate);

    // Whenever a device is lost disconnect
    ofAddListener(discovery->onLost, this, &ofxVimbaGrabber::onDiscoveryLost);
  }
  discovery->requestID(deviceID);
  discovery->start();
}

void ofxVimbaGrabber::stopDiscovery() {
  if (discovery) {
    ofRemoveListener(discovery->onFound, this,
                     &ofxVimbaGrabber::onDiscoveryFound);
    ofRemoveListener(discovery->onUpdate, this,
                     &ofxVimbaGrabber::onDiscoveryUpdate);
    ofRemoveListener(discovery->onLost, this,
                     &ofxVimbaGrabber::onDiscoveryLost);

    discovery = nullptr;
  }
}

void ofxVimbaGrabber::onDiscoveryFound(std::shared_ptr<Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (activeDevice && SP_ISEQUAL(activeDevice->getHandle(), device->getHandle())) {
    // We might already be connected to this camera, do nothing
    return;
  }

  if (!filterDevice(device)) return;

  discoveredDevice = device;
}

void ofxVimbaGrabber::onDiscoveryUpdate(std::shared_ptr<Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (activeDevice) {
    if (SP_ISEQUAL(activeDevice->getHandle(), device->getHandle()) &&
        !device->isAvailable()) {
      // Our access mode dropped, schedule a disconnect
      discoveredDevice = std::make_shared<Device>(AVT::VmbAPI::CameraPtr());
    }
  } else if (filterDevice(device)) {
    discoveredDevice = device;
  }
}

void ofxVimbaGrabber::onDiscoveryLost(std::shared_ptr<Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (!activeDevice || !SP_ISEQUAL(activeDevice->getHandle(), device->getHandle()))
    return;
  discoveredDevice = std::make_shared<Device>(AVT::VmbAPI::CameraPtr());
}

// -- DEVICE -------------------------------------------------------------------

bool ofxVimbaGrabber::filterDevice(std::shared_ptr<Device> &device) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  if (deviceID != DISCOVERY_ANY_ID && deviceID != device->getId()) {
    return false;
  }

  AccessMode requestedAccesMode = bReadOnly ? AccessModeRead : AccessModeMaster;

  if (device->getAvailableAccessMode() < requestedAccesMode) {
    logger.warning("requested access mode is not available");
    return false;
  }

  return true;
}

void ofxVimbaGrabber::openDevice(std::shared_ptr<Device> &device) {
  std::lock_guard<std::mutex> lock(deviceMutex);

  // We need to be disconnected, and have a next device
  if (activeDevice || !device || SP_ISNULL(device->getHandle())) return;
  if (deviceID != DISCOVERY_ANY_ID && deviceID != device->getId()) return;
  AccessMode requestedAccesMode = bReadOnly ? AccessModeRead : AccessModeMaster;

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
  if (deviceID == DISCOVERY_ANY_ID) deviceID = activeDevice->getId();
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

void ofxVimbaGrabber::configureDevice(std::shared_ptr<ofxVimba::Device> &device) {

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
    device->set("PixelFormat", toVimbaPixelFormat(pixelFormat));
  }

  string vmbPixelFormat;
  device->get("PixelFormat", vmbPixelFormat);
  auto v = toOfPixelFormat(vmbPixelFormat);

  if (v != toOfPixelFormat(vmbPixelFormat)) {
    logger.notice("pixel format set to " + ofToString(v));
    pixelFormat = v;
  }

  setWidth(width);
  setHeight(height);
  setFrameRate(framerate);
}

// -- STREAM -------------------------------------------------------------------

bool ofxVimbaGrabber::startStream() {
  if (stream) return true;

  if (activeDevice) {
    stream = std::make_shared<Stream>(activeDevice);
    ofAddListener(stream->onFrame, this, &ofxVimbaGrabber::onFrame);

    stream->start();
    return true;
  }
  return false;
}

void ofxVimbaGrabber::stopStream() {
  if (stream) {
    ofRemoveListener(stream->onFrame, this, &ofxVimbaGrabber::onFrame);

    stream->stop();
    stream = nullptr;
  }
}

void ofxVimbaGrabber::onFrame(const std::shared_ptr<ofxVimba::Frame> &frame) {
  int fc;
  if (frame->getAncillary("ChunkAcquisitionFrameCount", fc)) {
    frameCount = fc;
  }
  std::lock_guard<std::mutex> lock(frameMutex);
  receivedFrame = frame;
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
  } else {
    width = value;
  }
}

void ofxVimbaGrabber::setHeight(int value) {
  if (isConnected()) {
    activeDevice->set("Height", value);
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
    auto device = std::make_shared<Device>(cam);
    ofVideoDevice ofDevice;
    // convert to int to make compatable with ofVideoDevice
    ofDevice.id = hexIdToIntId(device->getId());
    ofDevice.deviceName = device->getModel();
    ofDevice.hardwareName = "Allied Vision";
    ofDevice.serialID = device->getSerial();
    ofDevice.bAvailable =
        (device->getAvailableAccessMode() == AccessModeMaster);
    ofDevices.push_back(ofDevice);
  }

  if (_verbose) {
    printCameras();
  }
}

void ofxVimbaGrabber::printCameras() const {
  cout << endl;
  cout << "####################################################################"
          "######################"
       << endl;
  cout << "##             LISTING CAMERAS" << endl;
  for (int i = 0; i < (int)ofDevices.size(); i++) {
    auto dev = ofDevices[i];
    cout << "##  " << i << "  name: " << dev.deviceName.c_str()
         << "  simple id: " << dev.id << "  id: " << intIdToHexId(dev.id)
         << "  available: " << dev.bAvailable << endl;
  }
  if (ofDevices.size() == 0) {
    cout << "## no cameras found" << endl;
  }

  cout << "##" << endl;
  cout << "####################################################################"
          "######################"
       << endl;
  cout << endl;
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
