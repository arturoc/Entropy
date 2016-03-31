#include "CMBApp.h"
#include "GLError.h"

namespace entropy
{
    //--------------------------------------------------------------
    void CMBApp::setup()
    {
        ofSetLogLevel(OF_LOG_VERBOSE);
        //ofSetVerticalSync(false);

        tintColor = ofColor::white;
        dropColor = ofColor::red;

        bDropOnPress = false;
        bDropUnderMouse = false;
        dropRate = 1;

        damping = 0.995f;
        radius = 10.0f;
        ringSize = 0.5f;

        bRestart = true;
        bGuiVisible = true;

#ifdef COMPUTE_OPENCL
        openCL.setupFromOpenGL();

        openCL.loadProgramFromFile("cl/ripples.cl");
#ifdef THREE_D
        dropKernel = openCL.loadKernel("drop3D");
        ripplesKernel = openCL.loadKernel("ripples3D");
        copyKernel = openCL.loadKernel("copy3D");
#else
        dropKernel = openCL.loadKernel("drop2D");
        ripplesKernel = openCL.loadKernel("ripples2D");
        copyKernel = openCL.loadKernel("copy2D");
#endif  // THREE_D
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
        orthoCamera.setupPerspective(false, 60.0f, 0.1f, 1000.0f);
        orthoCamera.enableOrtho();

        shader.load("shaders/ripples");
#endif
    }

    //--------------------------------------------------------------
    void CMBApp::restart()
    {
        activeIndex = 0;

#ifdef THREE_D
        dimensions.x = 256;
        dimensions.y = 256;
        dimensions.z = 256;
#else
        dimensions.x = ofGetWidth();
        dimensions.y = ofGetHeight();

#ifdef COMPUTE_GLSL
#endif  // COMPUTE_GLSL

#endif  // THREE_D

        for (int i = 0; i < 2; ++i) {
#ifdef COMPUTE_OPENCL
#ifdef THREE_D
            clImages[i].initWithTexture3D(dimensions.x, dimensions.y, dimensions.z, GL_RGBA32F);
#else
            clImages[i].initWithTexture(dimensions.x, dimensions.y, GL_RGBA32F);
#endif  // THREE_D
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
#ifdef USE_CUSTOM_FBO
#ifdef THREE_D
            textures[i].allocate(dimensions.x, dimensions.y, dimensions.z, GL_RGBA32F);
#else
            ofTextureData texData;
            texData.width = dimensions.x;
            texData.height = dimensions.y;
            texData.glInternalFormat = GL_RGBA32F;
            texData.bFlipTexture = (i % 2 == 0);
            //textures[i].allocate(dimensions.x, dimensions.y, GL_RGBA32F);
            textures[i].allocate(texData);
            cout << "Texture " << i << " flipped? " << textures[i].texData.bFlipTexture << endl;
#endif
            lb::CheckGLError();

            fbos[i].allocate();
            fbos[i].attachTexture(textures[i], 0);
            fbos[i].begin();
            {
                ofClear(0, 0);
            }
            fbos[i].end();
            fbos[i].checkStatus();

            lb::CheckGLError();
#else
            ofFbo::Settings fboSettings;
            fboSettings.width = dimensions.x;
            fboSettings.height = dimensions.y;
            fboSettings.internalformat = GL_RGBA32F;
            fbos[i].allocate(fboSettings);
#endif  // USE_CUSTOM_FBO
#endif  // COMPUTE_GLSL
        }

#ifdef COMPUTE_OPENCL
#ifdef THREE_D
        clImageTmp.initWithTexture3D(dimensions.x, dimensions.y, dimensions.z, GL_RGBA32F);
#else
        clImageTmp.initWithTexture(dimensions.x, dimensions.y, GL_RGBA32F);
#endif  // THREE_D
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
        // Build a mesh to render a quad.
        mesh.clear();
        mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);

        mesh.addVertex(ofVec3f(0, 0));
        mesh.addVertex(ofVec3f(dimensions.x, 0));
        mesh.addVertex(ofVec3f(dimensions.x, dimensions.y));
        mesh.addVertex(ofVec3f(0, dimensions.y));

