#ifndef TERRA_COMPONENT_H
#define TERRA_COMPONENT_H

#include "GameObjectComponent.h"
#include "Terra.h"
#include "OgreHlmsPbsTerraShadows.h"
#include "TerraWorkspaceListener.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED TerraComponent : public GameObjectComponent
	{
	public:
		friend class CreateTerraProcess;

		typedef boost::shared_ptr<NOWA::TerraComponent> TerraCompPtr;
	public:
	
		TerraComponent();

		virtual ~TerraComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;
		
		/**
		 * @see		GameObjectComponent::connect
		 */
		virtual bool connect(void) override;

		/**
		 * @see		GameObjectComponent::disconnect
		 */
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void) override;

		/**
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

		/**
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
		
		/**
		 * @see		GameObjectComponent::onCloned
		 */
		virtual bool onCloned(void) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("TerraComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "TerraComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "This component creates an Ogre terrain (called Terra).";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
		
		void setPixelSize(const Ogre::Vector2& pixelSize);
		
		Ogre::Vector2 getPixelSize(void) const;
		
		void setDimensions(const Ogre::Vector3& dimensions);
		
		Ogre::Vector3 getDimensions(void) const;
		
		void setLightId(unsigned long lightId);

		unsigned int getLightId(void) const;

		void setCameraId(unsigned long cameraId);

		unsigned int geCameraId(void) const;

		void setBasePixelDimension(unsigned int basePixelDimension);

		unsigned int getBasePixelDimension(void) const;

		void setStrength(int strength);

		int getStrength(void) const;

		void setBrushName(const Ogre::String& brushName);

		Ogre::String getBrushName(void) const;

		std::vector<Ogre::String> getAllBrushNames(void) const;

		void setBrushSize(short brushSize);

		short getBrushSize(void) const;

		void setBrushIntensity(int brushIntensity);

		int getBrushIntensity(void) const;

		void setImageLayer(const Ogre::String& imageLayer);

		Ogre::String getImageLayer(void) const;

		unsigned int getImageLayerId(void) const;

		void setImageLayerId(unsigned int imageLayerId);

		std::vector<Ogre::String> getAllImageLayer(void) const;

		Ogre::Terra* getTerra(void) const;

		void createTerra(void);

		void modifyTerrainStart(const Ogre::Vector3& position, float strength);

		void smoothTerrainStart(const Ogre::Vector3& position, float strength);

		void paintTerrainStart(const Ogre::Vector3& position, float intensity, int imageLayer);

		void modifyTerrain(const Ogre::Vector3& position, float strength);

		void smoothTerrain(const Ogre::Vector3& position, float strength);

		void paintTerrain(const Ogre::Vector3& position, float intensity, int imageLayer);

		// For lua
		void modifyTerrainEnd(void);

		void smoothTerrainEnd(void);

		void paintTerrainEnd(void);

		void modifyTerrainLoop(const Ogre::Vector3& position, float strength, unsigned int loopCount);

		void smoothTerrainLoop(const Ogre::Vector3& position, float strength, unsigned int loopCount);

		void paintTerrainLoop(const Ogre::Vector3& position, float intensity, int imageLayer, unsigned int loopCount);

		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> modifyTerrainFinished(void);

		std::pair<std::vector<Ogre::uint16>, std::vector<Ogre::uint16>> smoothTerrainFinished(void);

		std::pair<std::vector<Ogre::uint8>, std::vector<Ogre::uint8>> paintTerrainFinished(void);

		void setHeightData(const std::vector<Ogre::uint16>& heightData);

		void setBlendWeightData(const std::vector<Ogre::uint8>& blendWeightData);

		std::tuple<bool, Ogre::Vector3, Ogre::Vector3, Ogre::Real> checkRayIntersect(const Ogre::Ray& ray);
	public:
		static const Ogre::String AttrCenter(void) { return "Center"; }
		static const Ogre::String AttrPixelSize(void) { return "Pixel Size"; }
		static const Ogre::String AttrDimensions(void) { return "Dimensions"; }
		static const Ogre::String AttrLightId(void) { return "Light Id"; }
		static const Ogre::String AttrCameraId(void) { return "Camera Id"; }
		static const Ogre::String AttrBasePixelDimension(void) { return "Base Pixel Dimension"; }
		static const Ogre::String AttrStrength(void) { return "Strength"; }
		static const Ogre::String AttrBrush(void) { return "Brush"; }
		static const Ogre::String AttrBrushSize(void) { return "Brush Size"; }
		static const Ogre::String AttrBrushIntensity(void) { return "Brush Intensity"; }
		static const Ogre::String AttrImageLayer(void) { return "Image Layer"; }
	private:
		void destroyTerra(void);
		void handleSwitchCamera(NOWA::EventDataPtr eventData);
		void handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData);
	private:
		Variant* center;
		Variant* pixelSize;
		Variant* dimensions;
		Variant* lightId;
		Variant* cameraId;
		Variant* basePixelDimension;
		Variant* strength;
		Variant* brush;
		Variant* brushSize;
		Variant* brushIntensity;
		Variant* imageLayer;

		Ogre::Terra* terra;
		// Listener to make PBS objects also be affected by terrains shadows
		Ogre::HlmsPbsTerraShadows* hlmsPbsTerraShadows;
		Ogre::TerraWorkspaceListener* terraWorkspaceListener;
		Ogre::Light* sunLight;
		Ogre::Camera* usedCamera;
		bool postInitDone;
		bool terraLoadedFromFile;
	};

}; //namespace end

#endif