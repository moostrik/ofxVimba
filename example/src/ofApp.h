#pragma once

#include "ofMain.h"
#include "ofxVimbaGrabber.h"

class ofApp : public ofBaseApp {
 public:
  void setup();
  void update();
  void draw();
  void drawText();

  void keyReleased(ofKeyEventArgs& key);

 private:
  ofVideoGrabber grabber;
  ofPtr<ofxVimbaGrabber> vimbaGrabber;

  vector<ofVideoDevice> devices;
  int   selectDevice;
  bool  toggleReadOnly;
  bool  toggleMultiCast;
  bool  togglePixelMode;
  int   selectUserSet;

  string text;
};
