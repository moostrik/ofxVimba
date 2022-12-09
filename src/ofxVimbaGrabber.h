// Copyright (C) 2022 Matthias Oostrik
//
// Convenience class with inheritance of ofBaseVideoGrabber
// Note that the width, height and pixelformat are set based on the features of the camera,
// These variables are set based on the received frames;

#pragma once

#include "ofVideoBaseTypes.h"
#include "ofxVimba.h"

namespace ofxVimba {

class ofVideoGrabber : public ofBaseVideoGrabber {
// -- SET --------------------------------------------------------------------
public:
  ofVideoGrabber() {grabber = std::make_unique<ofxVimba::Grabber>(); }
  virtual ~ofVideoGrabber() { }

 bool setup(int w = 0, int h = 0) override {
   grabber->setup();
   return grabber->isInitialized();
   width = w;
   height = h;
   pixelFormat = grabber->getDesiredPixelFormat();
 }

 void update() override {
   grabber->update();
   if (grabber->isFrameNew()) {
     auto pixels = grabber->getPixels();
     auto w = pixels.getWidth();
     auto h = pixels.getHeight();
     auto f = pixels.getPixelFormat();
     if (w != width || h != height) bResolutionChanged = true;
     if (f != pixelFormat) bFormatChanged = true;
     width = w;
     height = h;
     pixelFormat = f;
   }
 }

 void close() override { grabber->close(); }

 // -- SET --------------------------------------------------------------------
  void setVerbose(bool bTalkToMe) override            { grabber->setVerbose(bTalkToMe); }
  void setDesiredFrameRate(int framerate) override    { grabber->setDesiredFrameRate(framerate); }
  bool setPixelFormat(ofPixelFormat format) override  { grabber->setDesiredPixelFormat(format);
                                                        pixelFormat = grabber->getDesiredPixelFormat();
                                                        return true; }
  void setDeviceID(int simpleId) override             { grabber->setDeviceID(simpleId); }
  void setDeviceID(string Id)                         { grabber->setDeviceID(Id); }
  void setMulticast(bool value)                       { grabber->setMulticast(value); }
  void setReadOnly(bool value)                        { grabber->setReadOnly(value); }
  void setLoadUserSet(int setToLoad = 1)              { grabber->setLoadUserSet(setToLoad); }

  // -- GET --------------------------------------------------------------------
  bool isInitialized() const override                 { return grabber->isInitialized(); }
  bool isFrameNew() const override                    { return grabber->isFrameNew(); }
  bool isConnected()                                  { return grabber->isConnected(); }
  bool isResolutionChanged()                          { return bResolutionChanged; }
  bool isPixelFormatChanged()                         { return bFormatChanged; }
  bool isPixelsChanged()                              { return isResolutionChanged() || isPixelFormatChanged(); }

  bool isMultiCast()                                  { return grabber->isMultiCast(); }
  bool isReadOnly()                                   { return grabber->isReadOnly(); }
  int  getUserSet()                                   { return grabber->getUserSet(); }

  float getWidth() const override                     { return width; }
  float getHeight() const override                    { return height; }
  float getFrameRate() const                          { return grabber->getFrameRate(); }
  string getDeviceId()                                { return grabber->getDeviceId(); };

  ofPixelFormat getPixelFormat() const override       { return pixelFormat; }
  const ofPixels& getPixels() const override          { return grabber->getPixels(); }
  ofPixels &getPixels() override                      { return grabber->getPixels(); }

  vector<ofVideoDevice> listDevices() const override  { return grabber->listDevices(); }

private:
  unique_ptr<ofxVimba::Grabber> grabber;
  ofPixelFormat pixelFormat;
  int   width;
  int   height;
  bool  bResolutionChanged;
  bool  bFormatChanged;
};

}
