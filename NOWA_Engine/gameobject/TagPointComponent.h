#ifndef TAG_POINT_COMPONENT_H
#define TAG_POINT_COMPONENT_H

#include "Animation/OgreTagPoint2.h" // v2 TagPoint (Ogre::TagPoint)
#include "GameObjectComponent.h"
#include "OgreTagPoint.h"

namespace NOWA
{
    class TagPointComponent;
    class PhysicsActiveComponent;

    void setSourceId(TagPointComponent* instance, const Ogre::String& sourceId);
    Ogre::String getSourceId(TagPointComponent* instance);

    /**
     * @class 	TagPointComponent
     * @brief 	This component can be used to attach another source game object to the local tag point (bone).
     *			Info: Several tag point components can be added to one game object.
     *			Example: A weapon (source game object) could be attached to the right hand of the player.
     *			Requirements: The item of this game object must have a skeleton with bones.
     */
    class EXPORTED TagPointComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<TagPointComponent> TagPointCompPtr;
        typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;

    public:
        TagPointComponent();

        virtual ~TagPointComponent();

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
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

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
            return NOWA::getIdFromName("TagPointComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "TagPointComponent";
        }

        /**
         * @see  GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::executeAction
         */
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::showDebugData
         */
        virtual void showDebugData(void) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "This component can be used to attach another source game object to the local tag point (bone). "
                   "Info: Several tag point components can be added to one game object. "
                   "Example : A weapon(source game object) could be attached to the right hand of the player. "
                   "Requirements : Tags a game object to a bone.Requirements: Item must have animations.";
        }

        /**
         * @brief Sets the tag point name the source game object should be attached to
         * @param[in] tagPointName The tag point name to set
         */
        void setTagPointName(const Ogre::String& tagPointName);

        /**
         * @brief Gets the current active tag point name
         * @return tagPointName The current tag point name to get
         */
        Ogre::String getTagPointName(void) const;

        /**
         * @brief Sets source id for the game object that should be attached to this tag point.
         * @param[in] sourceId The sourceId to set
         */
        void setSourceId(unsigned long sourceId);

        /**
         * @brief Gets the source id for the game object that is attached to this tag point.
         * @return sourceId The sourceId to get
         */
        unsigned long getSourceId(void) const;

        /**
         * @brief Sets an offset position at which the source game object should be attached
         * @param[in] offsetPosition The offset position to set
         */
        void setOffsetPosition(const Ogre::Vector3& offsetPosition);

        /**
         * @brief Gets the offset position at which the source game object is attached
         * @return offsetPosition The offset position to get
         */
        Ogre::Vector3 getOffsetPosition(void) const;

        /**
         * @brief Sets an offset orientation at which the source game object should be attached
         * @param[in] offsetOrientation The offset orientation to set (degreeX, degreeY, degreeZ)
         */
        void setOffsetOrientation(const Ogre::Vector3& offsetOrientation);

        /**
         * @brief Gets the offset orientation at which the source game object is attached
         * @return offsetOrientation The offset orientation to get (degreeX, degreeY, degreeZ)
         */
        Ogre::Vector3 getOffsetOrientation(void) const;

        /**
         * @brief Sets whether the offset is used as the FINAL attachment transform instead of
         *        being added on top of a transform derived at connect time.
         * @param[in] useBakedOffset The flag to set
         * @Note  Off (the default) keeps the historical behaviour: connect() reads the world
         *        transforms of the source and of this game object, works out where the source
         *        currently sits relative to the bone, and adds the offset on top of that. That is
         *        convenient while placing an object in the editor, but it makes the attachment
         *        depend on THREE things that all have to be right at that exact moment - where
         *        this game object stands, where the source stands, and which pose the skeleton is
         *        in. Move the character before this component connects and the source stays
         *        behind, so the derived transform is the distance between the two.
         *
         *        On, the offset IS the attachment, expressed in the bone's local space. Nothing
         *        is read from the world, so it no longer matters where anybody stands or which
         *        animation frame is showing. Use bakeOffset() to fill the values in once.
         */
        void setUseBakedOffset(bool useBakedOffset);

        /**
         * @brief Gets whether the offset is used as the final attachment transform.
         */
        bool getUseBakedOffset(void) const;

        /**
         * @brief Freezes the CURRENT attachment into the offset attributes and switches
         *        'Use Baked Offset' on.
         * @Note  Place the source where it belongs, let this component connect once so the tag
         *        point is built, then call this. The tag point's local transform is by definition
         *        the attachment relative to the bone, so it is simply copied into the offset -
         *        no world transform is involved and there is nothing left to get wrong later.
         *        Save the scene afterwards and the attachment is persisted.
         */
        void bakeOffset(void);

        /**
         * @brief Gets a named bone position in world space.
         * @param[in] name The bone name.
         * @return The bone position, or Ogre::Vector3::ZERO when unavailable.
         */
        Ogre::Vector3 getBonePosition(const Ogre::String& name) const;

        /**
         * @brief Gets a named bone orientation in world space.
         * @param[in] name The bone name.
         * @return The bone orientation, or Ogre::Quaternion::IDENTITY when unavailable.
         */
        Ogre::Quaternion getBoneOrientation(const Ogre::String& name) const;

        /**
         * @brief Gets the tag point Ogre pointer to work directly with the tag point
         * @return tagPoint The tag point pointer to get
         */
        Ogre::TagPoint* getTagPoint(void) const;

        /**
         * @brief Gets the tag point Ogre scene node that is used for this tag point
         * @return tagPointNode The tag point node to get
         */
        Ogre::SceneNode* getTagPointNode(void) const;

    public:
        static const Ogre::String AttrTagPointName(void)
        {
            return "Tag Point Name";
        }
        static const Ogre::String AttrSourceId(void)
        {
            return "Source Id";
        }
        static const Ogre::String AttrOffsetPosition(void)
        {
            return "Offset Position";
        }
        static const Ogre::String AttrOffsetOrientation(void)
        {
            return "Offset Orientation";
        }
        static const Ogre::String AttrUseBakedOffset(void)
        {
            return "Use Baked Offset";
        }
        static const Ogre::String AttrBakeOffsetAction(void)
        {
            return "Bake Offset";
        }

    private:
        void generateDebugData(void);
        void destroyDebugData(void);
        void resetTagPoint(void);

        void initializeV2Item(Ogre::Item* item);
        void connectV2Item(Ogre::Item* item);
        void setTagPointNameV2(Ogre::Item* item, const Ogre::String& tagPointName);
        void resetTagPointV2(Ogre::Item* item);
        // Physics update closure for V2 (registered only when source has physics)
        void updateV2PhysicsFromTagPoint(void);

        // Works out where the source currently sits relative to the given bone. This is the same
        // computation connect() uses, factored out so bakeOffset() can run it with the simulation
        // switched off - before any tag point exists.
        // Attention: RENDER THREAD only.
        bool internalComputeBaseLocalTransform(Ogre::Bone* bone, Ogre::Vector3& outLocalPosition, Ogre::Quaternion& outLocalOrientation);

        // Decomposes an attachment orientation into the degrees the offset attribute stores, and
        // verifies the round trip through MathHelper::degreesToQuat.
        Ogre::Vector3 internalOrientationToDegrees(const Ogre::Quaternion& orientation);

    private:
        // Common members
        Ogre::SceneNode* tagPointNode;
        PhysicsActiveComponent* sourcePhysicsActiveComponent;
        bool alreadyConnected;

        // Attributes
        Variant* tagPoints;
        Variant* sourceId;
        Variant* offsetPosition;
        Variant* offsetOrientation;
        Variant* useBakedOffset;
        Variant* bakeOffsetAction;

        // Debug visualization
        Ogre::SceneNode* debugGeometryArrowNode;
        Ogre::SceneNode* debugGeometrySphereNode;
        Ogre::Item* debugGeometryArrowItem;
        Ogre::Item* debugGeometrySphereItem;

        // V2 members
        Ogre::SkeletonInstance* skeletonInstance;
        Ogre::Bone* attachedBone;
        Ogre::TagPoint* tagPointV2; // The real v2 TagPoint (child of a Bone)
        Ogre::String updateClosureId;
    };

}; // namespace end

#endif