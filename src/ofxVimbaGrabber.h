#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "Common.h"
#include "Device.h"
#include "Discovery.h"
#include "Features.h"
#include "Logger.h"
#include "Parameters.h"
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
  void setMulticast(bool _value);
  void setReadOnly(bool _value);
  void setVerbose(bool bTalkToMe) override { bVerbose = bTalkToMe; }
  void setDesiredFrameRate(int framerate) override { setFrameRate(framerate); }
  void setFrameRate(int _value);
  void setWidth(int _value);
  void setHeight(int _value);
  void setFloatFeature(string _n, float _v) {
    features->setOffline(_n, ofToString(_v));
  }
  void setIntFeature(string _n, int _v) {
    features->setOffline(_n, ofToString(_v));
  }
  void setBoolFeature(string _n, bool _v) {
    features->setOffline(_n, ofToString(_v));
  }
  void setStringFeature(string _n, string _v) {
    features->setOffline(_n, ofToString(_v));
  }

  // -- GET --------------------------------------------------------------------
 public:
  bool isInitialized() const override { return bInited; }
  bool isConnected() const { return device != nullptr; }
  bool isDisconnected() const { return device == nullptr; }
  bool isConnecting() const { return discovery && isDisconnected(); }
  bool isStreaming() const { return stream != nullptr; }
  bool isConnectionChanged() const { return (isConnected() != bWasConnected); }
  bool isResolutionChanged() const { return bResolutionChange; }
  bool isROIChanged() const { return bROIChange; }
  bool isFrameNew() const override { return bNewFrame; }

  float getWidth() const override { return width; }
  float getHeight() const override { return height; }
  float getFrameRate();

  float getFloatFeature(string _name) { return features->getFloat(_name); }
  int getIntFeature(string _name) { return features->getInt(_name); }
  bool getBoolFeature(string _name) { return features->getBool(_name); }
  string getStringFeature(string _name) { return features->getString(_name); }

  glm::vec2 getFloatRange(string _nam) { return features->getFloatRange(_nam); }
  glm::ivec2 getIntRange(string _name) { return features->getIntRange(_name); }

  ofPixelFormat getPixelFormat() const override { return pixelFormat; }
  ofPixels &getPixels() override { return pixels; }
  const ofPixels &getPixels() const override { return pixels; }

  // -- LIST -------------------------------------------------------------------
 public:
  std::vector<ofVideoDevice> listDevices() const override;
  std::vector<string> listParameters() { return parameters->listParameters(); }

 private:
  void listCameras(bool _verbose);
  void printCameras() const;
  std::vector<ofVideoDevice> ofDevices;

  // -- CORE -------------------------------------------------------------------
 public:
  ofxVimbaGrabber();
  virtual ~ofxVimbaGrabber() { close(); }

  bool setup(int w, int h) override;
  void update() override;
  void close() override;
  void loadFeatures() {
    if (isConnected()) features->load();
  }
  void saveFeatures() {
    if (isConnected()) features->save();
  }

 private:
  void startDiscovery();
  void stopDiscovery();
  void onDiscoveryFound(std::shared_ptr<ofxVimba::Device> &device);
  void onDiscoveryUpdate(std::shared_ptr<ofxVimba::Device> &device);
  void onDiscoveryLost(std::shared_ptr<ofxVimba::Device> &device);

  bool filterDevice(std::shared_ptr<ofxVimba::Device> &device);
  void openDevice(std::shared_ptr<ofxVimba::Device> &device);
  void closeDevice();

  bool startStream();
  void stopStream();
  void onFrame(const std::shared_ptr<ofxVimba::Frame> &frame);

  std::shared_ptr<ofxVimba::System> system;
  std::shared_ptr<ofxVimba::Discovery> discovery;
  std::shared_ptr<ofxVimba::Device> device;
  std::shared_ptr<ofxVimba::Features> features;
  std::shared_ptr<ofxVimba::Parameters> parameters;
  std::shared_ptr<ofxVimba::Stream> stream;
  ofxVimba::Logger logger;

  std::shared_ptr<ofxVimba::Device> discoveredDevice;
  std::mutex deviceMutex;

  ofPixels pixels;
  std::mutex pixelUpload;
  std::atomic<int> pixelIndex;
  std::vector<ofPixels> pixelsVector;
  std::atomic<int> frameCount;
  std::atomic<bool> frameReceived;

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
  bool bWasConnected;
  bool bResolutionChange;
  bool bROIChange;

  // -- PARAMETERS -------------------------------------------------------------
 public:
  ofParameterGroup getParameters(vector<string> _params = vector<string>(),
                                 bool useCategories = true);

 private:
  ofParameterGroup grabberParameters;
  ofParameter<bool> pSave;
  ofParameter<bool> pLoad;
  ofParameter<bool> pStream;
  ofParameter<int> pFrameCount;

  void buildParameters();
  void pSaveListener(bool &_value);
  void pLoadListener(bool &_value);
  void pStreamListener(bool &_value);

  // -- TOOLS ------------------------------------------------------------------
 private:
  int hexIdToIntId(string _value) const;
  string intIdToHexId(int _intId) const;

  ofPixelFormat toOfPixelFormat(string _format);
  string toVimbaPixelFormat(ofPixelFormat _format);

  AVT::VmbAPI::CameraPtr getHandle() const {
    return device ? device->getHandle() : AVT::VmbAPI::CameraPtr();
  };
};
