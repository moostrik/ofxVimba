#include "ofxVimbaGrabber.h"

//using namespace ofxVimba;

ofxVimbaGrabber::ofxVimbaGrabber() :
  bInited(false),
  system(OosVim::System::getInstance()),
  discovery(nullptr),
  stream(nullptr),
  logger(std::make_shared<OosVim::Logger>("ofxVimbaGrabber ")),
  pixels(std::make_shared<ofPixels>()),
  pixelFormat(OF_PIXELS_UNKNOWN),
  width(0), height(0),
  bNewFrame(false),
  bResolutionChanged(false),
  bPixelFormatChanged(false),
  actionsRunning(false),
  deviceID(OosVim::DISCOVERY_ANY_ID),
  bReadOnly(false),
  bMulticast(false),
  userSet(-1),
  desiredPixelFormat(OF_PIXELS_NATIVE),
  desiredFrameRate(OosVim::MAX_FRAMERATE),
  framerate(0),
  deviceList(createDeviceList())
{ }

bool ofxVimbaGrabber::setup(int _w, int _h) {
  if (bInited) return true;

  actionsRunning = true;
  actionThread = std::make_shared<std::thread>(std::bind(&ofxVimbaGrabber::actionRunner, this));

  startDiscovery();
  bInited = true;
  return true;
}

void ofxVimbaGrabber::update() {
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

void ofxVimbaGrabber::close() {
  stopDiscovery();
//  if(activeDevice) closeDevice(activeDevice);

  std::shared_ptr<std::thread> threadToKill;
  {
    std::lock_guard<std::mutex> lock(actionMutex);
    std::deque<Action> emptyQueue;
    actionQueue.swap(emptyQueue);
    actionsRunning = false;
    actionThread.swap(threadToKill);
  }
  if (threadToKill) {
    actionSignal.notify_one();
    if (threadToKill->joinable()) threadToKill->join();
  }
}

// -- SET ----------------------------------------------------------------------

void ofxVimbaGrabber::setVerbose(bool bTalkToMe) {
  if (bTalkToMe) logger->setLevel(OosVim::VMB_LOG_VERBOSE);
  else logger->setLevel(OosVim::VMB_LOG_NOTICE);
}

void ofxVimbaGrabber::setDeviceID(string ID) {
  if (ID == deviceID) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  deviceID = ID;
  if (isInitialized()) addAction(ActionType::Disconnect, activeDevice);
}

void ofxVimbaGrabber::setReadOnly(bool value) {
  if (value == bReadOnly.load()) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  bReadOnly.store(value);
  if (isInitialized()) addAction(ActionType::Disconnect, activeDevice);
}

void ofxVimbaGrabber::setMulticast(bool value) {
  if (value == bMulticast.load()) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  bMulticast.store(value);
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
}

bool ofxVimbaGrabber::setPixelFormat(ofPixelFormat format) {
  if (format == desiredPixelFormat) return true;
  std::lock_guard<std::mutex> lock(deviceMutex);
  desiredPixelFormat.store(format);
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
  return true;
}

void ofxVimbaGrabber::setLoadUserSet(int setToLoad) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  userSet.store(setToLoad);
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
}

void ofxVimbaGrabber::setDesiredFrameRate(int framerate) {
  desiredFrameRate.store(framerate);
  if (isInitialized() && isConnected()) setFrameRate(activeDevice, desiredFrameRate.load());
}

// -- ACTION -------------------------------------------------------------------

void ofxVimbaGrabber::addAction(ActionType type, std::shared_ptr<OosVim::Device> device) {
  auto action = Action(type, device);
  std::lock_guard<std::mutex> lock(actionMutex);

  if (type == ActionType::Disconnect) {
    actionQueue.clear();
    actionQueue.push_back(action);
    actionSignal.notify_one();
    return;
  }

  if(find(actionQueue.begin(), actionQueue.end(), action) == actionQueue.end()) {
    actionQueue.push_back(action);
    actionSignal.notify_one();
  }
}

