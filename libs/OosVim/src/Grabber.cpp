#include "OosVim/Grabber.h"

using namespace OosVim;

Grabber::Grabber() :
  bInited(false),
  system(OosVim::System::getInstance()),
  discovery(nullptr),
  stream(nullptr),
  logger(std::make_shared<OosVim::Logger>("Grabber ")),
//  pixels(std::make_shared<ofPixels>()),
  pixelFormat(""),
  width(0), height(0),
  bNewFrame(false),
  bResolutionChanged(false),
  bPixelFormatChanged(false),
  actionsRunning(false),
  deviceID(OosVim::DISCOVERY_ANY_ID),
  bReadOnly(false),
  bMulticast(false),
  userSet(-1),
  desiredPixelFormat("BGR8Packed"),
  desiredFrameRate(OosVim::MAX_FRAMERATE),
  framerate(0),
  deviceList(createDeviceList())
{
  if (!system->isAvailable()) return;
  actionsRunning = true;
  actionThread = std::make_shared<std::thread>(std::bind(&Grabber::actionRunner, this));
  startDiscovery();
  bInited = true;
}

Grabber::~Grabber() {
  stopDiscovery();

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

/*
void Grabber::updateFrame() {
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
*/

// -- SET ----------------------------------------------------------------------

void Grabber::setVerbose(bool bTalkToMe) {
  if (bTalkToMe) logger->setLevel(OosVim::VMB_LOG_VERBOSE);
  else logger->setLevel(OosVim::VMB_LOG_NOTICE);
}

void Grabber::setDeviceID(std::string ID) {
  if (ID == deviceID) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  deviceID = ID;
  if (isInitialized()) addAction(ActionType::Disconnect, activeDevice);
}

void Grabber::setReadOnly(bool value) {
  if (value == bReadOnly.load()) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  bReadOnly.store(value);
  if (isInitialized()) addAction(ActionType::Disconnect, activeDevice);
}

void Grabber::setMulticast(bool value) {
  if (value == bMulticast.load()) return;
  std::lock_guard<std::mutex> lock(deviceMutex);
  bMulticast.store(value);
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
}

bool Grabber::setDesiredPixelFormat(std::string format) {
  if (format == desiredPixelFormat) return true;
  std::lock_guard<std::mutex> lock(deviceMutex);
  desiredPixelFormat = format;
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
  return true;
}

void Grabber::setLoadUserSet(int setToLoad) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  userSet.store(setToLoad);
  if (isInitialized() && activeDevice) addAction(ActionType::Configure, activeDevice);
}

void Grabber::setDesiredFrameRate(int framerate) {
  desiredFrameRate.store(framerate);
  if (isInitialized() && isConnected()) setFrameRate(activeDevice, desiredFrameRate.load());
}

// -- ACTION -------------------------------------------------------------------

