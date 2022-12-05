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
  void setVerbose(bool bTalkToMe) override;
  void setDesiredFrameRate(int fraemRate) override;
  bool setPixelFormat(ofPixelFormat format) override;
  void setDeviceID(int simpleID) override;
  void setDeviceID(string ID);
  void setMulticast(bool value);
  void setReadOnly(bool value);
  void setLoadUserSet(int setToLoad = 1);
  void loadUserSet() { setLoadUserSet(userSet.load()); }

  // -- GET --------------------------------------------------------------------
 public:
  bool isInitialized()        const override { return bInited; }
  bool isConnected()          { std::lock_guard<std::mutex> lock(deviceMutex); return activeDevice != nullptr; }
  bool isConnecting()         { return discovery && !(isConnected()); }
  bool isConnectionChanged()  { return (isConnected() != bIsConnected); }
  bool isFrameNew()           const override { return bNewFrame; }
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
 public:
  std::vector<ofVideoDevice> listDevices() const override;

 private:
  void listCameras(bool _verbose);
  void printCameras() const;
  std::vector<ofVideoDevice> ofDevices;

  // -- CORE -------------------------------------------------------------------
 public:
  ofxVimbaGrabber();
  virtual ~ofxVimbaGrabber() { ofxVimbaGrabber::close(); }

  bool setup(int w, int h) override;
  void update() override;
  void close() override;


 private:
  void startDiscovery();
  void stopDiscovery();

  void discoveryCallback(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger trigger);
  void onDiscoveryFound(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryUpdate(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryLost(std::shared_ptr<OosVimba::Device> &device);
  void updateDiscovery();

  bool filterDevice(std::shared_ptr<OosVimba::Device> &device);
  void openDevice(std::shared_ptr<OosVimba::Device> &device);
  void closeDevice();
  void configureDevice(std::shared_ptr<OosVimba::Device> &device);
  void setFrameRate(double value);

  bool startStream();
  void stopStream();

  std::mutex frameMutex;
  void streamFrameCallBack(const std::shared_ptr<OosVimba::Frame> frame);
  void updateFrame();

  std::shared_ptr<OosVimba::System> system;
  std::shared_ptr<OosVimba::Discovery> discovery;
  std::shared_ptr<OosVimba::Device> activeDevice;
  std::shared_ptr<OosVimba::Stream> stream;
  OosVimba::Logger logger;

  std::shared_ptr<OosVimba::Device> discoveredDevice;
  std::mutex deviceMutex;

  std::shared_ptr<ofPixels> pixels;
  std::shared_ptr<ofPixels> receivedPixels;

  string deviceID;
  int width;
  int height;
  atomic<float> framerate;
  atomic<float> desiredFrameRate;
  ofPixelFormat pixelFormat;
  atomic<ofPixelFormat> desiredPixelFormat;

  atomic<bool> bMulticast;
  atomic<bool> bReadOnly;
  atomic<int>  userSet;
  bool bInited;
  bool bNewFrame;
  bool bIsConnected;
  bool bResolutionChanged;
  bool bPixelFormatChanged;

  // -- TOOLS ------------------------------------------------------------------
 private:
  int hexIdToIntId(string _value) const;
  string intIdToHexId(int _intId) const;

  string getUserSetString(int userSet) const;

  AVT::VmbAPI::CameraPtr getHandle() const {
    return activeDevice ? activeDevice->getHandle() : AVT::VmbAPI::CameraPtr();
  };
};
