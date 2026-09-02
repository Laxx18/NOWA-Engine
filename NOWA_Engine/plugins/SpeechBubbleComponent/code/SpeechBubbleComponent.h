/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef SPEECHBUBBLECOMPONENT_H
#define SPEECHBUBBLECOMPONENT_H

#include "gameobject/GameObjectComponent.h"

#include "OgrePlugin.h"
#include "main/Events.h"

namespace NOWA
{
    // Only forward declared here; SpeechBubbleComponent.cpp includes the real header.
    class MovableText;
    class SimpleSoundComponent;
    class CameraComponent;

    /**
     * @class 	SpeechBubbleComponent
     * @brief 	This component draws a self contained speech bubble at the game object with an offset.
     *
     * The component owns its own MovableText and its own bubble geometry. It does NOT
     * depend on a GameObjectTitleComponent anymore.
     *
     * Why the text used to drift out of the bubble: MovableText::getWorldTransforms()
     * ignores its parent node's orientation and billboards towards the camera in the
     * world matrix, additionally halving the parent's scale. The old bubble geometry
     * was built from the parent node's derived orientation at full scale, so text and
     * bubble used two completely different transforms and could never line up. Here
     * the bubble node is driven from the very same values MovableText uses, so both
     * stay locked together by construction.
     */
    class EXPORTED SpeechBubbleComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<SpeechBubbleComponent> SpeechBubbleComponentPtr;

    public:
        SpeechBubbleComponent();

        virtual ~SpeechBubbleComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

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

    public:
        /**
         * @see		GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("SpeechBubbleComponent");
        }

        /**
         * @see		GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "SpeechBubbleComponent";
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
            return "This component draws a self contained speech bubble at the game object with an offset. "
                   "It brings its own text rendering, so no GameObjectTitleComponent is required. "
                   "Optionally, for the typewriter sound, a SimpleSoundComponent is required.";
        }

        /**
         * @see	GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        /**
         * @brief Sets whether to show the speech bubble.
         * @param[in] activated if set to true the speech bubble will be rendered.
         */
        virtual void setActivated(bool activated) override;

        /**
         * @brief Gets whether the speech bubble is rendered.
         * @return activated true if the speech bubble is rendered
         */
        virtual bool isActivated(void) const override;

        /**
         * @brief Sets the text caption currently shown. With a caption sequence this
         *        addresses the caption at the current index.
         * @param[in] caption The text caption to set
         */
        void setCaption(const Ogre::String& caption);

        /**
         * @brief Gets the text caption currently shown.
         * @return caption The text caption to get
         */
        Ogre::String getCaption(void) const;

        /**
         * @brief Sets the speech duration of the caption currently shown, in seconds.
         * @param[in] speechDurationSec The speech duration to set.
         */
        void setSpeechDuration(Ogre::Real speechDurationSec);

        /**
         * @brief Gets the speech duration of the caption currently shown, in seconds.
         * @return speechDurationSec
         */
        Ogre::Real getSpeechDuration(void) const;

        /**
         * @brief Sets the font name used for the speech text.
         * @param[in] fontName The font name to set.
         */
        void setFontName(const Ogre::String& fontName);

        /**
         * @brief Gets the font name used for the speech text.
         * @return fontName
         */
        Ogre::String getFontName(void) const;

        /**
         * @brief Sets the character height of the speech text in local units.
         * @param[in] charHeight The character height to set.
         */
        void setCharHeight(Ogre::Real charHeight);

        /**
         * @brief Gets the character height of the speech text in local units.
         * @return charHeight
         */
        Ogre::Real getCharHeight(void) const;

        /**
         * @brief Sets the text color (r, g, b, a).
         * @param[in] textColor The text color to set.
         */
        void setTextColor(const Ogre::Vector4& textColor);

        /**
         * @brief Gets the text color (r, g, b, a).
         * @return textColor
         */
        Ogre::Vector4 getTextColor(void) const;

