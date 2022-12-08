#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "ofMain.h"
#include "OosVim/OosVimba.h"
#include "ofxVimbaUtils.h"

//#define VIMBA_DEV_MODE

class ofxVimbaGrabber : public ofBaseVideoGrabber {
// -- SET --------------------------------------------------------------------
public:
 ofxVimbaGrabber();
 virtual ~ofxVimbaGrabber() { ofxVimbaGrabber::close(); }

 bool setup(int w = 0, int h = 0) override;
 void update() override;
 void close() override;

 // -- SET --------------------------------------------------------------------
  void setVerbose(bool bTalkToMe) override;
  void setDesiredFrameRate(int framerate) override;
  bool setPixelFormat(ofPixelFormat format) override;
  void setDeviceID(int simpleID) override { setDeviceID(intIdToHexId(simpleID)); }
  void setDeviceID(string ID);
  void setMulticast(bool value);
  void setReadOnly(bool value);
  void setLoadUserSet(int setToLoad = 1);
  void loadUserSet() { setLoadUserSet(userSet.load()); }

  // -- GET --------------------------------------------------------------------
  bool isInitialized()        const override { return bInited; }
  bool isFrameNew()           const override { return bNewFrame; }
  bool isConnected()          { return getActiveDevice() != nullptr; }
  bool isResolutionChanged()  { return bResolutionChanged; }
  bool isPixelFormatChanged() { return bPixelFormatChanged; }
  bool isPixelSizeChanged()   { return bResolutionChanged || bPixelFormatChanged; }

  bool isMultiCast()          { return bMulticast.load(); }
  bool isReadOnly()           { return bReadOnly.load(); }
  int  getUserSet()           { return userSet.load(); }

  float getWidth()            const override { return width; }
  float getHeight()           const override { return height; }
  float getFrameRate()        const { return framerate.load(); }
  string getDeviceId()        { std::lock_guard<std::mutex> lock(deviceMutex); return deviceID; };

  ofPixelFormat getPixelFormat()  const override { return pixelFormat; }
  const ofPixels& getPixels()     const override { return *pixels; }
  ofPixels &getPixels()           override { return *pixels; }

  std::vector<ofVideoDevice> listDevices() const override;

 private:

  // -- CORE -------------------------------------------------------------------
  bool bInited;
  std::shared_ptr<OosVim::System>     system;
  std::shared_ptr<OosVim::Discovery>  discovery;
  std::shared_ptr<OosVim::Stream>     stream;
  std::shared_ptr<OosVim::Logger>     logger;

  // -- FRAME ------------------------------------------------------------------
  std::shared_ptr<ofPixels> pixels;
  ofPixelFormat pixelFormat;
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
  string deviceID;
  atomic<bool> bReadOnly;
  atomic<bool> bMulticast;
  atomic<int>  userSet;
  atomic<ofPixelFormat> desiredPixelFormat;
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
  void streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame);
  std::shared_ptr<ofPixels> receivedPixels;

  // -- FRAMERATE --------------------------------------------------------------
  atomic<float> desiredFrameRate;
  atomic<float> framerate;
  void setFrameRate(std::shared_ptr<OosVim::Device> device, double value);

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
