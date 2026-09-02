#include "NOWAPrecompiled.h"
#include "SpeechBubbleComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/SimpleSoundComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/MovableText.h"
#include "utilities/XMLConverter.h"

#include "RenderQueueEnums.h"

#include "OgreAbiUtils.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

namespace
{
    // MovableText::_setupGeometry() advances the pen by
    //     glyphAspectRatio(ch) * charHeight * UV_RANGE   with UV_RANGE == 1.0
    // and uses glyphAspectRatio('A') * charHeight for a space. Everything in this
    // file that measures text has to use the exact same formula, otherwise the
    // bubble geometry and the visible glyphs disagree again.
    const Ogre::Real MOVABLE_TEXT_UV_RANGE = 1.0f;

    // MovableText::getWorldTransforms() is DEAD CODE under Ogre-Next. The Hlms
    // (HlmsUnlit::fillBuffersFor) fetches the world matrix from
    // MovableObject::_getParentNodeFullTransform() and never calls the v1
    // Renderable::getWorldTransforms() hook. That means MovableText is rendered
    // with the FULL transform of its parent scene node: no camera billboarding
    // and, above all, NO halved scale. Shrinking the bubble by 0.5 to "mirror" a
    // halving that never happens is precisely why the bubble came out at half the
    // size of the text and the caption ended up floating above it. Keep this at
    // 1.0; it is left as a named constant only to document the reasoning.
    const Ogre::Real MOVABLE_TEXT_SCALE_FACTOR = 1.0f;