        /**
         * @brief Sets the bubble body color (r, g, b, a).
         * @param[in] bubbleColor The bubble color to set.
         */
        void setBubbleColor(const Ogre::Vector4& bubbleColor);

        /**
         * @brief Gets the bubble body color (r, g, b, a).
         * @return bubbleColor
         */
        Ogre::Vector4 getBubbleColor(void) const;

        /**
         * @brief Sets the offset position of the whole speech bubble relative to the game object.
         * @param[in] offsetPosition The offset position to set.
         */
        void setOffsetPosition(const Ogre::Vector3& offsetPosition);

        /**
         * @brief Gets the offset position of the whole speech bubble relative to the game object.
         * @return offsetPosition
         */
        Ogre::Vector3 getOffsetPosition(void) const;

        /**
         * @brief Sets the maximum text width in local units. The caption is word wrapped
         *        to this width using the real font metrics, so the bubble and the visible
         *        text always agree.
         * @param[in] maxTextWidth The maximum text width to set.
         */
        void setMaxTextWidth(Ogre::Real maxTextWidth);

        /**
         * @brief Gets the maximum text width in local units.
         * @return maxTextWidth
         */
        Ogre::Real getMaxTextWidth(void) const;

        /**
         * @brief Sets whether the speech text shall appear char by char running.
         * @param[in] runSpeech The flag to set.
         */
        void setRunSpeech(bool runSpeech);

        /**
         * @brief Gets whether the speech text shall appear char by char running.
         * @return runSpeech
         */
        bool getRunSpeech(void) const;

        /**
         * @brief Sets whether to use a sound if the speech is running char by char.
         * @param[in] runSpeechSound The flag set.
         */
        void setRunSpeechSound(bool runSpeechSound);

        /**
         * @brief Gets whether to use a sound if the speech is running char by char.
         * @return runSpeechSound
         */
        bool getRunSpeechSound(void) const;

        /**
         * @brief Sets whether the caption should remain after the speech run.
         * @param[in] keepCaption The flag set.
         */
        void setKeepCaption(bool keepCaption);

        /**
         * @brief Gets whether the caption is remained after the speech run.
         * @return keepCaption
         */
        bool getKeepCaption(void) const;

        /**
         * @brief Sets the id of the game object the bubble and its text should face.
         *        Any game object works; if it carries a CameraComponent, that camera's
         *        position is used. If 0, the currently active camera is used.
         * @param[in] orientationTargetId The orientation target id to set.
         */
        void setOrientationTargetId(unsigned long orientationTargetId);

        /**
         * @brief Gets the id of the game object the bubble faces.
         * @return orientationTargetId
         */
        unsigned long getOrientationTargetId(void) const;

        /**
         * @brief Lua closure function gets called if the speech is done.
         * @param[in] closureFunction The closure function set.
         */
        void reactOnSpeechDone(luabind::object closureFunction);

        /**
         * @brief Sets the horizontal offset of the bubble body (ellipse + tail) relative
         *        to the text anchor, in the local +X direction. The text itself stays
         *        centered on the anchor - only the bubble shape shifts sideways, so the
         *        tail can point at the character's mouth while the bubble body sits off
         *        to one side (e.g. so it doesn't cover the character).
         * @param[in] xOffsetStart The horizontal offset to set.
         */
        void setXOffsetStart(Ogre::Real xOffsetStart);

        /**
         * @brief Gets the horizontal offset of the bubble body relative to the text anchor.
         * @return xOffsetStart
         */
        Ogre::Real getXOffsetStart(void) const;

        /**
         * @brief Sets how many captions are played back one after another.
         *        Creates/removes Caption<i> and Speech Duration<i> attributes.
         * @param[in] captionCount The caption count to set. Minimum 1.
         */
        void setCaptionCount(unsigned int captionCount);

        /**
         * @brief Gets how many captions are played back one after another.
         * @return captionCount
         */
        unsigned int getCaptionCount(void) const;

