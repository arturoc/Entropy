#include "ofApp.h"

#include "entropy/scene/Template.h"

//--------------------------------------------------------------
void ofApp::setup()
{
	this->sceneManager = make_unique<entropy::scene::Manager>();

	auto sceneTemplate = make_shared<entropy::scene::Template>();
	this->sceneManager->addScene(sceneTemplate);
	this->sceneManager->setCurrentScene(sceneTemplate->getName());
}

//--------------------------------------------------------------
void ofApp::exit()
{
	this->sceneManager.reset();
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
