#include "Base.h"

namespace entropy
{
	namespace scene
	{
		static const string kPresetDefaultName = "_default";

		//--------------------------------------------------------------
		ofCamera & Base::getCamera()
		{
			return this->camera;
		}
			
		//--------------------------------------------------------------
		Base::Base()
		{
			this->imgui.setup();
			this->overlayVisible = true;
		}

		//--------------------------------------------------------------
		Base::~Base()
		{
			
		}

		//--------------------------------------------------------------
		void Base::setup()
		{
			this->onSetup.notify();
			
			// List presets.
			this->populatePresets();

			// Setup timeline.
			this->timeline.setup();
			this->timeline.setLoopType(OF_LOOP_NONE);
			this->timeline.setFrameRate(30.0f);
			this->timeline.setDurationInSeconds(30);
			this->timeline.setAutosave(false);
			this->timeline.setPageName(this->getParameters().base.getName());

			const auto trackName = "Camera";
			const auto trackIdentifier = this->getParameters().base.getName() + "_" + trackName;
			this->cameraTrack = new ofxTLCameraTrack();
			this->cameraTrack->setCamera(this->getCamera());
			this->cameraTrack->setXMLFileName(this->timeline.nameToXMLName(trackIdentifier));
			this->timeline.addTrack(trackIdentifier, this->cameraTrack);
			this->cameraTrack->setDisplayName(trackName);
			this->cameraTrack->lockCameraToTrack = false;

			// List mappings.
			this->populateMappings(this->getParameters());

			// Load default preset.
			this->currPreset.clear();
			this->loadPreset(kPresetDefaultName);
		}

		//--------------------------------------------------------------
		void Base::exit()
		{
			this->onExit.notify();

			// Save default preset.
			this->savePreset(kPresetDefaultName);
		}

		//--------------------------------------------------------------
		void Base::update()
		{
			for (auto & it : this->mappings)
			{
				it.second->update();
			}
			
			auto dt = ofGetLastFrameTime();
			this->onUpdate.notify(dt);
		}

		//--------------------------------------------------------------
		void Base::draw()
		{
			this->drawBack();
			this->drawWorld();
			this->drawFront();

			this->guiSettings.mouseOverGui = false;
			if (this->overlayVisible)
			{
				this->drawOverlay();
			}

			if (this->guiSettings.mouseOverGui)
			{
				this->camera.disableMouseInput();
			}
			else /*if (this->parameters.camera.mouseEnabled)*/
			{
				this->camera.enableMouseInput();
			}
		}

		//--------------------------------------------------------------
		void Base::drawBack()
		{
			ofBackground(this->getParameters().base.background.get());

			this->onDrawBack.notify();
		}

		//--------------------------------------------------------------
		void Base::drawWorld()
		{
			this->getCamera().begin();
			{
				this->onDrawWorld.notify();
			}
			this->getCamera().end();
		}

		//--------------------------------------------------------------
		void Base::drawFront()
		{
			this->onDrawFront.notify();
		}

		//--------------------------------------------------------------
		void Base::drawOverlay()
		{
			this->imgui.begin(); 
			{
				this->guiSettings.windowPos = ofVec2f(kGuiMargin, kGuiMargin);
				this->guiSettings.windowSize = ofVec2f::zero(); 
				
				this->gui(this->guiSettings);
			}
			this->imgui.end();

			this->timeline.setOffset(ofVec2f(0.0, ofGetHeight() - this->timeline.getHeight()));
			this->timeline.draw();
			this->guiSettings.mouseOverGui |= this->timeline.getDrawRect().inside(ofGetMouseX(), ofGetMouseY());
		}

