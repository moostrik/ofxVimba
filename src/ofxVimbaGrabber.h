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
  string getDeviceId()        const { std::lock_guard<std::mutex> lock(deviceMutex); return deviceID; };

  ofPixelFormat getPixelFormat()  const override { return pixelFormat; }
  const ofPixels& getPixels()     const override { return *pixels; }
  ofPixels &getPixels()           override { return *pixels; }

  std::vector<ofVideoDevice> listDevices() const override;

 private:
  // -- LIST -------------------------------------------------------------------
  void listCameras(bool _verbose);
  void printCameras() const;
  mutable std::mutex listMutex;
  std::vector<ofVideoDevice> ofDevices;

  // -- CORE -------------------------------------------------------------------
  bool bInited;
  std::shared_ptr<OosVimba::System> system;
  std::shared_ptr<OosVimba::Discovery> discovery;
  std::shared_ptr<OosVimba::Stream> stream;
  OosVimba::Logger logger;

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
  string deviceID;
  mutable std::mutex deviceMutex;
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
  void streamFrameCallBack(const std::shared_ptr<OosVimba::Frame> frame);
  std::shared_ptr<ofPixels> receivedPixels;

  // -- FRAME ------------------------------------------------------------------
  bool bNewFrame;
  std::shared_ptr<ofPixels> pixels;
  ofPixelFormat pixelFormat;
  int width;
  int height;
  bool bResolutionChanged;
  bool bPixelFormatChanged;

  // -- FRAMERATE --------------------------------------------------------------
  atomic<float> framerate;
  void setFrameRate(std::shared_ptr<OosVimba::Device> device, double value);

  // -- SETTINGS ---------------------------------------------------------------
  atomic<bool> bReadOnly;
  atomic<bool> bMulticast;
  atomic<int>  userSet;
  atomic<ofPixelFormat> desiredPixelFormat;
  atomic<float> desiredFrameRate;

  // -- TOOLS ------------------------------------------------------------------
  int hexIdToIntId(string _value) const;
  string intIdToHexId(int _intId) const;
  string getUserSetString(int userSet) const;
};
