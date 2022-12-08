#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "OosVimba.h"

//#define VIMBA_DEV_MODE

namespace OosVimba {

class Grabber {
// -- SET --------------------------------------------------------------------
public:
 Grabber();
 virtual ~Grabber() { Grabber::close(); }

 bool setup(int w = 0, int h = 0);
 void update();
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
  string getDeviceId()        { std::lock_guard<std::mutex> lock(deviceMutex); return deviceID; };

  VmbPixelFormatType getPixelFormat()  const { return pixelFormat; }
//  const ofPixels& getPixels()     const { return *pixels; }
//  ofPixels &getPixels()           { return *pixels; }

  std::vector<ofVideoDevice> listDevices() const;

 private:

  // -- CORE -------------------------------------------------------------------
  bool bInited;
  std::shared_ptr<OosVimba::System>     system;
  std::shared_ptr<OosVimba::Discovery>  discovery;
  std::shared_ptr<OosVimba::Stream>     stream;
  std::shared_ptr<OosVimba::Logger>     logger;

  // -- FRAME ------------------------------------------------------------------
//  std::shared_ptr<ofPixels> pixels;
  VmbPixelFormatType pixelFormat;
  int width;
  int height;
  bool bNewFrame;
  bool bResolutionChanged;
  bool bPixelFormatChanged;

  // -- ACTION -----------------------------------------------------------------
  enum class ActionType { Connect, Disconnect, Configure };
  struct Action {
    ActionType type;
    std::shared_ptr<OosVimba::Device> device;
    Action(ActionType _type, std::shared_ptr<OosVimba::Device> _device) : type(_type), device(_device) {};
    bool operator == (Action action) { return action.type == type && action.device == device; }
  };

  std::mutex actionMutex;
  std::deque<Action> actionQueue;
  std::condition_variable actionSignal;
  std::shared_ptr<std::thread> actionThread;
  std::atomic<bool> actionsRunning;
  void addAction(ActionType type, std::shared_ptr<OosVimba::Device> device = nullptr);
  void actionRunner();

  // -- DISCOVERY --------------------------------------------------------------
  void startDiscovery();
  void stopDiscovery();
  void discoveryCallback(std::shared_ptr<OosVimba::Device>  device, const OosVimba::DiscoveryTrigger trigger);
  void onDiscoveryFound(std::shared_ptr<OosVimba::Device>   device);
  void onDiscoveryUpdate(std::shared_ptr<OosVimba::Device>  device);
  void onDiscoveryLost(std::shared_ptr<OosVimba::Device>    device);

  // -- DEVICE -----------------------------------------------------------------
  std::string deviceID;
  atomic<bool> bReadOnly;
  atomic<bool> bMulticast;
  atomic<int>  userSet;
  atomic<ofPixelFormat> desiredPixelFormat;
  std::mutex deviceMutex;
  std::shared_ptr<OosVimba::Device> activeDevice;
  bool filterDevice(std::shared_ptr<OosVimba::Device>     device, std::string id);
  bool openDevice(std::shared_ptr<OosVimba::Device>       device);
  void closeDevice(std::shared_ptr<OosVimba::Device>      device);
  bool configureDevice(std::shared_ptr<OosVimba::Device>  device);
  bool isEqualDevice(std::shared_ptr<OosVimba::Device> dev1, std::shared_ptr<OosVimba::Device> dev2);
  std::shared_ptr<OosVimba::Device> getActiveDevice();
  void setActiveDevice(std::shared_ptr<OosVimba::Device> device);

  // -- STREAM -----------------------------------------------------------------
  bool startStream(std::shared_ptr<OosVimba::Device> device);
  void stopStream();
  std::mutex frameMutex;
  virtual void streamFrameCallBack(const std::shared_ptr<OosVimba::Frame> frame) = 0;;
//  std::shared_ptr<ofPixels> receivedPixels;

  // -- FRAMERATE --------------------------------------------------------------
  atomic<float> desiredFrameRate;
  atomic<float> framerate;
  void setFrameRate(std::shared_ptr<OosVimba::Device> device, double value);

  // -- LIST -------------------------------------------------------------------
  mutable std::mutex listMutex;
  std::vector<ofVideoDevice> deviceList;
  std::vector<ofVideoDevice> getDeviceList() const;
  void updateDeviceList();
  std::vector<ofVideoDevice> createDeviceList();
  void printDeviceList(std::vector<ofVideoDevice> dList) const;
  std::string createCameraString(std::vector<ofVideoDevice> dList) const;

  // -- TOOLS ------------------------------------------------------------------
  static int hexIdToIntId(string _value);
  static std::string intIdToHexId(int _intId);
  static std::string getUserSetString(int userSet);
};
}
