#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxVimbaGrabber.h"

class ofApp : public ofBaseApp {
 public:
  void setup();
  void update();
  void draw();

  void keyPressed(int key);
  void minimizeNested(ofxGuiGroup &group, ofxGuiGroup *root = nullptr);

 private:
  // The camera instance
  ofPtr<ofxVimbaGrabber> vimbaGrabber;
  ofVideoGrabber grabber;

  ofxPanel panel;
  ofParameter<bool> pAllParameters;
  void pAllParametersListener(bool &_value) { updateParameters(); }
  void updateParameters();
};
