#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "ofMain.h"
#include "OosVim/Grabber.h"
#include "ofxVimbaUtils.h"

class ofxVimbaGrabber : public OosVim::Grabber {
public:
  ofxVimbaGrabber() : pixels(std::make_shared<ofPixels>()), pixelFormat(OF_PIXELS_UNKNOWN) { }
  virtual ~ofxVimbaGrabber() { }
  void update() { updateFrame(); }

  ofPixelFormat getPixelFormat() const  { return pixelFormat; }
  const ofPixels& getPixels() const     { return *pixels; }
  ofPixels& getPixels()                 { return *pixels; }

  void setDesiredPixelFormat(ofPixelFormat format);
  std::vector<ofVideoDevice> listDevices() const;

private:
  void updateFrame() override;
  void streamFrameCallBack(const std::shared_ptr<OosVim::Frame> frame) override;

  std::shared_ptr<ofPixels> pixels;
  std::shared_ptr<ofPixels> receivedPixels;
  ofPixelFormat pixelFormat;
};


// Convenience class with inheritance of ofBaseVideoGrabber
class ofxVimbaVideoGrabber : public ofBaseVideoGrabber {
// -- SET --------------------------------------------------------------------
public:
  ofxVimbaVideoGrabber()                              { grabber = std::make_unique<ofxVimbaGrabber>(); }
 virtual ~ofxVimbaVideoGrabber()                      { ofxVimbaVideoGrabber::close(); }

 bool setup(int w = 0, int h = 0) override            { if (!grabber) grabber = std::make_unique<ofxVimbaGrabber>(); return grabber->isInitialized(); }
 void update() override                               { grabber->update(); }
 void close() override                                { grabber = nullptr; }

 // -- SET --------------------------------------------------------------------
  void setVerbose(bool bTalkToMe) override            { grabber->setVerbose(bTalkToMe); }
  void setDesiredFrameRate(int framerate) override    { grabber->setDesiredFrameRate(framerate); }
  bool setPixelFormat(ofPixelFormat format) override  { grabber->setDesiredPixelFormat(format); return true; }
  void setDeviceID(int simpleId) override             { grabber->setDeviceID(simpleId); }
  void setDeviceID(string Id)                         { grabber->setDeviceID(Id); }
  void setMulticast(bool value)                       { grabber->setMulticast(value); }
  void setReadOnly(bool value)                        { grabber->setReadOnly(value); }
  void setLoadUserSet(int setToLoad = 1)              { grabber->setLoadUserSet(setToLoad); }

  // -- GET --------------------------------------------------------------------
  bool isInitialized() const override                 { return grabber && grabber->isInitialized(); }
  bool isFrameNew() const override                    { return grabber->isFrameNew(); }
  bool isConnected()                                  { return grabber->isConnected(); }
  bool isResolutionChanged()                          { return grabber->isResolutionChanged(); }
  bool isPixelFormatChanged()                         { return grabber->isPixelFormatChanged(); }
  bool isPixelSizeChanged()                           { return grabber->isPixelSizeChanged(); }

  bool isMultiCast()                                  { return grabber->isMultiCast(); }
  bool isReadOnly()                                   { return grabber->isReadOnly(); }
  int  getUserSet()                                   { return grabber->getUserSet(); }

  float getWidth() const override                     { return grabber->getWidth(); }
  float getHeight() const override                    { return grabber->getHeight(); }
  float getFrameRate() const                          { return grabber->getFrameRate(); }
  string getDeviceId()                                { return grabber->getDeviceId(); };

  ofPixelFormat getPixelFormat() const override       { return grabber->getPixelFormat(); }
  const ofPixels& getPixels() const override          { return grabber->getPixels(); }
  ofPixels &getPixels() override                      { return grabber->getPixels(); }

  vector<ofVideoDevice> listDevices() const override  { return grabber->listDevices(); }

private:
  unique_ptr<ofxVimbaGrabber> grabber;
};
