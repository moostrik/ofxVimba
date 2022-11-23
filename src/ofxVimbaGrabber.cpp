#include "ofxVimbaGrabber.h"

using namespace ofxVimba;

ofxVimbaGrabber::ofxVimbaGrabber()
    : logger("ofxVimbaGrabber"), system(ofxVimba::System::getInstance()) {
  deviceID = DISCOVERY_ANY_ID;  // = ""
  pixelFormat = OF_PIXELS_RGB;
  bMulticast = false;
  bReadOnly = false;
  bVerbose = false;
  bInited = false;
  frameReceived = false;
  bNewFrame = false;
  pixelsVector.resize(30);
  framerate = 120;
  bWasConnected = false;
  bResolutionChange = false;
  frameCount = 0;
  xOffset = 0;
  yOffset = 0;
  listCameras(false);

  //parameters = std::make_shared<Parameters>();
  features = std::make_shared<Features>();
}

bool ofxVimbaGrabber::setup(int _w, int _h) {
  startDiscovery();

  width = _w;
  height = _h;
  setWidth(width);
  setHeight(height);

  bInited = true;
  if (!bReadOnly) {
    features->setOffline("TriggerSource", "FixedRate");
    features->setOffline("AcquisitionFrameRateAbs", ofToString(framerate));
    features->setOffline("AcquisitionMode", "Continuous");
    features->setOffline("ChunkModeActive", "1");
    features->setOffline("PixelFormat", toVimbaPixelFormat(pixelFormat));

    grabberParameters.setName("SaveLoadStream");
    grabberParameters.add(pSave.set("save", false));
    grabberParameters.add(pLoad.set("load", false));
    grabberParameters.add(pStream.set("stream", true));
    grabberParameters.add(pFrameCount.set("frame count", 0, 0, 0));
    pSave.addListener(this, &ofxVimbaGrabber::pSaveListener);
    pLoad.addListener(this, &ofxVimbaGrabber::pLoadListener);
    pStream.addListener(this, &ofxVimbaGrabber::pStreamListener);
  }
  return true;
}

void ofxVimbaGrabber::update() {
  bWasConnected = isConnected();
  bNewFrame = false;
  bResolutionChange = false;
  bROIChange = false;

  if (frameReceived) {
    pixels = pixelsVector[pixelIndex];
    bResolutionChange =
        width != int(pixels.getWidth()) || height != int(pixels.getHeight());
    bROIChange = bResolutionChange || xOffset != features->getInt("OffsetX") ||
                 yOffset != features->getInt("OffsetY");
    width = pixels.getWidth();
    height = pixels.getHeight();
    xOffset = features->getInt("OffsetX");
    yOffset = features->getInt("OffsetY");

    frameReceived = false;
    bNewFrame = true;
    pFrameCount.set(frameCount);
  }

  std::unique_lock<std::mutex> lock(deviceMutex, std::try_to_lock);

  if (lock.owns_lock() && discoveredDevice) {
    auto next = discoveredDevice;

    // Reset our queued device and release the lock again
    discoveredDevice = nullptr;
    lock.unlock();

    if (device) {
      closeDevice();
    }

    if (!device && !SP_ISNULL(next->getHandle())) {
      openDevice(next);
    }

    listCameras(false);
  }

  //parameters->update();
}

void ofxVimbaGrabber::close() {
  stopDiscovery();
  closeDevice();
  pixelsVector.clear();
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

void ofxVimbaGrabber::onDiscoveryFound(std::shared_ptr<Device> &nextDevice) {
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (device && SP_ISEQUAL(device->getHandle(), nextDevice->getHandle())) {
    // We might already be connected to this camera, do nothing
    return;
  }

  if (!filterDevice(nextDevice)) return;

  discoveredDevice = nextDevice;
}

void ofxVimbaGrabber::onDiscoveryUpdate(std::shared_ptr<Device> &nextDevice) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (device) {
    if (SP_ISEQUAL(device->getHandle(), nextDevice->getHandle()) &&
        !nextDevice->isAvailable()) {
      // Our access mode dropped, schedule a disconnect
      discoveredDevice = std::make_shared<Device>(AVT::VmbAPI::CameraPtr());
    }
  } else if (filterDevice(nextDevice)) {
    discoveredDevice = nextDevice;
  }
}

