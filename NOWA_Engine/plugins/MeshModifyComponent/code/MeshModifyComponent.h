/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef MESHMODIFYCOMPONENT_H
#define MESHMODIFYCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief		Your documentation
	  */
	class EXPORTED MeshModifyComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener
	{
	public:
		typedef boost::shared_ptr<MeshModifyComponent> MeshModifyComponentPtr;
	public:

		MeshModifyComponent();

		virtual ~MeshModifyComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setBrushName(const Ogre::String& brushName); 

		Ogre::String getBrushName(void) const;

		void setBrushSize(int brushSize);

		int getBrushSize(void) const;

		void setBrushIntensity(Ogre::Real brushIntensity); 

		Ogre::Real getBrushIntensity(void) const;

		void setCategory(const Ogre::String& category); 

		Ogre::String getCategory(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MeshModifyComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "MeshModifyComponent";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: My usage text.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrBrushName(void) { return "BrushName"; }
		static const Ogre::String AttrBrushSize(void) { return "BrushSize"; }
		static const Ogre::String AttrBrushIntensity(void) { return "BrushIntensity"; }
		static const Ogre::String AttrCategory(void) { return "Category"; }
	protected:
		/**
		 * @brief		Actions on mouse moved event.
		 * @see			OIS::MouseListener::mouseMoved()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

		/**
		 * @brief		Actions on mouse pressed event.
		 * @see			OIS::MouseListener::mousePressed()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

		/**
		 * @brief		Actions on mouse released event.
		 * @see			OIS::MouseListener::mouseReleased()
		 * @param[in]	evt		The mouse event
		 * @return		true			if other key listener successors shall have the chance to react or not.
		 */
		virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	private:

		Ogre::Real getBrushValue(size_t vertexIndex);

		void modifyMeshWithBrush(const Ogre::Vector3& brushPosition);
	private:
		Ogre::String name;
		std::vector<Ogre::Real> brushData;
		Ogre::RaySceneQuery* raySceneQuery;
		bool isPressing;
		Ogre::Vector3 brushPosition;
		Ogre::MovableObject* currentEditingObject;
		Ogre::Vector3* originalVertices;
		size_t originalVertexCount;
		unsigned long* originalIndices;
		size_t originalIndexCount;
		Ogre::Vector3* originalNormals;
		Ogre::Vector2* originalTextureCoordinates;
		Ogre::Item* editableItem;
		Ogre::VertexBufferPacked* dynamicVertexBuffer;
		Ogre::IndexBufferPacked* dynamicIndexBuffer;
		bool isVET_HALF4;
		bool isIndices32;

		Variant* activated;
		Variant* brushName;
		Variant* brushSize;
		Variant* brushIntensity;
		Variant* category;
	};

}; // namespace end

#endif