		//--------------------------------------------------------------
		void Base::gui(ofxPreset::GuiSettings & settings)
		{
			auto & parameters = this->getParameters();

			if (ofxPreset::Gui::BeginWindow("App", this->guiSettings))
			{
				ImGui::Text("%.1f FPS (%.3f ms/frame)", ofGetFrameRate(), 1000.0f / ImGui::GetIO().Framerate);
			}
			ofxPreset::Gui::EndWindow(this->guiSettings);

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow("Presets", settings))
			{
				if (ImGui::Button("Save"))
				{
					this->savePreset(this->currPreset);
				}
				ImGui::SameLine();
				if (ImGui::Button("Save As..."))
				{
					auto name = ofSystemTextBoxDialog("Enter a name for the preset", "");
					if (!name.empty())
					{
						this->savePreset(name);
					}
				}

				ImGui::ListBoxHeader("Load", 3);
				for (auto & name : this->presets)
				{
					if (ImGui::Selectable(name.c_str(), (name == this->currPreset)))
					{
						this->loadPreset(name);
					}
				}
				ImGui::ListBoxFooter();
			}
			ofxPreset::Gui::EndWindow(settings);

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow("Mappings", settings))
			{
				for (auto & it : this->mappings)
				{
					auto mapping = it.second;
					if (ofxPreset::Gui::AddParameter(mapping->animated))
					{
						mapping->animated.update();
						if (mapping->animated)
						{
							mapping->addTrack(this->timeline);
						}
						else
						{
							mapping->removeTrack(this->timeline);
						}
					}
				}
			}
			ofxPreset::Gui::EndWindow(settings);

			ofxPreset::Gui::SetNextWindow(settings);
			if (ofxPreset::Gui::BeginWindow(parameters.base.getName(), settings))
			{
				ofxPreset::Gui::AddParameter(parameters.base.background);
			}
			ofxPreset::Gui::EndWindow(settings);