        mesh.addTexCoord(ofVec2f(0, 0));
        mesh.addTexCoord(ofVec2f(dimensions.x, 0));
        mesh.addTexCoord(ofVec2f(dimensions.x, dimensions.y));
        mesh.addTexCoord(ofVec2f(0, dimensions.y));
#endif  // COMPUTE_GLSL

        bRestart = false;
    }

    //--------------------------------------------------------------
    void CMBApp::update()
    {
        if (bRestart) {// || dimensions.x != ofGetWidth() || dimensions.y != ofGetHeight()) {
            restart();
        }

        int srcIdx = (activeIndex + 1) % 2;
        int dstIdx = activeIndex;

        // Add new drops.
        bool bMousePressed = ofGetMousePressed() && !bMouseOverGui;
        if ((bDropOnPress && bMousePressed) || (!bDropOnPress && ofGetFrameNum() % dropRate == 0)) {
#ifdef COMPUTE_OPENCL
            dropKernel->setArg(0, clImages[srcIdx]);
#ifdef THREE_D
            dropKernel->setArg(1, bDropUnderMouse ? ofVec4f(ofGetMouseX(), ofGetMouseY(), ofGetMouseY(), 0) : ofVec4f(ofRandom(dimensions.x), ofRandom(dimensions.y), ofRandom(dimensions.z), 0));
#else
            dropKernel->setArg(1, bDropUnderMouse ? ofVec2f(ofGetMouseX(), ofGetMouseY()) : ofVec2f(ofRandom(dimensions.x), ofRandom(dimensions.y)));
#endif  // THREE_D

            dropKernel->setArg(2, radius);
            dropKernel->setArg(3, ringSize);
            dropKernel->setArg(4, dropColor);
#ifdef THREE_D
            dropKernel->run3D(dimensions.x, dimensions.y, dimensions.z);
#else
            dropKernel->run2D(dimensions.x, dimensions.y);
#endif  // THREE_D

            openCL.finish();
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
            ofPushStyle();
            ofPushMatrix();

            //orthoCamera.begin();
            fbos[srcIdx].begin();
            ofScale(1.0, -1.0, 1.0);
            ofTranslate(0.0, -ofGetHeight(), 0.0);
            {
                if ((bDropOnPress && bMousePressed) || (!bDropOnPress && ofGetFrameNum() % dropRate == 0)) {
                    ofSetColor(dropColor);
                    ofNoFill();
                    if (bDropUnderMouse) {
                        ofDrawCircle(ofGetMouseX(), ofGetMouseY(), radius);
                    }
                    else {
                        ofDrawCircle(ofRandomWidth(), ofRandomHeight(), radius);
                    }
                }
            }
            fbos[srcIdx].end();
            //orthoCamera.end();

            ofPopMatrix();
            ofPopStyle();

#endif  // COMPUTE_GLSL
        }

        // Layer the drops.
#ifdef COMPUTE_OPENCL
        ripplesKernel->setArg(0, clImages[srcIdx]);
        ripplesKernel->setArg(1, clImages[dstIdx]);
        ripplesKernel->setArg(2, clImageTmp);
        ripplesKernel->setArg(3, damping / 10.0f + 0.9f);  // 0.9 - 1.0 range
#ifdef THREE_D
        ripplesKernel->run3D(dimensions.x, dimensions.y, dimensions.z);
#else
        ripplesKernel->run2D(dimensions.x, dimensions.y);
#endif  // THREE_D

        openCL.finish();

        // Copy temp image to dest (necessary since we can't read_write in OpenCL 1.2)
        copyKernel->setArg(0, clImageTmp);
        copyKernel->setArg(1, clImages[dstIdx]);
#ifdef THREE_D
        copyKernel->run3D(dimensions.x, dimensions.y, dimensions.z);

        openCL.finish();

        volumetrics.setup(&clImages[dstIdx].getTexture3D(), ofVec3f(1, 1, 1));
#else
        copyKernel->run2D(dimensions.x, dimensions.y);
#endif  // THREE_D
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
        // Layer the drops.
        //orthoCamera.begin();
        fbos[dstIdx].begin();
        shader.begin();
#ifdef THREE_D

#else
#ifdef USE_CUSTOM_FBO
        shader.setUniformTexture("uPrevBuffer", textures[dstIdx], 1);
        shader.setUniformTexture("uCurrBuffer", textures[srcIdx], 2);
#else
        shader.setUniformTexture("uPrevBuffer", fbos[dstIdx], 1);
        shader.setUniformTexture("uCurrBuffer", fbos[srcIdx], 2);
#endif  // USE_CUSTOM_FBO
#endif  // THREE_D
        shader.setUniform1f("uDamping", damping / 10.0f + 0.9f);  // 0.9 - 1.0 range
        {
            mesh.draw();
        }
        shader.end();
        fbos[dstIdx].end();
        //orthoCamera.end();

#endif  // COMPUTE_GLSL

#ifdef THREE_D
        volumetrics.setRenderSettings(1.0, 1.0, 1.0, 0.1);
#endif  // THREE_D

        activeIndex = 1 - activeIndex;
    }

    //--------------------------------------------------------------
    void CMBApp::imGui()
    {
        static const int kGuiMargin = 10;

        gui.begin();
        {
            ofVec2f windowPos(kGuiMargin, kGuiMargin);
            ofVec2f windowSize = ofVec2f::zero();

            ImGui::SetNextWindowPos(windowPos, ImGuiSetCond_Appearing);
            ImGui::SetNextWindowSize(ofVec2f(380, 94), ImGuiSetCond_Appearing);
            if (ImGui::Begin("CMB", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::Text("%.1f FPS (%.3f ms/frame)", ofGetFrameRate(), 1000.0f / ImGui::GetIO().Framerate);

                ImGui::Checkbox("Restart", &bRestart);

                ImGui::ColorEdit3("Tint Color", &tintColor[0]);
                ImGui::ColorEdit3("Drop Color", &dropColor[0]);

                ImGui::Checkbox("Drop On Press", &bDropOnPress);
                ImGui::Checkbox("Drop Under Mouse", &bDropUnderMouse);

                ImGui::SliderInt("Drop Rate", &dropRate, 1, 60);
                ImGui::SliderFloat("Damping", &damping, 0.0f, 1.0f);
                ImGui::SliderFloat("Radius", &radius, 1.0f, 50.0f);
                ImGui::SliderFloat("Ring Size", &ringSize, 0.0f, 5.0f);

                windowSize.set(ImGui::GetWindowSize());
                ImGui::End();
            }

            ofRectangle windowBounds(windowPos, windowSize.x, windowSize.y);
            bMouseOverGui = windowBounds.inside(ofGetMouseX(), ofGetMouseY());
        }
        gui.end();
    }

    //--------------------------------------------------------------
    void CMBApp::draw()
    {
        ofBackground(0);

        ofPushStyle();
        ofSetColor(tintColor);

#ifdef THREE_D
        cam.begin();
        {
            volumetrics.drawVolume(0, 0, 0, ofGetHeight(), 0);
        }
        cam.end();
#else
        int drawIdx = 1 - activeIndex;
        ofEnableAlphaBlending();

#ifdef COMPUTE_OPENCL
        clImages[drawIdx].getTexture().draw(0, 0);
#endif  // COMPUTE_OPENCL

#ifdef COMPUTE_GLSL
#ifdef USE_CUSTOM_FBO
        textures[drawIdx].draw(0, 0);
#else
        fbos[drawIdx].draw(0, 0);
#endif
#endif  // COMPUTE_GLSL

#endif  // THREE_D

        ofPopStyle();

        if (bGuiVisible) {
            imGui();
        }
    }

    //--------------------------------------------------------------
    void CMBApp::keyPressed(int key)
    {
        switch (key) {
            case '`':
                bGuiVisible ^= 1;
                break;

            case OF_KEY_TAB:
                ofToggleFullscreen();
                break;

            default:
                break;
        }
    }

    //--------------------------------------------------------------
    void CMBApp::keyReleased(int key){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseMoved(int x, int y ){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseDragged(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mousePressed(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseReleased(int x, int y, int button){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseEntered(int x, int y){

    }

    //--------------------------------------------------------------
    void CMBApp::mouseExited(int x, int y){

    }

    //--------------------------------------------------------------
    void CMBApp::windowResized(int w, int h)
    {
        bRestart = true;
    }

    //--------------------------------------------------------------
    void CMBApp::gotMessage(ofMessage msg){
        
    }
    
    //--------------------------------------------------------------
    void CMBApp::dragEvent(ofDragInfo dragInfo){ 
        
    }
}