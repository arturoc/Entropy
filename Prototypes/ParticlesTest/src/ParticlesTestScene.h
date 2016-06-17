#pragma once

#include "entropy/scene/Base.h"
#include "ParticleSystem.h"

namespace entropy
{
	namespace scene
	{
		class ParticlesTestScene
			: public Base
		{
		public:
			virtual string getName() const override 
			{
				return "entropy::scene::ParticlesTestScene";
			}

			ParticlesTestScene();
			~ParticlesTestScene();

			void setup();
			void exit();

			void update(double & dt);

			void drawBack();
			void drawWorld();
			void drawFront();

			void gui(ofxPreset::GuiSettings & settings);

			void serialize(nlohmann::json & json);
			void deserialize(const nlohmann::json & json);

		protected:
			ParticleSystem particleSystem;
			ofEasyCam cam;

			//ofSpherePrimitive sphere;

			virtual BaseParameters & getParameters() override
			{
				return this->parameters;
			}

			struct : BaseParameters
			{
				struct : ofParameterGroup
				{
					ofxPreset::Parameter<ofFloatColor> color{ "Color", ofFloatColor::crimson };
					ofxPreset::Parameter<bool> filled{ "Filled", false, true };
					ofxPreset::Parameter<float> radius{ "Radius", 20.0f, 0.0f, 200.0f };
					ofxPreset::Parameter<int> resolution{ "Resolution", 16, 3, 64 };

					PARAM_DECLARE("Sphere", color, filled, radius, resolution);
				} sphere;

				PARAM_DECLARE("Template", sphere);
			} parameters;
		};
	}
}