void ofxVimbaGrabber::onDiscoveryLost(std::shared_ptr<Device> &nextDevice) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  if (!device || !SP_ISEQUAL(device->getHandle(), nextDevice->getHandle()))
    return;
  discoveredDevice = std::make_shared<Device>(AVT::VmbAPI::CameraPtr());
}

// -- DEVICE -------------------------------------------------------------------

bool ofxVimbaGrabber::filterDevice(std::shared_ptr<Device> &next) {
  if (!next || SP_ISNULL(next->getHandle())) return false;

  if (deviceID != DISCOVERY_ANY_ID && deviceID != next->getId()) {
    return false;
  }

  AccessMode requestedAccesMode = bReadOnly ? AccessModeRead : AccessModeMaster;

  if (next->getAvailableAccessMode() < requestedAccesMode) {
    logger.warning("requested access mode is not available");
    return false;
  }

  return true;
}

void ofxVimbaGrabber::openDevice(std::shared_ptr<Device> &next) {
  std::lock_guard<std::mutex> lock(deviceMutex);

  // We need to be disconnected, and have a next device
  if (device || !next || SP_ISNULL(next->getHandle())) return;
  if (deviceID != DISCOVERY_ANY_ID && deviceID != next->getId()) return;
  AccessMode requestedAccesMode = bReadOnly ? AccessModeRead : AccessModeMaster;
  if (!next->open(requestedAccesMode)) return;

  device = next;

  // retain the current id
  if (deviceID == DISCOVERY_ANY_ID) {
    deviceID = device->getId();
    // discovery->requestID(deviceID);
    // discovery->restart(); // CRASH
  }

  logger.setScope(device->getId());
  logger.notice("Opened camera connection");
  if (bReadOnly) {
    logger.notice("running in read only mode");
  }

  features->bind(device);
  pixelFormat = toOfPixelFormat(features->getString("PixelFormat"));
  framerate = features->getFloat("AcquisitionFrameRateAbs");

  //parameters->bind(device);
  startStream();

  if (!bReadOnly) {
    device->run("GVSPAdjustPacketSize");
    VmbInt64_t GVSPPacketSize;
    device->get("GVSPPacketSize", GVSPPacketSize);
    ofLogVerbose("ofxVimbaGrabber::openDevice") << "Packet size set to " << GVSPPacketSize;
  }
}

void ofxVimbaGrabber::closeDevice() {
  std::lock_guard<std::mutex> lock(deviceMutex);

  if (stream) stopStream();
  if (device) {
    //  auto previous = device;

    device->close();
    device = nullptr;

    features->unbind();
    //parameters->unbind();

    logger.notice("Closed camera connection");
    logger.clearScope();
  }
}

// -- STREAM -------------------------------------------------------------------