void ofxVimbaGrabber::actionRunner() {
  std::unique_lock<std::mutex> lock(actionMutex);
  while (actionsRunning.load()) {
    if (!actionQueue.empty()) {
      auto action = actionQueue.front();
      actionQueue.pop_front();
      lock.unlock();

      if (action.type == ActionType::Disconnect){
        stopStream();
        closeDevice(action.device);
        setActiveDevice(nullptr);
        discovery->updateTriggers();
      }

      if (action.type == ActionType::Connect){
        if (openDevice(action.device)) {
          configureDevice(action.device);
          if (startStream(action.device)) {
            setActiveDevice(action.device);
          }
          else (closeDevice(action.device));
        }
      }

      if (action.type == ActionType::Configure){
        if (isEqualDevice(action.device, getActiveDevice())){
          stopStream();
          configureDevice(action.device);
          if (!startStream(action.device)){
            closeDevice(action.device);
            setActiveDevice(nullptr);
          }
        }
      }
      lock.lock();
    }

    actionSignal.wait(lock, [&] { return !actionQueue.empty() || !actionsRunning.load(); });
  }
}

// -- DISCOVERY ----------------------------------------------------------------

void ofxVimbaGrabber::startDiscovery() {
  if (!discovery) {
    discovery = std::make_shared<OosVim::Discovery>();
    std::function<void(std::shared_ptr<OosVim::Device> device, const OosVim::DiscoveryTrigger)> callback = std::bind(&ofxVimbaGrabber::discoveryCallback, this, std::placeholders::_1, std::placeholders::_2);
    discovery->setTriggerCallback(callback);
  }
//  discovery->requestID(deviceID);
  discovery->start();
}

void ofxVimbaGrabber::stopDiscovery() {
  if (discovery) {
    discovery->setTriggerCallback();
    discovery->stop();
    discovery = nullptr;
  }
}

void ofxVimbaGrabber::discoveryCallback(std::shared_ptr<OosVim::Device> device, const OosVim::DiscoveryTrigger trigger) {
  switch (trigger) {
    case OosVim::OOS_DISCOVERY_PLUGGED_IN:
      onDiscoveryFound(device);
      break;
    case OosVim::OOS_DISCOVERY_PLUGGED_OUT:
      onDiscoveryLost(device);
      break;
    case OosVim::OOS_DISCOVERY_STATE_CHANGED:
      onDiscoveryUpdate(device);
      break;
    default:
      break;
  }
  updateDeviceList();
}

void ofxVimbaGrabber::onDiscoveryFound(std::shared_ptr<OosVim::Device> device) {
  std::shared_ptr<OosVim::Device> currentDevice = getActiveDevice();
  std::string id = getDeviceId();
  if (isEqualDevice(currentDevice, device)) {
    logger->warning("Discovered device is already active");
    return;
  }
  if (!filterDevice(device, id)) return;
  addAction(ActionType::Connect, device);
  logger->verbose("Discovered Device " + device->getId());
}

void ofxVimbaGrabber::onDiscoveryLost(std::shared_ptr<OosVim::Device> device) {
  std::shared_ptr<OosVim::Device> currentDevice = getActiveDevice();
  if (!isEqualDevice(currentDevice, device)) return;

  logger->notice("Discovery Update: Device lost");
  addAction(ActionType::Disconnect, device);
}

void ofxVimbaGrabber::onDiscoveryUpdate(std::shared_ptr<OosVim::Device> device) {
  std::shared_ptr<OosVim::Device> currentDevice = getActiveDevice();
  if (isEqualDevice(currentDevice, device)) {
    if (!(bReadOnly || device->isAvailable())) {
      onDiscoveryLost(device);
      return;
    }
  }
  if (device->isAvailable()) onDiscoveryFound(device);
}

// -- DEVICE -------------------------------------------------------------------

bool ofxVimbaGrabber::filterDevice(std::shared_ptr<OosVim::Device> device, std::string id) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  if (id != OosVim::DISCOVERY_ANY_ID && id != device->getId()) {
    return false;
  }

  OosVim::AccessMode requestedAccesMode = bReadOnly ? OosVim::AccessModeRead : OosVim::AccessModeMaster;

  if (device->getAvailableAccessMode() < requestedAccesMode) {
    logger->warning("filterDevice " + device->getId() + " acces mode " + ofToString(requestedAccesMode) +
      " is not available. Availability is " + ofToString(device->getAvailableAccessMode()));
    return false;
  }

  return true;
}