    // Extra time the FULLY revealed caption stays on screen after the typewriter
    // has finished, so it can actually be read before the next caption of the
    // sequence takes over. Only used when "Run Speech" is on - with the typewriter
    // off, "Speech Duration" alone already is the on screen time.
    const Ogre::Real CAPTION_HOLD_AFTER_REVEAL_SECONDS = 3.0f;

    Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
    {
        size_t startPos = 0;
        while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
        {
            str.replace(startPos, from.length(), to);
            startPos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    }

    Ogre::String mid(const Ogre::String& str, unsigned int pos1, unsigned int pos2)
    {
        if (pos2 > str.size())
        {
            pos2 = static_cast<unsigned int>(str.size());
        }
        if (pos1 >= pos2)
        {
            return Ogre::String();
        }
        return str.substr(pos1, pos2 - pos1);
    }

    Ogre::Font* loadFont(const Ogre::String& fontName)
    {
        Ogre::Font* font = static_cast<Ogre::Font*>(Ogre::FontManager::getSingleton().getByName(fontName).getPointer());
        if (nullptr != font)
        {
            font->load();
        }
        return font;
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    SpeechBubbleComponent::SpeechBubbleComponent() :
        GameObjectComponent(),
        name("SpeechBubbleComponent"),
        textNode(nullptr),
        bubbleNode(nullptr),
        movableText(nullptr),
        manualObject(nullptr),
        simpleSoundComponent(nullptr),
        orientationTargetGameObject(nullptr),
        currentCaptionIndex(0),
        indices(0),
        currentCharIndex(0),
        timeSinceLastRun(0.0f),
        timeSinceLastChar(0.0f),
        couldDraw(false),
        manualObjectBegun(false),
        speechDone(false),
        activated(new Variant(SpeechBubbleComponent::AttrActivated(), true, this->attributes)),
        caption(new Variant(SpeechBubbleComponent::AttrCaption(), "MyCaption", this->attributes)),
        speechDuration(new Variant(SpeechBubbleComponent::AttrSpeechDuration(), 10.0f, this->attributes)),
        runSpeech(new Variant(SpeechBubbleComponent::AttrRunSpeech(), false, this->attributes)),
        runSpeechSound(new Variant(SpeechBubbleComponent::AttrRunSpeechSound(), false, this->attributes)),
        keepCaption(new Variant(SpeechBubbleComponent::AttrKeepCaption(), false, this->attributes)),
        xOffsetStart(new Variant(SpeechBubbleComponent::AttrXOffsetStart(), -0.5f, this->attributes)),
        fontName(new Variant(SpeechBubbleComponent::AttrFontName(), "BlueHighway", this->attributes)),
        charHeight(new Variant(SpeechBubbleComponent::AttrCharHeight(), 0.5f, this->attributes)),
        textColor(new Variant(SpeechBubbleComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f), this->attributes)),
        bubbleColor(new Variant(SpeechBubbleComponent::AttrBubbleColor(), Ogre::Vector4(1.0f, 1.0f, 1.0f, 1.0f), this->attributes)),
        offsetPosition(new Variant(SpeechBubbleComponent::AttrOffsetPosition(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes)),
        maxTextWidth(new Variant(SpeechBubbleComponent::AttrMaxTextWidth(), 6.0f, this->attributes)),
        orientationTargetId(new Variant(SpeechBubbleComponent::AttrOrientationTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        offsetOrientation(new Variant(SpeechBubbleComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
        padding(new Variant(SpeechBubbleComponent::AttrPadding(), 0.35f, this->attributes)),
        cornerRadius(new Variant(SpeechBubbleComponent::AttrCornerRadius(), 0.35f, this->attributes)),
        captionCount(new Variant(SpeechBubbleComponent::AttrCaptionCount(), static_cast<unsigned int>(1), this->attributes))
    {
        this->captionCount->addUserData(GameObject::AttrActionNeedRefresh());
        this->captionCount->setDescription("How many captions are played back one after another. Each gets its own Caption<i> and Speech Duration<i>.");

        this->runSpeech->setDescription("Sets whether the speech text shall appear char by char running.");
        this->speechDuration->setDescription("Sets the speech duration. That is how long the bubble shall remain in seconds.");
        this->runSpeechSound->setDescription("Sets whether to use a sound if the speech is running char by char. Requires a SimpleSoundComponent.");
        this->keepCaption->setDescription("Sets whether the caption should remain after the speech run.");
        this->xOffsetStart->setDescription("Horizontal offset of the bubble body (ellipse and tail) relative to the text anchor, "
                                           "in the local +X direction. Only the bubble shape shifts - the text stays centered "
                                           "on the anchor. Useful to keep the bubble from covering the character while the "
                                           "tail still points at its mouth.");
        this->fontName->setDescription("The font used for the speech text. Must be a registered Ogre font.");
        this->charHeight->setDescription("The character height of the speech text in local units.");
        this->textColor->setDescription("The text color (r, g, b, a).");
        this->bubbleColor->setDescription("The bubble body color (r, g, b, a).");
        this->offsetPosition->setDescription("The offset of the whole speech bubble relative to the game object.");
        this->maxTextWidth->setDescription("Maximum text width in local units. The caption is word wrapped to this width "
                                           "using the real font metrics, so bubble and text always agree.");
        this->orientationTargetId->setDescription("Id of a game object with a CameraComponent the bubble should face. "
                                                  "If 0, the currently active camera is used.");

        this->offsetOrientation->setDescription("Additional rotation in degrees, applied on top of the billboard rotation of both text and bubble.");
        this->padding->setDescription("Margin between the text block and the bubble border, in local units.");
        this->cornerRadius->setDescription("Corner radius of the rounded bubble body, in local units. Clamped to half the bubble size.");

        this->textColor->addUserData(GameObject::AttrActionColorDialog());
        this->bubbleColor->addUserData(GameObject::AttrActionColorDialog());

        // The indexed captions are the single source of truth. "Caption" and
        // "Speech Duration" only mirror whichever entry is on screen; they stay for
        // the existing single value API and for loading old scenes, but showing them
        // next to Caption0..N would just be two editable fields for the same thing.
        this->caption->setVisible(false);
        this->speechDuration->setVisible(false);

        // Create slot 0 right here. init() only runs when a scene is loaded, so for a
        // component added fresh in the editor captions[] stayed empty - and then
        // restartSequence() read an out of range index, got an empty string and wiped
        // the caption, which is why the bubble never appeared.
        this->setCaptionCount(1);
        this->captions[0]->setValue(Ogre::String("MyCaption"));
    }

    SpeechBubbleComponent::~SpeechBubbleComponent(void)
    {
    }

    void SpeechBubbleComponent::initialise()
    {
    }

    const Ogre::String& SpeechBubbleComponent::getName() const
    {
        return this->name;
    }

    void SpeechBubbleComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SpeechBubbleComponent>(SpeechBubbleComponent::getStaticClassId(), SpeechBubbleComponent::getStaticClassName());
    }

    void SpeechBubbleComponent::shutdown()
    {
    }

    void SpeechBubbleComponent::uninstall()
    {
    }

    void SpeechBubbleComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool SpeechBubbleComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
        {
            // Legacy single caption. Kept so old scenes keep working; it seeds Caption0.
            this->caption->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CaptionCount")
        {
            this->captionCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Only grow, never shrink here: on a snapshot restore the variants already
        // exist and the algorithm relies on their changed flags.
        if (this->captions.size() < this->captionCount->getUInt())
        {
            this->captions.resize(this->captionCount->getUInt());
            this->captionDurations.resize(this->captionCount->getUInt());
        }

        bool parsedIndexedCaption = false;

        for (size_t i = 0; i < this->captions.size(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption" + Ogre::StringConverter::toString(i))
            {
                parsedIndexedCaption = true;
                const Ogre::String captionText = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->captions[i])
                {
                    this->captions[i] = new Variant(SpeechBubbleComponent::AttrCaption() + Ogre::StringConverter::toString(i), captionText, this->attributes);
                }
                else
                {
                    this->captions[i]->setValue(captionText);
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpeechDuration" + Ogre::StringConverter::toString(i))
            {
                const Ogre::Real duration = XMLConverter::getAttribReal(propertyElement, "data", 10.0f);
                if (nullptr == this->captionDurations[i])
                {
                    this->captionDurations[i] = new Variant(SpeechBubbleComponent::AttrSpeechDuration() + Ogre::StringConverter::toString(i), duration, this->attributes);
                }
                else
                {
                    this->captionDurations[i]->setValue(duration);
                }
                propertyElement = propertyElement->next_sibling("property");
            }
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeech")
        {
            this->runSpeech->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpeechDuration")
        {
            this->speechDuration->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunSpeechSound")
        {
            this->runSpeechSound->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "KeepCaption")
        {
            this->keepCaption->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "XOffsetStart")
        {
            this->xOffsetStart->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        // New properties. Older scenes simply do not carry them, in which case the
        // constructor defaults stay in place and the ifs below never match.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontName")
        {
            this->fontName->setValue(XMLConverter::getAttrib(propertyElement, "data", "BlueHighway"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CharHeight")
        {
            this->charHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
        {
            this->textColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BubbleColor")
        {
            this->bubbleColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
        {
            this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxTextWidth")
        {
            this->maxTextWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 6.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OrientationTargetId")
        {
            this->orientationTargetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
        {
            this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Padding")
        {
            this->padding->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.35f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CornerRadius")
        {
            this->cornerRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.35f));
            propertyElement = propertyElement->next_sibling("property");
        }

        this->setCaptionCount(std::max(this->captionCount->getUInt(), 1u));

        // Pre sequence scenes only carry a single "Caption" property. Migrate it into
        // slot 0, but ONLY when the file really had no Caption0 - testing for an empty
        // slot would never fire, because the constructor already seeds it.
        if (false == parsedIndexedCaption && false == this->caption->getString().empty())
        {
            this->captions[0]->setValue(this->caption->getString());
        }

        return true;
    }

    GameObjectCompPtr SpeechBubbleComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        SpeechBubbleComponentPtr clonedCompPtr(boost::make_shared<SpeechBubbleComponent>());

        clonedCompPtr->setCaptionCount(this->captionCount->getUInt());
        for (size_t i = 0; i < this->captions.size(); i++)
        {
            clonedCompPtr->setCaption(static_cast<unsigned int>(i), this->captions[i]->getString());
            clonedCompPtr->setSpeechDuration(static_cast<unsigned int>(i), this->captionDurations[i]->getReal());
        }
        clonedCompPtr->setCaption(this->caption->getString());
        clonedCompPtr->setRunSpeech(this->runSpeech->getBool());
        clonedCompPtr->setSpeechDuration(this->speechDuration->getReal());
        clonedCompPtr->setRunSpeechSound(this->runSpeechSound->getBool());
        clonedCompPtr->setKeepCaption(this->keepCaption->getBool());
        clonedCompPtr->setXOffsetStart(this->xOffsetStart->getReal());
        clonedCompPtr->setFontName(this->fontName->getString());
        clonedCompPtr->setCharHeight(this->charHeight->getReal());
        clonedCompPtr->setTextColor(this->textColor->getVector4());
        clonedCompPtr->setBubbleColor(this->bubbleColor->getVector4());
        clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
        clonedCompPtr->setMaxTextWidth(this->maxTextWidth->getReal());
        clonedCompPtr->setOrientationTargetId(this->orientationTargetId->getULong());
        clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
        clonedCompPtr->setPadding(this->padding->getReal());
        clonedCompPtr->setCornerRadius(this->cornerRadius->getReal());

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool SpeechBubbleComponent::onCloned(void)
    {
        // The orientation target points at another game object, whose id is re-generated
        // during cloning. Remap it, exactly like GameObjectTitleComponent does.
        GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->orientationTargetId->getULong());
        if (nullptr != gameObjectPtr)
        {
            this->orientationTargetId->setValue(gameObjectPtr->getId());
        }
        else
        {
            this->orientationTargetId->setValue(static_cast<unsigned long>(0));
        }

        return true;
    }

    bool SpeechBubbleComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpeechBubbleComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // Everything is created up front so the bubble is already visible at design
        // time in the editor, just like the other visual components.
        this->createSpeechBubble();
        this->restartSequence();

        return true;
    }

    bool SpeechBubbleComponent::connect(void)
    {
        GameObjectComponent::connect();

        auto simpleSoundCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<SimpleSoundComponent>());
        if (nullptr != simpleSoundCompPtr)
        {
            this->simpleSoundComponent = simpleSoundCompPtr.get();
        }

        this->orientationTargetGameObject = nullptr; // re-resolved lazily on first use

        this->createSpeechBubble();
        this->restartSequence();

        this->currentCharIndex = 0;
        this->timeSinceLastChar = 0.0f;
        this->timeSinceLastRun = 0.0f;
        this->speechDone = false;

        return true;
    }

    bool SpeechBubbleComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->indices = 0;
        this->currentCharIndex = 0;
        this->timeSinceLastChar = 0.0f;
        this->timeSinceLastRun = 0.0f;
        this->speechDone = false;
        this->couldDraw = false;

        if (nullptr != this->simpleSoundComponent)
        {
            this->simpleSoundComponent->setActivated(false);
            this->simpleSoundComponent = nullptr;
        }
        this->orientationTargetGameObject = nullptr;

        // Restore the full caption, so the editor shows the configured text again
        // instead of whatever partial state the typewriter left behind.
        this->setCaption(this->caption->getString());

        return true;
    }

    void SpeechBubbleComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->activated->getBool())
        {
            if (nullptr == this->manualObject)
            {
                return;
            }

            // The begin()/beginUpdate()/end() bracket used to live HERE, around
            // drawSpeechBubble(). That was the crash: it opened the ManualObject on
            // every single frame, including the frames on which drawSpeechBubble()
            // bails out early and writes no vertex at all - and with the typewriter
            // enabled the very first frame is exactly such a frame, because the
            // caption starts out empty.
            //
            // Ogre::ManualObject::end() returns early when no vertex was written
            // ("Support calling begin() and end() without defining any geometry")
            // and leaves ManualObjectSection::mVao at nullptr. The next frame's
            // beginUpdate(0) then does mCurrentSection->mVao->getVertexBuffers()[0]
            // on that null VAO - the access violation at 0x20. On the update path
            // the same early return also skips the unmap of the persistent buffers,
            // so a following beginUpdate() would map them a second time.
            //
            // The bracket is therefore inside drawSpeechBubble() now, where the
            // geometry is known: it is opened only once the vertices for this frame
            // actually exist.
            auto closureFunction = [this](Ogre::Real renderDt)
            {
                this->drawSpeechBubble(renderDt);
            };
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    void SpeechBubbleComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (SpeechBubbleComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrCaptionCount() == attribute->getName())
        {
            this->setCaptionCount(attribute->getUInt());
        }
        else if (SpeechBubbleComponent::AttrCaption() == attribute->getName())
        {
            this->setCaption(attribute->getString());
        }
        else if (SpeechBubbleComponent::AttrRunSpeech() == attribute->getName())
        {
            this->setRunSpeech(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrSpeechDuration() == attribute->getName())
        {
            this->setSpeechDuration(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrRunSpeechSound() == attribute->getName())
        {
            this->setRunSpeechSound(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrKeepCaption() == attribute->getName())
        {
            this->setKeepCaption(attribute->getBool());
        }
        else if (SpeechBubbleComponent::AttrXOffsetStart() == attribute->getName())
        {
            this->setXOffsetStart(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrFontName() == attribute->getName())
        {
            this->setFontName(attribute->getString());
        }
        else if (SpeechBubbleComponent::AttrCharHeight() == attribute->getName())
        {
            this->setCharHeight(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrTextColor() == attribute->getName())
        {
            this->setTextColor(attribute->getVector4());
        }
        else if (SpeechBubbleComponent::AttrBubbleColor() == attribute->getName())
        {
            this->setBubbleColor(attribute->getVector4());
        }
        else if (SpeechBubbleComponent::AttrOffsetPosition() == attribute->getName())
        {
            this->setOffsetPosition(attribute->getVector3());
        }
        else if (SpeechBubbleComponent::AttrMaxTextWidth() == attribute->getName())
        {
            this->setMaxTextWidth(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrOrientationTargetId() == attribute->getName())
        {
            this->setOrientationTargetId(attribute->getULong());
        }
        else if (SpeechBubbleComponent::AttrOffsetOrientation() == attribute->getName())
        {
            this->setOffsetOrientation(attribute->getVector3());
        }
        else if (SpeechBubbleComponent::AttrPadding() == attribute->getName())
        {
            this->setPadding(attribute->getReal());
        }
        else if (SpeechBubbleComponent::AttrCornerRadius() == attribute->getName())
        {
            this->setCornerRadius(attribute->getReal());
        }
        else
        {
            // The indexed captions are created at runtime, so they can only be matched
            // by name here. This has to be the LAST branch of the chain - sitting in
            // the middle it swallowed every attribute below it.
            for (size_t i = 0; i < this->captions.size(); i++)
            {
                if (SpeechBubbleComponent::AttrCaption() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setCaption(static_cast<unsigned int>(i), attribute->getString());
                    return;
                }
                if (SpeechBubbleComponent::AttrSpeechDuration() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setSpeechDuration(static_cast<unsigned int>(i), attribute->getReal());
                    return;
                }
            }
        }
    }

    void SpeechBubbleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
        // Slot 0, not the mirror: after a sequence has run the mirror holds whatever
        // caption happened to be on screen last, which would drift on every save.
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getCaptionAt(0))));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CaptionCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->captionCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->captions.size(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Caption" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->captions[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SpeechDuration" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->captionDurations[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeech"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeech->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SpeechDuration"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speechDuration->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RunSpeechSound"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runSpeechSound->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "KeepCaption"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->keepCaption->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "XOffsetStart"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->xOffsetStart->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FontName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CharHeight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->charHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BubbleColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bubbleColor->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MaxTextWidth"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxTextWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OrientationTargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientationTargetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetOrientation->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Padding"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->padding->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CornerRadius"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cornerRadius->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    void SpeechBubbleComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->destroySpeechBubble();
    }

    Ogre::String SpeechBubbleComponent::getClassName(void) const
    {
        return "SpeechBubbleComponent";
    }

    Ogre::String SpeechBubbleComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    // ------------------------------------------------------------------------
    // Creation / destruction
    // ------------------------------------------------------------------------

    void SpeechBubbleComponent::createSpeechBubble(void)
    {
        if (nullptr != this->manualObject && nullptr != this->movableText)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            // Defensive: if either half survived from an earlier connect/disconnect
            // cycle without being cleaned up (e.g. a play/stop/edit loop during
            // development), tear it down completely before building a fresh pair.
            // Two live MovableText/ManualObject instances on the same game object
            // would look exactly like "a second, disconnected text floating
            // somewhere" - because it IS a second, genuinely separate object, not a
            // sync bug between text and bubble.
            if (nullptr != this->movableText || nullptr != this->manualObject)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] createSpeechBubble() found a leftover instance for game object: " + this->gameObjectPtr->getName() +
                                                                                        " (movableText: " + Ogre::StringConverter::toString(nullptr != this->movableText) +
                                                                                        ", manualObject: " + Ogre::StringConverter::toString(nullptr != this->manualObject) + "). Destroying it before creating a new one.");

                if (nullptr != this->manualObject)
                {
                    if (nullptr != this->bubbleNode)
                    {
                        this->bubbleNode->detachAllObjects();
                    }
                    sceneManager->destroyManualObject(this->manualObject);
                    this->manualObject = nullptr;
                    this->manualObjectBegun = false;
                }
                if (nullptr != this->bubbleNode)
                {
                    this->bubbleNode->getParentSceneNode()->removeAndDestroyChild(this->bubbleNode);
                    this->bubbleNode = nullptr;
                }
                if (nullptr != this->movableText)
                {
                    if (nullptr != this->textNode)
                    {
                        this->textNode->detachObject(this->movableText);
                    }
                    delete this->movableText;
                    this->movableText = nullptr;
                }
                if (nullptr != this->textNode)
                {
                    this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->textNode);
                    this->textNode = nullptr;
                }
            }

            // ---- Text -------------------------------------------------------
            if (nullptr == this->movableText)
            {
                Ogre::NameValuePairList params;
                params["name"] = Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_SpeechBubbleText_" + Ogre::StringConverter::toString(this->index);
                params["caption"] = this->caption->getString();
                params["fontName"] = this->fontName->getString();

                this->movableText = new MovableText(Ogre::Id::generateNewId<Ogre::MovableObject>(), &sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), sceneManager, &params);

                // H_CENTER / V_CENTER is the only combination the bubble maths is
                // built around, and it is not exposed as a property on purpose: a
                // left aligned text inside a centred ellipse never looks right, and
                // the old component allowed exactly that mismatch.
                this->movableText->setTextAlignment(MovableText::H_CENTER, MovableText::V_CENTER);
                this->movableText->setQueryFlags(0 << 0);
                this->movableText->setCastShadows(false);

                this->textNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
                this->textNode->setName("SpeechBubbleTextNodeChildOf: " + this->gameObjectPtr->getSceneNode()->getName());
                this->textNode->attachObject(this->movableText);

                // Deliberately NOT registered via GraphicsModule::addTrackedNode().
                //
                // addTrackedNode() -> resolveNodeSlotLocked() creates the slot with
                // active = true and seeds every interpolation buffer with the node's
                // transform at that moment. advanceTransformBuffer() re-snapshots it
                // once more (isNew) and then clears the isNew flag, after which the
                // buffers are frozen forever, while updateAllTransforms() keeps
                // writing setPosition()/setOrientation()/setScale() from those frozen
                // buffers onto the node on EVERY render frame.
                //
                // The result was that the very first offset (whatever applyTextSettings()
                // happened to have set at registration time) got baked in, and every
                // later change to Offset Position / Offset Orientation was overwritten
                // again on the next frame - the node did move for an instant and was
                // then snapped straight back. That is the whole "the offsets do nothing"
                // bug. GraphicsModule::setNodePosition()/setNodeOrientation() cannot
                // help here either: their re-pin block is commented out, so they only
                // warp the node and lose against updateAllTransforms() one frame later.
                //
                // Tracking this node buys nothing anyway: it is a plain static child of
                // the game object node, which is itself tracked and interpolated. Ogre's
                // scene graph propagates that to us for free.
            }

            // ---- Bubble geometry -------------------------------------------
            if (nullptr == this->manualObject)
            {
                // CHILD of the text node, at local origin, identity rotation and UNIT
                // scale.
                //
                // As a child there is nothing left to synchronise: bubble and text
                // inherit one single transform. The scale MUST stay 1.0 - see the
                // comment on MOVABLE_TEXT_SCALE_FACTOR at the top of this file. Under
                // Ogre-Next the glyphs are rendered from the parent node's full
                // transform, so a 0.5 here made the bubble half the size of the text
                // and moved its centre to half the text's local height, which is what
                // put the caption above the bubble instead of inside it.
                this->bubbleNode = this->textNode->createChildSceneNode();
                this->bubbleNode->setPosition(Ogre::Vector3::ZERO);
                this->bubbleNode->setOrientation(Ogre::Quaternion::IDENTITY);
                this->bubbleNode->setScale(Ogre::Vector3(MOVABLE_TEXT_SCALE_FACTOR, MOVABLE_TEXT_SCALE_FACTOR, MOVABLE_TEXT_SCALE_FACTOR));

                this->manualObject = sceneManager->createManualObject();
                // 212 is one below MovableText's own queue (213), so the bubble body
                // is always drawn before the glyphs sitting on top of it.
                this->manualObject->setRenderQueueGroup(212);
                this->manualObject->setName("SpeechBubble_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index));
                this->manualObject->setQueryFlags(0 << 0);
                this->manualObject->setCastShadows(false);
                this->bubbleNode->attachObject(this->manualObject);
                this->bubbleNode->setVisible(true);

                Ogre::HlmsDatablock* bubbleBaseDatablock = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault("WhiteNoLightingBackground");
                if (nullptr != bubbleBaseDatablock)
                {
                    Ogre::HlmsMacroblock macroblock = *bubbleBaseDatablock->getMacroblock();
                    macroblock.mDepthBiasConstant = 1.0f;
                    macroblock.mDepthBiasSlopeScale = 1.0f;
                    // The bubble is a flat quad that must stay readable from behind.
                    macroblock.mCullMode = Ogre::CULL_NONE;
                    bubbleBaseDatablock->setMacroblock(macroblock);
                }
            }

            this->applyTextSettings();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::createSpeechBubble");
    }

    void SpeechBubbleComponent::destroySpeechBubble(void)
    {
        if (nullptr == this->manualObject && nullptr == this->movableText)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            if (nullptr != this->manualObject)
            {
                this->bubbleNode->detachAllObjects();
                sceneManager->destroyManualObject(this->manualObject);
                this->manualObject = nullptr;
                this->manualObjectBegun = false;
            }
            if (nullptr != this->bubbleNode)
            {
                this->textNode->removeAndDestroyChild(this->bubbleNode);
                this->bubbleNode = nullptr;
            }

            if (nullptr != this->movableText)
            {
                this->textNode->detachObject(this->movableText);
                delete this->movableText;
                this->movableText = nullptr;
            }
            if (nullptr != this->textNode)
            {
                this->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->textNode);
                this->textNode = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::destroySpeechBubble");
    }

    void SpeechBubbleComponent::applyTextSettings(void)
    {
        if (nullptr == this->movableText)
        {
            return;
        }

        this->movableText->setFontName(this->fontName->getString());
        this->movableText->setCharacterHeight(this->charHeight->getReal());

        const Ogre::Vector4 c = this->textColor->getVector4();
        this->movableText->setColor(Ogre::ColourValue(c.x, c.y, c.z, c.w));

        // Always on top: the bubble body is drawn with a depth bias right behind the
        // glyphs, and without this the text z-fights against its own background.
        this->movableText->showOnTop(true);

        if (nullptr != this->textNode)
        {
            this->textNode->setPosition(this->offsetPosition->getVector3());
            this->applyStaticOrientation();
        }

        this->movableText->forceUpdate();
    }

    // ------------------------------------------------------------------------
    // Text measuring / wrapping
    // ------------------------------------------------------------------------

    Ogre::String SpeechBubbleComponent::wrapCaptionToWidth(const Ogre::String& text) const
    {
        const Ogre::Real limit = this->maxTextWidth->getReal();
        if (limit <= 0.0f)
        {
            return text;
        }

        Ogre::Font* font = loadFont(this->fontName->getString());
        if (nullptr == font)
        {
            // Falling back to a fixed character count is inaccurate for a proportional
            // font, but returning the text unwrapped produced a single mile-long line
            // and a bubble that spanned the whole scene. A rough wrap is the far
            // smaller evil, and the warning says why it happened.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[SpeechBubbleComponent] Could not load font '" + this->fontName->getString() + "' for game object: " + this->gameObjectPtr->getName() + ". Falling back to a rough character based word wrap.");

            Ogre::String fallback;
            size_t lineLen = 0;
            size_t lastSpace = Ogre::String::npos;
            for (size_t i = 0; i < text.size(); ++i)
            {
                const char c = text[i];
                if ('\n' == c)
                {
                    fallback += c;
                    lineLen = 0;
                    lastSpace = Ogre::String::npos;
                    continue;
                }
                if (' ' == c)
                {
                    lastSpace = fallback.size();
                }
                fallback += c;
                ++lineLen;
                if (lineLen >= 32u && Ogre::String::npos != lastSpace)
                {
                    fallback[lastSpace] = '\n';
                    lineLen = fallback.size() - lastSpace - 1u;
                    lastSpace = Ogre::String::npos;
                }
            }
            return fallback;
        }

        const Ogre::Real height = this->charHeight->getReal();
        const Ogre::Real spaceWidth = font->getGlyphAspectRatio('A') * height * MOVABLE_TEXT_UV_RANGE;

        Ogre::String result;
        result.reserve(text.size() + 8u);

        Ogre::Real lineWidth = 0.0f;
        size_t lastSpaceInResult = Ogre::String::npos;
        Ogre::Real widthAtLastSpace = 0.0f;

        for (size_t i = 0; i < text.size(); ++i)
        {
            const char c = text[i];

            if ('\n' == c)
            {
                result += c;
                lineWidth = 0.0f;
                lastSpaceInResult = Ogre::String::npos;
                continue;
            }

            const Ogre::Real advance = (' ' == c) ? spaceWidth : font->getGlyphAspectRatio(static_cast<unsigned char>(c)) * height * MOVABLE_TEXT_UV_RANGE;

            if (' ' == c)
            {
                lastSpaceInResult = result.size();
                widthAtLastSpace = lineWidth;
            }

            result += c;
            lineWidth += advance;

            if (lineWidth > limit)
            {
                if (Ogre::String::npos != lastSpaceInResult)
                {
                    // Turn the last space of this line into a line break and carry the
                    // width accumulated after that space over to the new line.
                    result[lastSpaceInResult] = '\n';
                    lineWidth = lineWidth - widthAtLastSpace - spaceWidth;
                    lastSpaceInResult = Ogre::String::npos;
                }
                else if (result.size() > 1u)
                {
                    // A single word longer than the whole line. Without this hard break
                    // it would run past the limit unchecked and blow the bubble up in x.
                    result.insert(result.size() - 1u, 1u, '\n');
                    lineWidth = advance;
                }
            }
        }

        return result;
    }

    // ------------------------------------------------------------------------
    // Transform sync
    // ------------------------------------------------------------------------

    bool SpeechBubbleComponent::resolveOrientationTargetPosition(Ogre::Vector3& outTargetPosition) const
    {
        if (0 == this->orientationTargetId->getULong())
        {
            // No target configured means NO orientation at all. The text and bubble
            // simply keep the game object's own orientation. Falling back to "face the
            // camera" here was wrong: it silently re-introduced billboarding that the
            // user never asked for.
            return false;
        }

        GameObject* target = this->orientationTargetGameObject;
        if (nullptr == target)
        {
            // Resolved lazily: at postInit time the target may not exist yet, and
            // resolving it only in connect() meant the editor never had it at all.
            auto targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->orientationTargetId->getULong());
            if (nullptr == targetGameObjectPtr)
            {
                return false;
            }
            const_cast<SpeechBubbleComponent*>(this)->orientationTargetGameObject = targetGameObjectPtr.get();
            target = this->orientationTargetGameObject;
        }

        // Any game object works as a target. If it happens to carry a camera, that
        // camera's position is the more accurate anchor.
        auto cameraCompPtr = NOWA::makeStrongPtr(target->getComponent<CameraComponent>());
        if (nullptr != cameraCompPtr && nullptr != cameraCompPtr->getCamera())
        {
            outTargetPosition = cameraCompPtr->getCamera()->getDerivedPosition();
        }
        else
        {
            outTargetPosition = target->getPosition();
        }

        return true;
    }

    void SpeechBubbleComponent::applyStaticOrientation(void)
    {
        if (nullptr == this->textNode)
        {
            return;
        }

        // Exactly the GameObjectTitleComponent pattern: the quad's own local forward
        // is +Z (that is what "correction" maps away from), rotated onto whatever
        // front axis the designer configured on the mesh. Set as the node's LOCAL
        // orientation and left alone - parented normally under the game object, so
        // Ogre's scene graph does the work of following the object's rotation every
        // frame automatically. No per-frame recompute needed for the no-target case.
        Ogre::Vector3 defaultDirection = this->gameObjectPtr->getDefaultDirection();
        if (defaultDirection.squaredLength() < 0.0001f)
        {
            defaultDirection = Ogre::Vector3::UNIT_Z;
        }

        const Ogre::Quaternion correction = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDirection);
        const Ogre::Quaternion offset = this->buildOffsetQuaternion();

        this->textNode->setOrientation(correction * offset);
    }

    Ogre::Quaternion SpeechBubbleComponent::buildOffsetQuaternion(void) const
    {
        // Built directly with Ogre::Quaternion instead of MathHelper::degreesToQuat():
        // that helper's axis order/convention could not be verified against this file,
        // and a silent mismatch there would look exactly like "OffsetOrientation does
        // nothing". This order is yaw (Y) then pitch (X) then roll (Z), each an
        // explicit, checkable rotation.
        const Ogre::Vector3 degrees = this->offsetOrientation->getVector3();
        return Ogre::Quaternion(Ogre::Degree(degrees.y), Ogre::Vector3::UNIT_Y) * Ogre::Quaternion(Ogre::Degree(degrees.x), Ogre::Vector3::UNIT_X) * Ogre::Quaternion(Ogre::Degree(degrees.z), Ogre::Vector3::UNIT_Z);
    }

    void SpeechBubbleComponent::updateOrientation(void)
    {
        if (nullptr == this->textNode || nullptr == this->movableText)
        {
            return;
        }

        Ogre::Vector3 targetPosition = Ogre::Vector3::ZERO;

        if (true == this->resolveOrientationTargetPosition(targetPosition))
        {
            // A target can move every frame (a walking character, an orbiting camera),
            // so this branch alone needs fresh per-frame work - and even here a single
            // getRotationTo() replaces the whole hand-rolled cross-product basis from
            // before.
            Ogre::Vector3 forward = targetPosition - this->textNode->_getDerivedPositionUpdated();
            if (forward.squaredLength() > 0.0001f)
            {
                forward.normalise();

                Ogre::SceneNode* parentNode = this->textNode->getParentSceneNode();
                const Ogre::Quaternion parentOrientation = (nullptr != parentNode) ? parentNode->_getDerivedOrientationUpdated() : Ogre::Quaternion::IDENTITY;

                // Face the target in WORLD space, deliberately ignoring the game
                // object's own current rotation - a speech bubble should look at its
                // target no matter which way the character happens to be facing.
                const Ogre::Quaternion worldOrientation = Ogre::Vector3::UNIT_Z.getRotationTo(forward) * this->buildOffsetQuaternion();

                this->textNode->setOrientation(parentOrientation.Inverse() * worldOrientation);
                this->movableText->setOrientationOverride(worldOrientation);
                return;
            }
        }

        // No target, or the target coincides with our own position: the local
        // orientation set by applyStaticOrientation() is still valid and was left
        // untouched. Only the DERIVED (world) orientation needs a fresh read here,
        // because the parent (the game object) can still be moving/rotating every
        // frame even though our own local offset is constant.
        this->movableText->setOrientationOverride(this->textNode->_getDerivedOrientationUpdated());
    }

    // ------------------------------------------------------------------------
    // Drawing
    // ------------------------------------------------------------------------

    void SpeechBubbleComponent::drawSpeechBubble(Ogre::Real dt)
    {
        this->couldDraw = false;

        if (nullptr == this->movableText || nullptr == this->manualObject || nullptr == this->bubbleNode)
        {
            return;
        }

        // "Nothing to show this frame". It deliberately does NOT touch the
        // ManualObject - see the comment on the closure in update() for why an
        // empty begin()/end() pair is what crashed Ogre.
        auto hideBubbleForThisFrame = [this]()
        {
            this->bubbleNode->setVisible(false);
        };

        // Gate on the wrapped text, not on the legacy variant: that one is only a
        // mirror and can lag behind the active caption of the sequence.
        if (true == this->wrappedCaption.empty())
        {
            hideBubbleForThisFrame();
            return;
        }

        // ---- Playback of the caption sequence ------------------------------
        //
        // This whole block used to sit INSIDE "if (runSpeech)". That is why the
        // sequence never moved on with the typewriter switched off: speechDone was
        // only ever evaluated in the run speech branch, so Caption0 stayed on
        // screen forever and Caption1..N were never reached. Playback is a property
        // of the sequence, not of the typewriter, so it runs in both modes now and
        // only the REVEAL of the characters is conditional.
        //
        // speechDone is now a terminal latch for "the whole sequence has finished"
        // instead of a flag that was set and cleared again in the same frame. The
        // old version zeroed the timers and the character index right after the
        // last caption, so the finished run silently started over and re-fired the
        // reactOnSpeechDone closure every few seconds. restartSequence(),
        // setCaption() and connect() clear it again.
        if (false == this->speechDone)
        {
            const size_t totalCharacters = this->wrappedCaption.length();
            const Ogre::Real captionDuration = std::max(this->speechDuration->getReal(), 0.0f);

            this->timeSinceLastRun += dt;

            if (true == this->runSpeech->getBool())
            {
                this->timeSinceLastChar += dt;

                const Ogre::Real timePerCharacter = (totalCharacters > 0u) ? captionDuration / static_cast<Ogre::Real>(totalCharacters) : 0.0f;

                bool revealedSomething = false;

                if (timePerCharacter <= 0.0f)
                {
                    // A duration of zero means there is no pacing to spread the
                    // characters over - show the caption in one go instead of
                    // emitting exactly one character per rendered frame.
                    if (this->currentCharIndex < totalCharacters)
                    {
                        this->currentCharIndex = static_cast<unsigned int>(totalCharacters);
                        revealedSomething = true;
                    }
                    this->timeSinceLastChar = 0.0f;
                }
                else
                {
                    // Subtract one character slot instead of zeroing the timer: the
                    // old code threw the remainder of every frame away, so the reveal
                    // was always slower than the configured duration and could never
                    // emit more than one character per frame on a short duration.
                    while (this->currentCharIndex < totalCharacters && this->timeSinceLastChar >= timePerCharacter)
                    {
                        this->timeSinceLastChar -= timePerCharacter;
                        this->currentCharIndex++;
                        revealedSomething = true;
                    }
                }

                if (true == revealedSomething)
                {
                    this->movableText->setCaption(mid(this->wrappedCaption, 0, this->currentCharIndex));
                    this->movableText->setVisibleRequested(this->activated->getBool());

                    if (true == this->runSpeechSound->getBool() && nullptr != this->simpleSoundComponent)
                    {
                        const Ogre::Real rndPitch = static_cast<Ogre::Real>(MathHelper::getInstance()->getRandomNumber(3, 10)) * 0.1f;
                        this->simpleSoundComponent->setPitch(rndPitch);
                        this->simpleSoundComponent->setActivated(true);
                    }
                }
            }
            else
            {
                // No typewriter: the full caption is on screen from the first frame,
                // so the reveal is complete right away and only the on screen time
                // of this caption is left to wait for.
                this->currentCharIndex = static_cast<unsigned int>(totalCharacters);
            }

            // With the typewriter the duration is spent revealing the text, so the
            // finished caption gets an extra reading pause on top. Without it the
            // duration IS the on screen time.
            const Ogre::Real captionOnScreenTime = (true == this->runSpeech->getBool()) ? captionDuration + CAPTION_HOLD_AFTER_REVEAL_SECONDS : captionDuration;

            if (this->currentCharIndex >= totalCharacters && this->timeSinceLastRun >= captionOnScreenTime)
            {
                if (nullptr != this->simpleSoundComponent)
                {
                    this->simpleSoundComponent->setActivated(false);
                }

                // More captions queued? Move on instead of finishing the whole run.
                if (this->currentCaptionIndex + 1u < static_cast<unsigned int>(this->captions.size()))
                {
                    // Resets the character index and both timers for the new caption.
                    this->advanceToNextCaption();
                    hideBubbleForThisFrame();
                    return;
                }

                this->speechDone = true;

                if (false == this->keepCaption->getBool())
                {
                    this->movableText->setVisibleRequested(false);
                    this->bubbleNode->setVisible(false);
                    this->currentCharIndex = 0;
                    this->movableText->setCaption("");
                }

                if (this->closureFunction.is_valid())
                {
                    NOWA::AppStateManager::LogicCommand logicCommand = [this]()
                    {
                        try
                        {
                            luabind::call_function<void>(this->closureFunction);
                        }
                        catch (luabind::error& error)
                        {
                            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                            std::stringstream msg;
                            msg << errorMsg;

                            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] Caught error in 'reactOnSpeechDone' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                        }
                    };
                    NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                }

                // No return here: with Keep Caption on, the finished caption stays
                // on screen and still has to be drawn. With it off the caption was
                // just cleared, and the empty caption guard below hides the bubble.
            }
        }

        if (true == this->movableText->getCaption().empty())
        {
            hideBubbleForThisFrame();
            return;
        }

        this->updateOrientation();

        // Everything below is built in the bubble node's LOCAL space, which is now
        // identical to the space MovableText lays its glyphs out in. No world space
        // maths, no derived transforms read from a foreign component, therefore no
        // parallax and no lag.
        const Ogre::Aabb liveTextAabb = this->movableText->getLocalAabb();

        const Ogre::Real textHalfWidth = (liveTextAabb.getMaximum().x - liveTextAabb.getMinimum().x) * 0.5f;
        const Ogre::Real textHalfHeight = (liveTextAabb.getMaximum().y - liveTextAabb.getMinimum().y) * 0.5f;
        const Ogre::Real textCenterX = (liveTextAabb.getMaximum().x + liveTextAabb.getMinimum().x) * 0.5f;
        const Ogre::Real textCenterY = (liveTextAabb.getMaximum().y + liveTextAabb.getMinimum().y) * 0.5f;

        // Rounded rectangle instead of an ellipse. An ellipse needs a factor of
        // sqrt(2) in BOTH axes just to touch the corners of the text block, so a
        // long line blew the width up far beyond the scene. A rounded rect hugs the
        // text with a constant margin and grows in y with the line count instead.
        const Ogre::Real pad = std::max(this->padding->getReal(), 0.0f);
        const Ogre::Real bubbleHorizontalOffset = this->xOffsetStart->getReal();

        // The text stays put while the body is shifted sideways, so the half width has
        // to cover the distance from the shifted centre out to the far text edge.
        const Ogre::Real halfWidth = std::max(textHalfWidth + Ogre::Math::Abs(bubbleHorizontalOffset) + pad, 0.05f);
        const Ogre::Real halfHeight = std::max(textHalfHeight + pad, 0.05f);

        const Ogre::Real centerX = textCenterX + bubbleHorizontalOffset;
        const Ogre::Real centerY = textCenterY;

        // Clamped so the corners can never overlap on a small bubble.
        const Ogre::Real radius = std::min(std::max(this->cornerRadius->getReal(), 0.0f), std::min(halfWidth, halfHeight));

        const Ogre::Vector4 bc = this->bubbleColor->getVector4();
        const Ogre::ColourValue bubbleColourValue(bc.x, bc.y, bc.z, bc.w);

        // ---- Outline of the rounded rect, counter clockwise ----------------
        const int cornerSegments = 5;
        const Ogre::Real quarter = Ogre::Math::PI * 0.5f;

        // Arc centres and start angles for the four corners, in ccw order starting
        // at the top right one.
        const Ogre::Real arcCenterX[4] = {centerX + halfWidth - radius, centerX - halfWidth + radius, centerX - halfWidth + radius, centerX + halfWidth - radius};
        const Ogre::Real arcCenterY[4] = {centerY + halfHeight - radius, centerY + halfHeight - radius, centerY - halfHeight + radius, centerY - halfHeight + radius};
        const Ogre::Real arcStart[4] = {0.0f, quarter, Ogre::Math::PI, 3.0f * quarter};

        std::vector<Ogre::Vector3> outline;
        outline.reserve(static_cast<size_t>(4 * (cornerSegments + 1)));

        for (int corner = 0; corner < 4; ++corner)
        {
            for (int s = 0; s <= cornerSegments; ++s)
            {
                const Ogre::Real angle = arcStart[corner] + quarter * (static_cast<Ogre::Real>(s) / static_cast<Ogre::Real>(cornerSegments));
                outline.push_back(Ogre::Vector3(arcCenterX[corner] + radius * Ogre::Math::Cos(angle), arcCenterY[corner] + radius * Ogre::Math::Sin(angle), 0.0f));
            }
        }

        const Ogre::Vector3 bubbleCenter(centerX, centerY, 0.0f);

        // ---- Tail ----------------------------------------------------------
        // Tip stays at the local origin, which is the anchor / mouth position and is
        // deliberately NOT shifted along with the body.
        const Ogre::Vector3 tailTip = Ogre::Vector3::ZERO;
        const Ogre::Real tailBaseY = centerY - halfHeight;
        const Ogre::Real tailHalfSpan = std::max(std::min(halfWidth - radius, pad + radius), 0.05f);

        // Anchor the tail to the part of the bottom edge closest to the tip, so it
        // always points at the character no matter how the body is offset.
        Ogre::Real tailAnchorX = 0.0f;
        const Ogre::Real tailMin = centerX - halfWidth + radius + tailHalfSpan;
        const Ogre::Real tailMax = centerX + halfWidth - radius - tailHalfSpan;
        if (tailMin <= tailMax)
        {
            tailAnchorX = std::min(std::max(tailAnchorX, tailMin), tailMax);
        }
        else
        {
            tailAnchorX = centerX;
        }

        // The triangles are collected here first and only handed to the
        // ManualObject further down. Nothing may be written into it before it is
        // known that there IS geometry, and the vertex count has to be identical on
        // every frame: ManualObject::position() performs no bounds check at all on
        // the beginUpdate() path, so writing more vertices than the buffer created
        // by the very first begin()/end() holds would silently run past the mapped
        // GPU buffer. The count below is constant (3 tail + 3 * 4 * (cornerSegments
        // + 1) body vertices) and independent of the caption, which is what makes
        // the update path safe.
        std::vector<Ogre::Vector3> triangleVertices;
        triangleVertices.reserve(3u + outline.size() * 3u);

        triangleVertices.push_back(Ogre::Vector3(tailAnchorX - tailHalfSpan, tailBaseY, 0.0f));
        triangleVertices.push_back(Ogre::Vector3(tailAnchorX + tailHalfSpan, tailBaseY, 0.0f));
        triangleVertices.push_back(tailTip);

        // ---- Body as a triangle fan around the centre ----------------------
        for (size_t i = 0; i < outline.size(); ++i)
        {
            const Ogre::Vector3& current = outline[i];
            const Ogre::Vector3& next = outline[(i + 1u) % outline.size()];

            triangleVertices.push_back(bubbleCenter);
            triangleVertices.push_back(current);
            triangleVertices.push_back(next);
        }

        if (true == triangleVertices.empty())
        {
            hideBubbleForThisFrame();
            return;
        }

        // ---- Upload --------------------------------------------------------
        // begin() runs EXACTLY ONCE over the whole lifetime of this ManualObject
        // and only on a frame that really produces geometry; every frame after that
        // is beginUpdate(0) + end(). clear() is never called here: calling it on a
        // section that was opened via beginUpdate() but not yet end()ed corrupts
        // ManualObject's internal state and produces "must call begin() before this
        // method" on a later frame.
        this->indices = 0;

        if (false == this->manualObjectBegun)
        {
            this->manualObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
        }
        else
        {
            this->manualObject->beginUpdate(0);
        }

        for (size_t i = 0; i < triangleVertices.size(); ++i)
        {
            this->manualObject->position(triangleVertices[i]);
            this->manualObject->colour(bubbleColourValue);
            this->manualObject->index(this->indices);
            this->indices++;
        }

        // Realllllllyyyyy important! Else the rectangle is a whole mess!
        this->manualObject->index(0);

        this->manualObject->end();

        // Only now, after an end() that actually saw vertices and therefore built
        // the VAO, is beginUpdate(0) safe on the next frame.
        this->manualObjectBegun = true;
        this->couldDraw = true;

        this->bubbleNode->setVisible(this->activated->getBool());
    }

    // ------------------------------------------------------------------------
    // Setters / getters
    // ------------------------------------------------------------------------

    void SpeechBubbleComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, activated]
        {
            if (nullptr != this->movableText)
            {
                this->movableText->setVisibleRequested(activated);
            }
            if (nullptr != this->bubbleNode)
            {
                this->bubbleNode->setVisible(activated);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setActivated");
    }

    bool SpeechBubbleComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void SpeechBubbleComponent::setCaption(const Ogre::String& caption)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, caption]
        {
            const Ogre::String tempCaption = replaceAll(caption, "\\n", "\n");
            this->caption->setValue(tempCaption);

            // Wrap BEFORE anything measures: the bubble sizing and the typewriter in
            // drawSpeechBubble() must both use this exact same wrapped string, or the
            // bubble and the visible text drift apart.
            this->wrappedCaption = this->wrapCaptionToWidth(tempCaption);

            this->currentCharIndex = 0;
            this->timeSinceLastRun = 0.0f;
            this->timeSinceLastChar = 0.0f;
            this->speechDone = false;

            if (nullptr != this->movableText)
            {
                // With the typewriter enabled the text starts out empty and grows one
                // character per tick; otherwise the full wrapped caption is shown.
                this->movableText->setCaption(true == this->runSpeech->getBool() ? Ogre::String("") : this->wrappedCaption);
                this->movableText->setVisibleRequested(this->activated->getBool());
            }

            if (nullptr != this->bubbleNode)
            {
                this->bubbleNode->setVisible(this->activated->getBool());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setCaption");
    }

    Ogre::String SpeechBubbleComponent::getCaption(void) const
    {
        return this->caption->getString();
    }

    void SpeechBubbleComponent::setFontName(const Ogre::String& fontName)
    {
        this->fontName->setValue(fontName);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            this->applyTextSettings();
            // Glyph widths changed, so the wrap has to be recomputed.
            this->wrappedCaption = this->wrapCaptionToWidth(this->caption->getString());
            if (nullptr != this->movableText && false == this->runSpeech->getBool())
            {
                this->movableText->setCaption(this->wrappedCaption);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setFontName");
    }

    Ogre::String SpeechBubbleComponent::getFontName(void) const
    {
        return this->fontName->getString();
    }

    void SpeechBubbleComponent::setCharHeight(Ogre::Real charHeight)
    {
        if (charHeight <= 0.0f)
        {
            charHeight = 0.01f;
        }
        this->charHeight->setValue(charHeight);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            this->applyTextSettings();
            // Advance scales linearly with the character height, so re-wrap.
            this->wrappedCaption = this->wrapCaptionToWidth(this->caption->getString());
            if (nullptr != this->movableText && false == this->runSpeech->getBool())
            {
                this->movableText->setCaption(this->wrappedCaption);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setCharHeight");
    }

    Ogre::Real SpeechBubbleComponent::getCharHeight(void) const
    {
        return this->charHeight->getReal();
    }

    void SpeechBubbleComponent::setTextColor(const Ogre::Vector4& textColor)
    {
        this->textColor->setValue(textColor);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, textColor]
        {
            if (nullptr != this->movableText)
            {
                this->movableText->setColor(Ogre::ColourValue(textColor.x, textColor.y, textColor.z, textColor.w));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setTextColor");
    }

    Ogre::Vector4 SpeechBubbleComponent::getTextColor(void) const
    {
        return this->textColor->getVector4();
    }

    void SpeechBubbleComponent::setBubbleColor(const Ogre::Vector4& bubbleColor)
    {
        // Picked up by drawSpeechBubble() on the next render tick, no command needed.
        this->bubbleColor->setValue(bubbleColor);
    }

    Ogre::Vector4 SpeechBubbleComponent::getBubbleColor(void) const
    {
        return this->bubbleColor->getVector4();
    }

    void SpeechBubbleComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
    {
        this->offsetPosition->setValue(offsetPosition);

        // Plain node write on the render thread. GraphicsModule::setNodePosition()
        // is the API for TRACKED nodes and this node is intentionally untracked
        // (see createSpeechBubble()), so there is no interpolation buffer that would
        // have to be kept in sync here.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, offsetPosition]
        {
            if (nullptr != this->textNode)
            {
                this->textNode->setPosition(offsetPosition);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setOffsetPosition");
    }

    Ogre::Vector3 SpeechBubbleComponent::getOffsetPosition(void) const
    {
        return this->offsetPosition->getVector3();
    }

    void SpeechBubbleComponent::setMaxTextWidth(Ogre::Real maxTextWidth)
    {
        if (maxTextWidth < 0.0f)
        {
            maxTextWidth = 0.0f;
        }
        this->maxTextWidth->setValue(maxTextWidth);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            this->wrappedCaption = this->wrapCaptionToWidth(this->caption->getString());
            if (nullptr != this->movableText && false == this->runSpeech->getBool())
            {
                this->movableText->setCaption(this->wrappedCaption);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setMaxTextWidth");
    }

    Ogre::Real SpeechBubbleComponent::getMaxTextWidth(void) const
    {
        return this->maxTextWidth->getReal();
    }

    void SpeechBubbleComponent::setRunSpeech(bool runSpeech)
    {
        this->runSpeech->setValue(runSpeech);
        // Re-applies the caption in the right starting state for the chosen mode.
        this->setCaption(this->caption->getString());
    }

    bool SpeechBubbleComponent::getRunSpeech(void) const
    {
        return this->runSpeech->getBool();
    }

    void SpeechBubbleComponent::setSpeechDuration(Ogre::Real speechDurationSec)
    {
        if (speechDurationSec < 0.0f)
        {
            speechDurationSec = 0.0f;
        }
        this->speechDuration->setValue(speechDurationSec);
    }

    Ogre::Real SpeechBubbleComponent::getSpeechDuration(void) const
    {
        return this->speechDuration->getReal();
    }

    void SpeechBubbleComponent::setRunSpeechSound(bool runSpeechSound)
    {
        if (true == runSpeechSound)
        {
            if (nullptr == this->simpleSoundComponent)
            {
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[SpeechBubbleComponent] Could not set run speech sound, because there is no simple sound component. Add one first! For game object: " + this->gameObjectPtr->getName());
                return;
            }
        }

        this->runSpeechSound->setValue(runSpeechSound);
    }

    bool SpeechBubbleComponent::getRunSpeechSound(void) const
    {
        return this->runSpeechSound->getBool();
    }

    void SpeechBubbleComponent::setXOffsetStart(Ogre::Real xOffsetStart)
    {
        this->xOffsetStart->setValue(xOffsetStart);
    }

    Ogre::Real SpeechBubbleComponent::getXOffsetStart(void) const
    {
        return this->xOffsetStart->getReal();
    }

    void SpeechBubbleComponent::setKeepCaption(bool keepCaption)
    {
        this->keepCaption->setValue(keepCaption);
    }

    bool SpeechBubbleComponent::getKeepCaption(void) const
    {
        return this->keepCaption->getBool();
    }

    void SpeechBubbleComponent::setOrientationTargetId(unsigned long orientationTargetId)
    {
        this->orientationTargetId->setValue(orientationTargetId);
        // Cleared, not resolved here: the target may not exist yet at load time.
        // resolveOrientationTargetPosition() picks it up on first use.
        this->orientationTargetGameObject = nullptr;

        if (0 == orientationTargetId)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]
            {
                // Restores the constant local orientation - without this the node
                // would keep whatever world-facing orientation updateOrientation()
                // last computed while a target was still active.
                this->applyStaticOrientation();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setOrientationTargetId");
        }
    }

    unsigned long SpeechBubbleComponent::getOrientationTargetId(void) const
    {
        return this->orientationTargetId->getULong();
    }

    void SpeechBubbleComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
    {
        this->offsetOrientation->setValue(offsetOrientation);

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]
        {
            // Only meaningful for the no-target case - with a target set,
            // updateOrientation() folds the offset in fresh every frame anyway.
            this->applyStaticOrientation();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SpeechBubbleComponent::setOffsetOrientation");
    }

    Ogre::Vector3 SpeechBubbleComponent::getOffsetOrientation(void) const
    {
        return this->offsetOrientation->getVector3();
    }

    void SpeechBubbleComponent::setPadding(Ogre::Real padding)
    {
        if (padding < 0.0f)
        {
            padding = 0.0f;
        }
        this->padding->setValue(padding);
    }

    Ogre::Real SpeechBubbleComponent::getPadding(void) const
    {
        return this->padding->getReal();
    }

    void SpeechBubbleComponent::setCornerRadius(Ogre::Real cornerRadius)
    {
        if (cornerRadius < 0.0f)
        {
            cornerRadius = 0.0f;
        }
        this->cornerRadius->setValue(cornerRadius);
    }

    Ogre::Real SpeechBubbleComponent::getCornerRadius(void) const
    {
        return this->cornerRadius->getReal();
    }

    Ogre::String SpeechBubbleComponent::getCaptionAt(unsigned int index) const
    {
        if (index >= this->captions.size() || nullptr == this->captions[index])
        {
            return Ogre::String();
        }
        return this->captions[index]->getString();
    }

    void SpeechBubbleComponent::advanceToNextCaption(void)
    {
        // Empty slots are skipped. drawSpeechBubble() bails out early on an empty
        // wrapped caption, so landing on one used to freeze the whole sequence for
        // good - which is exactly what happened after raising Caption Count without
        // filling in the new text right away. The guard counter keeps this
        // terminating when every single slot is empty.
        const unsigned int captionSlots = static_cast<unsigned int>(this->captions.size());
        for (unsigned int guard = 0; guard < captionSlots; guard++)
        {
            this->currentCaptionIndex++;
            if (this->currentCaptionIndex >= captionSlots)
            {
                this->currentCaptionIndex = 0;
            }

            if (false == this->getCaptionAt(this->currentCaptionIndex).empty())
            {
                break;
            }
        }

        // The active duration follows the caption, so each entry can have its own pace.
        this->speechDuration->setValue(this->captionDurations[this->currentCaptionIndex]->getReal());

        const Ogre::String next = this->getCaptionAt(this->currentCaptionIndex);
        this->caption->setValue(next);
        this->wrappedCaption = this->wrapCaptionToWidth(next);

        this->currentCharIndex = 0;
        this->timeSinceLastChar = 0.0f;
        this->timeSinceLastRun = 0.0f;
        this->speechDone = false;

        if (nullptr != this->movableText)
        {
            this->movableText->setCaption(true == this->runSpeech->getBool() ? Ogre::String("") : this->wrappedCaption);
            this->movableText->setVisibleRequested(this->activated->getBool());
        }
        if (nullptr != this->bubbleNode)
        {
            this->bubbleNode->setVisible(this->activated->getBool());
        }
    }

    void SpeechBubbleComponent::setCaptionCount(unsigned int captionCount)
    {
        // At least one caption always exists, so the rest of the component never has
        // to special case an empty list.
        if (captionCount < 1u)
        {
            captionCount = 1u;
        }

        this->captionCount->setValue(captionCount);

        const size_t oldSize = this->captions.size();

        if (captionCount > oldSize)
        {
            this->captions.resize(captionCount);
            this->captionDurations.resize(captionCount);

            for (unsigned int i = static_cast<unsigned int>(oldSize); i < captionCount; i++)
            {
                this->captions[i] = new Variant(SpeechBubbleComponent::AttrCaption() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
                this->captionDurations[i] = new Variant(SpeechBubbleComponent::AttrSpeechDuration() + Ogre::StringConverter::toString(i), 10.0f, this->attributes);
                this->captionDurations[i]->setDescription("How long this caption remains, in seconds.");
            }
        }
        else if (captionCount < oldSize)
        {
            this->eraseVariants(this->captions, captionCount);
            this->eraseVariants(this->captionDurations, captionCount);
        }

        if (this->currentCaptionIndex >= captionCount)
        {
            this->currentCaptionIndex = 0;
        }
    }

    unsigned int SpeechBubbleComponent::getCaptionCount(void) const
    {
        return this->captionCount->getUInt();
    }

    void SpeechBubbleComponent::setCaption(unsigned int index, const Ogre::String& caption)
    {
        if (index >= this->captions.size())
        {
            return;
        }

        this->captions[index]->setValue(caption);

        // Only push through to the visible text when this is the caption on screen.
        if (index == this->currentCaptionIndex)
        {
            this->setCaption(caption);
        }
    }

    Ogre::String SpeechBubbleComponent::getCaption(unsigned int index) const
    {
        return this->getCaptionAt(index);
    }

    void SpeechBubbleComponent::setSpeechDuration(unsigned int index, Ogre::Real speechDurationSec)
    {
        if (index >= this->captionDurations.size())
        {
            return;
        }
        if (speechDurationSec < 0.0f)
        {
            speechDurationSec = 0.0f;
        }

        this->captionDurations[index]->setValue(speechDurationSec);

        if (index == this->currentCaptionIndex)
        {
            this->speechDuration->setValue(speechDurationSec);
        }
    }

    Ogre::Real SpeechBubbleComponent::getSpeechDuration(unsigned int index) const
    {
        if (index >= this->captionDurations.size() || nullptr == this->captionDurations[index])
        {
            return 0.0f;
        }
        return this->captionDurations[index]->getReal();
    }

    void SpeechBubbleComponent::restartSequence(void)
    {
        // Defensive: every path into the component must find at least one slot.
        if (true == this->captions.empty())
        {
            this->setCaptionCount(1);
        }

        this->currentCaptionIndex = 0;
        this->speechDuration->setValue(this->captionDurations[0]->getReal());
        this->setCaption(this->getCaptionAt(0));
    }

    unsigned int SpeechBubbleComponent::getCurrentCaptionIndex(void) const
    {
        return this->currentCaptionIndex;
    }

    void SpeechBubbleComponent::reactOnSpeechDone(luabind::object closureFunction)
    {
        this->closureFunction = closureFunction;
    }

    MovableText* SpeechBubbleComponent::getMovableText(void) const
    {
        return this->movableText;
    }

    // Lua registration part

    SpeechBubbleComponent* getSpeechBubbleComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentWithOccurrence<SpeechBubbleComponent>(occurrenceIndex)).get();
    }

    SpeechBubbleComponent* getSpeechBubbleComponent(GameObject* gameObject)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponent<SpeechBubbleComponent>()).get();
    }

    SpeechBubbleComponent* getSpeechBubbleComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<SpeechBubbleComponent>(gameObject->getComponentFromName<SpeechBubbleComponent>(name)).get();
    }

    void SpeechBubbleComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<SpeechBubbleComponent, GameObjectComponent>("SpeechBubbleComponent")
                .def("setActivated", &SpeechBubbleComponent::setActivated)
                .def("isActivated", &SpeechBubbleComponent::isActivated)
                .def("setRunSpeech", &SpeechBubbleComponent::setRunSpeech)
                .def("getRunSpeech", &SpeechBubbleComponent::getRunSpeech)
                .def("setCaption", (void (SpeechBubbleComponent::*)(const Ogre::String&))&SpeechBubbleComponent::setCaption)
                .def("getCaption", (Ogre::String (SpeechBubbleComponent::*)(void) const) & SpeechBubbleComponent::getCaption)
                .def("setCaptionCount", &SpeechBubbleComponent::setCaptionCount)
                .def("getCaptionCount", &SpeechBubbleComponent::getCaptionCount)
                .def("setCaptionAt", (void (SpeechBubbleComponent::*)(unsigned int, const Ogre::String&))&SpeechBubbleComponent::setCaption)
                .def("getCaptionAt", (Ogre::String (SpeechBubbleComponent::*)(unsigned int) const) & SpeechBubbleComponent::getCaption)
                .def("setSpeechDurationAt", (void (SpeechBubbleComponent::*)(unsigned int, Ogre::Real))&SpeechBubbleComponent::setSpeechDuration)
                .def("getSpeechDurationAt", (Ogre::Real (SpeechBubbleComponent::*)(unsigned int) const) & SpeechBubbleComponent::getSpeechDuration)
                .def("restartSequence", &SpeechBubbleComponent::restartSequence)
                .def("getCurrentCaptionIndex", &SpeechBubbleComponent::getCurrentCaptionIndex)
                .def("setSpeechDuration", (void (SpeechBubbleComponent::*)(Ogre::Real))&SpeechBubbleComponent::setSpeechDuration)
                .def("getSpeechDuration", (Ogre::Real (SpeechBubbleComponent::*)(void) const) & SpeechBubbleComponent::getSpeechDuration)
                .def("setRunSpeechSound", &SpeechBubbleComponent::setRunSpeechSound)
                .def("getRunSpeechSound", &SpeechBubbleComponent::getRunSpeechSound)
                .def("setKeepCaption", &SpeechBubbleComponent::setKeepCaption)
                .def("getKeepCaption", &SpeechBubbleComponent::getKeepCaption)
                .def("setXOffsetStart", &SpeechBubbleComponent::setXOffsetStart)
                .def("getXOffsetStart", &SpeechBubbleComponent::getXOffsetStart)
                .def("setFontName", &SpeechBubbleComponent::setFontName)
                .def("getFontName", &SpeechBubbleComponent::getFontName)
                .def("setCharHeight", &SpeechBubbleComponent::setCharHeight)
                .def("getCharHeight", &SpeechBubbleComponent::getCharHeight)
                .def("setTextColor", &SpeechBubbleComponent::setTextColor)
                .def("getTextColor", &SpeechBubbleComponent::getTextColor)
                .def("setBubbleColor", &SpeechBubbleComponent::setBubbleColor)
                .def("getBubbleColor", &SpeechBubbleComponent::getBubbleColor)
                .def("setOffsetPosition", &SpeechBubbleComponent::setOffsetPosition)
                .def("getOffsetPosition", &SpeechBubbleComponent::getOffsetPosition)
                .def("setMaxTextWidth", &SpeechBubbleComponent::setMaxTextWidth)
                .def("getMaxTextWidth", &SpeechBubbleComponent::getMaxTextWidth)
                .def("setOrientationTargetId", &SpeechBubbleComponent::setOrientationTargetId)
                .def("getOrientationTargetId", &SpeechBubbleComponent::getOrientationTargetId)
                .def("setOffsetOrientation", &SpeechBubbleComponent::setOffsetOrientation)
                .def("getOffsetOrientation", &SpeechBubbleComponent::getOffsetOrientation)
                .def("setPadding", &SpeechBubbleComponent::setPadding)
                .def("getPadding", &SpeechBubbleComponent::getPadding)
                .def("setCornerRadius", &SpeechBubbleComponent::setCornerRadius)
                .def("getCornerRadius", &SpeechBubbleComponent::getCornerRadius)
                .def("reactOnSpeechDone", &SpeechBubbleComponent::reactOnSpeechDone)];

        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "class inherits GameObjectComponent", SpeechBubbleComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeech(bool runSpeech)", "Sets whether the speech text shall appear char by char running.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeech()", "Gets whether the speech text shall appear char by char running.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCaption(string caption)", "Sets the caption text to be displayed.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "string getCaption()", "Gets the caption text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setSpeechDuration(float speechDuration)", "Sets the speech duration. That is how long the bubble shall remain in seconds.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getSpeechDuration()", "Gets the speech duration. That is how long the bubble shall remain in seconds.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setKeepCaption(bool keepCaption)", "Sets to keep the caption after the run speech is done.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getKeepCaption()", "Gets whether the caption after the run speech is done is kept.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setXOffsetStart(float xOffsetStart)", "Sets the horizontal offset of the bubble body relative to the text anchor.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getXOffsetStart()", "Gets the horizontal offset of the bubble body relative to the text anchor.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setRunSpeechSound(bool runSpeechSound)", "Sets whether to use a sound if the speech is running char by char.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "bool getRunSpeechSound()", "Gets whether a sound is used if the speech is running char by char.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setFontName(string fontName)", "Sets the font used for the speech text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "string getFontName()", "Gets the font used for the speech text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCharHeight(float charHeight)", "Sets the character height of the speech text in local units.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getCharHeight()", "Gets the character height of the speech text in local units.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setTextColor(Vector4 color)", "Sets the color for the text.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector4 getTextColor()", "Gets the text color.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setBubbleColor(Vector4 color)", "Sets the color for the bubble body.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector4 getBubbleColor()", "Gets the bubble body color.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets the offset position of the speech bubble relative to the game object.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector3 getOffsetPosition()", "Gets the offset position of the speech bubble relative to the game object.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setMaxTextWidth(float maxTextWidth)", "Sets the maximum text width in local units used for word wrapping.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getMaxTextWidth()", "Gets the maximum text width in local units used for word wrapping.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setOrientationTargetId(number orientationTargetId)", "Sets the id of the game object with a CameraComponent the bubble should face. 0 uses the active camera.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "number getOrientationTargetId()", "Gets the id of the game object with a CameraComponent the bubble faces.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setOffsetOrientation(Vector3 offsetOrientation)", "Sets the additional rotation in degrees applied on top of the billboard rotation.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "Vector3 getOffsetOrientation()", "Gets the additional rotation in degrees.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setPadding(float padding)", "Sets the margin between the text block and the bubble border.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getPadding()", "Gets the margin between the text block and the bubble border.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCornerRadius(float cornerRadius)", "Sets the corner radius of the rounded bubble body.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getCornerRadius()", "Gets the corner radius of the rounded bubble body.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCaptionCount(int captionCount)", "Sets how many captions are played back one after another.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "int getCaptionCount()", "Gets how many captions are played back one after another.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setCaptionAt(int index, string caption)", "Sets the caption of the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "string getCaptionAt(int index)", "Gets the caption of the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void setSpeechDurationAt(int index, float speechDuration)", "Sets the speech duration of the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "float getSpeechDurationAt(int index)", "Gets the speech duration of the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void restartSequence()", "Restarts the whole caption sequence at index 0.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "int getCurrentCaptionIndex()", "Gets the index of the caption currently being played back.");
        LuaScriptApi::getInstance()->addClassToCollection("SpeechBubbleComponent", "void reactOnSpeechDone(func closureFunction)", "Sets whether to react if a speech is done.");

        gameObjectClass.def("getSpeechBubbleComponentFromName", &getSpeechBubbleComponentFromName);
        gameObjectClass.def("getSpeechBubbleComponent", (SpeechBubbleComponent * (*)(GameObject*)) & getSpeechBubbleComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SpeechBubbleComponent getSpeechBubbleComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castSpeechBubbleComponent", &GameObjectController::cast<SpeechBubbleComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SpeechBubbleComponent castSpeechBubbleComponent(SpeechBubbleComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool SpeechBubbleComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // The GameObjectTitleComponent requirement is gone - this component brings its
        // own text now. The occurrence guard below actually returns false when the
        // limit is reached; the previous version had "return true" in both branches,
        // so any number of bubbles could be stacked on one game object.
        return gameObject->getComponentCount<SpeechBubbleComponent>() < 1;
    }

}; // namespace end