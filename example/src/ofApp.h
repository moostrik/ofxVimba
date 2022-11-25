#pragma once

#include "ofMain.h"
#include "ofxVimbaGrabber.h"

class ofApp : public ofBaseApp {
 public:
  void setup();
  void update();
  void draw();

 private:
  ofVideoGrabber grabber;
  ofPtr<ofxVimbaGrabber> vimbaGrabber;
};
