#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Common.h"
#include "Device.h"
#include "Discovery.h"
#include "Logger.h"
#include "Stream.h"
#include "System.h"

#define VIMBA_DEV_MODE

class ofxVimbaGrabber : public ofBaseVideoGrabber {
 public:
  // -- SET BEFORE SETUP -------------------------------------------------------
 public:
  void setDeviceID(int _simpleDeviceID) override;
  void setDeviceID(string _deviceID);
  bool setPixelFormat(ofPixelFormat _value) override;
  void enableMulticast();
  void enableReadOnly();

  // -- SET --------------------------------------------------------------------
 public:
  void setVerbose(bool bTalkToMe) override { bVerbose = bTalkToMe; }
  void setDesiredFrameRate(int framerate) override { setFrameRate(framerate); }
  void setFrameRate(int _value);
  void setWidth(int _value);
  void setHeight(int _value);
  void setFeature(string _n, string _v) { "features->setOffline(_n, _v);"; }

  // -- GET --------------------------------------------------------------------
 public:
  bool isInitialized() const override { return bInited; }
  bool isConnected() const { return activeDevice != nullptr; }
  bool isDisconnected() const { return activeDevice == nullptr; }
  bool isConnecting() const { return discovery && isDisconnected(); }
  bool isStreaming() const { return stream != nullptr; }
  bool isConnectionChanged() const { return (isConnected() != bIsConnected); }
  bool isResolutionChanged() const { return bResolutionChange; }
  bool isFrameNew() const override { return bNewFrame; }

  float getWidth() const override { return width; }
  float getHeight() const override { return height; }
  float getFrameRate();

  string getFeature(string featureName) { return ""; }

  ofPixelFormat getPixelFormat() const override { return pixelFormat; }
  ofPixels &getPixels() override { return pixels; }
  const ofPixels &getPixels() const override { return pixels; }

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
  void onDiscoveryFound(std::shared_ptr<ofxVimba::Device> &device);
  void onDiscoveryUpdate(std::shared_ptr<ofxVimba::Device> &device);
  void onDiscoveryLost(std::shared_ptr<ofxVimba::Device> &device);

  bool filterDevice(std::shared_ptr<ofxVimba::Device> &device);
  void openDevice(std::shared_ptr<ofxVimba::Device> &device);
  void closeDevice();
  void configureDevice(std::shared_ptr<ofxVimba::Device> &device);

  bool startStream();
  void stopStream();

  std::mutex frameMutex;
  void onFrame(const std::shared_ptr<ofxVimba::Frame> &frame);
  std::shared_ptr<ofxVimba::Frame> receivedFrame;
  std::atomic<int> frameCount;

  std::shared_ptr<ofxVimba::System> system;
  std::shared_ptr<ofxVimba::Discovery> discovery;
  std::shared_ptr<ofxVimba::Device> activeDevice;
  std::shared_ptr<ofxVimba::Stream> stream;
  ofxVimba::Logger logger;

  std::shared_ptr<ofxVimba::Device> discoveredDevice;
  std::mutex deviceMutex;

  ofPixels pixels;

  string deviceID;
  int width;
  int height;
  int xOffset;
  int yOffset;
  float framerate;
  ofPixelFormat pixelFormat;

  bool bVerbose;
  bool bMulticast;
  bool bReadOnly;
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
