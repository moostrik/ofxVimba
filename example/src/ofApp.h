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
  ofPtr<ofxVimbaVideoGrabber> vimbaGrabber;

  vector<ofVideoDevice> devices;
  int   selectDevice;
  bool  toggleReadOnly;
  bool  toggleMultiCast;
  bool  toggleMono;
  int   selectUserSet;

  string text;
};