        /**
         * @brief Sets the caption of the given index.
         */
        void setCaption(unsigned int index, const Ogre::String& caption);

        /**
         * @brief Gets the caption of the given index.
         */
        Ogre::String getCaption(unsigned int index) const;

        /**
         * @brief Sets the speech duration of the given index.
         */
        void setSpeechDuration(unsigned int index, Ogre::Real speechDurationSec);

        /**
         * @brief Gets the speech duration of the given index.
         */
        Ogre::Real getSpeechDuration(unsigned int index) const;

        /**
         * @brief Restarts the whole caption sequence at index 0.
         */
        void restartSequence(void);

        /**
         * @brief Gets the index of the caption currently being played back.
         */
        unsigned int getCurrentCaptionIndex(void) const;

        /**
         * @brief Sets the offset orientation (degrees) applied on top of the orientation
         *        towards the target, for both the text and the bubble.
         * @param[in] offsetOrientation The offset orientation to set.
         */
        void setOffsetOrientation(const Ogre::Vector3& offsetOrientation);

        /**
         * @brief Gets the offset orientation (degrees).
         * @return offsetOrientation
         */
        Ogre::Vector3 getOffsetOrientation(void) const;

        /**
         * @brief Sets the margin between the text block and the bubble border, in local units.
         * @param[in] padding The padding to set.
         */
        void setPadding(Ogre::Real padding);

        /**
         * @brief Gets the margin between the text block and the bubble border.
         * @return padding
         */
        Ogre::Real getPadding(void) const;

        /**
         * @brief Sets the corner radius of the rounded bubble body, in local units.
         * @param[in] cornerRadius The corner radius to set.
         */
        void setCornerRadius(Ogre::Real cornerRadius);

        /**
         * @brief Gets the corner radius of the rounded bubble body.
         * @return cornerRadius
         */
        Ogre::Real getCornerRadius(void) const;

        /**
         * @brief Gets the internally owned movable text, e.g. to tweak it further.
         * @return movableText The movable text or nullptr if not yet created.
         */
        MovableText* getMovableText(void) const;

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrCaption(void)
        {
            return "Caption";
        }
        static const Ogre::String AttrCaptionCount(void)
        {
            return "Caption Count";
        }
        static const Ogre::String AttrSpeechDuration(void)
        {
            return "Speech Duration";
        }
        static const Ogre::String AttrRunSpeech(void)
        {
            return "Run Speech";
        }
        static const Ogre::String AttrRunSpeechSound(void)
        {
            return "Run Speech Sound";
        }
        static const Ogre::String AttrKeepCaption(void)
        {
            return "Keep Caption";
        }
        static const Ogre::String AttrXOffsetStart(void)
        {
            return "X Offset Start";
        }
        static const Ogre::String AttrFontName(void)
        {
            return "Font Name";
        }
        static const Ogre::String AttrCharHeight(void)
        {
            return "Char Height";
        }
        static const Ogre::String AttrTextColor(void)
        {
            return "Text Color";
        }
        static const Ogre::String AttrBubbleColor(void)
        {
            return "Bubble Color";
        }
        static const Ogre::String AttrOffsetPosition(void)
        {
            return "Offset Position";
        }
        static const Ogre::String AttrMaxTextWidth(void)
        {
            return "Max Text Width";
        }
        static const Ogre::String AttrOrientationTargetId(void)
        {
            return "Orientation Target Id";
        }
        static const Ogre::String AttrOffsetOrientation(void)
        {
            return "Offset Orientation";
        }
        static const Ogre::String AttrPadding(void)
        {
            return "Padding";
        }
        static const Ogre::String AttrCornerRadius(void)
        {
            return "Corner Radius";
        }

    protected:
        virtual void drawSpeechBubble(Ogre::Real dt);

        void createSpeechBubble(void);

        void destroySpeechBubble(void);