			this->onGui.notify(settings);
		}

		//--------------------------------------------------------------
		void Base::serialize(nlohmann::json & json)
		{
			ofxPreset::Serializer::Serialize(json, this->getParameters()); 
			ofxPreset::Serializer::Serialize(json, this->getCamera(), "camera");
			
			this->onSerialize.notify(json);

			auto & jsonGroup = json["Mappings"];
			for (auto & it : this->mappings)
			{
				ofxPreset::Serializer::Serialize(jsonGroup, it.second->animated);
			}
		}

		//--------------------------------------------------------------
		void Base::deserialize(const nlohmann::json & json)
		{
			ofxPreset::Serializer::Deserialize(json, this->getParameters());
			if (json.count("camera"))
			{
				// Disable auto distance so that it doesn't interfere with the camera matrix.
				// This is done here because getCamera() returns an ofCamera and not an ofEasyCam.
				this->camera.setAutoDistance(false);

				ofxPreset::Serializer::Deserialize(json, this->getCamera(), "camera");
			}

			this->onDeserialize.notify(json);

			for (auto & it : this->mappings)
			{
				it.second->animated.set(false);
			}
			if (json.count("Mappings"))
			{
				auto & jsonGroup = json["Mappings"];
				for (auto & it : this->mappings)
				{
					ofxPreset::Serializer::Deserialize(jsonGroup, it.second->animated);
				}
			}
			this->refreshMappings();
		}

		//--------------------------------------------------------------
		string Base::getDataPath(const string & file)
		{
			if (this->dataPath.empty())
			{
				auto tokens = ofSplitString(this->getName(), "::", true, true);
				auto dataPath = ofFilePath::addTrailingSlash(ofToDataPath("../../../../Shared/data"));
				for (auto & component : tokens)
				{
					dataPath = ofFilePath::addTrailingSlash(dataPath.append(component));
				}
				this->dataPath = dataPath;
			}
			if (file.empty()) 
			{
				return this->dataPath;
			}

			auto filePath = this->dataPath;
			filePath.append(file);
			return filePath;
		}

		//--------------------------------------------------------------
		string Base::getPresetPath(const string & preset)
		{
			auto presetPath = ofFilePath::addTrailingSlash(this->getDataPath("presets"));
			if (!preset.empty())
			{
				presetPath.append(ofFilePath::addTrailingSlash(preset));
			}
			return presetPath;
		}

		//--------------------------------------------------------------
		bool Base::loadPreset(const string & presetName)
		{
			const auto presetPath = this->getPresetPath(presetName);
			auto presetFile = ofFile(presetPath);
			if (!presetFile.exists())
			{
				ofLogWarning("Base::loadPreset") << "File not found at path " << presetPath;
				return false;
			}

			auto paramsPath = presetPath;
			paramsPath.append("parameters.json");
			auto paramsFile = ofFile(paramsPath);
			if (paramsFile.exists())
			{
				nlohmann::json json;
				paramsFile >> json;

				this->deserialize(json);
			}

			this->timeline.loadTracksFromFolder(presetPath);

			this->currPreset = presetName;

			return true;
		}

		//--------------------------------------------------------------
		bool Base::savePreset(const string & presetName)
		{
			const auto presetPath = this->getPresetPath(presetName);

			auto paramsPath = presetPath;
			paramsPath.append("parameters.json");
			auto paramsFile = ofFile(paramsPath, ofFile::WriteOnly);
			nlohmann::json json;
			this->serialize(json);
			paramsFile << json;

			this->timeline.saveTracksToFolder(presetPath);

			this->populatePresets();

			return true;
		}

		//--------------------------------------------------------------
		void Base::populatePresets()
		{
			auto presetsPath = this->getPresetPath();
			auto presetsDir = ofDirectory(presetsPath);
			presetsDir.listDir();
			presetsDir.sort();

			this->presets.clear();
			for (auto i = 0; i < presetsDir.size(); ++i)
			{
				if (presetsDir.getFile(i).isDirectory())
				{
					this->presets.push_back(presetsDir.getName(i));
				}
			}
		}

		//--------------------------------------------------------------
		void Base::populateMappings(const ofParameterGroup & group, string name)
		{
			for (const auto & parameter : group)
			{
				// Group.
				auto parameterGroup = dynamic_pointer_cast<ofParameterGroup>(parameter);
				if (parameterGroup)
				{
					// Recurse through contents.
					this->populateMappings(*parameterGroup, name);
					continue;
				}

				// Parameters.
				auto parameterFloat = dynamic_pointer_cast<ofParameter<float>>(parameter);
				if (parameterFloat)
				{
					auto mapping = make_shared<Mapping<float, ofxTLCurves>>();
					mapping->setup(parameterFloat);
					this->mappings.emplace(mapping->getName(), mapping);
					continue;
				}
				auto parameterInt = dynamic_pointer_cast<ofParameter<int>>(parameter);
				if (parameterInt)
				{
					auto mapping = make_shared<Mapping<int, ofxTLCurves>>();
					mapping->setup(parameterInt);
					this->mappings.emplace(mapping->getName(), mapping);
					continue;
				}
				auto parameterBool = dynamic_pointer_cast<ofParameter<bool>>(parameter);
				if (parameterBool)
				{
					auto mapping = make_shared<Mapping<bool, ofxTLSwitches>>();
					mapping->setup(parameterBool);
					this->mappings.emplace(mapping->getName(), mapping);
					continue;
				}
				auto parameterColor = dynamic_pointer_cast<ofParameter<ofFloatColor>>(parameter);
				if (parameterColor)
				{
					auto mapping = make_shared<Mapping<ofFloatColor, ofxTLColorTrack>>();
					mapping->setup(parameterColor);
					this->mappings.emplace(mapping->getName(), mapping);
					continue;
				}
			}
		}

		//--------------------------------------------------------------
		void Base::refreshMappings()
		{
			for (auto & it : this->mappings)
			{
				auto mapping = it.second;
				if (mapping->animated) 
				{
					mapping->addTrack(this->timeline);
				}
				else
				{
					mapping->removeTrack(this->timeline);
				}
			}
		}

		//--------------------------------------------------------------
		void Base::setOverlayVisible(bool overlayVisible)
		{
			this->overlayVisible = overlayVisible;
		}

		//--------------------------------------------------------------
		void Base::toggleOverlayVisible()
		{
			this->overlayVisible ^= 1;
		}

		//--------------------------------------------------------------
		bool Base::isOverlayVisible() const
		{
			return this->overlayVisible;
		}

		//--------------------------------------------------------------
		void Base::setCameraLocked(bool cameraLocked)
		{
			this->cameraTrack->lockCameraToTrack = cameraLocked;
		}

		//--------------------------------------------------------------
		void Base::toggleCameraLocked()
		{
			this->cameraTrack->lockCameraToTrack ^= 1;
		}

		//--------------------------------------------------------------
		bool Base::isCameraLocked() const
		{
			return this->cameraTrack->lockCameraToTrack;
		}

		//--------------------------------------------------------------
		void Base::addCameraKeyframe()
		{
			this->cameraTrack->addKeyframe();
		}
	}
}