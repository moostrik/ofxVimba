#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {

  vimbaGrabber = std::make_shared<ofxVimbaGrabber>();

  vimbaGrabber2 = std::make_shared<ofxVimbaGrabber>();

  grabber.setGrabber(vimbaGrabber);

  devices = grabber.listDevices();
  selectDevice = 0;
  if (devices.size() > 0) {
    grabber.setDeviceID(devices[selectDevice].id);
    vimbaGrabber2->setDeviceID(devices[1].id);
  }

//  vimbaGrabber->setReadOnly(true);
  vimbaGrabber->setMulticast(true);
  vimbaGrabber->setLoadUserSet(1);

  grabber.setPixelFormat(OF_PIXELS_RGB);  // or vimbaGrabber->setPixelFormat(OF_PIXELS_RGB);
  grabber.setDesiredFrameRate(30);        // or vimbaGrabber->setDesiredFrameRate(30);
  grabber.setup(ofGetWindowWidth(), ofGetWindowHeight(), true);

  vimbaGrabber2->setup();
//  grabber.setVerbose(true);

  // after setup this will not work
//  grabber.setPixelFormat(OF_PIXELS_GRAY);
  // but this will
//  vimbaGrabber->setPixelFormat(OF_PIXELS_GRAY);

}

//--------------------------------------------------------------
void ofApp::update() {
  grabber.update();
  vimbaGrabber2->update();

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

  if (vimbaGrabber2->isConnected()) {
    if (vimbaGrabber2->getPixels().isAllocated()) {
      ofImage image;
      image.setFromPixels(vimbaGrabber2->getPixels());
      image.draw(0,0,100,100);
    }
  }

  ofDrawCircle(glm::vec2(ofGetWindowWidth() - 20, sin(fmod(ofGetElapsedTimef() * 0.2, PI)) * ofGetWindowHeight()), 10);
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
    selectUserSet = (vimbaGrabber->getUserSet() + 1) % 3;
    vimbaGrabber->setLoadUserSet(selectUserSet);
    break;
  case 'd':
    devices = grabber.listDevices();
    selectDevice = (selectDevice + 1) % devices.size();
    vimbaGrabber->setDeviceID(devices.at(selectDevice).id);
    break;
  default:
    break;
  }
}
