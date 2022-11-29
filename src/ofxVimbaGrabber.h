#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "ofxVimba.h"
#include "ofxVimbaUtils.h"

#define VIMBA_DEV_MODE

class ofxVimbaGrabber : public ofBaseVideoGrabber {
 public:
  // -- SET BEFORE SETUP -------------------------------------------------------
 public:
  void setDeviceID(int simpleDeviceID) override;
  void setDeviceID(string deviceID);
  bool setPixelFormat(ofPixelFormat value) override;
  void enableMulticast();
  void enableReadOnly();
  void enableUserSetLoad();

  // -- SET --------------------------------------------------------------------
 public:
  void setVerbose(bool bTalkToMe)         override { ; }
  void setDesiredFrameRate(int framerate) override { setFrameRate(framerate); }
  void setFrameRate(int _value);
  void setWidth(int _value);
  void setHeight(int _value);

  // -- GET --------------------------------------------------------------------
 public:
  bool isInitialized()        const override { return bInited; }
  bool isConnected()          const { return activeDevice != nullptr; }
  bool isDisconnected()       const { return activeDevice == nullptr; }
  bool isConnecting()         const { return discovery && isDisconnected(); }
  bool isStreaming()          const { return stream != nullptr; }
  bool isConnectionChanged()  const { return (isConnected() != bIsConnected); }
  bool isResolutionChanged()  const { return bResolutionChange; }
  bool isFrameNew()           const override { return bNewFrame; }

  float getWidth()            const override { return width; }
  float getHeight()           const override { return height; }
  float getFrameRate()        const { return framerate; }
  string getDeviceId()        const { return deviceID; };

  ofPixelFormat getPixelFormat()  const override { return pixelFormat; }
  ofPixels &getPixels()           override { return *pixels; }
  const ofPixels &getPixels()     const override { return *pixels; }

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

  void triggerCallback(std::shared_ptr<OosVimba::Device> device, const OosVimba::DiscoveryTrigger trigger);
  void onDiscoveryFound(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryUpdate(std::shared_ptr<OosVimba::Device> &device);
  void onDiscoveryLost(std::shared_ptr<OosVimba::Device> &device);

  bool filterDevice(std::shared_ptr<OosVimba::Device> &device);
  void openDevice(std::shared_ptr<OosVimba::Device> &device);
  void closeDevice();
  void configureDevice(std::shared_ptr<OosVimba::Device> &device);

  bool startStream();
  void stopStream();

  std::mutex frameMutex;
  void frameCallBack(const std::shared_ptr<OosVimba::Frame> frame);

  void onFrame(const std::shared_ptr<OosVimba::Frame> &frame);

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
  int xOffset;
  int yOffset;
  float framerate;
  ofPixelFormat pixelFormat;

  bool bMulticast;
  bool bReadOnly;
  bool bUserSetLoad;
  bool bInited;
  bool bNewFrame;
  bool bIsConnected;
  bool bResolutionChange;

  // -- TOOLS ------------------------------------------------------------------
 private:
  int hexIdToIntId(string _value) const;
  string intIdToHexId(int _intId) const;

  AVT::VmbAPI::CameraPtr getHandle() const {
    return activeDevice ? activeDevice->getHandle() : AVT::VmbAPI::CameraPtr();
  };
};
