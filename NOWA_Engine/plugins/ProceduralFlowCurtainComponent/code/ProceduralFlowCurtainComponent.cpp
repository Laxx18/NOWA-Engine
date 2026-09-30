/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralFlowCurtainComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/Helper.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "RenderQueueEnums.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreHlmsUnlit.h"
#include "OgreHlmsUnlitDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreAbiUtils.h"
// =============================================================================
// ProceduralFlowCurtainComponent
//
// See the class comment in the header for the full geometry/scrolling reasoning. The short
// version, since it is easy to conflate with ProceduralConveyorLoopComponent (which this is
// deliberately modelled after):
//
//   - Same core trick: PBS has no material-level scrolling texture animation, so this rewrites
//     the mesh's own V coordinate every frame via a BT_DYNAMIC_DEFAULT vertex buffer.
//   - But this shape is OPEN (a flat sheet), not a closed loop - so none of the belt's
//     perimeter/arc-length/integer-repeat-count machinery applies here. Each vertex's base V is
//     just its row's height fraction times that layer's own V tiling, fixed for the vertex's
//     entire lifetime; the per-frame scroll offset is wrapped against a single texture repeat
//     (1.0), which is always safe for a tileable texture and needs no special constraint.
//   - Two layers (flow + foam), not two materials on one shape split into scrolling/static parts
//     like the belt/end-caps - here BOTH layers scroll, just usually at different speeds, so both
//     get their own dynamic vertex buffer.
//   - No physics collision is ever created. That is intentional, not an oversight - see the
//     class comment.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    namespace
    {
        // How many full sine waves the PRIMARY static ripple bulge completes across the
        // curtain's width. 1.5 reads as an irregular, organic curve rather than a single
        // symmetric bulge (an integer count would look too much like a deliberate wave pattern).
        const Ogre::Real FLOW_RIPPLE_WAVE_COUNT = 1.5f;

        // A second, higher-frequency, smaller-amplitude term layered on top of the primary wave
        // (classic multi-octave/turbulence composition) - this is what actually reads as "chaos"
        // instead of "one clean, mechanical-looking wave".
        const Ogre::Real FLOW_RIPPLE_WAVE_COUNT_2 = 3.5f;
        const Ogre::Real FLOW_RIPPLE_SECONDARY_STRENGTH = 0.4f;

        // How many times the ripple's phase twists as height (Y) goes from 0 to 1. Without this,
        // the Z curve is identical at every row - literally the same profile extruded straight
        // down - which is exactly what read as "an exactly downward-facing plane" despite having
        // a ripple at all. A non-integer count keeps the twist from ever looking like a clean,
        // regular pattern either.
        const Ogre::Real FLOW_RIPPLE_VERTICAL_TWIST_COUNT = 0.85f;
        const Ogre::Real FLOW_RIPPLE_VERTICAL_TWIST_STRENGTH = 1.6f;

        // Sideways (X) sway applied per ROW, on top of the Z ripple - a slower, lower-frequency
        // sine term that shifts an entire row left/right together. This breaks the silhouette out
        // of being a perfect rectangle and, combined with the twisting Z ripple, is most of what
        // reads as "volume" rather than "flat scrolling card".
        const Ogre::Real FLOW_SWAY_WAVE_COUNT = 1.3f;
        const Ogre::Real FLOW_SWAY_STRENGTH = 0.5f;

        /**
         * @brief Cheap deterministic pseudo-random float in [0, 1) from an integer seed - no
         *        <random> engine/state needed, and fully deterministic per game object id, so
         *        every placed curtain gets its own fixed random phase (no two curtains ripple in
         *        obvious lockstep) without needing to store or serialize any extra per-instance
         *        state - the seed is recomputed identically every time from the game object's own
         *        id. Standard integer hash (a variant of Bob Jenkins' one-at-a-time / Thomas Wang
         *        mix), not cryptographic, just needs to scatter well enough that nearby ids don't
         *        produce visibly-correlated phases.
         */
        inline Ogre::Real flowHashNoise(unsigned int seed)
        {
            seed = (seed ^ 61u) ^ (seed >> 16);
            seed = seed + (seed << 3);
            seed = seed ^ (seed >> 4);
            seed = seed * 0x27d4eb2du;
            seed = seed ^ (seed >> 15);
            return static_cast<Ogre::Real>(seed & 0x00FFFFFFu) / static_cast<Ogre::Real>(0x00FFFFFFu);
        }
    }

    ProceduralFlowCurtainComponent::ProceduralFlowCurtainComponent() :
        GameObjectComponent(),
        name("ProceduralFlowCurtainComponent"),
        activated(new Variant(ProceduralFlowCurtainComponent::AttrActivated(), true, this->attributes)),
        width(new Variant(ProceduralFlowCurtainComponent::AttrWidth(), 4.0f, this->attributes)),
        height(new Variant(ProceduralFlowCurtainComponent::AttrHeight(), 3.0f, this->attributes)),
        // Raised from 6x4 - the old low-resolution defaults left barely enough geometry for even
        // a single clean ripple wave, let alone the twisting, multi-octave one buildLayerGrid()
        // now bakes in. See the class comment's RIPPLE section.
        rows(new Variant(ProceduralFlowCurtainComponent::AttrRows(), 10, this->attributes)),
        cols(new Variant(ProceduralFlowCurtainComponent::AttrCols(), 10, this->attributes)),
        rippleAmount(new Variant(ProceduralFlowCurtainComponent::AttrRippleAmount(), Ogre::Real(0.05f), this->attributes)),
        horizontalUVTiling(new Variant(ProceduralFlowCurtainComponent::AttrHorizontalUVTiling(), Ogre::Real(0.5f), this->attributes)),
        flowDatablock(new Variant(ProceduralFlowCurtainComponent::AttrFlowDatablock(), Ogre::String("Flow/WaterA"), this->attributes)),
        flowSpeed(new Variant(ProceduralFlowCurtainComponent::AttrFlowSpeed(), Ogre::Real(0.6f), this->attributes)),
        flowVTiling(new Variant(ProceduralFlowCurtainComponent::AttrFlowVTiling(), Ogre::Real(1.0f), this->attributes)),
        foamEnabled(new Variant(ProceduralFlowCurtainComponent::AttrFoamEnabled(), true, this->attributes)),
        foamDatablock(new Variant(ProceduralFlowCurtainComponent::AttrFoamDatablock(), Ogre::String("Flow/FoamA"), this->attributes)),
        foamSpeed(new Variant(ProceduralFlowCurtainComponent::AttrFoamSpeed(), Ogre::Real(0.9f), this->attributes)),
        foamVTiling(new Variant(ProceduralFlowCurtainComponent::AttrFoamVTiling(), Ogre::Real(1.5f), this->attributes)),
        foamZOffset(new Variant(ProceduralFlowCurtainComponent::AttrFoamZOffset(), Ogre::Real(0.03f), this->attributes)),
        flowColourRed(new Variant(ProceduralFlowCurtainComponent::AttrFlowColourRed(), Ogre::Real(0.45f), this->attributes)),
        flowColourGreen(new Variant(ProceduralFlowCurtainComponent::AttrFlowColourGreen(), Ogre::Real(0.75f), this->attributes)),
        flowColourBlue(new Variant(ProceduralFlowCurtainComponent::AttrFlowColourBlue(), Ogre::Real(0.98f), this->attributes)),
        flowOpacity(new Variant(ProceduralFlowCurtainComponent::AttrFlowOpacity(), Ogre::Real(0.6f), this->attributes)),
        flowItem(nullptr),
        flowColourCloneCounter(0u)
    {
        this->activated->setDescription("Activates the curtain. When deactivated the mesh is removed.");

        this->width->setDescription("Width of the curtain in meters, centred on local X.");
        this->width->setConstraints(0.2f, 1000.0f);

        this->height->setDescription("Height of the curtain in meters, from y=0 (base - place the node where the "
                                     "curtain meets the ground/pool) up to y=Height (the top edge).");
        this->height->setConstraints(0.2f, 1000.0f);

        this->rows->setDescription("Vertical subdivisions of the grid. Higher values give 'Ripple Amount' more "
                                   "geometry to curve and improve normal-mapped lighting over a tall surface.");
        this->rows->setConstraints(1, 200);

        this->cols->setDescription("Horizontal subdivisions of the grid. Same reasoning as 'Rows', across the width.");
        this->cols->setConstraints(1, 200);

        this->rippleAmount->setDescription("Fixed (non-animated) sine bulge across the width, in meters - breaks up "
                                           "an otherwise perfectly flat sheet. 0 = flat.");
        this->rippleAmount->setConstraints(0.0f, 10.0f);

        this->horizontalUVTiling->setDescription("Texture tiling across the width (the non-scrolling axis), in "
                                                 "repeats per meter. Applies to both layers.");

        this->flowDatablock->setDescription("Datablock covering the main falling surface (always built while Activated).");

        this->flowSpeed->setDescription("How fast the flow layer's texture scrolls, in meters per second. Negative "
                                        "reverses direction.");

        this->flowVTiling->setDescription("Flow layer texture tiling along the height (the scrolling axis), in "
                                          "repeats per meter.");

        this->foamEnabled->setDescription("Adds a second, optional layer in front of the flow layer for spray/foam "
                                          "detail.");

        this->foamDatablock->setDescription("Datablock covering the optional foam layer.");

        this->foamSpeed->setDescription("How fast the foam layer's texture scrolls, in meters per second. Negative "
                                        "reverses direction.");

        this->foamVTiling->setDescription("Foam layer texture tiling along the height, in repeats per meter.");

        this->foamZOffset->setDescription("How far in front of the flow layer the foam layer sits, in meters along "
                                          "local Z.");
        this->foamZOffset->setConstraints(-10.0f, 10.0f);

        this->flowColourRed->setDescription("Red tint applied to THIS instance's flow layer only, via a per-object "
                                            "cloned datablock - does not affect the shared 'Flow Datablock' material "
                                            "or any other curtain using it.");
        this->flowColourRed->setConstraints(0.0f, 1.0f);

        this->flowColourGreen->setDescription("Green tint applied to this instance's flow layer only. See 'Flow Colour Red'.");
        this->flowColourGreen->setConstraints(0.0f, 1.0f);

        this->flowColourBlue->setDescription("Blue tint applied to this instance's flow layer only. See 'Flow Colour Red'.");
        this->flowColourBlue->setConstraints(0.0f, 1.0f);

        this->flowOpacity->setDescription("Overall opacity of this instance's flow layer, multiplied with the "
                                          "texture's own alpha. Lower = more translucent/less 'plastic' looking.");
        this->flowOpacity->setConstraints(0.0f, 1.0f);

        this->flowMeshName = "";
    }

    ProceduralFlowCurtainComponent::~ProceduralFlowCurtainComponent()
    {
    }

    const Ogre::String& ProceduralFlowCurtainComponent::getName() const
    {
        return this->name;
    }

    void ProceduralFlowCurtainComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralFlowCurtainComponent>(ProceduralFlowCurtainComponent::getStaticClassId(), ProceduralFlowCurtainComponent::getStaticClassName());
    }

    void ProceduralFlowCurtainComponent::shutdown()
    {
    }

    void ProceduralFlowCurtainComponent::uninstall()
    {
    }

    void ProceduralFlowCurtainComponent::initialise()
    {
    }

    void ProceduralFlowCurtainComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralFlowCurtainComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrWidth())
        {
            this->width->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 4.0f), 0.2f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrHeight())
        {
            this->height->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 3.0f), 0.2f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrRows())
        {
            this->rows->setValue(Ogre::Math::Clamp(XMLConverter::getAttribInt(propertyElement, "data", 10), 1, 200));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrCols())
        {
            this->cols->setValue(Ogre::Math::Clamp(XMLConverter::getAttribInt(propertyElement, "data", 10), 1, 200));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrRippleAmount())
        {
            this->rippleAmount->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.05f), 0.0f, 10.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrHorizontalUVTiling())
        {
            this->horizontalUVTiling->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowDatablock())
        {
            this->flowDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "Flow/WaterA"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowSpeed())
        {
            this->flowSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.6f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowVTiling())
        {
            this->flowVTiling->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFoamEnabled())
        {
            this->foamEnabled->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFoamDatablock())
        {
            this->foamDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "Flow/FoamA"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFoamSpeed())
        {
            this->foamSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.9f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFoamVTiling())
        {
            this->foamVTiling->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFoamZOffset())
        {
            this->foamZOffset->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.03f), -10.0f, 10.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowColourRed())
        {
            this->flowColourRed->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.45f), 0.0f, 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowColourGreen())
        {
            this->flowColourGreen->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.75f), 0.0f, 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowColourBlue())
        {
            this->flowColourBlue->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.98f), 0.0f, 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFlowCurtainComponent::AttrFlowOpacity())
        {
            this->flowOpacity->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.6f), 0.0f, 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralFlowCurtainComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralFlowCurtainCompPtr clonedCompPtr(boost::make_shared<ProceduralFlowCurtainComponent>());

        // setOwner() must come before any setter below - every one of them calls rebuildMesh()
        // internally, which reaches through this->gameObjectPtr for the scene manager and scene
        // node (same ordering requirement ProceduralConveyorLoopComponent::clone() documents).
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setWidth(this->width->getReal());
        clonedCompPtr->setHeight(this->height->getReal());
        clonedCompPtr->setRows(this->rows->getInt());
        clonedCompPtr->setCols(this->cols->getInt());
        clonedCompPtr->setRippleAmount(this->rippleAmount->getReal());
        clonedCompPtr->setHorizontalUVTiling(this->horizontalUVTiling->getReal());
        clonedCompPtr->setFlowDatablock(this->flowDatablock->getString());
        clonedCompPtr->setFlowSpeed(this->flowSpeed->getReal());
        clonedCompPtr->setFlowVTiling(this->flowVTiling->getReal());
        clonedCompPtr->setFoamEnabled(this->foamEnabled->getBool());
        clonedCompPtr->setFoamDatablock(this->foamDatablock->getString());
        clonedCompPtr->setFoamSpeed(this->foamSpeed->getReal());
        clonedCompPtr->setFoamVTiling(this->foamVTiling->getReal());
        clonedCompPtr->setFoamZOffset(this->foamZOffset->getReal());
        clonedCompPtr->setFlowColourRed(this->flowColourRed->getReal());
        clonedCompPtr->setFlowColourGreen(this->flowColourGreen->getReal());
        clonedCompPtr->setFlowColourBlue(this->flowColourBlue->getReal());
        clonedCompPtr->setFlowOpacity(this->flowOpacity->getReal());

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool ProceduralFlowCurtainComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFlowCurtainComponent] Init flow curtain component for game object: " + this->gameObjectPtr->getName());

        this->flowMeshName = "ProceduralFlowCurtainMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }

        return true;
    }

    void ProceduralFlowCurtainComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFlowCurtainComponent] Remove flow curtain component for game object: " + this->gameObjectPtr->getName());

        this->destroyFlowMesh();
    }

    void ProceduralFlowCurtainComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == this->activated->getBool() || nullptr == this->flowItem)
        {
            return;
        }

        this->scrollLayer(this->flowLayer, this->flowSpeed->getReal(), this->flowVTiling->getReal(), dt);

        if (true == this->foamEnabled->getBool())
        {
            this->scrollLayer(this->foamLayer, this->foamSpeed->getReal(), this->foamVTiling->getReal(), dt);
        }
    }

    void ProceduralFlowCurtainComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralFlowCurtainComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralFlowCurtainComponent::AttrWidth() == attribute->getName())
        {
            this->setWidth(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrHeight() == attribute->getName())
        {
            this->setHeight(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrRows() == attribute->getName())
        {
            this->setRows(attribute->getInt());
        }
        else if (ProceduralFlowCurtainComponent::AttrCols() == attribute->getName())
        {
            this->setCols(attribute->getInt());
        }
        else if (ProceduralFlowCurtainComponent::AttrRippleAmount() == attribute->getName())
        {
            this->setRippleAmount(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrHorizontalUVTiling() == attribute->getName())
        {
            this->setHorizontalUVTiling(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowDatablock() == attribute->getName())
        {
            this->setFlowDatablock(attribute->getString());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowSpeed() == attribute->getName())
        {
            this->setFlowSpeed(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowVTiling() == attribute->getName())
        {
            this->setFlowVTiling(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFoamEnabled() == attribute->getName())
        {
            this->setFoamEnabled(attribute->getBool());
        }
        else if (ProceduralFlowCurtainComponent::AttrFoamDatablock() == attribute->getName())
        {
            this->setFoamDatablock(attribute->getString());
        }
        else if (ProceduralFlowCurtainComponent::AttrFoamSpeed() == attribute->getName())
        {
            this->setFoamSpeed(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFoamVTiling() == attribute->getName())
        {
            this->setFoamVTiling(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFoamZOffset() == attribute->getName())
        {
            this->setFoamZOffset(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowColourRed() == attribute->getName())
        {
            this->setFlowColourRed(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowColourGreen() == attribute->getName())
        {
            this->setFlowColourGreen(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowColourBlue() == attribute->getName())
        {
            this->setFlowColourBlue(attribute->getReal());
        }
        else if (ProceduralFlowCurtainComponent::AttrFlowOpacity() == attribute->getName())
        {
            this->setFlowOpacity(attribute->getReal());
        }
    }

    void ProceduralFlowCurtainComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int, 6 = real, 7 = string, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrWidth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->width->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrHeight().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->height->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrRows().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rows->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrCols().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cols->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrRippleAmount().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rippleAmount->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrHorizontalUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->horizontalUVTiling->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowSpeed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowSpeed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowVTiling->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFoamEnabled().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamEnabled->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFoamDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFoamSpeed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamSpeed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFoamVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamVTiling->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFoamZOffset().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->foamZOffset->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowColourRed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowColourRed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowColourGreen().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowColourGreen->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowColourBlue().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowColourBlue->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralFlowCurtainComponent::AttrFlowOpacity().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowOpacity->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================================
    // Mesh generation
    // =========================================================================================

    void ProceduralFlowCurtainComponent::addLayerQuad(FlowLayer& layer, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normalHint, Ogre::Real u0, Ogre::Real u1,
        Ogre::Real baseV0, Ogre::Real baseV1)
    {
        // The actual per-quad normal is computed from the (possibly ripple-displaced) positions
        // themselves, not assumed constant - so lighting follows the ripple curve automatically.
        // normalHint only decides which of the two possible perpendicular directions is "front"
        // and therefore which winding to emit - same self-correcting idea as
        // ProceduralConveyorLoopComponent::addBeltQuad, applied one level further since there the
        // normal itself was already known-correct and passed in directly.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v3 - v0;
        Ogre::Vector3 rawNormal = edge1.crossProduct(edge2);

        Ogre::Vector3 normal;
        bool flip = false;
        if (rawNormal.squaredLength() < 1e-12f)
        {
            // Degenerate quad (zero width or height) - fall back to the hint rather than divide
            // by zero normalising it.
            normal = normalHint;
        }
        else
        {
            rawNormal.normalise();
            flip = rawNormal.dotProduct(normalHint) < 0.0f;
            normal = (false == flip) ? rawNormal : -rawNormal;
        }

        Ogre::Vector3 tangent = edge1;
        if (tangent.squaredLength() < 0.0001f)
        {
            tangent = edge2;
        }
        tangent.normalise();

        const Ogre::Vector3 positions[4] = {v0, v1, v2, v3};
        const Ogre::Real us[4] = {u0, u1, u1, u0};
        const Ogre::Real baseVs[4] = {baseV0, baseV0, baseV1, baseV1};

        for (int i = 0; i < 4; ++i)
        {
            layer.vertices.push_back(positions[i].x);
            layer.vertices.push_back(positions[i].y);
            layer.vertices.push_back(positions[i].z);
            layer.vertices.push_back(normal.x);
            layer.vertices.push_back(normal.y);
            layer.vertices.push_back(normal.z);
            layer.vertices.push_back(tangent.x);
            layer.vertices.push_back(tangent.y);
            layer.vertices.push_back(tangent.z);
            layer.vertices.push_back(1.0f); // handedness
            layer.vertices.push_back(static_cast<float>(us[i]));
            // Written once here as baseV + the layer's CURRENT scrollOffset (usually still 0 at
            // build time), so the very first rendered frame - before update()/scrollLayer() has
            // run even once - already shows the correct UV instead of a one-frame pop back to an
            // un-scrolled state.
            layer.vertices.push_back(static_cast<float>(baseVs[i] + layer.scrollOffset));

            layer.vertexBaseV.push_back(static_cast<float>(baseVs[i]));
        }

        if (false == flip)
        {
            layer.indices.push_back(layer.currentVertexIndex + 0);
            layer.indices.push_back(layer.currentVertexIndex + 1);
            layer.indices.push_back(layer.currentVertexIndex + 2);
            layer.indices.push_back(layer.currentVertexIndex + 0);
            layer.indices.push_back(layer.currentVertexIndex + 2);
            layer.indices.push_back(layer.currentVertexIndex + 3);
        }
        else
        {
            layer.indices.push_back(layer.currentVertexIndex + 0);
            layer.indices.push_back(layer.currentVertexIndex + 2);
            layer.indices.push_back(layer.currentVertexIndex + 1);
            layer.indices.push_back(layer.currentVertexIndex + 0);
            layer.indices.push_back(layer.currentVertexIndex + 3);
            layer.indices.push_back(layer.currentVertexIndex + 2);
        }

        layer.currentVertexIndex += 4;
    }

    void ProceduralFlowCurtainComponent::buildLayerGrid(FlowLayer& layer, Ogre::Real zOffset, Ogre::Real vTiling, unsigned int layerSeedSalt)
    {
        layer.clear();

        const int numRows = std::max(1, this->rows->getInt());
        const int numCols = std::max(1, this->cols->getInt());
        const Ogre::Real w = this->width->getReal();
        const Ogre::Real h = this->height->getReal();
        const Ogre::Real halfWidth = w * 0.5f;
        const Ogre::Real ripple = this->rippleAmount->getReal();
        const Ogre::Real uTiling = this->horizontalUVTiling->getReal();

        // Per-instance (and per-layer, via layerSeedSalt) random phases - deterministic from the
        // game object's own id, so this curtain always rebuilds identically, but two different
        // curtains (or the flow vs. foam layer of the SAME curtain) get different phases instead
        // of rippling/swaying in obvious lockstep. See flowHashNoise()'s comment above.
        const unsigned int instanceSeed = (nullptr != this->gameObjectPtr ? this->gameObjectPtr->getId() : 0u) + layerSeedSalt;
        const Ogre::Real phasePrimary = flowHashNoise(instanceSeed * 2654435761u + 1u) * (2.0f * Ogre::Math::PI);
        const Ogre::Real phaseSecondary = flowHashNoise(instanceSeed * 2654435761u + 2u) * (2.0f * Ogre::Math::PI);
        const Ogre::Real phaseTwist = flowHashNoise(instanceSeed * 2654435761u + 3u) * (2.0f * Ogre::Math::PI);
        const Ogre::Real phaseSway = flowHashNoise(instanceSeed * 2654435761u + 4u) * (2.0f * Ogre::Math::PI);

        // Fixed (non-animated) Z displacement, now a function of BOTH xFrac and yFrac instead of
        // xFrac alone. The old xFrac-only version produced the exact same curve at every height -
        // literally the same profile extruded straight down - which is what read as "an exactly
        // downward-facing plane" no matter how strong 'Ripple Amount' was turned up. See the
        // class comment's RIPPLE section for the full reasoning.
        auto rippleZ = [ripple, phasePrimary, phaseSecondary, phaseTwist](Ogre::Real xFrac, Ogre::Real yFrac) -> Ogre::Real
        {
            // The primary wave's own phase slowly twists as height increases, so no two rows ever
            // share the exact same bulge shape.
            const Ogre::Real twist = FLOW_RIPPLE_VERTICAL_TWIST_STRENGTH * Ogre::Math::Sin(Ogre::Radian((2.0f * Ogre::Math::PI) * FLOW_RIPPLE_VERTICAL_TWIST_COUNT * yFrac + phaseTwist));
            const Ogre::Real primary = Ogre::Math::Sin(Ogre::Radian((2.0f * Ogre::Math::PI) * FLOW_RIPPLE_WAVE_COUNT * xFrac + twist + phasePrimary));
            // Higher-frequency, smaller-amplitude term also drifting with height, on top of the
            // primary one - the multi-octave composition that actually reads as chaotic
            // turbulence rather than a single clean wave.
            const Ogre::Real secondary = FLOW_RIPPLE_SECONDARY_STRENGTH * Ogre::Math::Sin(Ogre::Radian((2.0f * Ogre::Math::PI) * FLOW_RIPPLE_WAVE_COUNT_2 * xFrac + (2.0f * Ogre::Math::PI) * 1.7f * yFrac + phaseSecondary));
            return ripple * (primary + secondary);
        };

        // Whole-ROW sideways (X) sway, layered on top of the Z ripple - shifts an entire row left/
        // right together, breaking the silhouette out of being a perfect rectangle. Depends only
        // on yFrac (not xFrac), so it never distorts a single quad, only offsets it - safe to add
        // straight onto x0/x1 without affecting winding or the normal computed from the resulting
        // positions in addLayerQuad().
        auto swayX = [ripple, phaseSway](Ogre::Real yFrac) -> Ogre::Real
        {
            return ripple * FLOW_SWAY_STRENGTH * Ogre::Math::Sin(Ogre::Radian((2.0f * Ogre::Math::PI) * FLOW_SWAY_WAVE_COUNT * yFrac + phaseSway));
        };

        // Per-column V stagger, baked into that column's base V once here (not animated) - gives
        // each vertical strip of the curtain a fixed offset into the scrolling texture instead of
        // every column showing the exact same texture row at the exact same time, which is a
        // large part of what made the scroll itself look like a single mechanically uniform sheet
        // moving down rather than a wide, turbulent flow. +-0.3 of one texture repeat, per column,
        // deterministic per instance/layer/column.
        auto columnVStagger = [instanceSeed](int c) -> Ogre::Real
        {
            return (flowHashNoise(instanceSeed * 2654435761u + 1000u + static_cast<unsigned int>(c) * 97u) - 0.5f) * 0.6f;
        };

        // The curtain's front face looks toward +Z; the foam layer (built with a positive
        // zOffset) is just the same sheet moved forward, so it shares the same hint.
        const Ogre::Vector3 normalHint(0.0f, 0.0f, 1.0f);

        for (int r = 0; r < numRows; ++r)
        {
            const Ogre::Real yFrac0 = static_cast<Ogre::Real>(r) / static_cast<Ogre::Real>(numRows);
            const Ogre::Real yFrac1 = static_cast<Ogre::Real>(r + 1) / static_cast<Ogre::Real>(numRows);
            const Ogre::Real y0 = yFrac0 * h;
            const Ogre::Real y1 = yFrac1 * h;
            const Ogre::Real baseV0 = yFrac0 * h * vTiling;
            const Ogre::Real baseV1 = yFrac1 * h * vTiling;
            const Ogre::Real sway0 = swayX(yFrac0);
            const Ogre::Real sway1 = swayX(yFrac1);

            for (int c = 0; c < numCols; ++c)
            {
                const Ogre::Real xFrac0 = static_cast<Ogre::Real>(c) / static_cast<Ogre::Real>(numCols);
                const Ogre::Real xFrac1 = static_cast<Ogre::Real>(c + 1) / static_cast<Ogre::Real>(numCols);
                const Ogre::Real x0 = -halfWidth + xFrac0 * w;
                const Ogre::Real x1 = -halfWidth + xFrac1 * w;
                const Ogre::Real u0 = xFrac0 * w * uTiling;
                const Ogre::Real u1 = xFrac1 * w * uTiling;
                const Ogre::Real vStagger = columnVStagger(c);

                const Ogre::Vector3 v0(x0 + sway0, y0, zOffset + rippleZ(xFrac0, yFrac0));
                const Ogre::Vector3 v1(x1 + sway0, y0, zOffset + rippleZ(xFrac1, yFrac0));
                const Ogre::Vector3 v2(x1 + sway1, y1, zOffset + rippleZ(xFrac1, yFrac1));
                const Ogre::Vector3 v3(x0 + sway1, y1, zOffset + rippleZ(xFrac0, yFrac1));

                this->addLayerQuad(layer, v0, v1, v2, v3, normalHint, u0, u1, baseV0 + vStagger, baseV1 + vStagger);
            }
        }
    }

    void ProceduralFlowCurtainComponent::rebuildMesh(void)
    {
        // Different, arbitrary salts per layer (see buildLayerGrid()'s doc comment) - just needs
        // to be non-zero and different between the two calls so flow and foam don't end up with
        // identical random ripple/sway/stagger phases despite sharing the same game object id.
        this->buildLayerGrid(this->flowLayer, 0.0f, this->flowVTiling->getReal(), 0u);

        if (true == this->foamEnabled->getBool())
        {
            this->buildLayerGrid(this->foamLayer, this->foamZOffset->getReal(), this->foamVTiling->getReal(), 1000u);
        }
        else
        {
            this->foamLayer.clear();
        }

        this->createFlowMesh();
    }

    void ProceduralFlowCurtainComponent::createFlowMesh(void)
    {
        if (0 == this->flowLayer.currentVertexIndex)
        {
            this->destroyFlowMesh();
            return;
        }

        std::vector<float> flowVerticesCopy = this->flowLayer.vertices;
        std::vector<Ogre::uint32> flowIndicesCopy = this->flowLayer.indices;
        const size_t numFlowVertices = this->flowLayer.currentVertexIndex;

        std::vector<float> foamVerticesCopy = this->foamLayer.vertices;
        std::vector<Ogre::uint32> foamIndicesCopy = this->foamLayer.indices;
        const size_t numFoamVertices = this->foamLayer.currentVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, flowVerticesCopy, flowIndicesCopy, numFlowVertices, foamVerticesCopy, foamIndicesCopy, numFoamVertices]()
        {
            this->createFlowMeshInternal(flowVerticesCopy, flowIndicesCopy, numFlowVertices, foamVerticesCopy, foamIndicesCopy, numFoamVertices);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralFlowCurtainComponent::createFlowMesh");

        // Deliberately no updatePhysicsCollision() call here, unlike ProceduralConveyorLoopComponent -
        // this component never creates collision. See the class comment.
    }

    void ProceduralFlowCurtainComponent::createFlowMeshInternal(const std::vector<float>& flowVerts, const std::vector<Ogre::uint32>& flowInds, size_t numFlowVerts, const std::vector<float>& foamVerts, const std::vector<Ogre::uint32>& foamInds,
        size_t numFoamVerts)
    {
        //  RUNS ON RENDER THREAD!
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        if (nullptr != this->flowItem)
        {
            this->gameObjectPtr->getSceneNode()->detachObject(this->flowItem);
            sceneManager->destroyItem(this->flowItem);
            this->flowItem = nullptr;
            this->gameObjectPtr->nullMovableObject();
        }
        // The item owned the mesh's VAOs, which owned the previous dynamic vertex buffers - once
        // the item above is gone, those pointers are dangling and must not be reused.
        this->flowLayer.dynamicVertexBuffer = nullptr;
        this->foamLayer.dynamicVertexBuffer = nullptr;

        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->flowMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(this->flowMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);
        mesh->_setVaoManager(vaoManager);

        const size_t floatsPerVertex = 12u;

        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        Ogre::Vector3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Ogre::Vector3 aabbMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        // Both submeshes are BT_DYNAMIC_DEFAULT - unlike ProceduralConveyorLoopComponent's static
        // end caps, BOTH layers here scroll every frame, so there is no immutable submesh to
        // spare. Local lambda instead of duplicating this block twice by hand.
        auto createLayerSubmesh = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts, FlowLayer& layer) -> bool
        {
            if (0 == numVerts)
            {
                return false;
            }

            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numVerts * floatsPerVertex * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(vertexData, verts.data(), numVerts * floatsPerVertex * sizeof(float));

            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(inds.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, inds.data(), inds.size() * sizeof(Ogre::uint32));

            for (size_t vi = 0u; vi < numVerts; ++vi)
            {
                const Ogre::Vector3 position(verts[vi * floatsPerVertex + 0], verts[vi * floatsPerVertex + 1], verts[vi * floatsPerVertex + 2]);
                aabbMin.makeFloor(position);
                aabbMax.makeCeil(position);
            }

            Ogre::VertexBufferPacked* vertexBuffer = nullptr;
            Ogre::IndexBufferPacked* indexBuffer = nullptr;

            try
            {
                vertexBuffer = vaoManager->createVertexBuffer(elements, numVerts, Ogre::BT_DYNAMIC_DEFAULT, vertexData, false);
                indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, inds.size(), Ogre::BT_IMMUTABLE, indexData, true);
            }
            catch (const Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFlowCurtainComponent] Failed to create layer buffers: " + e.getDescription());
                return false;
            }

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);
            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = mesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            layer.dynamicVertexBuffer = vertexBuffer;
            return true;
        };

        // Submesh 0 = flow layer, submesh 1 = foam layer (only if it has vertices) - matching the
        // exact createSubMesh() call order above and the datablock assignment order below.
        const bool hasFlow = createLayerSubmesh(flowVerts, flowInds, numFlowVerts, this->flowLayer);
        const bool hasFoam = createLayerSubmesh(foamVerts, foamInds, numFoamVerts, this->foamLayer);

        if (false == hasFlow)
        {
            // Nothing to show at all - not even the main layer had vertices.
            return;
        }

        if (aabbMin.x > aabbMax.x)
        {
            aabbMin = aabbMax = Ogre::Vector3::ZERO;
        }

        Ogre::Aabb bounds;
        bounds.setExtents(aabbMin, aabbMax);
        mesh->_setBounds(bounds, false);
        mesh->_setBoundingSphereRadius(bounds.getRadius());

        if (false == mesh->hasValidShadowMappingVaos())
        {
            mesh->prepareForShadowMapping(true);
        }

        this->flowItem = sceneManager->createItem(mesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->flowItem->setName("ProceduralFlowCurtainItem_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
        this->flowItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->flowItem->setQueryFlags(this->gameObjectPtr->getCategoryId());
        // A falling curtain of water/lava casting a shadow strip usually looks wrong - off by
        // default, unlike the conveyor belt (a solid surface, which keeps shadows on).
        this->flowItem->setCastShadows(false);

        if (this->flowItem->getNumSubItems() > 0u)
        {
            const Ogre::String flowDbName = this->flowDatablock->getString();
            if (false == flowDbName.empty())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(flowDbName);
                if (nullptr != db)
                {
                    this->flowItem->getSubItem(0u)->setDatablock(db);
                }
            }
        }
        // Immediately re-tints submesh 0 with a per-object clone, from whatever 'Flow Colour
        // Red/Green/Blue'/'Flow Opacity' are currently set to - overwrites the plain base
        // datablock assignment just above with the tinted clone. Calls the Internal variant
        // DIRECTLY, with no further thread dispatch - this whole function already runs on the
        // render thread (see createFlowMesh()'s enqueueAndWait), so routing through
        // applyFlowColour()'s own enqueueAndWait here would wait on the render thread from the
        // render thread itself and deadlock.
        this->applyFlowColourInternal();

        if (true == hasFoam && this->flowItem->getNumSubItems() > 1u)
        {
            const Ogre::String foamDbName = this->foamDatablock->getString();
            if (false == foamDbName.empty())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(foamDbName);
                if (nullptr != db)
                {
                    this->flowItem->getSubItem(1u)->setDatablock(db);
                }
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->flowItem);

        // Registers this item as the game object's own movable object - without this the object
        // cannot be selected in the editor, because its bounding box never gets picked up. Same
        // fix every procedural mesh component in this project has needed.
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->flowItem);

        if (false == this->gameObjectPtr->isDynamic())
        {
            sceneManager->notifyStaticAabbDirty(this->flowItem);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFlowCurtainComponent] Flow curtain mesh created: " + Ogre::StringConverter::toString(static_cast<unsigned int>(numFlowVerts)) + " flow vertices, " +
                                                                               Ogre::StringConverter::toString(static_cast<unsigned int>(numFoamVerts)) + " foam vertices.");
    }

    void ProceduralFlowCurtainComponent::destroyFlowMesh(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // ORDER MATTERS: destroy/detach the ITEM (which unlinks its subitems from whatever
            // datablocks they point at, including our own colour clone) BEFORE destroying the
            // colour clone datablock itself. Destroying a datablock while a SubItem is still
            // linked to it crashes inside ~HlmsDatablock() - that used to happen here because the
            // clone was destroyed first, while flowItem's subitem 0 was still pointing at it. See
            // applyFlowColourInternal()'s comment for the same ordering requirement and the
            // matching pattern already used elsewhere in this project by
            // DatablockPbsComponent::onRemoveComponent() / GameObjectController::
            // tryDestroyDatablockIfUnused() (reassign/detach the renderable first, destroy the
            // datablock only once nothing points at it any more).
            if (nullptr != this->flowItem)
            {
                Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

                if (nullptr != this->gameObjectPtr->getSceneNode())
                {
                    this->gameObjectPtr->getSceneNode()->detachObject(this->flowItem);
                }
                sceneManager->destroyItem(this->flowItem);
                this->flowItem = nullptr;
                this->flowLayer.dynamicVertexBuffer = nullptr;
                this->foamLayer.dynamicVertexBuffer = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }

            // Now safe: nothing references the per-object tinted clone (if applyFlowColour() ever
            // created one) any more - without this cleanup entirely, every curtain that used Flow
            // Colour/Opacity would leak one Hlms datablock each time it is removed or its mesh
            // rebuilt from scratch. getLinkedRenderables().empty() is a second line of defence,
            // not the actual fix - the real fix is the reordering above; this just turns any
            // future ordering mistake into a log line instead of a crash.
            if (false == this->flowColourDatablockName.empty())
            {
                Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
                Ogre::HlmsDatablock* clone = hlmsManager->getDatablockNoDefault(this->flowColourDatablockName);
                if (nullptr != clone && nullptr != clone->getCreator())
                {
                    if (true == clone->getLinkedRenderables().empty())
                    {
                        clone->getCreator()->destroyDatablock(this->flowColourDatablockName);
                    }
                    else
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                            "[ProceduralFlowCurtainComponent] Not destroying colour clone datablock '" + this->flowColourDatablockName + "' - still linked to a renderable (this should not happen; investigate call order).");
                    }
                }
                this->flowColourDatablockName.clear();
            }

            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->flowMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralFlowCurtainComponent::destroyFlowMesh");
    }

    void ProceduralFlowCurtainComponent::applyFlowColourInternal(void)
    {
        //  RUNS ON RENDER THREAD! Never call this directly except from createFlowMeshInternal()
        //  (already on the render thread) or from applyFlowColour()'s own render command below.
        if (nullptr == this->flowItem || 0u == this->flowItem->getNumSubItems())
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        // Re-resolved from the CURRENT 'Flow Datablock' every call, not from any previous clone -
        // so this also does the right thing right after setFlowDatablock() switched to a
        // different base material.
        const Ogre::String baseDbName = this->flowDatablock->getString();
        Ogre::HlmsDatablock* baseDb = hlmsManager->getDatablockNoDefault(baseDbName);
        if (nullptr == baseDb || nullptr == baseDb->getCreator())
        {
            return;
        }

        // Remember the PREVIOUS clone's name, but do NOT touch it yet - it may still be linked to
        // flowItem's subitem 0 at this point (every call after the very first one). Destroying it
        // now, before the subitem is repointed at the new clone below, is exactly what used to
        // crash inside ~HlmsDatablock(). See this method's own class-comment note in the header.
        const Ogre::String previousCloneName = this->flowColourDatablockName;

        // A fresh, uniquely-named clone every call (never re-using the previous call's name) -
        // required so the new clone can be created and assigned BEFORE the old one is destroyed,
        // without Hlms::clone() colliding with a name that is still in use. Counter is a plain
        // member, incremented unconditionally.
        const Ogre::String cloneName = baseDbName + "_Colour_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->flowColourCloneCounter++);

        // UNVERIFIED: HlmsDatablock::clone()/Hlms::destroyDatablock() and
        // HlmsUnlitDatablock::setUseColour()/setColour() are the standard Ogre-Next Hlms names for
        // this - inferred from the "useColour" key already seen in this project's own Hlms JSON
        // files, not confirmed against this engine's actual HlmsUnlitDatablock.h. If this does not
        // compile, that header is the one file to check for the real method names/signatures.
        Ogre::HlmsDatablock* clonedDb = baseDb->clone(cloneName);
        Ogre::HlmsUnlitDatablock* unlitClone = dynamic_cast<Ogre::HlmsUnlitDatablock*>(clonedDb);
        if (nullptr != unlitClone)
        {
            const Ogre::ColourValue colour(this->flowColourRed->getReal(), this->flowColourGreen->getReal(), this->flowColourBlue->getReal(), this->flowOpacity->getReal());
            unlitClone->setUseColour(true);
            unlitClone->setColour(colour);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFlowCurtainComponent] 'Flow Datablock' (" + baseDbName + ") is not an Unlit datablock - Flow Colour/Opacity cannot be applied to it.");
        }

        // Point the subitem at the NEW clone FIRST - this is what unlinks flowItem from the OLD
        // clone, making it safe to destroy afterwards.
        this->flowColourDatablockName = cloneName;
        this->flowItem->getSubItem(0u)->setDatablock(clonedDb);

        // Only now, with nothing linked to it any more, drop the previous clone (empty on the
        // very first call for this game object - nothing to drop yet).
        if (false == previousCloneName.empty())
        {
            Ogre::HlmsDatablock* oldClone = hlmsManager->getDatablockNoDefault(previousCloneName);
            if (nullptr != oldClone && nullptr != oldClone->getCreator())
            {
                if (true == oldClone->getLinkedRenderables().empty())
                {
                    oldClone->getCreator()->destroyDatablock(previousCloneName);
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[ProceduralFlowCurtainComponent] Not destroying previous colour clone datablock '" + previousCloneName + "' - still linked to a renderable (this should not happen; investigate call order).");
                }
            }
        }
    }

    void ProceduralFlowCurtainComponent::applyFlowColour(void)
    {
        // Logic-thread entry point - dispatches applyFlowColourInternal() onto the render thread,
        // exactly the same enqueueAndWait pattern createFlowMesh()/destroyFlowMesh() already use
        // for their own Ogre work. Every colour/opacity setter calls THIS, never
        // applyFlowColourInternal() directly - calling the Internal version straight from a
        // setter is exactly what crashed: Ogre::Hlms/Ogre::Item must only be touched on the
        // render thread.
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyFlowColourInternal();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralFlowCurtainComponent::applyFlowColour");
    }

    void ProceduralFlowCurtainComponent::scrollLayer(FlowLayer& layer, Ogre::Real speed, Ogre::Real vTiling, Ogre::Real dt)
    {
        if (true == layer.vertices.empty() || nullptr == layer.dynamicVertexBuffer)
        {
            return;
        }

        // Advance in the SAME repeats-per-meter units vertexBaseV is already expressed in, then
        // wrap against a single texture repeat. A tileable texture looks identical at every
        // integer repeat boundary, so this wrap can never produce a visible pop - unlike
        // ProceduralConveyorLoopComponent's belt, there is no closed loop here whose own total
        // perimeter the wrap would need to land on exactly (see the class comment).
        layer.scrollOffset = std::fmod(layer.scrollOffset + speed * vTiling * dt, 1.0f);
        if (layer.scrollOffset < 0.0f)
        {
            layer.scrollOffset += 1.0f;
        }

        const size_t floatsPerVertex = 12u;
        const size_t vOffsetWithinVertex = 11u;

        for (size_t i = 0; i < layer.vertexBaseV.size(); ++i)
        {
            layer.vertices[i * floatsPerVertex + vOffsetWithinVertex] = layer.vertexBaseV[i] + static_cast<float>(layer.scrollOffset);
        }

        // Captures the target buffer POINTER by value rather than 'this' - this runs once per
        // frame per active layer, and the component could in principle be removed between this
        // enqueue and the render thread picking it up; capturing 'this' would then dereference a
        // dangling component. The buffer pointer itself stays valid until destroyFlowMesh()/
        // createFlowMeshInternal() replace it, both of which run as their own render commands
        // sequenced through the same queue.
        std::vector<float> verticesCopy = layer.vertices;
        const size_t vertexCount = layer.currentVertexIndex;
        Ogre::VertexBufferPacked* targetBuffer = layer.dynamicVertexBuffer;

        GraphicsModule::RenderCommand renderCommand = [targetBuffer, verticesCopy, vertexCount]()
        {
            if (nullptr != targetBuffer)
            {
                targetBuffer->upload(verticesCopy.data(), 0, vertexCount);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralFlowCurtainComponent::scrollLayer");
    }

    // =========================================================================================
    // Setters and getters
    // =========================================================================================

    void ProceduralFlowCurtainComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyFlowMesh();
        }
    }

    bool ProceduralFlowCurtainComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralFlowCurtainComponent::setWidth(Ogre::Real width)
    {
        this->width->setValue(Ogre::Math::Clamp(width, 0.2f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getWidth(void) const
    {
        return this->width->getReal();
    }

    void ProceduralFlowCurtainComponent::setHeight(Ogre::Real height)
    {
        this->height->setValue(Ogre::Math::Clamp(height, 0.2f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getHeight(void) const
    {
        return this->height->getReal();
    }

    void ProceduralFlowCurtainComponent::setRows(int rows)
    {
        this->rows->setValue(Ogre::Math::Clamp(rows, 1, 200));
        this->rebuildMesh();
    }

    int ProceduralFlowCurtainComponent::getRows(void) const
    {
        return this->rows->getInt();
    }

    void ProceduralFlowCurtainComponent::setCols(int cols)
    {
        this->cols->setValue(Ogre::Math::Clamp(cols, 1, 200));
        this->rebuildMesh();
    }

    int ProceduralFlowCurtainComponent::getCols(void) const
    {
        return this->cols->getInt();
    }

    void ProceduralFlowCurtainComponent::setRippleAmount(Ogre::Real rippleAmount)
    {
        this->rippleAmount->setValue(Ogre::Math::Clamp(rippleAmount, 0.0f, 10.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getRippleAmount(void) const
    {
        return this->rippleAmount->getReal();
    }

    void ProceduralFlowCurtainComponent::setHorizontalUVTiling(Ogre::Real horizontalUVTiling)
    {
        this->horizontalUVTiling->setValue(horizontalUVTiling);
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getHorizontalUVTiling(void) const
    {
        return this->horizontalUVTiling->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowDatablock(const Ogre::String& flowDatablock)
    {
        this->flowDatablock->setValue(flowDatablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralFlowCurtainComponent::getFlowDatablock(void) const
    {
        return this->flowDatablock->getString();
    }

    void ProceduralFlowCurtainComponent::setFlowSpeed(Ogre::Real flowSpeed)
    {
        this->flowSpeed->setValue(flowSpeed);
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowSpeed(void) const
    {
        return this->flowSpeed->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowVTiling(Ogre::Real flowVTiling)
    {
        this->flowVTiling->setValue(flowVTiling);
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowVTiling(void) const
    {
        return this->flowVTiling->getReal();
    }

    void ProceduralFlowCurtainComponent::setFoamEnabled(bool foamEnabled)
    {
        this->foamEnabled->setValue(foamEnabled);
        this->rebuildMesh();
    }

    bool ProceduralFlowCurtainComponent::getFoamEnabled(void) const
    {
        return this->foamEnabled->getBool();
    }

    void ProceduralFlowCurtainComponent::setFoamDatablock(const Ogre::String& foamDatablock)
    {
        this->foamDatablock->setValue(foamDatablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralFlowCurtainComponent::getFoamDatablock(void) const
    {
        return this->foamDatablock->getString();
    }

    void ProceduralFlowCurtainComponent::setFoamSpeed(Ogre::Real foamSpeed)
    {
        this->foamSpeed->setValue(foamSpeed);
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFoamSpeed(void) const
    {
        return this->foamSpeed->getReal();
    }

    void ProceduralFlowCurtainComponent::setFoamVTiling(Ogre::Real foamVTiling)
    {
        this->foamVTiling->setValue(foamVTiling);
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFoamVTiling(void) const
    {
        return this->foamVTiling->getReal();
    }

    void ProceduralFlowCurtainComponent::setFoamZOffset(Ogre::Real foamZOffset)
    {
        this->foamZOffset->setValue(Ogre::Math::Clamp(foamZOffset, -10.0f, 10.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFoamZOffset(void) const
    {
        return this->foamZOffset->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowColourRed(Ogre::Real red)
    {
        this->flowColourRed->setValue(Ogre::Math::Clamp(red, 0.0f, 1.0f));
        this->applyFlowColour();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowColourRed(void) const
    {
        return this->flowColourRed->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowColourGreen(Ogre::Real green)
    {
        this->flowColourGreen->setValue(Ogre::Math::Clamp(green, 0.0f, 1.0f));
        this->applyFlowColour();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowColourGreen(void) const
    {
        return this->flowColourGreen->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowColourBlue(Ogre::Real blue)
    {
        this->flowColourBlue->setValue(Ogre::Math::Clamp(blue, 0.0f, 1.0f));
        this->applyFlowColour();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowColourBlue(void) const
    {
        return this->flowColourBlue->getReal();
    }

    void ProceduralFlowCurtainComponent::setFlowOpacity(Ogre::Real opacity)
    {
        this->flowOpacity->setValue(Ogre::Math::Clamp(opacity, 0.0f, 1.0f));
        this->applyFlowColour();
    }

    Ogre::Real ProceduralFlowCurtainComponent::getFlowOpacity(void) const
    {
        return this->flowOpacity->getReal();
    }

    // =========================================================================================
    // Lua API
    // =========================================================================================

    ProceduralFlowCurtainComponent* getProceduralFlowCurtainComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralFlowCurtainComponent>(gameObject->getComponentWithOccurrence<ProceduralFlowCurtainComponent>(occurrenceIndex)).get();
    }

    ProceduralFlowCurtainComponent* getProceduralFlowCurtainComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralFlowCurtainComponent>(gameObject->getComponent<ProceduralFlowCurtainComponent>()).get();
    }

    ProceduralFlowCurtainComponent* getProceduralFlowCurtainComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralFlowCurtainComponent>(gameObject->getComponentFromName<ProceduralFlowCurtainComponent>(name)).get();
    }

    void ProceduralFlowCurtainComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<ProceduralFlowCurtainComponent, GameObjectComponent>("ProceduralFlowCurtainComponent")
                .def("setActivated", &ProceduralFlowCurtainComponent::setActivated)
                .def("isActivated", &ProceduralFlowCurtainComponent::isActivated)
                .def("setWidth", &ProceduralFlowCurtainComponent::setWidth)
                .def("getWidth", &ProceduralFlowCurtainComponent::getWidth)
                .def("setHeight", &ProceduralFlowCurtainComponent::setHeight)
                .def("getHeight", &ProceduralFlowCurtainComponent::getHeight)
                .def("setRows", &ProceduralFlowCurtainComponent::setRows)
                .def("getRows", &ProceduralFlowCurtainComponent::getRows)
                .def("setCols", &ProceduralFlowCurtainComponent::setCols)
                .def("getCols", &ProceduralFlowCurtainComponent::getCols)
                .def("setRippleAmount", &ProceduralFlowCurtainComponent::setRippleAmount)
                .def("getRippleAmount", &ProceduralFlowCurtainComponent::getRippleAmount)
                .def("setHorizontalUVTiling", &ProceduralFlowCurtainComponent::setHorizontalUVTiling)
                .def("getHorizontalUVTiling", &ProceduralFlowCurtainComponent::getHorizontalUVTiling)
                .def("setFlowDatablock", &ProceduralFlowCurtainComponent::setFlowDatablock)
                .def("getFlowDatablock", &ProceduralFlowCurtainComponent::getFlowDatablock)
                .def("setFlowSpeed", &ProceduralFlowCurtainComponent::setFlowSpeed)
                .def("getFlowSpeed", &ProceduralFlowCurtainComponent::getFlowSpeed)
                .def("setFlowVTiling", &ProceduralFlowCurtainComponent::setFlowVTiling)
                .def("getFlowVTiling", &ProceduralFlowCurtainComponent::getFlowVTiling)
                .def("setFoamEnabled", &ProceduralFlowCurtainComponent::setFoamEnabled)
                .def("getFoamEnabled", &ProceduralFlowCurtainComponent::getFoamEnabled)
                .def("setFoamDatablock", &ProceduralFlowCurtainComponent::setFoamDatablock)
                .def("getFoamDatablock", &ProceduralFlowCurtainComponent::getFoamDatablock)
                .def("setFoamSpeed", &ProceduralFlowCurtainComponent::setFoamSpeed)
                .def("getFoamSpeed", &ProceduralFlowCurtainComponent::getFoamSpeed)
                .def("setFoamVTiling", &ProceduralFlowCurtainComponent::setFoamVTiling)
                .def("getFoamVTiling", &ProceduralFlowCurtainComponent::getFoamVTiling)
                .def("setFoamZOffset", &ProceduralFlowCurtainComponent::setFoamZOffset)
                .def("getFoamZOffset", &ProceduralFlowCurtainComponent::getFoamZOffset)
                .def("setFlowColourRed", &ProceduralFlowCurtainComponent::setFlowColourRed)
                .def("getFlowColourRed", &ProceduralFlowCurtainComponent::getFlowColourRed)
                .def("setFlowColourGreen", &ProceduralFlowCurtainComponent::setFlowColourGreen)
                .def("getFlowColourGreen", &ProceduralFlowCurtainComponent::getFlowColourGreen)
                .def("setFlowColourBlue", &ProceduralFlowCurtainComponent::setFlowColourBlue)
                .def("getFlowColourBlue", &ProceduralFlowCurtainComponent::getFlowColourBlue)
                .def("setFlowOpacity", &ProceduralFlowCurtainComponent::setFlowOpacity)
                .def("getFlowOpacity", &ProceduralFlowCurtainComponent::getFlowOpacity)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "class inherits GameObjectComponent", ProceduralFlowCurtainComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setWidth(float width)", "Sets the curtain's width in meters. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getWidth()", "Gets the curtain's width in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setHeight(float height)", "Sets the curtain's height in meters. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getHeight()", "Gets the curtain's height in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setRows(int rows)", "Sets the vertical grid subdivisions. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "int getRows()", "Gets the vertical grid subdivisions.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setCols(int cols)", "Sets the horizontal grid subdivisions. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "int getCols()", "Gets the horizontal grid subdivisions.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setRippleAmount(float amount)", "Sets the fixed sine bulge across the width, in meters. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getRippleAmount()", "Gets the ripple bulge amount.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setHorizontalUVTiling(float tiling)", "Sets the texture tiling across the width, in repeats per meter. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getHorizontalUVTiling()", "Gets the horizontal texture tiling.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowDatablock(String datablock)", "Sets the datablock covering the main falling surface.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "String getFlowDatablock()", "Gets the flow layer's datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowSpeed(float speed)", "Sets how fast the flow layer's texture scrolls, in meters per second. Negative reverses direction.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowSpeed()", "Gets the flow layer's scroll speed in meters per second.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowVTiling(float tiling)", "Sets the flow layer's texture tiling along the height, in repeats per meter. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowVTiling()", "Gets the flow layer's vertical texture tiling.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFoamEnabled(bool enabled)", "Enables/disables the optional foam layer. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "bool getFoamEnabled()", "Gets whether the foam layer is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFoamDatablock(String datablock)", "Sets the datablock covering the optional foam layer.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "String getFoamDatablock()", "Gets the foam layer's datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFoamSpeed(float speed)", "Sets how fast the foam layer's texture scrolls, in meters per second. Negative reverses direction.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFoamSpeed()", "Gets the foam layer's scroll speed in meters per second.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFoamVTiling(float tiling)", "Sets the foam layer's texture tiling along the height, in repeats per meter. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFoamVTiling()", "Gets the foam layer's vertical texture tiling.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFoamZOffset(float offset)", "Sets how far in front of the flow layer the foam layer sits, in meters. Regenerates the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFoamZOffset()", "Gets the foam layer's Z offset in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowColourRed(float red)", "Sets this instance's flow layer red tint (0-1), via a per-object cloned datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowColourRed()", "Gets this instance's flow layer red tint.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowColourGreen(float green)", "Sets this instance's flow layer green tint (0-1).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowColourGreen()", "Gets this instance's flow layer green tint.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowColourBlue(float blue)", "Sets this instance's flow layer blue tint (0-1).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowColourBlue()", "Gets this instance's flow layer blue tint.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "void setFlowOpacity(float opacity)", "Sets this instance's flow layer opacity (0-1), multiplied with the texture's own alpha.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralFlowCurtainComponent", "float getFlowOpacity()", "Gets this instance's flow layer opacity.");

        gameObjectClass.def("getProceduralFlowCurtainComponentFromName", &getProceduralFlowCurtainComponentFromName);
        gameObjectClass.def("getProceduralFlowCurtainComponent", (ProceduralFlowCurtainComponent * (*)(GameObject*)) & getProceduralFlowCurtainComponent);
        gameObjectClass.def("getProceduralFlowCurtainComponent2", (ProceduralFlowCurtainComponent * (*)(GameObject*, unsigned int)) & getProceduralFlowCurtainComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralFlowCurtainComponent getProceduralFlowCurtainComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralFlowCurtainComponent getProceduralFlowCurtainComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralFlowCurtainComponent getProceduralFlowCurtainComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castProceduralFlowCurtainComponent", &GameObjectController::cast<ProceduralFlowCurtainComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralFlowCurtainComponent castProceduralFlowCurtainComponent(ProceduralFlowCurtainComponent other)",
            "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end