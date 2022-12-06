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


  ofPtr<ofxVimbaGrabber> vimbaGrabber2;

  bool  toggleMultiCast;
  bool  toggleReadOnly;
  int   selectUserSet;
  int   selectDevice;
};
