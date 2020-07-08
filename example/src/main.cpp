#include "ofApp.h"
#include "ofMain.h"


//========================================================================
int main() {
  ofSetLogLevel(OF_LOG_NOTICE);
  ofSetupOpenGL(1024, 768, OF_WINDOW);  // <-------- setup the GL context

  // this kicks off the running of my app
  // can be OF_WINDOW or OF_FULLSCREEN
  // pass in width and height too:
  ofRunApp(new ofApp());
}