bool ofxVimbaGrabber::startStream() {
  if (stream) return true;

  if (device) {
    stream = std::make_shared<Stream>(device);
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
  auto &source = frame->getPixels();

  int fc;
  if (frame->getAncillary("ChunkAcquisitionFrameCount", fc)) {
    frameCount = fc;
  }

  if (source.getPixelFormat() == OF_PIXELS_UNKNOWN) {
    ofLogWarning("onFrame") << "Received unknown pixel format";
    return;
  }

  std::lock_guard<std::mutex> lock(pixelUpload);
  auto nextPixelIndex = (pixelIndex + 1) % pixelsVector.size();
  ofPixels &dest = pixelsVector.at(nextPixelIndex);

  // Allocate already checks if we even need to update
  dest.allocate(source.getWidth(), source.getHeight(), source.getPixelFormat());

  // Finally copy our data
  memcpy(dest.getData(), source.getData(), dest.getTotalBytes());

  // And update our source
  pixelIndex = nextPixelIndex;
  frameReceived = true;
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

bool ofxVimbaGrabber::setPixelFormat(ofPixelFormat _value) {
  if (bInited) {
    logger.warning("Cannot set pixelformat when inizialized");
    return false;
  }

  features->setOffline("PixelFormat", toVimbaPixelFormat(pixelFormat));
  pixelFormat = _value;
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

  features->setOffline("MulticastEnable", ofToString(true));
  bMulticast = true;
}

void ofxVimbaGrabber::enableReadOnly() {
  if (bInited) {
    logger.warning("Cannot enable read only access mode when inizialized");
    return;
  }

  if (bMulticast) {
    logger.warning(
        "Cannot enable read only access mode when multicast is enabled from "
        "this grabber");
    return;
  }

  bReadOnly = true;
}

// -- SET ----------------------------------------------------------------------

void ofxVimbaGrabber::setWidth(int _value) {
  if (isConnected()) {
    features->set("Width", framerate);
  } else {
    features->setOffline("Width", ofToString(_value));
  }
}

void ofxVimbaGrabber::setHeight(int _value) {
  if (isConnected()) {
    features->set("Height", framerate);
  } else {
    features->setOffline("Height", ofToString(_value));
  }
}

void ofxVimbaGrabber::setFrameRate(int _value) {
  if (isConnected()) {
    features->set("AcquisitionFrameRateAbs", framerate);
    framerate = features->getFloat("AcquisitionFrameRateAbs");
  } else {
    framerate = _value;
    features->setOffline("AcquisitionFrameRateAbs", ofToString(framerate));
  }
}

// -- GET ----------------------------------------------------------------------

float ofxVimbaGrabber::getFrameRate() {
  if (isConnected()) {
    framerate = features->getFloat("AcquisitionFrameRateAbs");
  }
  return framerate;
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

// -- PARAMETERS ---------------------------------------------------------------

//ofParameterGroup ofxVimbaGrabber::getParameters(vector<string> _params,
//                                                bool useCategories) {
//  ofParameterGroup npg;
//  npg.setName("LOST: " + deviceID);
//
//  if (isConnected()) {
//    ofParameterGroup pg;
//    if (_params.size() == 0) {
//      pg = parameters->get();
//    } else {
//      pg = parameters->get(_params, useCategories);
//    }
//
//    if (bReadOnly) {
//      npg.setName("READ ONLY: " + deviceID);
//    } else {
//      npg.setName("Camera: " + deviceID);
//      npg.add(pSave);
//      npg.add(pLoad);
//#ifdef VIMBA_DEV_MODE
//      // unexpected behaviour when used with ofxVideoGrabber
//      npg.add(pStream);
//#endif
//    }
//    for (int i = 0; i < pg.size(); i++) {
//      npg.add(pg[i]);
//    }
//  }
//  return npg;
//}

void ofxVimbaGrabber::pSaveListener(bool &_value) {
  if (_value) {
    _value = false;
    features->save();
  }
}

void ofxVimbaGrabber::pLoadListener(bool &_value) {
  if (_value) {
    _value = false;
    features->load();
  }
}

void ofxVimbaGrabber::pStreamListener(bool &_value) {
  if (_value) {
    if (!startStream()) _value = false;
  } else {
    stopStream();
  }
}

// -- TOOLS --------------------------------------------------------------------

int ofxVimbaGrabber::hexIdToIntId(string _value) const {
  int intId;
  std::stringstream ss;
  // Last 6 hex values: the unique ID as used in the predecessor of Vimba, PvAPI
  string uniqueID = _value.substr(10, 6);
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

ofPixelFormat ofxVimbaGrabber::toOfPixelFormat(string _format) {
  if (_format == "Mono8") {
    return OF_PIXELS_GRAY;
  } else if (_format == "RGB8Packed") {
    return OF_PIXELS_RGB;
  } else if (_format == "BGR8Packed") {
    return OF_PIXELS_BGR;
  } else if (_format == "YUV422Packed") {
    return OF_PIXELS_YUY2;
  }
  logger.warning("pixel format unknown");

  return OF_PIXELS_UNKNOWN;
}

string ofxVimbaGrabber::toVimbaPixelFormat(ofPixelFormat _format) {
  switch (_format) {
    case OF_PIXELS_GRAY:
      return "Mono8";
    case OF_PIXELS_RGB:
      return "RGB8Packed";
    case OF_PIXELS_BGR:
      return "BGR8Packed";
    case OF_PIXELS_YUY2:
      return "YUV422Packed";
    default:
      logger.warning("pixel format unknown");
      return "";
  }
}
