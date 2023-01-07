#ifndef RIBBON_TRAIL_COMPONENT_H
#define RIBBON_TRAIL_COMPONENT_H

#include "GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
	/**
	 * @class 	RibbonTrailComponent
	 * @brief 	This component can be used to show nice trailing effects.
	 *			Info: Several components can be attached to the owner game object and played simultanously, but maybe this does not make sense...
	 *			Example: Let a comet fly with a trailing effect.
	 *			Requirements: None
	 */
	 
	class EXPORTED RibbonTrailComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::RibbonTrailComponent> RibbonTrailCompPtr;
	public:
	
		RibbonTrailComponent();

		virtual ~RibbonTrailComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		 * @see		GameObjectComponent::postInit
		 */
		virtual bool postInit(void) override;
		
		/**
		 * @see		GameObjectComponent::connect
		 */
		virtual bool connect(void) override;

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

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("RibbonTrailComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "RibbonTrailComponent";
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
			return "Usage: With this component ribbon trail effect is created.";
		}

		/**
		 * @see		GameObjectComponent::update
		 */
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @brief Sets data block with the trail graphics definition.
		 * @param[in] datablockName The datablock name to set.
		 * @note	The datablock must exist and conform the trail specifigs. See ogre forum and examples or e.g. "LightRibbonTrail".
		 */
		void setDatablockName(const Ogre::String& datablockName);

		/**
		 * @brief Gets currently used datablock name.
		 * @return datablockName The datablock name to get
		 */
		Ogre::String getDatablockName(void) const;
		
		/**
		 * @brief Sets max elements for one chain to build this trail.
		 * @param[in] maxChainElements The max number of chain elements to set. The higher the number the more complex the trail.
		 */
		void setMaxChainElements(unsigned int maxChainElements);
		
		/**
		 * @brief Gets max number of used chain elements.
		 * @return maxChainElements The max chain elements to get
		 */
		unsigned int getMaxChainElements(void) const;
		
		/**
		 * @brief Sets number of chains to build this trail.
		 * @param[in] numberOfChains The number of chains to set. The higher the number the more complex the trail.
		 */
		void setNumberOfChains(unsigned int numberOfChains);
		
		/**
		 * @brief Gets the number of chains.
		 * @return numberOfChains The number of chains to get
		 */
		unsigned int getNumberOfChains(void) const;
		
		/**
		 * @brief Sets color change for a chain, that is changed each second.
		 * @param[in] chainIndex 	 The chain index in which the color should be changed
		 * @param[in] valuePerSecond The new color that is changed each second
		 */
		void setColorChange(unsigned int chainIndex, const Ogre::Vector3& valuePerSecond);
		
		/**
		 * @brief Gets the color change for the given chain index.
		 * @param[in] chainIndex 	 The chain index to get the color
		 * @return color The color to get
		 */
		Ogre::Vector3 getColorChange(unsigned int chainIndex) const;
		
		/**
		 * @brief Sets relative width change for a chain, that is changed each second.
		 * @param[in] chainIndex 	 	  The chain index in which the color should be changed
		 * @param[in] widthDeltaPerSecond The relative width to set
		 */
		void setWidthChange(unsigned int chainIndex, Ogre::Real widthDeltaPerSecond);
		
		/**
		 * @brief Gets the relative width change for the given chain index.
		 * @param[in] chainIndex 	 The chain index to get the color
		 * @return width The width to get
		 */
		Ogre::Real getWidthChange(unsigned int chainIndex) const;
		
		/**
		 * @brief Sets initial color for a chain.
		 * @param[in] chainIndex 	 The chain index in which the initial color should be set
		 * @param[in] color 		 The new initial color to set
		 */
		void setInitialColor(unsigned int chainIndex, const Ogre::Vector3& color);
		
		/**
		 * @brief Gets the initial color for the given chain index.
		 * @param[in] chainIndex 	 The chain index to get the initial color
		 * @return color The initial color to get
		 */
		Ogre::Vector3 getInitialColor(unsigned int chainIndex) const;
		
		/**
		 * @brief Sets initial width for a chain.
		 * @param[in] chainIndex 	 The chain index in which the initial width should be set
		 * @param[in] width 		 The new initial width to set
		 */
		void setInitialWidth(unsigned int chainIndex, Ogre::Real width);
		
		/**
		 * @brief Gets the initial width for the given chain index.
		 * @param[in] chainIndex 	 The chain index to get the initial width
		 * @return width The initial width to get
		 */
		Ogre::Real getInitialWidth(unsigned int chainIndex) const;
		
		/**
		 * @brief Sets face camera id, so that the ribbon trail always faces the current camera direction.
		 * @param[in] faceCameraId 	 The face camera id to set.
		 * @note	When no or an invalid id is set, the ribbon trail will not face the camera.
		 */
		void setFaceCameraId(unsigned long faceCameraId);
		
		/**
		 * @brief Gets the face camera id
		 * @return faceCameraId The face camera id to get
		 */
		unsigned long getFaceCameraId(void) const;
		
		/**
		 * @brief Sets a different texture coordinates to offset a picture for the ribbon trail.
		 * @param[in] texCoordChange 	 The texture coordinates to change
		 */
		void setTexCoordChange(const Ogre::Vector2& texCoordChange);
		
		/**
		 * @brief Gets the texture coordinates
		 * @return texCoordChange The texture coordinates to get
		 */
		Ogre::Vector2 getTexCoordChange(void) const;
		
		/**
		 * @brief Sets the ribbon trail length
		 * @param[in] trailLength 	 The trail length to set
		 */
		void setTrailLength(Ogre::Real trailLength);
		
		/**
		 * @brief Gets the trail length
		 * @return trailLength The trail length to get
		 */
		Ogre::Real getTrailLength(void) const;

		/**
		 * @brief Sets the render distance at which the ribbon trail will be visible.
		 * @param[in] renderDistance 	 The render distance to set
		 */
		void setRenderDistance(Ogre::Real renderDistance);

		/**
		 * @brief Gets the render distance at which the ribbon trail will be visible.
		 * @return renderDistance The render distance to get
		 */
		Ogre::Real getRenderDistance(void) const;

		/**
		 * @brief Gets the ribbton trail internal pointer for manual manipulation.
		 * @return ribbonTrail The ribbon trail to get
		 */
		Ogre::v1::RibbonTrail* getRibbonTrail(void) const;
	public:
		static const Ogre::String AttrDatablockName(void) { return "Datablock Name"; }
		static const Ogre::String AttrMaxChainElements(void) { return "Max. Chain Elements"; }
		static const Ogre::String AttrNumberOfChains(void) { return "Number Of Chains"; }
		static const Ogre::String AttrColorChange(void) { return "Color Change "; }
		static const Ogre::String AttrWidthChange(void) { return "Width Change "; }
		static const Ogre::String AttrInitialColor(void) { return "Initial Color "; }
		static const Ogre::String AttrInitialWidth(void) { return "Initial Width "; }
		static const Ogre::String AttrFaceCameraId(void) { return "Face Camera Id"; }
		static const Ogre::String AttrTextureCoordRange(void) { return "Coordinate Range"; }
		static const Ogre::String AttrTrailLength(void) { return "Trail Length"; }
		static const Ogre::String AttrRenderDistance(void) { return "Render Distance"; }
	private:
		void createRibbonTrail(void);
		
		void handleRemoveCamera(NOWA::EventDataPtr eventData);
	private:
		Ogre::v1::RibbonTrail* ribbonTrail;
		Ogre::Camera* camera;
		
		Variant* datablockName;
		Variant* maxChainElements;
		Variant* numberOfChains;
		std::vector<Variant*> colorChanges; // vec3
		std::vector<Variant*> widthChanges; // real
		std::vector<Variant*> initialColors; // vec3
		std::vector<Variant*> initialWidths; // real
		Variant* faceCameraId; // ulong
		Variant* textureCoordRange; // vec2
		Variant* trailLength;
		Variant* renderDistance;
	};

}; //namespace end

#endif