void Grabber::addAction(ActionType type, std::shared_ptr<OosVim::Device> device) {
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

void Grabber::actionRunner() {
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

void Grabber::startDiscovery() {
  if (!discovery) {
    discovery = std::make_shared<OosVim::Discovery>();
    std::function<void(std::shared_ptr<OosVim::Device> device, const OosVim::DiscoveryTrigger)> callback = std::bind(&Grabber::discoveryCallback, this, std::placeholders::_1, std::placeholders::_2);
    discovery->setTriggerCallback(callback);
  }
//  discovery->requestID(deviceID);
  discovery->start();
}

void Grabber::stopDiscovery() {
  if (discovery) {
    discovery->setTriggerCallback();
    discovery->stop();
    discovery = nullptr;
  }
}

void Grabber::discoveryCallback(std::shared_ptr<OosVim::Device> device, const OosVim::DiscoveryTrigger trigger) {
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

void Grabber::onDiscoveryFound(std::shared_ptr<OosVim::Device> device) {
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

void Grabber::onDiscoveryLost(std::shared_ptr<OosVim::Device> device) {
  std::shared_ptr<OosVim::Device> currentDevice = getActiveDevice();
  if (!isEqualDevice(currentDevice, device)) return;

  logger->notice("Discovery Update: Device lost");
  addAction(ActionType::Disconnect, device);
}

void Grabber::onDiscoveryUpdate(std::shared_ptr<OosVim::Device> device) {
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

bool Grabber::filterDevice(std::shared_ptr<OosVim::Device> device, std::string id) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  if (id != OosVim::DISCOVERY_ANY_ID && id != device->getId()) {
    return false;
  }

  OosVim::AccessMode requestedAccesMode = bReadOnly ? OosVim::AccessModeRead : OosVim::AccessModeMaster;

  if (device->getAvailableAccessMode() < requestedAccesMode) {
    logger->warning("filterDevice " + device->getId() + " acces mode " + std::to_string(requestedAccesMode) +
      " is not available. Availability is " + std::to_string(device->getAvailableAccessMode()));
    return false;
  }

  return true;
}

bool Grabber::openDevice(std::shared_ptr<OosVim::Device> device) {
  if (!device || SP_ISNULL(device->getHandle())) return false;

  OosVim::AccessMode requestedAccesMode = bReadOnly ? OosVim::AccessModeRead : OosVim::AccessModeMaster;

  if (!isAccessModeAvailable(requestedAccesMode, device->getAvailableAccessMode())) {
    logger->warning("openDevice " + device->getId() + " acces mode " + std::to_string(requestedAccesMode) +
      " is not available. Availability is " + std::to_string(device->getAvailableAccessMode()));
    return false;
  }

  if (!device->open(requestedAccesMode)) {
    logger->warning("openDevice : unable to open" + device->getId() +" with acces mode " + std::to_string(requestedAccesMode));
    return false;
  }

  // retain the current id
  logger->setScope(device->getId());

  if (!bReadOnly) logger->verbose("Opened connection");
  else logger->notice("Opened read only connection");
  return true;
}

void Grabber::closeDevice(std::shared_ptr<OosVim::Device> device) {
  if (device && device->isOpen()) {
    device->close();
    if (!bReadOnly) logger->verbose("Closed connection");
    else logger->verbose("Closed read only connection");
  }
  logger->clearScope();
}

bool Grabber::configureDevice(std::shared_ptr<OosVim::Device> device) {
  if (bReadOnly) return true;

  device->run("GVSPAdjustPacketSize");
  VmbInt64_t GVSPPacketSize;
  device->get("GVSPPacketSize", GVSPPacketSize);
  logger->verbose("Packet size set to " + std::to_string(GVSPPacketSize));

  device->set("MulticastEnable", bMulticast);

  if (userSet.load() >= 0) {
    device->set("UserSetSelector", getUserSetString(userSet));
    device->run("UserSetLoad");
  }

  //device->set("ChunkModeActive", true);

  auto desiredFormat = getDesiredPixelFormat();
  device->set("PixelFormat", desiredFormat);
  std::string currentPixelFormat;
  device->get("PixelFormat", currentPixelFormat);

  if (desiredFormat != currentPixelFormat)
    logger->notice("Desired pixel format not set, format set to " + currentPixelFormat);

  setFrameRate(device, desiredFrameRate.load());
  logger->notice("Device Configured");
  return true;
}

std::shared_ptr<OosVim::Device> Grabber::getActiveDevice() {
  std::lock_guard<std::mutex> lock(deviceMutex);
  return activeDevice;
}

void Grabber::setActiveDevice(std::shared_ptr<OosVim::Device> device) {
  std::lock_guard<std::mutex> lock(deviceMutex);
  activeDevice = device;
}

bool Grabber::isEqualDevice(std::shared_ptr<OosVim::Device> dev1, std::shared_ptr<OosVim::Device> dev2) {
  return dev1 && dev2 && SP_ISEQUAL(dev1->getHandle(), dev2->getHandle());
}

void Grabber::setFrameRate(std::shared_ptr<OosVim::Device> device, double value) {
  if (!device || !device->isOpen() || !device->isMaster()) return;

  double min, max;
  device->getRange("AcquisitionFrameRateAbs", min, max);
  framerate.store(min(max(value, min + 0.1), max - 0.1));
  device->set("AcquisitionFrameRateAbs", framerate);
  double fr;
  device->get("AcquisitionFrameRateAbs", fr);
  framerate.store(fr);

  if (framerate != value) {
    logger->notice("Desired framerate not set, framerate set to " + std::to_string(framerate));
  }

}

// -- STREAM -------------------------------------------------------------------

bool Grabber::startStream(std::shared_ptr<OosVim::Device> device) {
  if (stream) return true;

  if (device) {
    stream = std::make_shared<OosVim::Stream>(device);
    std::function<void(const std::shared_ptr<OosVim::Frame>)> callback = std::bind(&Grabber::streamFrameCallBack, this, std::placeholders::_1);
    stream->setFrameCallback(callback);
    stream->start();
    return true;
  }
  return false;
}

void Grabber::stopStream() {
  if (stream) {
    stream->setFrameCallback();
    stream->stop();
    stream = nullptr;
  }
}

/*
void Grabber::streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) {
  auto newPixels = std::make_shared<ofPixels>();
  auto format = ofxVimbaUtils::getOfPixelFormat(frame->getImageFormat());
  if (format == OF_PIXELS_UNKNOWN) return;

  // The data from the frame should NOT be used outside the scope of this function.
  // The setFromPixels method copies the pixel data.
  newPixels->setFromPixels(frame->getImageData(), frame->getWidth(), frame->getHeight(), format);

  std::lock_guard<std::mutex> lock(frameMutex);
  receivedPixels.swap(newPixels);
}
*/

// -- LIST ---------------------------------------------------------------------

Device_List_t Grabber::listDevices() const {
  Device_List_t deviceList = getDeviceList();
  printDeviceList(deviceList);
  return deviceList;
}

Device_List_t Grabber::getDeviceList() const {
  std::lock_guard<std::mutex> lock(listMutex);
  return deviceList;
}

void Grabber::updateDeviceList() {
  std::lock_guard<std::mutex> lock(listMutex);
  deviceList = createDeviceList();
}

Device_List_t Grabber::createDeviceList() {
  Device_List_t videoDevices;
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
    videoDevices.push_back(device);
  }

  return videoDevices;
}

void Grabber::printDeviceList(Device_List_t dList) const {
  auto cameraString = createCameraString(dList);
  std::cout << cameraString << std::endl;
}

std::string  Grabber::createCameraString(Device_List_t dList) const {
  std::ostringstream out;
  out << std::endl;
  out << "##########################################################################################";
  out << std::endl;
  out << "##           LISTING CAMERAS" << std::endl;
  for (int i = 0; i < (int)dList.size(); i++) {
    auto& dev = dList.at(i);
    out << "##  " << i << "  name: " << dev->getName().c_str()
        << "  simple id: " << hexIdToIntId(dev->getId()) << "  id: " << dev->getId()
         << "  available: " << dev->getAvailableAccessMode() << std::endl;
  }
  if (dList.size() == 0) {
    out << "## no cameras found" << std::endl;
  }

  out << "##" << std::endl;
  out << "##########################################################################################";
  out << std::endl;
  return out.str();;
}

// -- TOOLS --------------------------------------------------------------------

int Grabber::hexIdToIntId(std::string value) {
  int intId;
  std::stringstream ss;
  // Last 6 hex values: the unique ID as used in the predecessor of Vimba, PvAPI
  std::string uniqueID = value.substr(10, 6);
  ss << std::hex << uniqueID;
  ss >> intId;
  return intId;
}

std::string Grabber::intIdToHexId(int _intId) {
  std::stringstream ss;
  ss << std::uppercase << std::hex << _intId;
  std::string stringId(ss.str());
  while (stringId.length() < 6) {
    stringId = "0" + stringId;
  }
  stringId = "DEV_000F31" + stringId;
  return stringId;
}

std::string Grabber::getUserSetString(int userSet) {
  std::string us = "Default";
  if (userSet >= 1 && userSet <= 3) {
    us = "UserSet" + std::to_string(userSet);
  }
  return us;
}
