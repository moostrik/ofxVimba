#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <string>

#include "OosVimba.h"

//#define VIMBA_DEV_MODE

namespace OosVim {

class Grabber {
// -- SET --------------------------------------------------------------------
public:
 Grabber();
 virtual ~Grabber() { Grabber::close(); }

 bool setup(int w = 0, int h = 0);
 virtual void update() = 0;
 void close();

 // -- SET --------------------------------------------------------------------
  void setVerbose(bool bTalkToMe);
  void setDesiredFrameRate(int framerate);
  bool setPixelFormat(VmbPixelFormatType format);
  void setDeviceID(int simpleID) { setDeviceID(intIdToHexId(simpleID)); }
  void setDeviceID(std::string ID);
  void setMulticast(bool value);
  void setReadOnly(bool value);
  void setLoadUserSet(int setToLoad = 1);
  void loadUserSet() { setLoadUserSet(userSet.load()); }

  // -- GET --------------------------------------------------------------------
  bool isInitialized()        const { return bInited; }
  bool isFrameNew()           const { return bNewFrame; }
  bool isConnected()          { return getActiveDevice() != nullptr; }
  bool isResolutionChanged()  { return bResolutionChanged; }
  bool isPixelFormatChanged() { return bPixelFormatChanged; }
  bool isPixelSizeChanged()   { return bResolutionChanged || bPixelFormatChanged; }

  bool isMultiCast()          { return bMulticast.load(); }
  bool isReadOnly()           { return bReadOnly.load(); }
  int  getUserSet()           { return userSet.load(); }

  float getWidth()            const { return width; }
  float getHeight()           const { return height; }
  float getFrameRate()        const { return framerate.load(); }
  std::string getDeviceId()        { std::lock_guard<std::mutex> lock(deviceMutex); return deviceID; };

  VmbPixelFormatType getPixelFormat()  const { return pixelFormat; }
//  const ofPixels& getPixels()     const { return *pixels; }
//  ofPixels &getPixels()           { return *pixels; }

  Device_List_t listDevices() const;

 private:

  // -- CORE -------------------------------------------------------------------
  bool bInited;
  std::shared_ptr<OosVim::System>     system;
  std::shared_ptr<OosVim::Discovery>  discovery;
  std::shared_ptr<OosVim::Stream>     stream;
  std::shared_ptr<OosVim::Logger>     logger;

  // -- FRAME ------------------------------------------------------------------
//  std::shared_ptr<ofPixels> pixels;
  std::atomic<VmbPixelFormatType> pixelFormat;
  int width;
  int height;
  bool bNewFrame;
  bool bResolutionChanged;
  bool bPixelFormatChanged;

  // -- ACTION -----------------------------------------------------------------
  enum class ActionType { Connect, Disconnect, Configure };
  struct Action {
    ActionType type;
    std::shared_ptr<OosVim::Device> device;
    Action(ActionType _type, std::shared_ptr<OosVim::Device> _device) : type(_type), device(_device) {};
    bool operator == (Action action) { return action.type == type && action.device == device; }
  };

  std::mutex actionMutex;
  std::deque<Action> actionQueue;
  std::condition_variable actionSignal;
  std::shared_ptr<std::thread> actionThread;
  std::atomic<bool> actionsRunning;
  void addAction(ActionType type, std::shared_ptr<OosVim::Device> device = nullptr);
  void actionRunner();

  // -- DISCOVERY --------------------------------------------------------------
  void startDiscovery();
  void stopDiscovery();
  void discoveryCallback(std::shared_ptr<OosVim::Device>  device, const OosVim::DiscoveryTrigger trigger);
  void onDiscoveryFound(std::shared_ptr<OosVim::Device>   device);
  void onDiscoveryUpdate(std::shared_ptr<OosVim::Device>  device);
  void onDiscoveryLost(std::shared_ptr<OosVim::Device>    device);

  // -- DEVICE -----------------------------------------------------------------
  std::string deviceID;
  std::atomic<bool> bReadOnly;
  std::atomic<bool> bMulticast;
  std::atomic<int>  userSet;
  std::atomic<VmbPixelFormatType> desiredPixelFormat;
  std::mutex deviceMutex;
  std::shared_ptr<OosVim::Device> activeDevice;
  bool filterDevice(std::shared_ptr<OosVim::Device>     device, std::string id);
  bool openDevice(std::shared_ptr<OosVim::Device>       device);
  void closeDevice(std::shared_ptr<OosVim::Device>      device);
  bool configureDevice(std::shared_ptr<OosVim::Device>  device);
  bool isEqualDevice(std::shared_ptr<OosVim::Device> dev1, std::shared_ptr<OosVim::Device> dev2);
  std::shared_ptr<OosVim::Device> getActiveDevice();
  void setActiveDevice(std::shared_ptr<OosVim::Device> device);

  // -- STREAM -----------------------------------------------------------------
  bool startStream(std::shared_ptr<OosVim::Device> device);
  void stopStream();
  std::mutex frameMutex;
  virtual void streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) = 0;;
//  std::shared_ptr<ofPixels> receivedPixels;

  // -- FRAMERATE --------------------------------------------------------------
  std::atomic<float> desiredFrameRate;
  std::atomic<float> framerate;
  void setFrameRate(std::shared_ptr<OosVim::Device> device, double value);

  // -- LIST -------------------------------------------------------------------
  mutable std::mutex listMutex;
  Device_List_t deviceList;
  Device_List_t getDeviceList() const;
  void updateDeviceList();
  Device_List_t createDeviceList();
  void printDeviceList(Device_List_t dList) const;
  std::string createCameraString(Device_List_t dList) const;

  // -- TOOLS ------------------------------------------------------------------
  static int hexIdToIntId(std::string _value);
  static std::string intIdToHexId(int _intId);
  static std::string getUserSetString(int userSet);
};
}
