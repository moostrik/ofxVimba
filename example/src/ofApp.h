#pragma once

#include "ofMain.h"
#include "ofxVimbaGrabber.h"

class ofApp : public ofBaseApp {
 public:
  void setup();
  void update();
  void draw();

  void keyReleased(ofKeyEventArgs& key);

 private:
  ofVideoGrabber grabber;
  ofPtr<ofxVimbaGrabber> vimbaGrabber; 
  vector<ofVideoDevice> devices;

  bool  toggleMultiCast;
  bool  toggleReadOnly;
  int   toggleUserSet;
  bool  toggleNextDevice;
};
