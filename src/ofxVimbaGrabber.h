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

 bool setup(int w, int h) override;
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
  bool isConnected()          { return deviceConnected.load(); }
  bool isResolutionChanged()  { return bResolutionChanged; }
  bool isPixelFormatChanged() { return bPixelFormatChanged; }
  bool isPixelSizeChanged()   { return bResolutionChanged || bPixelFormatChanged; }

  bool isMultiCast()          { return bMulticast.load(); }
  bool isReadOnly()           { return bReadOnly.load(); }
  int  getUserSet()           { return userSet.load(); }

  float getWidth()            const override { return width; }
  float getHeight()           const override { return height; }
  float getFrameRate()        const { return framerate; }
  string getDeviceId()        const { return deviceID; };

  ofPixelFormat getPixelFormat()  const override { return pixelFormat; }
  const ofPixels& getPixels()     const override { return *pixels; }
  ofPixels &getPixels()           override { return *pixels; }

  // -- LIST -------------------------------------------------------------------
  std::vector<ofVideoDevice> listDevices() const override;

 private:
  void listCameras(bool _verbose);
  void printCameras() const;
  std::vector<ofVideoDevice> ofDevices;

  // -- CORE -------------------------------------------------------------------
  std::shared_ptr<OosVimba::System> system;
  std::shared_ptr<OosVimba::Discovery> discovery;
  std::shared_ptr<OosVimba::Device> activeDevice;
  std::shared_ptr<OosVimba::Stream> stream;
  OosVimba::Logger logger;

  // -- ACTION -----------------------------------------------------------------
  enum class ActionType { Connect, Disconnect, Configure };
  struct Action {
    ActionType type;
    std::shared_ptr<OosVimba::Device> device;
    Action(ActionType _type, std::shared_ptr<OosVimba::Device> _device) : type(_type), device(_device) {};
  };

  std::mutex actionMutex;
  std::queue<Action> actionQueue;
  std::condition_variable actionSignal;
  std::shared_ptr<std::thread> actionThread;
  std::atomic<bool> actionsRunning;
  void addAction(ActionType type, std::shared_ptr<OosVimba::Device> device = nullptr);
  void actionRunner();

  // -- DISCOVERY --------------------------------------------------------------
  void startDiscovery();
  void stopDiscovery();
  void discoveryCallback(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger trigger);
  void onDiscoveryFound(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryUpdate(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryLost(std::shared_ptr<OosVimba::Device> &device);

  // DEVICE
  string deviceID;
  std::mutex deviceMutex;
  atomic<bool> deviceConnected;
  bool filterDevice(std::shared_ptr<OosVimba::Device> &device);
  void openDevice(std::shared_ptr<OosVimba::Device> &device);
  void closeDevice();
  void configureDevice(std::shared_ptr<OosVimba::Device> &device);
  std::shared_ptr<OosVimba::Device> getActiveDevice();
  bool isEqualDevice(std::shared_ptr<OosVimba::Device> dev1, std::shared_ptr<OosVimba::Device> dev2);

  //

  void setFrameRate(double value);

  // STREAM
  bool startStream();
  void stopStream();
  std::mutex frameMutex;
  void streamFrameCallBack(const std::shared_ptr<OosVimba::Frame> frame);
  std::shared_ptr<ofPixels> receivedPixels;

  // FRAME
  bool bNewFrame;
  std::shared_ptr<ofPixels> pixels;
  int width;
  int height;
  bool bResolutionChanged;

  ofPixelFormat pixelFormat;
  atomic<ofPixelFormat> desiredPixelFormat;
  bool bPixelFormatChanged;

  atomic<float> framerate;
  atomic<float> desiredFrameRate;

  atomic<bool> bMulticast;
  atomic<bool> bReadOnly;
  atomic<int>  userSet;
  bool bInited;

  // -- TOOLS ------------------------------------------------------------------
  int hexIdToIntId(string _value) const;
  string intIdToHexId(int _intId) const;

  string getUserSetString(int userSet) const;


};