bool ofxVimbaGrabber::openDevice(std::shared_ptr<OosVim::Device> device) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  OosVim::AccessMode requestedAccesMode = bReadOnly ? OosVim::AccessModeRead : OosVim::AccessModeMaster;

  if (!isAccessModeAvailable(requestedAccesMode, device->getAvailableAccessMode())) {
    logger->warning("openDevice " + device->getId() + " acces mode " + ofToString(requestedAccesMode) +
      " is not available. Availability is " + ofToString(device->getAvailableAccessMode()));
    return false;
  }

  if (!device->open(requestedAccesMode)) {
    logger->warning("openDevice : unable to open" + device->getId() +" with acces mode " + ofToString(requestedAccesMode));
    return false;
  }

  // retain the current id
  logger->setScope(device->getId());

  if (!bReadOnly) logger->verbose("Opened connection");
  else logger->notice("Opened read only connection");
  return true;
}

void ofxVimbaGrabber::closeDevice(std::shared_ptr<OosVim::Device> device) {
  if (device && device->isOpen()) {
    device->close();
    if (!bReadOnly) logger->verbose("Closed connection");
    else logger->verbose("Closed read only connection");
  }
  logger->clearScope();
}

bool ofxVimbaGrabber::configureDevice(std::shared_ptr<OosVim::Device> device) {
  if (bReadOnly) return true;

  device->run("GVSPAdjustPacketSize");
  VmbInt64_t GVSPPacketSize;
  device->get("GVSPPacketSize", GVSPPacketSize);
  logger->verbose("Packet size set to " + ofToString(GVSPPacketSize));

  device->set("MulticastEnable", bMulticast);

  if (userSet.load() >= 0) {
    device->set("UserSetSelector", getUserSetString(userSet));
    device->run("UserSetLoad");
  }

  //device->set("ChunkModeActive", true);

  if (desiredPixelFormat >= 0) device->set("PixelFormat", ofxVimbaUtils::getVimbaPixelFormat(desiredPixelFormat));
  string vmbPixelFormat;
  device->get("PixelFormat", vmbPixelFormat);
  pixelFormat = ofxVimbaUtils::getOfPixelFormat(vmbPixelFormat);

  if (pixelFormat < 0) logger->warning("pixel format set to " + ofToString(pixelFormat));
  else if (pixelFormat != desiredPixelFormat) logger->notice("pixel format set to " + ofToString(pixelFormat));

  setFrameRate(device, desiredFrameRate.load());
  logger->notice("Device Configured");
  return true;
}

std::shared_ptr<OosVim::Device> ofxVimbaGrabber::getActiveDevice() {
  std::lock_guard<std::mutex> lock(deviceMutex);
  return activeDevice;
}

void ofxVimbaGrabber::setActiveDevice(std::shared_ptr<OosVim::Device> device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  activeDevice = device;
}

bool ofxVimbaGrabber::isEqualDevice(std::shared_ptr<OosVim::Device> dev1, std::shared_ptr<OosVim::Device> dev2) {
  return dev1 && dev2 && SP_ISEQUAL(dev1->getHandle(), dev2->getHandle());
}


void ofxVimbaGrabber::setFrameRate(std::shared_ptr<OosVim::Device> device, double value) {
  if (!device || !device->isOpen() || !device->isMaster()) return;

  double min, max;
  device->getRange("AcquisitionFrameRateAbs", min, max);
  framerate.store(ofClamp(value, min + 0.1, max - 0.1));
  device->set("AcquisitionFrameRateAbs", framerate);
  double fr;
  device->get("AcquisitionFrameRateAbs", fr);
  framerate.store(fr);

  if (framerate != value) {
    logger->notice("framerate set to " + ofToString(framerate));
  }

}

