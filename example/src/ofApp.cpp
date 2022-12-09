#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  selectDevice = 0;
  toggleReadOnly = false;
  toggleMultiCast = false;
  toggleMono = false;
  selectUserSet = 1;
  vimbaGrabber = std::make_shared<ofxVimba::ofVideoGrabber>();

  grabber.setGrabber(vimbaGrabber);

  devices = grabber.listDevices();
  selectDevice = 0;
  if (devices.size() > 0) {
    grabber.setDeviceID(devices[selectDevice].id);
  }

  vimbaGrabber->setReadOnly(toggleReadOnly);
  vimbaGrabber->setMulticast(toggleMultiCast);
  vimbaGrabber->setLoadUserSet(selectUserSet);

  ofPixelFormat format = toggleMono? OF_PIXELS_MONO: OF_PIXELS_RGB;
  grabber.setPixelFormat(format);  // or vimbaGrabber->setPixelFormat(OF_PIXELS_RGB);
  grabber.setDesiredFrameRate(30);        // or vimbaGrabber->setDesiredFrameRate(30);
  grabber.setup(ofGetWindowWidth(), ofGetWindowHeight(), true);

  text = "Press Key: \n"
         "'d' to select next device \n"
         "'r' to toggle readonly \n"
         "'c' to toggle multicast \n"
         "'m' to toggle mono \n"
         "'l' to load next userset";
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
  }
  drawText();
}

void ofApp::drawText() {
  string status;
  if (grabber.isInitialized()) {
    status = vimbaGrabber->getDeviceId();
    if(vimbaGrabber->isConnected()) {
      status += " connected";
      if (vimbaGrabber->isReadOnly()) status += " in readonly mode";
      else if (vimbaGrabber->isMultiCast()) status += " with multicast enabled";
    }
    else
      status += " not connected";
  } else
    status = "grabber not initialized";

  ofDrawBitmapStringHighlight(status, glm::vec2(10,20));
  ofDrawBitmapStringHighlight(ofToString(int(ofGetFrameRate())), glm::vec2(ofGetWindowWidth() - 50 ,20));
  ofDrawBitmapStringHighlight(text, glm::vec2(10, ofGetWindowHeight() - 80));
}

//--------------------------------------------------------------

void ofApp::keyReleased(ofKeyEventArgs& key) {
  switch (key.key)
  {
  case 'd':
    devices = grabber.listDevices();
    if (devices.empty()) break;
    selectDevice = (selectDevice + 1) % devices.size();
    vimbaGrabber->setDeviceID(devices.at(selectDevice).id);
    //  notice that the similar method
    //    grabber.setDeviceID(devices.at(selectDevice).id);
    //  will not work, as ofVideoGrabber will not allow
    //  the device to be set while the grabber is running
    break;
  case 'r':
    toggleReadOnly = !vimbaGrabber->isReadOnly();
    vimbaGrabber->setReadOnly(toggleReadOnly);
    break;
  case 'c':
    toggleMultiCast = !vimbaGrabber->isMultiCast();
    vimbaGrabber->setMulticast(toggleMultiCast);
    break;
  case 'm':
    toggleMono = !toggleMono;
    vimbaGrabber->setPixelFormat(toggleMono? OF_PIXELS_MONO: OF_PIXELS_RGB);
    break;
  case 'l':
    selectUserSet = (vimbaGrabber->getUserSet() + 1) % 3;
    vimbaGrabber->setLoadUserSet(selectUserSet);
    break;
  default:
    break;
  }
}