        /**
         * @brief Pushes font, char height, color and offset onto the owned movable text.
         *        Must be called on the render thread.
         */
        void applyTextSettings(void);

        /**
         * @brief Word wraps the given text to maxTextWidth using the real glyph metrics
         *        of the configured font, mirroring MovableText::_setupGeometry()'s
         *        advance calculation exactly. A character count based wrap could never
         *        match a proportional font.
         */
        Ogre::String wrapCaptionToWidth(const Ogre::String& text) const;

        /**
         * @brief Resolves the world position to face, from the configured orientation
         *        target game object. Returns false when no target is configured or it
         *        cannot be resolved - in that case NO orientation happens at all.
         */
        bool resolveOrientationTargetPosition(Ogre::Vector3& outTargetPosition) const;

        /**
         * @brief Sets the text node's LOCAL orientation once, for the no-target case.
         *        Left untouched afterwards - parented normally under the game object,
         *        so it keeps facing correctly as the object itself rotates, with no
         *        per-frame recompute needed. Must be called on the render thread.
         */
        void applyStaticOrientation(void);

        /**
         * @brief Builds the configured OffsetOrientation as a quaternion.
         */
        Ogre::Quaternion buildOffsetQuaternion(void) const;

        /**
         * @brief Per-frame orientation update. Cheap when no target is configured
         *        (just re-reads the already-correct derived orientation); only does
         *        real work when a moving target needs to be faced.
         *        Must be called on the render thread.
         */
        void updateOrientation(void);

        /**
         * @brief Advances to the next caption of the sequence, wrapping around.
         */
        void advanceToNextCaption(void);

        /**
         * @brief Gets the caption of the given index, empty string when out of range.
         */
        Ogre::String getCaptionAt(unsigned int index) const;

    protected:
        Ogre::String name;

        Ogre::SceneNode* textNode;
        Ogre::SceneNode* bubbleNode;
        MovableText* movableText;
        Ogre::ManualObject* manualObject;
        SimpleSoundComponent* simpleSoundComponent;
        GameObject* orientationTargetGameObject;
        unsigned int currentCaptionIndex;
        unsigned long indices;
        unsigned int currentCharIndex;
        Ogre::Real timeSinceLastRun;
        Ogre::Real timeSinceLastChar;
        bool couldDraw;
        // Tracks whether begin() has EVER run on manualObject, independent of Ogre's
        // own getNumSections() bookkeeping. Needed because the previous logic re-read
        // getNumSections()/couldDraw to decide begin() vs beginUpdate() vs clear()
        // every frame, and calling clear() on a section that was opened via
        // beginUpdate() but never end()ed corrupted ManualObject's internal state -
        // the exact cause of the "must call begin() before this method" crashes.
        bool manualObjectBegun;
        bool speechDone;
        Ogre::String wrappedCaption;

        luabind::object closureFunction;
        Variant* activated;
        // Legacy single caption / duration. They mirror whichever entry of the
        // sequence is currently on screen, so the existing single value API and old
        // scene files keep working unchanged.
        Variant* caption;
        Variant* speechDuration;
        Variant* runSpeech;
        Variant* runSpeechSound;
        Variant* keepCaption;
        Variant* xOffsetStart;
        Variant* fontName;
        Variant* charHeight;
        Variant* textColor;
        Variant* bubbleColor;
        Variant* offsetPosition;
        Variant* maxTextWidth;
        Variant* orientationTargetId;
        Variant* offsetOrientation;
        Variant* padding;
        Variant* cornerRadius;
        // Declared LAST on purpose. Variants appear in the editor in construction
        // order, and members are constructed in declaration order regardless of how
        // the initialiser list is written. Keeping these at the end puts "Caption
        // Count" and the runtime created Caption0..N together at the bottom of the
        // property list instead of stranding the count near the top.
        Variant* captionCount;
        std::vector<Variant*> captions;
        std::vector<Variant*> captionDurations;
    };

}; // namespace end

#endif