// -- STREAM -------------------------------------------------------------------

bool ofxVimbaGrabber::startStream(std::shared_ptr<OosVim::Device> device) {
  if (stream) return true;

  if (device) {
    stream = std::make_shared<OosVim::Stream>(device);
    std::function<void(const std::shared_ptr<OosVim::Frame>)> callback = std::bind(&ofxVimbaGrabber::streamFrameCallBack, this, std::placeholders::_1);
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

// -- LIST ---------------------------------------------------------------------

std::vector<ofVideoDevice> ofxVimbaGrabber::listDevices() const {
  std::vector<ofVideoDevice> deviceList = getDeviceList();
  printDeviceList(deviceList);
  return deviceList;
}

std::vector<ofVideoDevice> ofxVimbaGrabber::getDeviceList() const {
  std::lock_guard<std::mutex> lock(listMutex);
  return deviceList;
}

void ofxVimbaGrabber::updateDeviceList() {
  std::lock_guard<std::mutex> lock(listMutex);
  deviceList = createDeviceList();
}

std::vector<ofVideoDevice> ofxVimbaGrabber::createDeviceList() {
  std::vector<ofVideoDevice> videoDevices;
  if (!system->isAvailable()) {
    logger->error(
        "Failed to retrieve current camera list, system is unavailable");
    return videoDevices;
  }

  AVT::VmbAPI::CameraPtrVector cameras;
  auto err = system->getAPI().GetCameras(cameras);

  if (err != VmbErrorSuccess) {
    logger->error("Failed to retrieve current camera list");
    return videoDevices;
  }

  for (auto cam : cameras) {
    auto device = std::make_shared<OosVim::Device>(cam);
    ofVideoDevice ofDevice;
    // convert to int to make compatable with ofVideoDevice
    ofDevice.id = hexIdToIntId(device->getId());
    ofDevice.deviceName = device->getModel();
    ofDevice.hardwareName = "Allied Vision";
    ofDevice.serialID = device->getSerial();
    ofDevice.bAvailable =
        (device->getAvailableAccessMode() == OosVim::AccessModeMaster);
    videoDevices.push_back(ofDevice);
  }

  return videoDevices;
}

void ofxVimbaGrabber::printDeviceList(std::vector<ofVideoDevice> dList) const {
  auto cameraString = createCameraString(dList);
  std::cout << cameraString << std::endl;
}

std::string  ofxVimbaGrabber::createCameraString(std::vector<ofVideoDevice> dList) const {
  std::ostringstream out;
  out << endl;
  out << "##########################################################################################";
  out << endl;
  out << "##           LISTING CAMERAS" << endl;
  for (int i = 0; i < (int)dList.size(); i++) {
    auto dev = dList[i];
    out << "##  " << i << "  name: " << dev.deviceName.c_str()
         << "  simple id: " << dev.id << "  id: " << intIdToHexId(dev.id)
         << "  available: " << dev.bAvailable << endl;
  }
  if (dList.size() == 0) {
    out << "## no cameras found" << endl;
  }

  out << "##" << endl;
  out << "##########################################################################################";
  out << endl;
  return out.str();;
}

// -- TOOLS --------------------------------------------------------------------

int ofxVimbaGrabber::hexIdToIntId(string value) {
  int intId;
  std::stringstream ss;
  // Last 6 hex values: the unique ID as used in the predecessor of Vimba, PvAPI
  string uniqueID = value.substr(10, 6);
  ss << std::hex << uniqueID;
  ss >> intId;
  return intId;
}

std::string ofxVimbaGrabber::intIdToHexId(int _intId) {
  std::stringstream ss;
  ss << std::uppercase << std::hex << _intId;
  std::string stringId(ss.str());
  while (stringId.length() < 6) {
    stringId = "0" + stringId;
  }
  stringId = "DEV_000F31" + stringId;
  return stringId;
}

std::string ofxVimbaGrabber::getUserSetString(int userSet) {
  std::string us = "Default";
  if (userSet >= 1 && userSet <= 3) {
    us = "UserSet" + ofToString(userSet);
  }
  return us;
}
