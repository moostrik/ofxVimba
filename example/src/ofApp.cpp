#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofSetLogLevel(OF_LOG_VERBOSE);

  vimbaGrabber = std::make_shared<ofxVimbaGrabber>();

//  vimbaGrabber->enableMulticast();
//  vimbaGrabber->enableReadOnly();
//  vimbaGrabber->enableUserSetLoad();

  grabber.setGrabber(vimbaGrabber);
  vector<ofVideoDevice> devices = grabber.listDevices();
  if (devices.size() > 0) {
    grabber.setDeviceID(devices[0].id);
  }

  grabber.setDesiredFrameRate(60);
  grabber.setup(ofGetWindowWidth(), ofGetWindowHeight(), true);
}

//--------------------------------------------------------------
void ofApp::update() {
  grabber.update();

  if (vimbaGrabber->isResolutionChanged()) {
    ofSetWindowShape(max((int)vimbaGrabber->getWidth(), 128),
                     max((int)vimbaGrabber->getHeight(), 128));
  }
}

//--------------------------------------------------------------
void ofApp::draw() {
  ofClear(0, 0, 0);

  if (grabber.isInitialized()) {
    grabber.draw(0, 0);
    ofDrawBitmapStringHighlight(vimbaGrabber->getDeviceId(), glm::vec2(10,20));
  }
}
