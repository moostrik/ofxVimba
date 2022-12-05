#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

  toggleNextDevice = 0;
  vimbaGrabber = std::make_shared<ofxVimbaGrabber>();
  grabber.setGrabber(vimbaGrabber);
  devices = grabber.listDevices();
  if (devices.size() > 0) {
    grabber.setDeviceID(devices[toggleNextDevice].id);
  }

  //vimbaGrabber->setPixelFormat(OF_PIXELS_GRAY);
  vimbaGrabber->setMulticast(true);
  //vimbaGrabber->setReadOnly(true);
  vimbaGrabber->setLoadUserSet(1);

  grabber.setVerbose(true);
  grabber.setDesiredFrameRate(60);
  //grabber.setPixelFormat(OF_PIXELS_GRAY);
  grabber.setup(ofGetWindowWidth(), ofGetWindowHeight(), true);

  // run after setup
}

//--------------------------------------------------------------
void ofApp::update() {
  grabber.update();

  if (vimbaGrabber->isResolutionChanged()) {
    ofSetWindowShape(max((int)vimbaGrabber->getWidth(), 128), max((int)vimbaGrabber->getHeight(), 128));
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

//--------------------------------------------------------------

void ofApp::keyReleased(ofKeyEventArgs& key) {
  switch (key.key)
  {
  case 'm':
    toggleMultiCast = !vimbaGrabber->isMultiCast();
    vimbaGrabber->setMulticast(toggleMultiCast);
    break;
  case 'r':
    toggleReadOnly = !vimbaGrabber->isReadOnly();
    vimbaGrabber->setReadOnly(toggleReadOnly);
    break;
  case 'l':
    toggleUserSet = (vimbaGrabber->getUserSet() + 1) % 3;
    vimbaGrabber->setLoadUserSet(toggleUserSet);
    break;
  case 'd':
    toggleNextDevice = (toggleNextDevice + 1) % devices.size();
    vimbaGrabber->setDeviceID(devices.at(toggleNextDevice).id);
    break;
  default:
    break;
  }
}