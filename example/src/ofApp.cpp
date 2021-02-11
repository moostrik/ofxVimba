#include "ofApp.h"
// artist and technologist
// my main activity is capture and remix human behavior

//--------------------------------------------------------------
void ofApp::setup() {
  // ofSetLogLevel(OF_LOG_VERBOSE);
  vimbaGrabber = std::make_shared<ofxVimbaGrabber>();
  vimbaGrabber->enableMulticast();

  // vimbaGrabber->setIntFeature("DecimationHorizontal", 2);
  // vimbaGrabber->setIntFeature("DecimationVertical", 2);
  // vimbaGrabber->enableReadOnly();

  grabber.setGrabber(vimbaGrabber);
  vector<ofVideoDevice> devices = grabber.listDevices();
  if (devices.size() > 0) {
    grabber.setDeviceID(devices[0].id);
  }

  grabber.setDesiredFrameRate(60);
  grabber.setup(1280, 720, true);

  panel.setDefaultWidth(300);
  panel.setup("settings");
  updateParameters();
}

//--------------------------------------------------------------
void ofApp::update() {
  grabber.update();

  if (vimbaGrabber->isConnectionChanged()) {
    updateParameters();
    if (vimbaGrabber->isConnected()) {
      vimbaGrabber->loadFeatures();
    }
  }

  if (vimbaGrabber->isResolutionChanged()) {
    ofSetWindowShape(max((int)vimbaGrabber->getWidth(), 1024),
                     max((int)vimbaGrabber->getHeight(), 768));
  }
}

//--------------------------------------------------------------
void ofApp::draw() {
  ofClear(0, 0, 0);

  if (grabber.isInitialized()) {
    grabber.draw(0, 0);
  }

  panel.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {}

//--------------------------------------------------------------
void ofApp::minimizeNested(ofxGuiGroup &group, ofxGuiGroup *root) {
  if (group.getParent() && group.getParent() != root) {
    group.minimize();
  }

  for (std::size_t i = 0; i < group.getNumControls(); i++) {
    ofxGuiGroup *child = dynamic_cast<ofxGuiGroup *>(group.getControl(i));
    if (child) minimizeNested(*child, root ? root : &group);
  }
}

//--------------------------------------------------------------

void ofApp::updateParameters() {
  panel.clear();
  if (SHOW_ALL_PARAMETERS) {
    panel.add(vimbaGrabber->getParameters());
  } else {
    panel.add(vimbaGrabber->getParameters(ofxVimba::PARAMETERS_AUTOEXPOSURE));
  }
  minimizeNested(panel);
}
