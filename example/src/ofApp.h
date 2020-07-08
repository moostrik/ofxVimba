#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxVimbaGrabber.h"

#define SHOW_ALL_PARAMETERS TRUE

class ofApp : public ofBaseApp {
 public:
  void setup();
  void update();
  void draw();

  void keyPressed(int key);

 private:
  // The camera instance
  ofPtr<ofxVimbaGrabber> vimbaGrabber;
  ofVideoGrabber grabber;

  ofxPanel panel;
  void updateParameters();
  void minimizeNested(ofxGuiGroup &group, ofxGuiGroup *root = nullptr);
};
