/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralDecorBandComponent.h"
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
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreAbiUtils.h"

// =============================================================================
// ProceduralDecorBandComponent - rock silhouette bands for dressing 2.5D levels.
//
// Much smaller than the other procedural components in this project, and deliberately so.
// There is no path, no mouse editing, no segment mode, no undo blob and no serialized
// geometry: a decoration band is a rectangle filled from its attributes, and the attributes
// plus a seed reproduce it exactly. That removes about four fifths of what
// ProceduralPlatformComponent and ProceduralPipeComponent have to carry.
//
// The one idea worth keeping in mind while reading: ROWS ARE DEPTH. Row 0 sits on the
// GameObject's plane, each further row is pushed back, scaled down and shaded darker. Each row
// becomes its own submesh with its own cloned datablock, and that brightness ramp is what makes
// a band read as a ridge rather than as a flat picket fence.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralDecorBandComponent::ProceduralDecorBandComponent() :
        GameObjectComponent(),
        name("ProceduralDecorBandComponent"),
        activated(new Variant(ProceduralDecorBandComponent::AttrActivated(), true, this->attributes)),
        bandWidth(new Variant(ProceduralDecorBandComponent::AttrBandWidth(), 20.0f, this->attributes)),
        rows(new Variant(ProceduralDecorBandComponent::AttrRows(), 3, this->attributes)),
        columns(new Variant(ProceduralDecorBandComponent::AttrColumns(), 28, this->attributes)),
        rowDepthSpacing(new Variant(ProceduralDecorBandComponent::AttrRowDepthSpacing(), -0.4f, this->attributes)),
        rowScale(new Variant(ProceduralDecorBandComponent::AttrRowScale(), 1.3f, this->attributes)),
        growDirection(new Variant(ProceduralDecorBandComponent::AttrGrowDirection(), std::vector<Ogre::String>{"Up (+Y)", "Down (-Y)", "Towards Camera (-Z)", "Away From Camera (+Z)"}, this->attributes)),
        rockStyle(new Variant(ProceduralDecorBandComponent::AttrRockStyle(), std::vector<Ogre::String>{"Jagged", "Rounded", "Columnar", "Stalagmite"}, this->attributes)),
        rockHeight(new Variant(ProceduralDecorBandComponent::AttrRockHeight(), 2.0f, this->attributes)),
        rockDepth(new Variant(ProceduralDecorBandComponent::AttrRockDepth(), 0.4f, this->attributes)),
        rockWidthScale(new Variant(ProceduralDecorBandComponent::AttrRockWidthScale(), 1.8f, this->attributes)),
        baseFill(new Variant(ProceduralDecorBandComponent::AttrBaseFill(), 0.5f, this->attributes)),
        heightVariation(new Variant(ProceduralDecorBandComponent::AttrHeightVariation(), 0.4f, this->attributes)),
        widthJitter(new Variant(ProceduralDecorBandComponent::AttrWidthJitter(), 0.6f, this->attributes)),
        density(new Variant(ProceduralDecorBandComponent::AttrDensity(), 1.0f, this->attributes)),
        profilePoints(new Variant(ProceduralDecorBandComponent::AttrProfilePoints(), 40, this->attributes)),
        seed(new Variant(ProceduralDecorBandComponent::AttrSeed(), 1337, this->attributes)),
        datablockName(new Variant(ProceduralDecorBandComponent::AttrDatablockName(), Ogre::String("rockClif_D"), this->attributes)),
        frontBrightness(new Variant(ProceduralDecorBandComponent::AttrFrontBrightness(), 0.12f, this->attributes)),
        backBrightness(new Variant(ProceduralDecorBandComponent::AttrBackBrightness(), 0.35f, this->attributes)),
        uvTiling(new Variant(ProceduralDecorBandComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
        castShadows(new Variant(ProceduralDecorBandComponent::AttrCastShadows(), false, this->attributes)),
        decorItem(nullptr),
        decorClonedNeedsRebuild(false)
    {
        this->bandWidth->setDescription("Horizontal extent of the band in meters, centred on this GameObject's node.");
        this->bandWidth->setConstraints(0.1f, 2000.0f);

        this->rows->setDescription("Number of DEPTH rows, not stacked rows. Row 0 sits on this object's own plane and every "
                                   "further row is pushed back by Row Depth Spacing, scaled by Row Scale and shaded towards "
                                   "Back Brightness. Two or three rows is usually enough - the overlap and the brightness ramp "
                                   "do the work, not the count.");
        this->rows->setConstraints(1, 12);

        this->columns->setDescription("How many rock clusters fit across the band. Together with Band Width this sets the cluster "
                                      "size, so a wider band at the same column count gives bigger rocks.");
        this->columns->setConstraints(1, 512);

        this->rowDepthSpacing->setDescription("How far each row sits from the one before it, in meters, along the axis Grow Direction "
                                              "leaves free. MAY BE NEGATIVE: with Grow Direction Up the camera looks along +Z, so a "
                                              "negative value brings the rows TOWARDS the camera, which is what a foreground occluder "
                                              "band in front of the player needs; a positive value pushes them away behind him. Row 0 "
                                              "always stays on this object's own plane, so with a negative spacing row 0 is the REARMOST "
                                              "row - swap Front and Back Brightness in that case.");
        this->rowDepthSpacing->setConstraints(-200.0f, 200.0f);

        this->rowScale->setDescription("Size multiplier applied per row going back, on HEIGHT and DEPTH only. It deliberately does not "
                                       "touch the width: scaling a cluster narrower without also scaling the cell spacing it sits in "
                                       "would open a gap in every row behind the first. Below 1 the rows shrink with distance, which "
                                       "reads as depth even before perspective gets involved; 1.0 keeps them equal.");
        this->rowScale->setConstraints(0.1f, 2.0f);

        this->growDirection->setDescription("Axis the rock height grows along. The band itself always runs along local X.\n"
                                            "Up (+Y): rocks standing on a floor. Rows are then offset along Z.\n"
                                            "Down (-Y): stalactites hanging from a ceiling. Rows are then offset along Z.\n"
                                            "Towards Camera (-Z): a mat growing out of the play plane towards the viewer. Rows along Y.\n"
                                            "Away From Camera (+Z): the same, growing away from the viewer. Rows along Y.");

        this->rockStyle->setDescription("Outline of a cluster.\n"
                                        "Jagged: sharp broken peaks, the default cave look.\n"
                                        "Rounded: weathered humps for sand, moss or old stone.\n"
                                        "Columnar: flat topped steps of differing height - basalt, ruins, crystal.\n"
                                        "Stalagmite: one tall narrow spike per cell, for cave floors and ceilings.");

        this->rockHeight->setDescription("Height of a row 0 cluster in meters, before Height Variation is applied.");
        this->rockHeight->setConstraints(0.05f, 500.0f);

        this->rockDepth->setDescription("Thickness of each cluster, along the axis Grow Direction leaves free. Keep it small for a "
                                        "foreground band: it is a silhouette, and nobody ever sees its sides.");
        this->rockDepth->setConstraints(0.01f, 100.0f);

        this->rockWidthScale->setDescription("Cluster width as a multiple of the cell width. Above 1 the neighbours overlap, which is "
                                             "what removes the regular gaps that give a grid away. 1.35 is a good starting point; "
                                             "1.0 leaves the clusters just touching and the band looks tiled. A cluster is never built "
                                             "narrower than its cell plus the Width Jitter, so no combination of this and the jitter can "
                                             "open a gap.");
        this->rockWidthScale->setConstraints(0.1f, 4.0f);

        this->baseFill->setDescription("Fraction of Rock Height the outline never drops below, 0 to 1. At 0 the Rounded and Stalagmite "
                                       "outlines fall to zero at their edges, so two clusters can overlap horizontally and still meet at "
                                       "zero height - a notch shows through regardless of Rock Width Scale. 0.3 to 0.4 welds the band "
                                       "into one continuous ridge. Set it to 0 for free standing spikes with real sky between them.");
        this->baseFill->setConstraints(0.0f, 1.0f);

        this->heightVariation->setDescription("How much the cluster heights differ, 0 to 1. At 0 every cluster is exactly Rock Height "
                                              "and the band reads as wallpaper however good the single rock is. 0.5 to 0.7 looks natural.");
        this->heightVariation->setConstraints(0.0f, 1.0f);

        this->widthJitter->setDescription("Sideways offset and width variation per cluster, 0 to 1. Breaks the column rhythm.");
        this->widthJitter->setConstraints(0.0f, 1.0f);

        this->density->setDescription("Fraction of cells that actually get a cluster, 0 to 1. Below 1 random gaps appear, which is how "
                                      "a band gets holes to see through instead of being a solid wall.");
        this->density->setConstraints(0.0f, 1.0f);

        this->profilePoints->setDescription("Outline resolution per cluster. 8 to 16 is plenty for a silhouette; higher only adds "
                                            "triangles nobody can see at this brightness.");
        this->profilePoints->setConstraints(2, 64);

        this->seed->setDescription("Picks the layout. The same seed always gives the same band on every machine, which is why none of "
                                   "the geometry is written to the scene file - only these attributes are.");

        this->datablockName->setDescription("PBS datablock for the rocks. It is CLONED once per row so the brightness ramp can be "
                                            "applied without touching the original or anything else using it.");
        this->datablockName->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->frontBrightness->setDescription("Diffuse multiplier for the FIRST row. A foreground occluder between camera and player "
                                              "wants roughly 0.1 - it is meant to be a dark silhouette framing the action, not a rock "
                                              "the player stops to look at.");
        this->frontBrightness->setConstraints(0.0f, 4.0f);

        this->backBrightness->setDescription("Diffuse multiplier for the LAST row, with the rows between interpolated. A ridge behind "
                                             "the play plane wants 0.3 to 0.5. The gap between this and Front Brightness is what the "
                                             "eye reads as depth, so keep them clearly apart.");
        this->backBrightness->setConstraints(0.0f, 4.0f);

        this->uvTiling->setDescription("Texture repeats per meter: x across the band, y up the rock faces.");

        this->castShadows->setDescription("Off by default. A foreground occluder is between the camera and everything else, so its "
                                          "shadow would fall across the whole play area for no gain.");
    }

    ProceduralDecorBandComponent::~ProceduralDecorBandComponent()
    {
    }

    void ProceduralDecorBandComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralDecorBandComponent>(ProceduralDecorBandComponent::getStaticClassId(), ProceduralDecorBandComponent::getStaticClassName());
    }

    void ProceduralDecorBandComponent::initialise()
    {
    }

    void ProceduralDecorBandComponent::shutdown()
    {
    }

    void ProceduralDecorBandComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralDecorBandComponent::getName() const
    {
        return this->name;
    }

    void ProceduralDecorBandComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralDecorBandComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

    bool ProceduralDecorBandComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        // Strictly sequential reader: every property is guarded on its own, so a scene saved
        // before a given attribute existed skips it instead of desynchronising everything
        // after it.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrBandWidth())
        {
            this->bandWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 20.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRows())
        {
            this->rows->setValue(XMLConverter::getAttribInt(propertyElement, "data", 2));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrColumns())
        {
            this->columns->setValue(XMLConverter::getAttribInt(propertyElement, "data", 14));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRowDepthSpacing())
        {
            this->rowDepthSpacing->setValue(XMLConverter::getAttribReal(propertyElement, "data", -1.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRowScale())
        {
            this->rowScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.85f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrGrowDirection())
        {
            this->growDirection->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRockStyle())
        {
            this->rockStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRockHeight())
        {
            this->rockHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRockDepth())
        {
            this->rockDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.8f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrRockWidthScale())
        {
            this->rockWidthScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.35f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrBaseFill())
        {
            this->baseFill->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.35f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrHeightVariation())
        {
            this->heightVariation->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.55f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrWidthJitter())
        {
            this->widthJitter->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.4f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrDensity())
        {
            this->density->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrProfilePoints())
        {
            this->profilePoints->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrSeed())
        {
            this->seed->setValue(XMLConverter::getAttribInt(propertyElement, "data", 1337));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrDatablockName())
        {
            this->datablockName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrFrontBrightness())
        {
            this->frontBrightness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.12f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrBackBrightness())
        {
            this->backBrightness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.35f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrUVTiling())
        {
            this->uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDecorBandComponent::AttrCastShadows())
        {
            this->castShadows->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    void ProceduralDecorBandComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        // Attributes only. The geometry is a pure function of them plus Seed, so writing a
        // single vertex here would only be a way for the file to start disagreeing with what
        // the component generates.
        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrBandWidth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bandWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRows().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rows->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrColumns().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->columns->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRowDepthSpacing().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rowDepthSpacing->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRowScale().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rowScale->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrGrowDirection().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->growDirection->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRockStyle().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rockStyle->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRockHeight().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rockHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRockDepth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rockDepth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrRockWidthScale().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rockWidthScale->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrBaseFill().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->baseFill->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrHeightVariation().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->heightVariation->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrWidthJitter().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->widthJitter->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrDensity().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->density->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrProfilePoints().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->profilePoints->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrSeed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->seed->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrDatablockName().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablockName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrFrontBrightness().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->frontBrightness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrBackBrightness().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backBrightness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralDecorBandComponent::AttrCastShadows().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->castShadows->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    GameObjectCompPtr ProceduralDecorBandComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralDecorBandComponentPtr clonedCompPtr(boost::make_shared<ProceduralDecorBandComponent>());

        clonedCompPtr->setOwner(clonedGameObjectPtr);

        // Values are copied straight onto the Variants rather than through the setters. A
        // setter rebuilds, and rebuilding here would run against a GameObject that has not been
        // initialised yet: createDecorMeshInternal ends with gameObjectPtr->init(item), and the
        // cloned object's own initialisation afterwards would replace exactly that - leaving a
        // clone with correct attributes and no visible mesh, and no error anywhere to explain it.
        // The single build happens in postInit instead.
        clonedCompPtr->bandWidth->setValue(this->bandWidth->getReal());
        clonedCompPtr->rows->setValue(this->rows->getInt());
        clonedCompPtr->columns->setValue(this->columns->getInt());
        clonedCompPtr->rowDepthSpacing->setValue(this->rowDepthSpacing->getReal());
        clonedCompPtr->rowScale->setValue(this->rowScale->getReal());
        clonedCompPtr->growDirection->setListSelectedValue(this->growDirection->getListSelectedValue());
        clonedCompPtr->rockStyle->setListSelectedValue(this->rockStyle->getListSelectedValue());
        clonedCompPtr->rockHeight->setValue(this->rockHeight->getReal());
        clonedCompPtr->rockDepth->setValue(this->rockDepth->getReal());
        clonedCompPtr->rockWidthScale->setValue(this->rockWidthScale->getReal());
        clonedCompPtr->baseFill->setValue(this->baseFill->getReal());
        clonedCompPtr->heightVariation->setValue(this->heightVariation->getReal());
        clonedCompPtr->widthJitter->setValue(this->widthJitter->getReal());
        clonedCompPtr->density->setValue(this->density->getReal());
        clonedCompPtr->profilePoints->setValue(this->profilePoints->getInt());
        clonedCompPtr->seed->setValue(this->seed->getInt());
        clonedCompPtr->datablockName->setValue(this->datablockName->getString());
        clonedCompPtr->frontBrightness->setValue(this->frontBrightness->getReal());
        clonedCompPtr->backBrightness->setValue(this->backBrightness->getReal());
        clonedCompPtr->uvTiling->setValue(this->uvTiling->getVector2());
        clonedCompPtr->castShadows->setValue(this->castShadows->getBool());
        clonedCompPtr->activated->setValue(this->activated->getBool());

        clonedCompPtr->decorClonedNeedsRebuild = true;

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool ProceduralDecorBandComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDecorBandComponent] Init decor band for game object: " + this->gameObjectPtr->getName());

        // Built right here, for a fresh object, a loaded scene and a clone alike. Unlike the
        // path based components there is nothing to wait for: the band depends on its own
        // attributes and nothing else in the scene, so there is no reason to defer it to
        // EventDataSceneParsed.
        this->decorClonedNeedsRebuild = false;

        this->regenerate();

        return true;
    }

    bool ProceduralDecorBandComponent::connect(void)
    {
        return true;
    }

    bool ProceduralDecorBandComponent::disconnect(void)
    {
        return true;
    }

    void ProceduralDecorBandComponent::onRemoveComponent(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDecorBandComponent] Removing decor band for game object: " + this->gameObjectPtr->getName());

        this->destroyDecorMesh();

        // After destroyDecorMesh, so no Item can still reference a clone by the time it goes.
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->destroyClonedDatablocks();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::onRemoveComponent::destroyClonedDatablocks");
        }

        GameObjectComponent::onRemoveComponent();
    }

    void ProceduralDecorBandComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void ProceduralDecorBandComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralDecorBandComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralDecorBandComponent::AttrBandWidth() == attribute->getName())
        {
            this->setBandWidth(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrRows() == attribute->getName())
        {
            this->setRows(attribute->getInt());
        }
        else if (ProceduralDecorBandComponent::AttrColumns() == attribute->getName())
        {
            this->setColumns(attribute->getInt());
        }
        else if (ProceduralDecorBandComponent::AttrRowDepthSpacing() == attribute->getName())
        {
            this->setRowDepthSpacing(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrRowScale() == attribute->getName())
        {
            this->setRowScale(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrGrowDirection() == attribute->getName())
        {
            this->setGrowDirection(attribute->getListSelectedValue());
        }
        else if (ProceduralDecorBandComponent::AttrRockStyle() == attribute->getName())
        {
            this->setRockStyle(attribute->getListSelectedValue());
        }
        else if (ProceduralDecorBandComponent::AttrRockHeight() == attribute->getName())
        {
            this->setRockHeight(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrRockDepth() == attribute->getName())
        {
            this->setRockDepth(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrRockWidthScale() == attribute->getName())
        {
            this->setRockWidthScale(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrBaseFill() == attribute->getName())
        {
            this->setBaseFill(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrHeightVariation() == attribute->getName())
        {
            this->setHeightVariation(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrWidthJitter() == attribute->getName())
        {
            this->setWidthJitter(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrDensity() == attribute->getName())
        {
            this->setDensity(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrProfilePoints() == attribute->getName())
        {
            this->setProfilePoints(attribute->getInt());
        }
        else if (ProceduralDecorBandComponent::AttrSeed() == attribute->getName())
        {
            this->setSeed(attribute->getInt());
        }
        else if (ProceduralDecorBandComponent::AttrDatablockName() == attribute->getName())
        {
            this->setDatablockName(attribute->getString());
        }
        else if (ProceduralDecorBandComponent::AttrFrontBrightness() == attribute->getName())
        {
            this->setFrontBrightness(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrBackBrightness() == attribute->getName())
        {
            this->setBackBrightness(attribute->getReal());
        }
        else if (ProceduralDecorBandComponent::AttrUVTiling() == attribute->getName())
        {
            this->setUVTiling(attribute->getVector2());
        }
        else if (ProceduralDecorBandComponent::AttrCastShadows() == attribute->getName())
        {
            this->setCastShadows(attribute->getBool());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Geometry
    ///////////////////////////////////////////////////////////////////////////////////////////////

    namespace
    {
        // Deterministic hash in 0..1. The same inputs give the same value on every machine and
        // in every run, which is the reason this component stores no geometry at all: a seed
        // plus the attributes reproduce the band exactly, so the scene file never has to.
        //
        // Plain integer mixing rather than std::rand or a distribution, because those depend on
        // library version and on call ORDER - change one loop and every rock moves.
        inline Ogre::Real decorHash(int a, int b, int c)
        {
            uint32_t h = static_cast<uint32_t>(a) * 374761393u + static_cast<uint32_t>(b) * 668265263u + static_cast<uint32_t>(c) * 2246822519u;
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= (h >> 16);

            return static_cast<Ogre::Real>(h & 0x00FFFFFFu) / static_cast<Ogre::Real>(0x01000000u);
        }

        // Smoothstep-interpolated value noise along one axis. Enough for a silhouette; nothing
        // here is ever seen at a brightness where gradient noise would pay for itself.
        inline Ogre::Real decorValueNoise(Ogre::Real t, int laneA, int laneB)
        {
            const int i0 = static_cast<int>(std::floor(t));
            const Ogre::Real f = t - static_cast<Ogre::Real>(i0);

            const Ogre::Real a = decorHash(i0, laneA, laneB);
            const Ogre::Real b = decorHash(i0 + 1, laneA, laneB);

            const Ogre::Real s = f * f * (3.0f - 2.0f * f);

            return a + (b - a) * s;
        }

        // Three octaves is the sweet spot: one gives smooth humps with no detail, five gives
        // detail that vanishes the moment the rock is shaded at 10% brightness.
        inline Ogre::Real decorFbm(Ogre::Real t, int laneA, int laneB)
        {
            Ogre::Real sum = 0.0f;
            Ogre::Real amplitude = 1.0f;
            Ogre::Real frequency = 1.0f;
            Ogre::Real total = 0.0f;

            for (int octave = 0; octave < 3; ++octave)
            {
                sum += decorValueNoise(t * frequency, laneA + octave * 101, laneB);
                total += amplitude;
                amplitude *= 0.5f;
                frequency *= 2.3f;
            }

            return (total > 0.0f) ? (sum / total) * 0.5f : 0.0f;
        }
    } // namespace

    std::vector<Ogre::Real> ProceduralDecorBandComponent::buildRockProfile(int row, int columnIndex, int points) const
    {
        std::vector<Ogre::Real> profile;
        profile.reserve(points);

        const RockStyle style = this->getRockStyleEnum();
        const int baseSeed = this->seed->getInt();

        // Floor under the whole outline. Without it the Rounded and Stalagmite windows fall to
        // zero at u = 0 and u = 1, so two clusters can overlap horizontally and STILL meet at
        // zero height - the band then shows a notch at every cell boundary no matter how wide
        // Rock Width Scale makes the clusters. This is the second of the two independent causes
        // of the gaps; the first is handled by the minimum cluster width in buildGeometry.
        const Ogre::Real fill = Ogre::Math::Clamp(this->baseFill->getReal(), 0.0f, 1.0f);

        // Every cluster gets its own noise lane, so changing one column never shifts another.
        const int laneA = columnIndex * 7919 + row * 104729;

        for (int i = 0; i < points; ++i)
        {
            const Ogre::Real u = (points > 1) ? (static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(points - 1)) : 0.5f;

            Ogre::Real value = 0.0f;

            switch (style)
            {
            case RockStyle::ROUNDED:
            {
                // Weathered humps: the noise is pushed towards its low end so peaks are
                // rare, and a strong window rounds the shoulders off.
                const Ogre::Real n = decorFbm(u * 2.0f, laneA, baseSeed);
                const Ogre::Real window = std::pow(std::sin(Ogre::Math::PI * u), 0.6f);
                value = (0.35f + 0.65f * std::pow(Ogre::Math::Clamp(n * 2.0f, 0.0f, 1.0f), 1.5f)) * window;
                break;
            }
            case RockStyle::COLUMNAR:
            {
                // Flat tops: the noise index is quantised so neighbouring sample points
                // share a height, which is what turns a profile into steps rather than a
                // staircase of single points. Basalt, broken masonry, crystal.
                const int stepIndex = i / 2;
                const Ogre::Real n = decorHash(stepIndex, laneA, baseSeed);
                const Ogre::Real quantised = std::floor(n * 5.0f) / 5.0f;
                value = 0.35f + 0.65f * quantised;
                break;
            }
            case RockStyle::STALAGMITE:
            {
                // One narrow spike per cell. The exponent is what makes it a spike rather
                // than a hump; the noise only shifts its tip off centre a little.
                const Ogre::Real tipOffset = (decorHash(0, laneA, baseSeed) - 0.5f) * 0.3f;
                const Ogre::Real centred = Ogre::Math::Clamp(u + tipOffset, 0.0f, 1.0f);
                value = std::pow(std::sin(Ogre::Math::PI * centred), 3.0f);
                break;
            }
            case RockStyle::JAGGED:
            default:
            {
                // Broken cave rock. The low exponent lifts the mid tones so most of the
                // profile is high with sharp dips, instead of low with occasional peaks -
                // that is the difference between a rock face and a row of teeth.
                const Ogre::Real n = decorFbm(u * 3.2f, laneA, baseSeed);
                const Ogre::Real window = 0.6f + 0.4f * std::sin(Ogre::Math::PI * u);
                value = (0.3f + 0.7f * std::pow(Ogre::Math::Clamp(n * 2.0f, 0.0f, 1.0f), 0.65f)) * window;
                break;
            }
            }

            // The style only ever produces the SHAPE in 0..1; the fill is applied once, here,
            // so every style gets the same guaranteed floor and no style has to know about it.
            const Ogre::Real shape = Ogre::Math::Clamp(value, 0.0f, 1.0f);

            profile.push_back(Ogre::Math::Clamp(fill + (1.0f - fill) * shape, 0.02f, 1.0f));
        }

        return profile;
    }

    void ProceduralDecorBandComponent::addDecorQuad(int row, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1,
        const Ogre::Vector2& uv2, const Ogre::Vector2& uv3)
    {
        if (row < 0 || row >= static_cast<int>(this->rowVertices.size()))
        {
            return;
        }

        std::vector<float>& verts = this->rowVertices[row];
        std::vector<Ogre::uint32>& inds = this->rowIndices[row];
        Ogre::uint32& currentIdx = this->rowVertexCounts[row];

        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v2 - v0;
        Ogre::Vector3 triNormal = edge1.crossProduct(edge2);

        // Degeneracy threshold scaled by the edge lengths. A fixed absolute epsilon breaks
        // exactly where the geometry is finest - here, the near-vertical slivers at a sharp
        // peak - and silently stops correcting the winding there.
        const Ogre::Real edgeScale = std::max(edge1.squaredLength(), edge2.squaredLength());
        const Ogre::Real degenerateThreshold = std::max(1e-10f, edgeScale * 1e-6f);

        bool flipWinding = false;
        if (triNormal.squaredLength() > degenerateThreshold)
        {
            triNormal.normalise();
            if (triNormal.dotProduct(normal) < 0.0f)
            {
                flipWinding = true;
                triNormal = -triNormal;
            }
        }
        else
        {
            triNormal = normal;
        }

        auto addVertex = [&](const Ogre::Vector3& pos, const Ogre::Vector2& uv)
        {
            verts.push_back(pos.x);
            verts.push_back(pos.y);
            verts.push_back(pos.z);
            verts.push_back(triNormal.x);
            verts.push_back(triNormal.y);
            verts.push_back(triNormal.z);
            verts.push_back(uv.x);
            verts.push_back(uv.y);
        };

        const Ogre::uint32 baseIdx = currentIdx;
        addVertex(v0, uv0);
        addVertex(v1, uv1);
        addVertex(v2, uv2);
        addVertex(v3, uv3);

        if (false == flipWinding)
        {
            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 1);
            inds.push_back(baseIdx + 2);

            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 2);
            inds.push_back(baseIdx + 3);
        }
        else
        {
            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 2);
            inds.push_back(baseIdx + 1);

            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 3);
            inds.push_back(baseIdx + 2);
        }

        currentIdx += 4;
    }

    void ProceduralDecorBandComponent::resolveAxes(Ogre::Vector3& alongAxis, Ogre::Vector3& growAxis, Ogre::Vector3& thickAxis) const
    {
        // The band always runs along local X. Grow Direction picks the axis the rock height
        // grows along, and the REMAINING axis becomes both the cluster thickness and the
        // direction the depth rows are offset along. Deriving the row axis instead of giving it
        // its own attribute is what makes it impossible to put growth and rows on the same axis,
        // which would stack every row inside the one before it.
        alongAxis = Ogre::Vector3::UNIT_X;

        switch (this->getGrowDirectionEnum())
        {
        case GrowDirection::DOWN:
        {
            growAxis = Ogre::Vector3::NEGATIVE_UNIT_Y;
            thickAxis = Ogre::Vector3::UNIT_Z;
            break;
        }
        case GrowDirection::TOWARDS_CAMERA:
        {
            growAxis = Ogre::Vector3::NEGATIVE_UNIT_Z;
            thickAxis = Ogre::Vector3::UNIT_Y;
            break;
        }
        case GrowDirection::AWAY_FROM_CAMERA:
        {
            growAxis = Ogre::Vector3::UNIT_Z;
            thickAxis = Ogre::Vector3::UNIT_Y;
            break;
        }
        case GrowDirection::UP:
        default:
        {
            growAxis = Ogre::Vector3::UNIT_Y;
            thickAxis = Ogre::Vector3::UNIT_Z;
            break;
        }
        }
    }

    void ProceduralDecorBandComponent::buildRockCluster(int row, const Ogre::Vector3& rowOrigin, const Ogre::Vector3& alongAxis, const Ogre::Vector3& growAxis, const Ogre::Vector3& thickAxis, Ogre::Real centreAlong, Ogre::Real width,
        Ogre::Real height, Ogre::Real depth, int columnIndex)
    {
        const int points = std::max(2, this->profilePoints->getInt());
        const std::vector<Ogre::Real> profile = this->buildRockProfile(row, columnIndex, points);

        const Ogre::Vector2 tiling = this->uvTiling->getVector2();

        // Every vertex of the cluster goes through this one mapping. Written as explicit x/y/z
        // literals instead, a change of growth axis would leave individual faces behind on the
        // old axis - which is exactly the kind of bug that shows up as one open side.
        auto point = [&rowOrigin, &alongAxis, &growAxis, &thickAxis](Ogre::Real a, Ogre::Real g, Ogre::Real t) -> Ogre::Vector3
        {
            return rowOrigin + alongAxis * a + growAxis * g + thickAxis * t;
        };

        const Ogre::Real tFront = -depth * 0.5f;
        const Ogre::Real tBack = depth * 0.5f;

        const Ogre::Real left = centreAlong - width * 0.5f;

        // Absolute band coordinate, not cluster local, so the UVs flow continuously from one
        // cluster into the next instead of restarting at every cell.
        std::vector<Ogre::Real> as(points);
        std::vector<Ogre::Real> gs(points);

        for (int i = 0; i < points; ++i)
        {
            const Ogre::Real u = (points > 1) ? (static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(points - 1)) : 0.0f;
            as[i] = left + width * u;
            gs[i] = height * profile[i];
        }

        auto uvAt = [&tiling](Ogre::Real a, Ogre::Real b) -> Ogre::Vector2
        {
            return Ogre::Vector2(a * tiling.x, b * tiling.y);
        };

        // ── Front and back faces ─────────────────────────────────────────────────────
        // Built as a quad strip from the baseline up to the profile rather than as a
        // triangulated polygon. The silhouette is a height function over the band axis, so the
        // strip is always correct and needs no polygon triangulation at all.
        for (int i = 0; i + 1 < points; ++i)
        {
            this->addDecorQuad(row, point(as[i], 0.0f, tFront), point(as[i + 1], 0.0f, tFront), point(as[i + 1], gs[i + 1], tFront), point(as[i], gs[i], tFront), -thickAxis, uvAt(as[i], 0.0f), uvAt(as[i + 1], 0.0f), uvAt(as[i + 1], gs[i + 1]),
                uvAt(as[i], gs[i]));

            this->addDecorQuad(row, point(as[i], 0.0f, tBack), point(as[i + 1], 0.0f, tBack), point(as[i + 1], gs[i + 1], tBack), point(as[i], gs[i], tBack), thickAxis, uvAt(as[i], 0.0f), uvAt(as[i + 1], 0.0f), uvAt(as[i + 1], gs[i + 1]),
                uvAt(as[i], gs[i]));
        }

        // ── Top rim ──────────────────────────────────────────────────────────────────
        // The band between the front and back outline. This is the only part of a foreground
        // occluder that ever catches light, so it is worth having even though the sides are
        // never seen.
        for (int i = 0; i + 1 < points; ++i)
        {
            const Ogre::Real da = as[i + 1] - as[i];
            const Ogre::Real dg = gs[i + 1] - gs[i];

            // Perpendicular to the profile segment, inside the (along, grow) plane. The along
            // coordinate increases with i, so rotating the segment direction by 90 degrees in
            // that plane always yields the outward-facing one.
            Ogre::Vector3 rimNormal = alongAxis * (-dg) + growAxis * da;
            if (rimNormal.squaredLength() < 1e-9f)
            {
                rimNormal = growAxis;
            }
            rimNormal.normalise();

            this->addDecorQuad(row, point(as[i], gs[i], tFront), point(as[i + 1], gs[i + 1], tFront), point(as[i + 1], gs[i + 1], tBack), point(as[i], gs[i], tBack), rimNormal, uvAt(as[i], tFront), uvAt(as[i + 1], tFront), uvAt(as[i + 1], tBack),
                uvAt(as[i], tBack));
        }

        // ── End caps and bottom ──────────────────────────────────────────────────────
        // Cheap, and they make the cluster a closed solid - which matters the moment someone
        // turns Cast Shadows on, because an open shell throws a shadow with holes in it.
        this->addDecorQuad(row, point(as[0], 0.0f, tFront), point(as[0], gs[0], tFront), point(as[0], gs[0], tBack), point(as[0], 0.0f, tBack), -alongAxis, uvAt(tFront, 0.0f), uvAt(tFront, gs[0]), uvAt(tBack, gs[0]), uvAt(tBack, 0.0f));

        const int last = points - 1;
        this->addDecorQuad(row, point(as[last], 0.0f, tFront), point(as[last], gs[last], tFront), point(as[last], gs[last], tBack), point(as[last], 0.0f, tBack), alongAxis, uvAt(tFront, 0.0f), uvAt(tFront, gs[last]), uvAt(tBack, gs[last]),
            uvAt(tBack, 0.0f));

        this->addDecorQuad(row, point(as[0], 0.0f, tFront), point(as[last], 0.0f, tFront), point(as[last], 0.0f, tBack), point(as[0], 0.0f, tBack), -growAxis, uvAt(as[0], tFront), uvAt(as[last], tFront), uvAt(as[last], tBack), uvAt(as[0], tBack));
    }

    void ProceduralDecorBandComponent::buildGeometry(void)
    {
        const int rowCount = std::max(1, this->rows->getInt());
        const int columnCount = std::max(1, this->columns->getInt());

        this->rowVertices.assign(rowCount, std::vector<float>());
        this->rowIndices.assign(rowCount, std::vector<Ogre::uint32>());
        this->rowVertexCounts.assign(rowCount, 0u);

        const Ogre::Real width = std::max(0.1f, this->bandWidth->getReal());
        const Ogre::Real cellWidth = width / static_cast<Ogre::Real>(columnCount);
        const Ogre::Real baseHeight = std::max(0.05f, this->rockHeight->getReal());
        const Ogre::Real baseDepth = std::max(0.01f, this->rockDepth->getReal());
        const Ogre::Real widthScale = std::max(0.1f, this->rockWidthScale->getReal());
        const Ogre::Real heightVar = Ogre::Math::Clamp(this->heightVariation->getReal(), 0.0f, 1.0f);
        const Ogre::Real jitter = Ogre::Math::Clamp(this->widthJitter->getReal(), 0.0f, 1.0f);
        const Ogre::Real densityValue = Ogre::Math::Clamp(this->density->getReal(), 0.0f, 1.0f);
        const Ogre::Real spacing = this->rowDepthSpacing->getReal();
        const Ogre::Real scalePerRow = Ogre::Math::Clamp(this->rowScale->getReal(), 0.1f, 2.0f);
        const int baseSeed = this->seed->getInt();

        const Ogre::Real halfWidth = width * 0.5f;

        // The smallest cluster width that CANNOT leave a gap. Two neighbouring centres are at
        // most cellWidth * (1 + jitter) apart - one cell plus both jitter offsets pulling away
        // from each other - so two clusters of this width always still overlap. Without this
        // floor the jitter can shrink a cluster below the separation of its own neighbours, and
        // the band tears open exactly where it was made irregular.
        const Ogre::Real minClusterWidth = cellWidth * (1.0f + jitter);

        Ogre::Vector3 alongAxis = Ogre::Vector3::UNIT_X;
        Ogre::Vector3 growAxis = Ogre::Vector3::UNIT_Y;
        Ogre::Vector3 thickAxis = Ogre::Vector3::UNIT_Z;
        this->resolveAxes(alongAxis, growAxis, thickAxis);

        for (int row = 0; row < rowCount; ++row)
        {
            // Each row back is scaled down and offset. The scale is cumulative, so the falloff
            // reads as perspective rather than as a single step. Spacing may be negative, which
            // brings the rows towards the camera instead of away from it.
            const Ogre::Real rowSize = std::pow(scalePerRow, static_cast<Ogre::Real>(row));
            const Ogre::Vector3 rowOrigin = thickAxis * (spacing * static_cast<Ogre::Real>(row));

            for (int column = 0; column < columnCount; ++column)
            {
                // Four independent hash lanes per cell. Drawing them from one sequence would
                // couple the values - change the density and every rock would also change
                // height, which makes the band impossible to tune.
                const Ogre::Real cellRoll = decorHash(column, row, baseSeed);
                const Ogre::Real heightRoll = decorHash(column, row, baseSeed + 7717);
                const Ogre::Real offsetRoll = decorHash(column, row, baseSeed + 15451);
                const Ogre::Real widthRoll = decorHash(column, row, baseSeed + 22229);

                if (cellRoll > densityValue)
                {
                    continue;
                }

                const Ogre::Real centreAlong = -halfWidth + cellWidth * (static_cast<Ogre::Real>(column) + 0.5f) + (offsetRoll - 0.5f) * cellWidth * jitter;

                // Clusters are deliberately WIDER than their cell by default. Without the
                // overlap the regular cell boundaries stay visible as a rhythm of gaps, and no
                // amount of per-rock detail hides that.
                //
                // Row Scale is deliberately NOT applied to the width. It would shrink the
                // cluster without shrinking the cell spacing it has to bridge, so every row
                // behind the first would open up - the gaps that appear when the band is scaled
                // down. Row Scale works on height and depth, where there is nothing to bridge.
                const Ogre::Real clusterWidth = std::max(minClusterWidth, cellWidth * widthScale * (1.0f - jitter * 0.4f + widthRoll * jitter * 0.8f));

                const Ogre::Real clusterHeight = baseHeight * rowSize * (1.0f - heightVar * heightRoll);

                this->buildRockCluster(row, rowOrigin, alongAxis, growAxis, thickAxis, centreAlong, clusterWidth, clusterHeight, baseDepth * rowSize, column);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Mesh
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDecorBandComponent::regenerate(void)
    {
        this->destroyDecorMesh();

        if (false == this->activated->getBool())
        {
            return;
        }

        this->buildGeometry();

        this->createDecorMesh();
    }

    void ProceduralDecorBandComponent::createDecorMesh(void)
    {
        bool anything = false;
        for (const Ogre::uint32 count : this->rowVertexCounts)
        {
            if (count > 0u)
            {
                anything = true;
                break;
            }
        }

        if (false == anything)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDecorBandComponent] Nothing to build - density or size left the band empty");
            return;
        }

        const std::vector<std::vector<float>> rowVerts = this->rowVertices;
        const std::vector<std::vector<Ogre::uint32>> rowInds = this->rowIndices;
        const std::vector<Ogre::uint32> counts = this->rowVertexCounts;

        GraphicsModule::RenderCommand renderCommand = [this, rowVerts, rowInds, counts]()
        {
            this->createDecorMeshInternal(rowVerts, rowInds, counts);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::createDecorMesh");

        // The CPU-side copies have served their purpose; everything that matters now lives in
        // the VAOs. Keeping them would just be a second truth to go stale.
        this->rowVertices.clear();
        this->rowIndices.clear();
        this->rowVertexCounts.clear();
    }

    void ProceduralDecorBandComponent::createDecorMeshInternal(const std::vector<std::vector<float>>& rowVerts, const std::vector<std::vector<Ogre::uint32>>& rowInds, const std::vector<Ogre::uint32>& rowVertexCountsIn)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

        const Ogre::String meshName = this->gameObjectPtr->getName() + "_DecorBand_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        {
            Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
            Ogre::MeshPtr existing = meshMgr.getByName(meshName, groupName);
            if (false == existing.isNull())
            {
                meshMgr.remove(existing->getHandle());
            }
        }

        this->decorMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName, &NOWA::gDummyMeshLoader);

        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        const size_t srcFloatsPerVertex = 8;
        const size_t dstFloatsPerVertex = 12;

        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

        // An empty row gets NO submesh. A submesh with no geometry needs a dummy VAO with zero
        // indices to keep Ogre from crashing, and anything that later walks the submeshes and
        // rebuilds them dies on that zero-byte buffer. With Density below 1 an empty row is a
        // perfectly ordinary outcome here, not a corner case.
        for (size_t row = 0; row < rowVerts.size(); ++row)
        {
            const Ogre::uint32 numVerts = rowVertexCountsIn[row];
            if (0u == numVerts || true == rowInds[row].empty())
            {
                continue;
            }

            Ogre::SubMesh* subMesh = this->decorMesh->createSubMesh();

            const size_t vertexDataSize = numVerts * dstFloatsPerVertex * sizeof(float);
            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < numVerts; ++i)
            {
                const size_t srcOffset = i * srcFloatsPerVertex;
                const size_t dstOffset = i * dstFloatsPerVertex;

                vertexData[dstOffset + 0] = rowVerts[row][srcOffset + 0];
                vertexData[dstOffset + 1] = rowVerts[row][srcOffset + 1];
                vertexData[dstOffset + 2] = rowVerts[row][srcOffset + 2];

                const Ogre::Vector3 pos(rowVerts[row][srcOffset + 0], rowVerts[row][srcOffset + 1], rowVerts[row][srcOffset + 2]);
                minBounds.makeFloor(pos);
                maxBounds.makeCeil(pos);

                const Ogre::Vector3 normal(rowVerts[row][srcOffset + 3], rowVerts[row][srcOffset + 4], rowVerts[row][srcOffset + 5]);
                vertexData[dstOffset + 3] = normal.x;
                vertexData[dstOffset + 4] = normal.y;
                vertexData[dstOffset + 5] = normal.z;

                Ogre::Vector3 tangent;
                if (std::abs(normal.y) < 0.9f)
                {
                    tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
                }
                else
                {
                    tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
                }
                tangent.normalise();

                vertexData[dstOffset + 6] = tangent.x;
                vertexData[dstOffset + 7] = tangent.y;
                vertexData[dstOffset + 8] = tangent.z;
                vertexData[dstOffset + 9] = 1.0f;

                vertexData[dstOffset + 10] = rowVerts[row][srcOffset + 6];
                vertexData[dstOffset + 11] = rowVerts[row][srcOffset + 7];
            }

            Ogre::VertexBufferPacked* vertexBuffer = nullptr;
            try
            {
                vertexBuffer = vaoManager->createVertexBuffer(vertexElements, numVerts, Ogre::BT_IMMUTABLE, vertexData, true);
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDecorBandComponent] Vertex buffer creation failed: " + e.getDescription());
                return;
            }

            const size_t indexDataSize = rowInds[row].size() * sizeof(Ogre::uint32);
            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, rowInds[row].data(), indexDataSize);

            Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, rowInds[row].size(), Ogre::BT_IMMUTABLE, indexData, true);

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);
        }

        if (0 == this->decorMesh->getNumSubMeshes())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDecorBandComponent] No submesh had geometry");
            return;
        }

        if (minBounds.x == std::numeric_limits<float>::max())
        {
            minBounds = Ogre::Vector3(-1, -1, -1);
            maxBounds = Ogre::Vector3(1, 1, 1);
        }

        Ogre::Aabb bounds;
        bounds.setExtents(minBounds, maxBounds);
        this->decorMesh->_setBounds(bounds, false);
        this->decorMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->decorItem = this->gameObjectPtr->getSceneManager()->createItem(this->decorMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->decorItem->setCastShadows(this->castShadows->getBool());

        this->applyDatablocks();

        this->gameObjectPtr->getSceneNode()->attachObject(this->decorItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->decorItem);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDecorBandComponent] Band built: " + Ogre::StringConverter::toString(this->decorMesh->getNumSubMeshes()) + " rows, brightness " +
                                                                               Ogre::StringConverter::toString(this->frontBrightness->getReal()) + " to " + Ogre::StringConverter::toString(this->backBrightness->getReal()));
    }

    void ProceduralDecorBandComponent::applyDatablocks(void)
    {
        if (nullptr == this->decorItem)
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        const Ogre::String sourceName = this->datablockName->getString();
        Ogre::HlmsDatablock* source = (false == sourceName.empty()) ? hlmsManager->getDatablockNoDefault(sourceName) : nullptr;

        if (nullptr == source)
        {
            if (false == sourceName.empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralDecorBandComponent] Datablock '" + sourceName + "' not found - the band keeps the default material");
            }
            this->destroyClonedDatablocks();
            return;
        }

        Ogre::HlmsPbsDatablock* sourcePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(source);

        const size_t numSubItems = this->decorItem->getNumSubItems();

        if (nullptr == sourcePbs)
        {
            // Unlit, Wind or any other Hlms: there is no diffuse to scale, so the brightness
            // ramp cannot be applied. Said out loud rather than silently producing a band where
            // every row looks identical and nobody knows why.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralDecorBandComponent] Datablock '" + sourceName + "' is not a PBS datablock - the brightness ramp cannot be applied and every row will look the same");

            this->destroyClonedDatablocks();

            for (size_t i = 0; i < numSubItems; ++i)
            {
                this->decorItem->getSubItem(i)->setDatablock(source);
            }
            return;
        }

        // The source changed, so the existing clones carry the wrong textures.
        if (this->clonedSourceName != sourceName)
        {
            this->destroyClonedDatablocks();
        }

        const Ogre::Real front = this->frontBrightness->getReal();
        const Ogre::Real back = this->backBrightness->getReal();

        const Ogre::Vector3 sourceDiffuse = sourcePbs->getDiffuse();

        this->clonedDatablockNames.resize(numSubItems);

        for (size_t i = 0; i < numSubItems; ++i)
        {
            const Ogre::Real t = (numSubItems > 1) ? (static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(numSubItems - 1)) : 0.0f;
            const Ogre::Real brightness = front + (back - front) * t;

            const Ogre::String cloneName = "ProceduralDecorBand_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_r" + Ogre::StringConverter::toString(static_cast<unsigned int>(i));

            Ogre::HlmsPbsDatablock* clonePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(hlmsManager->getDatablockNoDefault(cloneName));

            if (nullptr == clonePbs)
            {
                try
                {
                    clonePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(sourcePbs->clone(cloneName));
                }
                catch (Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDecorBandComponent] Could not clone datablock: " + e.getDescription());
                    this->decorItem->getSubItem(i)->setDatablock(source);
                    continue;
                }
            }

            if (nullptr == clonePbs)
            {
                this->decorItem->getSubItem(i)->setDatablock(source);
                continue;
            }

            // Scaled off the SOURCE's diffuse every time, never off the clone's own. Reading the
            // clone back would compound the factor on each call, and a band set to 0.3 would
            // walk itself to black over a few property edits.
            clonePbs->setDiffuse(sourceDiffuse * brightness);

            this->decorItem->getSubItem(i)->setDatablock(clonePbs);

            this->clonedDatablockNames[i] = cloneName;
        }

        this->clonedSourceName = sourceName;
    }

    void ProceduralDecorBandComponent::destroyClonedDatablocks(void)
    {
        if (true == this->clonedDatablockNames.empty())
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        // Point the submeshes somewhere else FIRST. Destroying a datablock an Item still
        // references is a crash, not a leak.
        if (nullptr != this->decorItem)
        {
            const Ogre::String sourceName = this->datablockName->getString();
            Ogre::HlmsDatablock* source = (false == sourceName.empty()) ? hlmsManager->getDatablockNoDefault(sourceName) : nullptr;

            if (nullptr != source)
            {
                for (size_t i = 0; i < this->decorItem->getNumSubItems(); ++i)
                {
                    this->decorItem->getSubItem(i)->setDatablock(source);
                }
            }
        }

        for (const Ogre::String& cloneName : this->clonedDatablockNames)
        {
            if (true == cloneName.empty())
            {
                continue;
            }

            Ogre::HlmsDatablock* clone = hlmsManager->getDatablockNoDefault(cloneName);
            if (nullptr != clone && nullptr != clone->getCreator())
            {
                try
                {
                    clone->getCreator()->destroyDatablock(cloneName);
                }
                catch (Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDecorBandComponent] Could not destroy cloned datablock: " + e.getDescription());
                }
            }
        }

        this->clonedDatablockNames.clear();
        this->clonedSourceName.clear();
    }

    void ProceduralDecorBandComponent::destroyDecorMesh(void)
    {
        if (nullptr == this->decorItem && nullptr == this->decorMesh)
        {
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Clones first, while the Item still exists to be pointed away from them.
            this->destroyClonedDatablocks();

            if (nullptr != this->decorItem)
            {
                if (this->decorItem->getParentSceneNode())
                {
                    this->decorItem->getParentSceneNode()->detachObject(this->decorItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->decorItem);
                this->decorItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }

            if (this->decorMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->decorMesh->getHandle());
                this->decorMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::destroyDecorMesh");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Attribute access
    //
    // Everything that changes the SHAPE regenerates. Only the two brightness values and Cast
    // Shadows get away without it - they touch materials and flags, not geometry.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDecorBandComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
        this->regenerate();
    }

    bool ProceduralDecorBandComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralDecorBandComponent::setBandWidth(Ogre::Real width)
    {
        this->bandWidth->setValue(Ogre::Math::Clamp(width, 0.1f, 2000.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getBandWidth(void) const
    {
        return this->bandWidth->getReal();
    }

    void ProceduralDecorBandComponent::setRows(int rows)
    {
        this->rows->setValue(Ogre::Math::Clamp(rows, 1, 12));
        this->regenerate();
    }

    int ProceduralDecorBandComponent::getRows(void) const
    {
        return this->rows->getInt();
    }

    void ProceduralDecorBandComponent::setColumns(int columns)
    {
        this->columns->setValue(Ogre::Math::Clamp(columns, 1, 512));
        this->regenerate();
    }

    int ProceduralDecorBandComponent::getColumns(void) const
    {
        return this->columns->getInt();
    }

    void ProceduralDecorBandComponent::setRowDepthSpacing(Ogre::Real spacing)
    {
        // Negative is valid and useful: it brings the rows towards the camera, which is what a
        // foreground occluder band in front of the player needs.
        this->rowDepthSpacing->setValue(Ogre::Math::Clamp(spacing, -200.0f, 200.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getRowDepthSpacing(void) const
    {
        return this->rowDepthSpacing->getReal();
    }

    void ProceduralDecorBandComponent::setRowScale(Ogre::Real scale)
    {
        this->rowScale->setValue(Ogre::Math::Clamp(scale, 0.1f, 2.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getRowScale(void) const
    {
        return this->rowScale->getReal();
    }

    void ProceduralDecorBandComponent::setGrowDirection(const Ogre::String& direction)
    {
        this->growDirection->setListSelectedValue(direction);
        this->regenerate();
    }

    Ogre::String ProceduralDecorBandComponent::getGrowDirection(void) const
    {
        return this->growDirection->getListSelectedValue();
    }

    ProceduralDecorBandComponent::GrowDirection ProceduralDecorBandComponent::getGrowDirectionEnum(void) const
    {
        const Ogre::String value = this->growDirection->getListSelectedValue();

        if ("Down (-Y)" == value)
        {
            return GrowDirection::DOWN;
        }
        if ("Towards Camera (-Z)" == value)
        {
            return GrowDirection::TOWARDS_CAMERA;
        }
        if ("Away From Camera (+Z)" == value)
        {
            return GrowDirection::AWAY_FROM_CAMERA;
        }
        return GrowDirection::UP;
    }

    void ProceduralDecorBandComponent::setRockStyle(const Ogre::String& style)
    {
        this->rockStyle->setListSelectedValue(style);
        this->regenerate();
    }

    Ogre::String ProceduralDecorBandComponent::getRockStyle(void) const
    {
        return this->rockStyle->getListSelectedValue();
    }

    ProceduralDecorBandComponent::RockStyle ProceduralDecorBandComponent::getRockStyleEnum(void) const
    {
        const Ogre::String value = this->rockStyle->getListSelectedValue();

        if ("Rounded" == value)
        {
            return RockStyle::ROUNDED;
        }
        if ("Columnar" == value)
        {
            return RockStyle::COLUMNAR;
        }
        if ("Stalagmite" == value)
        {
            return RockStyle::STALAGMITE;
        }
        return RockStyle::JAGGED;
    }

    void ProceduralDecorBandComponent::setRockHeight(Ogre::Real height)
    {
        this->rockHeight->setValue(Ogre::Math::Clamp(height, 0.05f, 500.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getRockHeight(void) const
    {
        return this->rockHeight->getReal();
    }

    void ProceduralDecorBandComponent::setRockDepth(Ogre::Real depth)
    {
        this->rockDepth->setValue(Ogre::Math::Clamp(depth, 0.01f, 100.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getRockDepth(void) const
    {
        return this->rockDepth->getReal();
    }

    void ProceduralDecorBandComponent::setRockWidthScale(Ogre::Real scale)
    {
        this->rockWidthScale->setValue(Ogre::Math::Clamp(scale, 0.1f, 4.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getRockWidthScale(void) const
    {
        return this->rockWidthScale->getReal();
    }

    void ProceduralDecorBandComponent::setBaseFill(Ogre::Real fill)
    {
        this->baseFill->setValue(Ogre::Math::Clamp(fill, 0.0f, 1.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getBaseFill(void) const
    {
        return this->baseFill->getReal();
    }

    void ProceduralDecorBandComponent::setHeightVariation(Ogre::Real variation)
    {
        this->heightVariation->setValue(Ogre::Math::Clamp(variation, 0.0f, 1.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getHeightVariation(void) const
    {
        return this->heightVariation->getReal();
    }

    void ProceduralDecorBandComponent::setWidthJitter(Ogre::Real jitter)
    {
        this->widthJitter->setValue(Ogre::Math::Clamp(jitter, 0.0f, 1.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getWidthJitter(void) const
    {
        return this->widthJitter->getReal();
    }

    void ProceduralDecorBandComponent::setDensity(Ogre::Real density)
    {
        this->density->setValue(Ogre::Math::Clamp(density, 0.0f, 1.0f));
        this->regenerate();
    }

    Ogre::Real ProceduralDecorBandComponent::getDensity(void) const
    {
        return this->density->getReal();
    }

    void ProceduralDecorBandComponent::setProfilePoints(int points)
    {
        this->profilePoints->setValue(Ogre::Math::Clamp(points, 2, 64));
        this->regenerate();
    }

    int ProceduralDecorBandComponent::getProfilePoints(void) const
    {
        return this->profilePoints->getInt();
    }

    void ProceduralDecorBandComponent::setSeed(int seed)
    {
        this->seed->setValue(seed);
        this->regenerate();
    }

    int ProceduralDecorBandComponent::getSeed(void) const
    {
        return this->seed->getInt();
    }

    void ProceduralDecorBandComponent::setDatablockName(const Ogre::String& datablockName)
    {
        this->datablockName->setValue(datablockName);

        // Material only - the clones are rebuilt from the new source, the geometry is untouched.
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::setDatablockName");
    }

    Ogre::String ProceduralDecorBandComponent::getDatablockName(void) const
    {
        return this->datablockName->getString();
    }

    void ProceduralDecorBandComponent::setFrontBrightness(Ogre::Real brightness)
    {
        this->frontBrightness->setValue(Ogre::Math::Clamp(brightness, 0.0f, 4.0f));

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::setFrontBrightness");
    }

    Ogre::Real ProceduralDecorBandComponent::getFrontBrightness(void) const
    {
        return this->frontBrightness->getReal();
    }

    void ProceduralDecorBandComponent::setBackBrightness(Ogre::Real brightness)
    {
        this->backBrightness->setValue(Ogre::Math::Clamp(brightness, 0.0f, 4.0f));

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::setBackBrightness");
    }

    Ogre::Real ProceduralDecorBandComponent::getBackBrightness(void) const
    {
        return this->backBrightness->getReal();
    }

    void ProceduralDecorBandComponent::setBrightness(Ogre::Real front, Ogre::Real back)
    {
        this->frontBrightness->setValue(Ogre::Math::Clamp(front, 0.0f, 4.0f));
        this->backBrightness->setValue(Ogre::Math::Clamp(back, 0.0f, 4.0f));

        // One render command for both, so a script fading a band in and out does not enqueue
        // two every frame.
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::setBrightness");
    }

    void ProceduralDecorBandComponent::setUVTiling(const Ogre::Vector2& tiling)
    {
        this->uvTiling->setValue(tiling);
        this->regenerate();
    }

    Ogre::Vector2 ProceduralDecorBandComponent::getUVTiling(void) const
    {
        return this->uvTiling->getVector2();
    }

    void ProceduralDecorBandComponent::setCastShadows(bool castShadows)
    {
        this->castShadows->setValue(castShadows);

        if (nullptr != this->decorItem)
        {
            GraphicsModule::RenderCommand renderCommand = [this, castShadows]()
            {
                if (nullptr != this->decorItem)
                {
                    this->decorItem->setCastShadows(castShadows);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDecorBandComponent::setCastShadows");
        }
    }

    bool ProceduralDecorBandComponent::getCastShadows(void) const
    {
        return this->castShadows->getBool();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //  Lua API
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralDecorBandComponent* getProceduralDecorBandComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralDecorBandComponent>()).get();
    }

    ProceduralDecorBandComponent* getProceduralDecorBandComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralDecorBandComponent>(name)).get();
    }

    void ProceduralDecorBandComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<ProceduralDecorBandComponent, GameObjectComponent>("ProceduralDecorBandComponent")

                .def("setActivated", &ProceduralDecorBandComponent::setActivated)
                .def("isActivated", &ProceduralDecorBandComponent::isActivated)

                // ── Layout ────────────────────────────────────────────────────
                .def("setBandWidth", &ProceduralDecorBandComponent::setBandWidth)
                .def("getBandWidth", &ProceduralDecorBandComponent::getBandWidth)
                .def("setRows", &ProceduralDecorBandComponent::setRows)
                .def("getRows", &ProceduralDecorBandComponent::getRows)
                .def("setColumns", &ProceduralDecorBandComponent::setColumns)
                .def("getColumns", &ProceduralDecorBandComponent::getColumns)
                .def("setRowDepthSpacing", &ProceduralDecorBandComponent::setRowDepthSpacing)
                .def("getRowDepthSpacing", &ProceduralDecorBandComponent::getRowDepthSpacing)
                .def("setRowScale", &ProceduralDecorBandComponent::setRowScale)
                .def("getRowScale", &ProceduralDecorBandComponent::getRowScale)
                .def("setGrowDirection", &ProceduralDecorBandComponent::setGrowDirection)
                .def("getGrowDirection", &ProceduralDecorBandComponent::getGrowDirection)

                // ── Shape ─────────────────────────────────────────────────────
                .def("setRockStyle", &ProceduralDecorBandComponent::setRockStyle)
                .def("getRockStyle", &ProceduralDecorBandComponent::getRockStyle)
                .def("setRockHeight", &ProceduralDecorBandComponent::setRockHeight)
                .def("getRockHeight", &ProceduralDecorBandComponent::getRockHeight)
                .def("setRockDepth", &ProceduralDecorBandComponent::setRockDepth)
                .def("getRockDepth", &ProceduralDecorBandComponent::getRockDepth)
                .def("setRockWidthScale", &ProceduralDecorBandComponent::setRockWidthScale)
                .def("getRockWidthScale", &ProceduralDecorBandComponent::getRockWidthScale)
                .def("setBaseFill", &ProceduralDecorBandComponent::setBaseFill)
                .def("getBaseFill", &ProceduralDecorBandComponent::getBaseFill)
                .def("setHeightVariation", &ProceduralDecorBandComponent::setHeightVariation)
                .def("getHeightVariation", &ProceduralDecorBandComponent::getHeightVariation)
                .def("setWidthJitter", &ProceduralDecorBandComponent::setWidthJitter)
                .def("getWidthJitter", &ProceduralDecorBandComponent::getWidthJitter)
                .def("setDensity", &ProceduralDecorBandComponent::setDensity)
                .def("getDensity", &ProceduralDecorBandComponent::getDensity)
                .def("setProfilePoints", &ProceduralDecorBandComponent::setProfilePoints)
                .def("getProfilePoints", &ProceduralDecorBandComponent::getProfilePoints)
                .def("setSeed", &ProceduralDecorBandComponent::setSeed)
                .def("getSeed", &ProceduralDecorBandComponent::getSeed)

                // ── Shading ───────────────────────────────────────────────────
                .def("setDatablockName", &ProceduralDecorBandComponent::setDatablockName)
                .def("getDatablockName", &ProceduralDecorBandComponent::getDatablockName)
                .def("setFrontBrightness", &ProceduralDecorBandComponent::setFrontBrightness)
                .def("getFrontBrightness", &ProceduralDecorBandComponent::getFrontBrightness)
                .def("setBackBrightness", &ProceduralDecorBandComponent::setBackBrightness)
                .def("getBackBrightness", &ProceduralDecorBandComponent::getBackBrightness)
                .def("setBrightness", &ProceduralDecorBandComponent::setBrightness)
                .def("setUVTiling", &ProceduralDecorBandComponent::setUVTiling)
                .def("getUVTiling", &ProceduralDecorBandComponent::getUVTiling)
                .def("setCastShadows", &ProceduralDecorBandComponent::setCastShadows)
                .def("getCastShadows", &ProceduralDecorBandComponent::getCastShadows)

                .def("regenerate", &ProceduralDecorBandComponent::regenerate)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "class inherits GameObjectComponent", ProceduralDecorBandComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setActivated(bool activated)", "Activates or deactivates the band. Deactivating destroys its geometry.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setSeed(int seed)", "Sets the layout seed and rebuilds. The same seed always gives the same band.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setDensity(float density)", "Fraction of cells that get a rock cluster, 0 to 1. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setRockHeight(float height)", "Height of a row 0 cluster in meters. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setRockStyle(string style)", "Sets the cluster outline: 'Jagged', 'Rounded', 'Columnar' or 'Stalagmite'. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setGrowDirection(string direction)",
            "Axis the rock height grows along: 'Up (+Y)', 'Down (-Y)', 'Towards Camera (-Z)' or 'Away From Camera (+Z)'. The remaining axis carries the cluster thickness and the row offsets. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setBaseFill(float fill)",
            "Fraction of Rock Height the outline never drops below, 0 to 1. Keeps overlapping clusters welded into one ridge instead of meeting at zero height. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setRowDepthSpacing(float spacing)",
            "Offset per depth row along the free axis. Negative brings the rows towards the camera, positive pushes them away. Rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setFrontBrightness(float brightness)",
            "Diffuse multiplier for the first row. Material only, no rebuild - the datablock is cloned per row so the original stays untouched.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setBackBrightness(float brightness)", "Diffuse multiplier for the last row, rows between interpolated. Material only, no rebuild.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void setBrightness(float front, float back)",
            "Both brightness values in one call and one render command. Cheap enough to drive per frame, e.g. to fade a foreground band out while the player stands behind it.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralDecorBandComponent", "void regenerate()", "Rebuilds the band. Only needed to collapse several changes into one rebuild.");

        gameObjectClass.def("getProceduralDecorBandComponent", (ProceduralDecorBandComponent * (*)(GameObject*)) & getProceduralDecorBandComponent);
        gameObjectClass.def("getProceduralDecorBandComponentFromName", &getProceduralDecorBandComponentFromName);
        gameObjectControllerClass.def("castProceduralDecorBandComponent", &GameObjectController::cast<ProceduralDecorBandComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralDecorBandComponent getProceduralDecorBandComponent()", "Gets the ProceduralDecorBandComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralDecorBandComponent getProceduralDecorBandComponentFromName(string name)", "Gets a named ProceduralDecorBandComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralDecorBandComponent castProceduralDecorBandComponent(ProceduralDecorBandComponent other)", "Casts for Lua auto-completion support.");
    }

} // namespace NOWA