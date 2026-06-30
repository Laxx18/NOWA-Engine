#include "NOWAPrecompiled.h"
#include "PlanetMinimapComponent.h"
#include "../../PlanetTerraComponent/code/PlanetTerraComponent.h"
#include "RenderQueueEnums.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/WorkspaceComponents.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "planetTerra/PlanetTerra.h"
#include "utilities/XMLConverter.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClear.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassMipmap/OgreCompositorPassMipmapDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "OgreStagingTexture.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PlanetMinimapComponent::PlanetMinimapComponent() :
        GameObjectComponent(),
        name("PlanetMinimapComponent"),
        planetGameObject(nullptr),
        planetTerraComponent(nullptr),
        minimapTexture(nullptr),
        minimapStagingTexture(nullptr),
        fogOfWarTexture(nullptr),
        fogOfWarStagingTexture(nullptr),
        textureManager(nullptr),
        workspace(nullptr),
        minimapWidget(nullptr),
        maskWidget(nullptr),
        fogOfWarWidget(nullptr),
        targetMarkerWidget(nullptr),
        cameraComponent(nullptr),
        timeSinceLastUpdate(0.25f),
        followCameraLogged(false),
        compassLogCounter(0u),
        activated(new Variant(PlanetMinimapComponent::AttrActivated(), true, this->attributes)),
        planetId(new Variant(PlanetMinimapComponent::AttrPlanetId(), static_cast<unsigned long>(0), this->attributes, true)),
        targetId(new Variant(PlanetMinimapComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        textureSize(new Variant(PlanetMinimapComponent::AttrTextureSize(), static_cast<unsigned int>(256), this->attributes)),
        minimapGeometry(new Variant(PlanetMinimapComponent::AttrMinimapGeometry(), Ogre::Vector4(0.8f, 0.0f, 0.2f, 0.2f), this->attributes)),
        transparent(new Variant(PlanetMinimapComponent::AttrTransparent(), false, this->attributes)),
        useFogOfWar(new Variant(PlanetMinimapComponent::AttrUseFogOfWar(), false, this->attributes)),
        useDiscovery(new Variant(PlanetMinimapComponent::AttrUseDiscovery(), false, this->attributes)),
        persistDiscovery(new Variant(PlanetMinimapComponent::AttrPersistDiscovery(), false, this->attributes)),
        visibilityRadius(new Variant(PlanetMinimapComponent::AttrVisibilityRadius(), Ogre::Real(5.0f), this->attributes)),
        useVisibilitySpotlight(new Variant(PlanetMinimapComponent::AttrUseVisibilitySpotlight(), false, this->attributes)),
        wholeSceneVisible(new Variant(PlanetMinimapComponent::AttrWholeSceneVisible(), true, this->attributes)),
        cameraHeight(new Variant(PlanetMinimapComponent::AttrCameraHeight(), Ogre::Real(10.0f), this->attributes)),
        minimapMask(new Variant(PlanetMinimapComponent::AttrMinimapMask(), "", this->attributes)),
        targetMarkerImage(new Variant(PlanetMinimapComponent::AttrTargetMarkerImage(), "", this->attributes)),
        compassObjectCount(new Variant(PlanetMinimapComponent::AttrCompassObjectCount(), static_cast<unsigned int>(0), this->attributes)),
        baseTerrainColor(new Variant(PlanetMinimapComponent::AttrBaseTerrainColor(), Ogre::Vector3(0.3f, 0.5f, 0.2f), this->attributes)),
        layer0Color(new Variant(PlanetMinimapComponent::AttrLayer0Color(), Ogre::Vector3(0.76f, 0.7f, 0.5f), this->attributes)),
        layer1Color(new Variant(PlanetMinimapComponent::AttrLayer1Color(), Ogre::Vector3(0.55f, 0.2f, 0.1f), this->attributes)),
        layer2Color(new Variant(PlanetMinimapComponent::AttrLayer2Color(), Ogre::Vector3(0.45f, 0.42f, 0.38f), this->attributes)),
        layer3Color(new Variant(PlanetMinimapComponent::AttrLayer3Color(), Ogre::Vector3(0.15f, 0.1f, 0.08f), this->attributes)),
        useRoundMinimap(new Variant(PlanetMinimapComponent::AttrUseRoundMinimap(), false, this->attributes)),
        rotateMinimap(new Variant(PlanetMinimapComponent::AttrRotateMinimap(), false, this->attributes))
    {
        this->planetId->setDescription("Sets the game object id of the planet (must hold a PlanetTerraComponent), which shall be displayed on the minimap.");
        this->targetId->setDescription("Sets the target id for the game object, which shall be tracked on the planet's surface.");
        this->textureSize->setDescription("Sets the minimap texture size in pixels. Note: The texture is quadratic. Also note: The higher the texture size, the less performant the application will run.");
        this->minimapGeometry->setDescription("Sets the geometry of the minimap relative to the window screen in the format Vector4(pos.x, pos.y, width, height).");
        this->transparent->setDescription("Sets whether the minimap shall be somewhat transparent.");
        this->useFogOfWar->setDescription("If whole scene visible is set to true. Sets whether fog of war is used.");
        this->useFogOfWar->addUserData(GameObject::AttrActionNeedRefresh());
        this->useDiscovery->setDescription("If fog of war is used, sets whether places in which the target game object has visited remain visible.");
        this->persistDiscovery->setDescription("If fog of war is used and use discovery, sets whether places in which the target game object has visited remain visible and also stored and loaded.");
        this->visibilityRadius->setDescription("If fog of war is used, sets the visibilty radius which discovers the fog of war.");
        this->useVisibilitySpotlight->setDescription("If fog of war is used, sets a spotlight instead of rounded radius area. The spotlight is as big as the visibility radius.");
        this->wholeSceneVisible
            ->setDescription("Sets whether the whole planet is visible. If set to true, the WHOLE planet is shown as a baked equirectangular height-shaded snapshot (no camera is used in this mode); the target is shown as a small marker icon. "
                             " If set to false, a real camera follows the target along the planet's surface, always looking back toward the planet's center.");
        this->wholeSceneVisible->addUserData(GameObject::AttrActionNeedRefresh());
        this->cameraHeight->setDescription("If whole scene visible is set to false, sets the camera height, which is added along the local outward (radial) direction at the top of the target game object's position.");

        this->minimapMask->addUserData(GameObject::AttrActionFileOpenDialog(), "Minimap");
        this->minimapMask->setDescription("Sets a minimap image mask, which can be created in gimp and added to the Minimap.xml in the MyGui/Minimap folder.");

        this->targetMarkerImage->addUserData(GameObject::AttrActionFileOpenDialog(), "Minimap");
        this->targetMarkerImage->setDescription("If whole scene visible is set to true, sets a small icon image (e.g. an arrow or dot, added to the Minimap.xml in the MyGui/Minimap folder like MinimapMask) shown at the tracked target's "
                                                "position on the baked planet overview. Left empty, no marker is shown.");

        this->compassObjectCount
            ->setDescription("Sets the count of generic compass objects to find/track (e.g. the player's ship, a quest objective, an NPC). Each gets its own CompassGameObjectId/CompassImage/CompassToolTipText. "
                             "In WholeSceneVisible mode each is shown as a true-position marker on the baked overview (always representable, the bake covers the whole sphere). In follow mode each is shown as a marker wandering around the rim "
                             "of the local minimap once out of view, pointing in the direction to walk.");
        this->compassObjectCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->baseTerrainColor->setDescription("If whole scene visible is set to true, sets the colour shown on the baked overview where none of the 4 paint layers are active (PlanetTerra's base diffuse state). Tune to roughly match this planet's "
                                               "base diffuse colour.");
        this->layer0Color->setDescription("If whole scene visible is set to true, sets the colour shown on the baked overview where paint layer 0 (PlanetTerra blend weight R channel, Detail0TextureName) dominates.");
        this->layer1Color->setDescription("If whole scene visible is set to true, sets the colour shown on the baked overview where paint layer 1 (PlanetTerra blend weight G channel, Detail1TextureName) dominates.");
        this->layer2Color->setDescription("If whole scene visible is set to true, sets the colour shown on the baked overview where paint layer 2 (PlanetTerra blend weight B channel, Detail2TextureName) dominates.");
        this->layer3Color->setDescription("If whole scene visible is set to true, sets the colour shown on the baked overview where paint layer 3 (PlanetTerra blend weight A channel, Detail3TextureName) dominates.");

        this->useRoundMinimap
            ->setDescription("If whole scene visible is set to false, sets whether to render a rounded minimap texture, for more sophistacted effects. This can e.g. be used in conjunction with the minimap mask to create a nice minimap overlay.");
        this->rotateMinimap->setDescription("If whole scene visible is set to false, sets whether the minimap is rotated according to the target game object's heading along the planet's surface tangent plane.");
    }

    PlanetMinimapComponent::~PlanetMinimapComponent(void)
    {
    }

    const Ogre::String& PlanetMinimapComponent::getName() const
    {
        return this->name;
    }

    void PlanetMinimapComponent::install(const Ogre::NameValuePairList* options)
    {
        Ogre::ResourceGroupManager::getSingletonPtr()->addResourceLocation("../../media/MyGUI_Media/minimap", "FileSystem", "Minimap");

        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PlanetMinimapComponent>(PlanetMinimapComponent::getStaticClassId(), PlanetMinimapComponent::getStaticClassName());
    }

    void PlanetMinimapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool PlanetMinimapComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PlanetId")
        {
            this->planetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
        {
            this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextureSize")
        {
            this->textureSize->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinimapGeometry")
        {
            this->minimapGeometry->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Transparent")
        {
            this->transparent->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseFogOfWar")
        {
            this->setUseFogOfWar(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseDiscovery")
        {
            this->useDiscovery->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PersistDiscovery")
        {
            this->persistDiscovery->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VisibilityRadius")
        {
            this->visibilityRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseVisibilitySpotlight")
        {
            this->useVisibilitySpotlight->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WholeSceneVisible")
        {
            this->setWholeSceneVisible(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraHeight")
        {
            this->cameraHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinimapMask")
        {
            this->minimapMask->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseRoundMinimap")
        {
            this->useRoundMinimap->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetMarkerImage")
        {
            this->targetMarkerImage->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CompassObjectCount")
        {
            this->compassObjectCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Only create new variants, if fresh loading. If a snapshot is done, no new variant must be created!
        // Because the undo/redo algorithm relies on the changed-flag of each EXISTING variant (same convention as
        // AnimationSequenceComponent::init).
        if (this->compassGameObjectIds.size() < this->compassObjectCount->getUInt())
        {
            this->compassGameObjectIds.resize(this->compassObjectCount->getUInt());
            this->compassImages.resize(this->compassObjectCount->getUInt());
            this->compassToolTipTexts.resize(this->compassObjectCount->getUInt());
        }

        for (size_t i = 0; i < this->compassGameObjectIds.size(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CompassGameObjectId" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->compassGameObjectIds[i])
                {
                    this->compassGameObjectIds[i] = new Variant(PlanetMinimapComponent::AttrCompassGameObjectId() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data"), this->attributes, true);
                }
                else
                {
                    this->compassGameObjectIds[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CompassImage" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->compassImages[i])
                {
                    this->compassImages[i] = new Variant(PlanetMinimapComponent::AttrCompassImage() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                    this->compassImages[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Minimap");
                }
                else
                {
                    this->compassImages[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CompassToolTipText" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->compassToolTipTexts[i])
                {
                    this->compassToolTipTexts[i] = new Variant(PlanetMinimapComponent::AttrCompassToolTipText() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->compassToolTipTexts[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BaseTerrainColor")
        {
            this->baseTerrainColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer0Color")
        {
            this->layer0Color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer1Color")
        {
            this->layer1Color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer2Color")
        {
            this->layer2Color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer3Color")
        {
            this->layer3Color->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotateMinimap")
        {
            this->rotateMinimap->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr PlanetMinimapComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    bool PlanetMinimapComponent::postInit(void)
    {
        if (true == this->bShowDebugData)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetMinimapComponent] Init component for game object: " + this->gameObjectPtr->getName());
        }

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetMinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());

        this->textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();

        // Removes complexity, by setting default direction to -z, which is Ogre default.
        this->gameObjectPtr->setDefaultDirection(Ogre::Vector3::NEGATIVE_UNIT_Z);
        this->gameObjectPtr->getAttribute(GameObject::AttrDefaultDirection())->setVisible(false);

        return true;
    }

    void PlanetMinimapComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && nullptr != this->planetGameObject)
        {
            unsigned long targetId = this->targetId->getULong();

            auto closureFunction = [this, targetId](Ogre::Real renderDt)
            {
                Ogre::Vector3 targetPos = Ogre::Vector3::ZERO;
                Ogre::Quaternion targetOrientation = Ogre::Quaternion::IDENTITY;
                Ogre::Vector3 targetDefaultDirection = Ogre::Vector3::UNIT_Z;
                const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(targetId);
                if (nullptr == targetGameObjectPtr)
                {
                    if (nullptr != this->targetMarkerWidget)
                    {
                        this->targetMarkerWidget->setVisible(false);
                    }
                }
                else
                {
                    targetPos = targetGameObjectPtr->getPosition();
                    targetOrientation = targetGameObjectPtr->getOrientation();
                    targetDefaultDirection = targetGameObjectPtr->getDefaultDirection();
                }

                if (false == this->wholeSceneVisible->getBool())
                {
                    // Dynamic surface-tracking camera: keep the minimap camera positioned above the target along the
                    // planet's local outward direction, oriented back toward the planet's center. Mirrors how
                    // MinimapComponent::scrollMinimap keeps the flat-terrain camera above the target along world Y,
                    // just measured along the local radial axis instead of the fixed world up axis.
                    Ogre::Vector3 followCameraPosition = Ogre::Vector3::ZERO;
                    Ogre::Quaternion followCameraOrientation = Ogre::Quaternion::IDENTITY;
                    Ogre::Real followCameraDistance = this->cameraHeight->getReal();
                    this->updateFollowCamera(targetPos, targetOrientation, targetDefaultDirection, followCameraPosition, followCameraOrientation, followCameraDistance);

                    if (0 != this->compassObjectCount->getUInt())
                    {
                        // updateCompassObjects() touches MyGUI widgets directly though, so unlike updateFollowCamera()
                        // above it DOES need to run on the render thread. Runs every tick (not throttled) so the rim
                        // markers keep up smoothly as the player turns/moves, same reasoning as the camera update itself.
                        // Passing the camera transform updateFollowCamera just computed THIS tick, rather than letting
                        // updateCompassObjects read it back from the Camera object itself (see updateFollowCamera's
                        // comment for why that read-back was unreliable).
                        this->updateCompassObjects(targetPos, followCameraPosition, followCameraOrientation, followCameraDistance);
                    }
                }
                else
                {
                    this->timeSinceLastUpdate += renderDt;

                    if (this->timeSinceLastUpdate >= 0.25f)
                    {
                        this->updateOverviewTargetMarker(targetPos);
                        // Overview mode's branch inside updateSingleCompassObject never touches the camera transform
                        // parameters (it positions the marker via UV, like updateOverviewTargetMarker) -- these are
                        // unused placeholders here, only meaningful in the follow-mode call above.
                        this->updateCompassObjects(targetPos, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, this->cameraHeight->getReal());

                        if (true == this->useFogOfWar->getBool())
                        {
                            this->updateFogOfWarTexture(targetPos, targetOrientation, targetDefaultDirection, this->visibilityRadius->getReal());
                        }

                        this->timeSinceLastUpdate = 0.0f;
                    }
                }
            };
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    bool PlanetMinimapComponent::connect(void)
    {
        GameObjectComponent::connect();

        // setActivated(true) resolves targetGameObject/planetGameObject/planetTerraComponent from the current
        // TargetId/PlanetId and sets up the workspace; setActivated(false) just clears pointers. Routing through it
        // here too (instead of duplicating the resolution) keeps connect() and Lua-driven re-activation in sync.
        this->setActivated(this->activated->getBool());

        return true;
    }

    bool PlanetMinimapComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->planetGameObject = nullptr;
        this->planetTerraComponent = nullptr;
        this->cameraComponent = nullptr;

        this->timeSinceLastUpdate = 0.25f;

        this->removeWorkspace();

        return true;
    }

    bool PlanetMinimapComponent::onCloned(void)
    {

        return true;
    }

    void PlanetMinimapComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->cameraComponent = nullptr;

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlanetMinimapComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());

        if (true == this->persistDiscovery->getBool())
        {
            this->saveDiscoveryState();
        }

        this->removeWorkspace();
    }

    void PlanetMinimapComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->cameraComponent && index == this->cameraComponent->getIndex())
        {
            this->cameraComponent = nullptr;
        }
    }

    void PlanetMinimapComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    void PlanetMinimapComponent::setupPlanetMinimap(void)
    {
        const auto& cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
        if (nullptr == cameraCompPtr || this->gameObjectPtr->getId() == GameObjectController::MAIN_CAMERA_ID)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PlanetMinimapComponent] Error setting up planet minimap workspace, because the game object: " + this->gameObjectPtr->getName() + " is the main camera. Choose a different camera!");
            return;
        }

        if (nullptr == this->planetGameObject || nullptr == this->planetTerraComponent)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PlanetMinimapComponent] Error setting up planet minimap workspace, because no valid planet game object with a PlanetTerraComponent is referenced for game object: " + this->gameObjectPtr->getName());
            return;
        }

        this->cameraComponent = cameraCompPtr.get();

        // The camera is only actually used (and therefore only needs to be Active) in follow mode -- in
        // WholeSceneVisible mode the overview is a CPU bake, no camera/compositor workspace is involved at all.
        if (false == this->wholeSceneVisible->getBool() && false == this->cameraComponent->isActivated())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PlanetMinimapComponent] WARNING: CameraComponent on game object '" + this->gameObjectPtr->getName() +
                    "' is not activated (Active=false). WholeSceneVisible is false here, so the follow camera's scene pass needs it active, otherwise the minimap RT stays blank/black. Check the CameraComponent's Active property.");
        }

        NOWA::GraphicsModule::RenderCommand setupRdCmd = [this]
        {
            // Remove the workspace from the camera, as it is used for a new planet minimap workspace, which is created here
            const auto& workspaceCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBaseComponent>());
            if (nullptr != workspaceCompPtr)
            {
                this->gameObjectPtr->deleteComponent(workspaceCompPtr.get());
            }

            if (true == this->persistDiscovery->getBool())
            {
                this->loadDiscoveryState();
            }

            Ogre::String minimapTextureName = "PlanetMinimapRT";

            if (true == this->useRoundMinimap->getBool())
            {
                minimapTextureName = "PlanetMinimapRT_Round";
            }
            this->minimapTexture = this->createMinimapTexture(minimapTextureName, this->textureSize->getUInt(), this->textureSize->getUInt());

            if (true == this->useFogOfWar->getBool())
            {
                this->fogOfWarTexture = this->createFogOfWarTexture("PlanetFogOfWarTexture", this->textureSize->getUInt(), this->textureSize->getUInt());
                this->fogOfWarStagingTexture = this->textureManager->getStagingTexture(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());
            }

            if (true == this->useDiscovery->getBool())
            {
                this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));
            }

            if (true == this->wholeSceneVisible->getBool())
            {
                // Whole-planet mode: no camera, no compositor workspace -- the overview is a one-shot CPU bake into
                // minimapTexture, mirroring how PlanetTerra already represents the whole sphere as flat UV-indexed
                // data (its blend weight texture / height array), rather than a literal camera render.
                this->bakePlanetOverviewTexture();
            }
            else
            {
                // Follow mode: a real 3D camera renders the local surroundings, same compositor plumbing the flat
                // MinimapComponent uses.
                this->followCameraLogged = false;
                this->cameraComponent->getOwner()->setDynamic(true);

                if (false == this->useRoundMinimap->getBool())
                {
                    this->createMinimapWorkspace();
                }
                else
                {
                    this->createRoundMinimapWorkspace();
                }
            }

            // Create MyGUI widget for the planet minimap
            Ogre::Vector4 geometry = this->minimapGeometry->getVector4();
            this->minimapWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
            this->minimapWidget->setImageTexture(this->minimapTexture->getNameStr());

            if (false == this->minimapMask->getString().empty())
            {
                // Overlay the circular mask on top of the minimap
                this->maskWidget = minimapWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, this->minimapWidget->getWidth(), this->minimapWidget->getHeight()), MyGUI::Align::Stretch);
                this->maskWidget->setImageTexture(this->minimapMask->getString());
                // Makes sure the mask doesn't block mouse events
                this->maskWidget->setNeedMouseFocus(false);
                // Higher depth to ensure it's on top
                this->maskWidget->setDepth(1);

                // Diagnostic: MyGUI silently falls back to a default (often solid white, fully opaque) placeholder
                // when an image resource can't be resolved -- which would sit on top of minimapWidget at depth 1
                // and hide the correctly baked terrain underneath entirely. getImageSize() returning (0,0) or an
                // unexpectedly small/round value (commonly 1x1 for the fallback) is the tell.
                MyGUI::IntSize maskImageSize = this->maskWidget->getImageSize();
                if (true == this->bShowDebugData)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' MinimapMask='" + this->minimapMask->getString() + "' loaded image size=" + Ogre::StringConverter::toString(maskImageSize.width) + "x" +
                            Ogre::StringConverter::toString(maskImageSize.height) +
                            (maskImageSize.width <= 1 || maskImageSize.height <= 1 ? " (SUSPICIOUS - likely failed to load, check the file exists in the 'Minimap' resource group and has been added to Minimap.xml)" : ""));
                }
            }

            if (true == this->useFogOfWar->getBool())
            {
                this->fogOfWarWidget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", MyGUI::FloatCoord(geometry.x, geometry.y, geometry.z, geometry.w), MyGUI::Align::HCenter, "Overlapped");
                this->fogOfWarWidget->setImageTexture(this->fogOfWarTexture->getNameStr());

                if (false == this->persistDiscovery->getBool())
                {
                    this->clearFogOfWar();
                }
            }

            if (true == this->wholeSceneVisible->getBool() && false == this->targetMarkerImage->getString().empty())
            {
                // Small marker icon for the tracked target's position on the baked overview, created as a child of
                // the minimap widget (and above the fog of war widget) so it is positioned in the same pixel space
                // and never hidden by either. This is the standard "blip on the map" approach -- the target is
                // never baked into minimapTexture itself.
                this->targetMarkerWidget = this->minimapWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 32, 32), MyGUI::Align::Default);
                this->targetMarkerWidget->setImageTexture(this->targetMarkerImage->getString());
                this->targetMarkerWidget->setNeedMouseFocus(false);
                this->targetMarkerWidget->setDepth(2);
            }
            else if (true == this->wholeSceneVisible->getBool())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() +
                        "' TargetMarkerImage is empty, no marker will be shown on the overview map for the tracked target. Set TargetMarkerImage to an icon in the MyGui/Minimap folder (same place as MinimapMask) to enable it.");
            }

            // Generic compass objects (e.g. the player's ship, a quest objective, an NPC) -- valid in BOTH modes:
            // true-position blips on the baked overview when WholeSceneVisible is true (the bake covers the whole
            // sphere, so any of them are always representable, no matter how far away), or fixed-radius rim markers
            // in follow mode. One marker + one distance label per configured compass object.
            this->generateCompassObjects();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(setupRdCmd), "PlanetMinimapComponent::setupPlanetMinimap");
    }

    Ogre::TextureGpu* PlanetMinimapComponent::createMinimapTexture(const Ogre::String& name, unsigned int width, unsigned int height)
    {
        // Threadsafe from the outside
        Ogre::TextureGpu* texture = nullptr;
        if (false == this->textureManager->hasTextureResource(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
        {
            if (false == this->transparent->getBool())
            {
                texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
                texture->setResolution(width, height);
                texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
                texture->setNumMipmaps(1u);
                // MUST use _transitionTo (immediate/synchronous), NOT scheduleTransitionTo (async, queued to the
                // TextureGpuManager streaming worker thread). In WholeSceneVisible mode this texture is written via
                // a staging texture immediately afterwards (see bakePlanetOverviewTexture), in the SAME render
                // command -- scheduleTransitionTo would leave the texture not-yet-actually-resident at that point,
                // so the write either silently fails or lands on an invalid resource, which displays as a blank/
                // white texture. Exactly matches PlanetTerra::createBlendWeightTexture's own documented pattern for
                // a texture meant to be painted via staging texture right after creation.
                texture->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
                texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
            }
            else
            {
                texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::Discard, Ogre::TextureFlags::RenderToTexture | Ogre::TextureFlags::AllowAutomipmaps, Ogre::TextureTypes::Type2D);
                texture->setResolution(width, height);
                texture->setNumMipmaps(Ogre::PixelFormatGpuUtils::getMaxMipmapCount(width, height));
                texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
                texture->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
                texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
            }
        }
        else
        {
            texture = this->textureManager->findTextureNoThrow(name);
        }
        return texture;
    }

    Ogre::TextureGpu* PlanetMinimapComponent::createFogOfWarTexture(const Ogre::String& name, unsigned int width, unsigned int height)
    {
        Ogre::TextureGpu* texture = nullptr;
        if (false == this->textureManager->hasTextureResource(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME))
        {
            texture = this->textureManager->createTexture(name, Ogre::GpuPageOutStrategy::SaveToSystemRam, Ogre::TextureFlags::RenderToTexture, Ogre::TextureTypes::Type2D);
            texture->setResolution(width, height);
            texture->setPixelFormat(Ogre::PFG_RGBA8_UNORM_SRGB);
            texture->setNumMipmaps(1u);
            // Same fix as createMinimapTexture above -- this texture is also written immediately via staging
            // texture (clearFogOfWar/updateFogOfWarTexture), so it needs the synchronous transition too. This was
            // silently working before mostly by luck: a not-yet-resident fog-of-war texture defaulting to
            // undefined/black content happened to look correct for the "fully unexplored" starting state, masking
            // the same underlying race that breaks the (non-black) planet overview bake outright.
            texture->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
            texture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }
        else
        {
            texture = this->textureManager->findTextureNoThrow(name);
        }
        return texture;
    }

    void PlanetMinimapComponent::createRoundMinimapWorkspace(void)
    {
        // Threadsafe from the outside
        Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

        this->minimapNodeName = "PlanetMinimapNode_Round";
        if (false == compositorManager->hasNodeDefinition(this->minimapNodeName))
        {
            Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition(this->minimapNodeName);

            nodeDef->addTextureSourceName("rt0", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

            // Texture definition
            Ogre::TextureDefinitionBase::TextureDefinition* texDef = nodeDef->addTextureDefinition("PlanetMinimapRT");
            texDef->width = 1024;
            texDef->height = 1024;
            texDef->format = Ogre::PFG_RGBA8_UNORM_SRGB;

            Ogre::RenderTargetViewDef* rtv = nodeDef->addRenderTextureView("PlanetMinimapRT");
            Ogre::RenderTargetViewEntry attachment;
            attachment.textureName = "PlanetMinimapRT";
            rtv->colourAttachments.push_back(attachment);

            nodeDef->setNumTargetPass(2);
            {
                Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("PlanetMinimapRT");
                targetDef->setNumPasses(2);
                {
                    // Clear Pass
                    {
                        Ogre::CompositorPassClearDef* passClear;
                        passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

                        passClear->mClearColour[0] = Ogre::ColourValue(0.0f, 0.0f, 0.0f);
                        passClear->setAllStoreActions(Ogre::StoreAction::Store);
                    }

                    {
                        // Scene pass for planet minimap
                        Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
                        passScene->mCameraName = this->cameraComponent->getCamera()->getName();

                        passScene->mClearColour[0] = Ogre::ColourValue::Black;

                        passScene->setAllStoreActions(Ogre::StoreAction::Store);

                        // Sets the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
                        unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->cameraComponent->getExcludeRenderCategories());

                        // Always exclude procedural grass/trees from the planet minimap -- it serves no navigational
                        // purpose overhead and costs significant GPU time (wind shader, alpha hash).
                        finalRenderMask &= ~NOWA::VISIBILITY_FLAG_GRASS;
                        finalRenderMask &= ~NOWA::VISIBILITY_FLAG_TREE;

                        passScene->setVisibilityMask(finalRenderMask);

                        passScene->mIncludeOverlays = false;
                    }
                }

                targetDef = nodeDef->addTargetPass("rt0");
                targetDef->setNumPasses(1);
                {
                    // Render Quad
                    {
                        Ogre::CompositorPassQuadDef* passQuad;
                        passQuad = static_cast<Ogre::CompositorPassQuadDef*>(targetDef->addPass(Ogre::PASS_QUAD));

                        passQuad->addQuadTextureSource(0, "PlanetMinimapRT");
                        passQuad->mMaterialName = "MinimapMask";
                    }
                }
            }
            nodeDef->setNumOutputChannels(1);
            nodeDef->mapOutputChannel(0, "PlanetMinimapRT");
        }

        this->minimapWorkspaceName = "PlanetMinimapWorkspaceWithFoW_Round";
        if (false == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
        {
            Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(this->minimapWorkspaceName);
            workspaceDef->clearAllInterNodeConnections();

            workspaceDef->connectExternal(0, this->minimapNodeName, 0);
        }

        this->externalChannels.resize(1);
        this->externalChannels[0] = this->minimapTexture;

        this->workspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->minimapWorkspaceName, true);
    }

    void PlanetMinimapComponent::createMinimapWorkspace(void)
    {
        // Threadsafe from the outside
        Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

        this->minimapNodeName = "PlanetMinimapNode";
        if (false == compositorManager->hasNodeDefinition(this->minimapNodeName))
        {
            Ogre::CompositorNodeDef* nodeDef = compositorManager->addNodeDefinition(this->minimapNodeName);

            nodeDef->addTextureSourceName("PlanetMinimapRT", 0, Ogre::TextureDefinitionBase::TEXTURE_INPUT);

            nodeDef->setNumTargetPass(1);
            {
                Ogre::CompositorTargetDef* targetDef = nodeDef->addTargetPass("PlanetMinimapRT");
                targetDef->setNumPasses(2);
                {
                    // Clear Pass
                    {
                        Ogre::CompositorPassClearDef* passClear;
                        passClear = static_cast<Ogre::CompositorPassClearDef*>(targetDef->addPass(Ogre::PASS_CLEAR));

                        passClear->mClearColour[0] = Ogre::ColourValue(0.0f, 0.0f, 0.0f);
                        passClear->setAllStoreActions(Ogre::StoreAction::Store);
                    }

                    {
                        // Scene pass for planet minimap
                        Ogre::CompositorPassSceneDef* passScene = static_cast<Ogre::CompositorPassSceneDef*>(targetDef->addPass(Ogre::PASS_SCENE));
                        passScene->mCameraName = this->cameraComponent->getCamera()->getName();

                        passScene->mClearColour[0] = Ogre::ColourValue::Black;

                        passScene->setAllStoreActions(Ogre::StoreAction::Store);

                        // Sets the corresponding render category. All game objects which do not match that category, will not be rendered for this camera
                        unsigned int finalRenderMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateRenderCategoryId(this->cameraComponent->getExcludeRenderCategories());

                        finalRenderMask &= ~NOWA::VISIBILITY_FLAG_GRASS;
                        finalRenderMask &= ~NOWA::VISIBILITY_FLAG_TREE;

                        passScene->setVisibilityMask(finalRenderMask);

                        passScene->mIncludeOverlays = false;
                    }
                }
            }
            nodeDef->mapOutputChannel(0, "PlanetMinimapRT");
        }

        this->minimapWorkspaceName = "PlanetMinimapWorkspaceWithFoW";

        if (false == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
        {
            Ogre::CompositorWorkspaceDef* workspaceDef = compositorManager->addWorkspaceDefinition(this->minimapWorkspaceName);
            workspaceDef->clearAllInterNodeConnections();

            workspaceDef->connectExternal(0, this->minimapNodeName, 0);
        }

        this->externalChannels.resize(1);
        this->externalChannels[0] = this->minimapTexture;

        this->workspace = compositorManager->addWorkspace(this->gameObjectPtr->getSceneManager(), this->externalChannels, this->cameraComponent->getCamera(), this->minimapWorkspaceName, true);
    }

    void PlanetMinimapComponent::clearFogOfWar(void)
    {
        // Threadsafe from the outside
        this->discoveryState.clear();
        this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));

        unsigned int texSize = this->textureSize->getUInt();

        this->fogOfWarStagingTexture->startMapRegion();
        Ogre::TextureBox texBox = this->fogOfWarStagingTexture->mapRegion(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());

        const size_t bytesPerPixel = texBox.bytesPerPixel;

        for (Ogre::uint32 y = 0; y < texSize; y++)
        {
            Ogre::uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<Ogre::uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
            for (Ogre::uint32 x = 0; x < texSize; x++)
            {
                const size_t dstIdx = x * bytesPerPixel;
                float rgba[4];
                // Create blank texture without details. NOTE: packColour expects [0,1] floats, not [0,255] bytes
                // -- this is the convention PlanetTerra::createBlendWeightTexture itself uses.
                rgba[0] = 0.0f;
                rgba[1] = 0.0f;
                rgba[2] = 0.0f;
                rgba[3] = 1.0f;
                Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
            }
        }

        this->fogOfWarStagingTexture->stopMapRegion();
        this->fogOfWarStagingTexture->upload(texBox, this->fogOfWarTexture, 0u);
    }

    void PlanetMinimapComponent::updateFogOfWarTexture(const Ogre::Vector3& position, const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetDefaultDirection, Ogre::Real radius)
    {
        if (nullptr == this->fogOfWarStagingTexture || nullptr == this->planetTerraComponent || nullptr == this->planetGameObject)
        {
            return;
        }

        // Threadsafe from the outside
        unsigned int texSize = this->textureSize->getUInt();

        const Ogre::Vector3 planetCenter = this->planetGameObject->getPosition();
        Ogre::Vector3 direction = position - planetCenter;
        if (direction.squaredLength() < 0.000001f)
        {
            return;
        }
        direction.normalise();

        const Ogre::Vector2 targetUV = this->worldDirectionToUV(direction);
        const Ogre::Vector2 targetPixel = this->uvToPixelCoordinates(targetUV, texSize, texSize);

        // Converts the world-space visibility radius (a linear distance) into an angular radius on the sphere
        // (s = r*theta -> theta = s/planetRadius), then into pixels along the v-axis, which spans phi in [0, PI]
        // uniformly. The SAME pixel radius is then also used along the u-axis below; this is the standard
        // equirectangular-projection distortion (longitude lines converge toward the poles, same reason Greenland
        // looks huge on a Mercator map) -- acceptable for a minimap, but worth knowing if the circle looks
        // stretched near a planet's poles.
        const Ogre::Real planetRadius = this->planetTerraComponent->getRadius();
        const Ogre::Real angularRadiusRad = (planetRadius > 0.0001f) ? (this->visibilityRadius->getReal() / planetRadius) : 0.0f;
        int radiusInPixels = static_cast<int>((angularRadiusRad / Ogre::Math::PI) * static_cast<float>(texSize));

        // IMPORTANT: at planet scale, VisibilityRadius (a world-unit/meter distance) converts to a TINY angular
        // fraction of the sphere -- e.g. VisibilityRadius=5 on a radius-2618 planet at texSize=256 works out to
        // radiusInPixels=0, meaning "distance < radiusInPixels" is never true for ANY pixel (distance is always
        // >= 0), so literally nothing ever gets revealed -- the fog stays fully opaque everywhere, even directly
        // under the player. Floor it to a minimum of 2 pixels so there is always at least a visible reveal dot;
        // increase VisibilityRadius (proportional to planetRadius, not to a flat scene's scale) for a bigger one.
        if (radiusInPixels < 2)
        {
            if (true == this->bShowDebugData)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' updateFogOfWarTexture: computed radiusInPixels=" + Ogre::StringConverter::toString(radiusInPixels) +
                        " (VisibilityRadius=" + Ogre::StringConverter::toString(this->visibilityRadius->getReal()) + " is far too small relative to planetRadius=" + Ogre::StringConverter::toString(planetRadius) +
                        " at texSize=" + Ogre::StringConverter::toString(texSize) + ") -- flooring to 2 pixels so something is actually revealed. Increase VisibilityRadius for a larger reveal area.");
            }
            radiusInPixels = 2;
        }

        bool visibilitySpotlight = this->useVisibilitySpotlight->getBool();

        // Determines spotlight direction. Same convention as MinimapComponent -- this is only valid in
        // WholeSceneVisible mode, where the target is projected onto world X/Z exactly like a flat scene.
        // onto its own local tangent plane -- same basis computeTangentHeadingDegrees uses for RotateMinimap.
        Ogre::Vector2 spotlightDirection2D = Ogre::Vector2::ZERO;
        if (true == visibilitySpotlight)
        {
            Ogre::Vector3 localNorth = Ogre::Vector3::UNIT_Y - direction * Ogre::Vector3::UNIT_Y.dotProduct(direction);
            if (localNorth.squaredLength() < 0.000001f)
            {
                localNorth = Ogre::Vector3::UNIT_Z - direction * Ogre::Vector3::UNIT_Z.dotProduct(direction);
            }
            localNorth.normalise();
            Ogre::Vector3 localEast = localNorth.crossProduct(direction);
            localEast.normalise();

            Ogre::Vector3 worldForward = targetOrientation * targetDefaultDirection;
            Ogre::Vector3 forwardTangent = worldForward - direction * worldForward.dotProduct(direction);
            if (forwardTangent.squaredLength() < 0.000001f)
            {
                forwardTangent = localNorth;
            }
            forwardTangent.normalise();

            spotlightDirection2D.x = forwardTangent.dotProduct(localEast);
            spotlightDirection2D.y = -forwardTangent.dotProduct(localNorth);
            if (spotlightDirection2D.squaredLength() > 0.000001f)
            {
                spotlightDirection2D.normalise();
            }
        }

        this->fogOfWarStagingTexture->startMapRegion();
        Ogre::TextureBox texBox = this->fogOfWarStagingTexture->mapRegion(this->fogOfWarTexture->getWidth(), this->fogOfWarTexture->getHeight(), 1u, 1u, this->fogOfWarTexture->getPixelFormat());

        const size_t bytesPerPixel = texBox.bytesPerPixel;

        float padding = 0.0f;
        if (false == this->minimapMask->getString().empty())
        {
            padding = 2.0f;
        }

        if (false == this->useDiscovery->getBool())
        {
            for (Ogre::uint32 y = 0; y < texSize; y++)
            {
                Ogre::uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<Ogre::uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (Ogre::uint32 x = 0; x < texSize; x++)
                {
                    float dx = static_cast<float>(x) - targetPixel.x;
                    float dy = static_cast<float>(y) - targetPixel.y;
                    float distance = sqrt(dx * dx + dy * dy);

                    float distanceRound = 0.0f;
                    if (true == this->useRoundMinimap->getBool())
                    {
                        float rx = x - (static_cast<float>(texSize) * 0.5f);
                        float ry = y - (static_cast<float>(texSize) * 0.5f);
                        distanceRound = sqrt(rx * rx + ry * ry);
                    }

                    if (distance < radiusInPixels || distanceRound >= (static_cast<float>(texSize) * 0.5f) - padding)
                    {
                        const size_t dstIdx = x * bytesPerPixel;
                        float rgba[4] = {1.0f, 0.0f, 0.0f, 0.0f};
                        Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
                    }
                    else
                    {
                        const size_t dstIdx = x * bytesPerPixel;
                        float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                        Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
                    }
                }
            }
        }
        else
        {
            for (Ogre::uint32 y = 0; y < this->fogOfWarTexture->getHeight(); y++)
            {
                Ogre::uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<Ogre::uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
                for (Ogre::uint32 x = 0; x < this->fogOfWarTexture->getWidth(); x++)
                {
                    float dx = static_cast<float>(x) - targetPixel.x;
                    float dy = static_cast<float>(y) - targetPixel.y;
                    float distance = sqrt(dx * dx + dy * dy);

                    float distanceRound = 0.0f;
                    if (true == this->useRoundMinimap->getBool())
                    {
                        float rx = x - (static_cast<float>(texSize) * 0.5f);
                        float ry = y - (static_cast<float>(texSize) * 0.5f);
                        distanceRound = sqrt(rx * rx + ry * ry);
                    }

                    if ((distance < radiusInPixels) || (distanceRound >= (static_cast<float>(texSize) * 0.5f) - padding) || true == this->discoveryState[x][y])
                    {
                        if (true == visibilitySpotlight)
                        {
                            Ogre::Vector2 pointDirection2D(dx, dy);
                            if (pointDirection2D.squaredLength() > 0.000001f)
                            {
                                pointDirection2D.normalise();
                                Ogre::Real angle = Ogre::Math::ACos(Ogre::Math::Clamp(spotlightDirection2D.dotProduct(pointDirection2D), -1.0f, 1.0f)).valueDegrees();
                                if (angle <= 45.0f / 2)
                                {
                                    this->discoveryState[x][y] = true;
                                    const size_t dstIdx = x * bytesPerPixel;
                                    float rgba[4] = {1.0f, 0.0f, 0.0f, 0.0f};
                                    Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
                                }
                            }
                        }
                        else
                        {
                            this->discoveryState[x][y] = true;
                            const size_t dstIdx = x * bytesPerPixel;
                            float rgba[4] = {1.0f, 0.0f, 0.0f, 0.0f};
                            Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
                        }
                    }
                    else
                    {
                        const size_t dstIdx = x * bytesPerPixel;
                        float rgba[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                        Ogre::PixelFormatGpuUtils::packColour(rgba, this->fogOfWarTexture->getPixelFormat(), &pixBoxData[dstIdx]);
                    }
                }
            }
        }

        this->fogOfWarStagingTexture->stopMapRegion();
        this->fogOfWarStagingTexture->upload(texBox, this->fogOfWarTexture, 0u);
    }

    void PlanetMinimapComponent::removeWorkspace(void)
    {
        // Projects the target's world X/Z onto the fixed overhead bounds (planetCenter +/- maxRadius), exactly like
        // MinimapComponent does for a flat scene's bounding box. Only valid in WholeSceneVisible mode.

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr != this->workspace)
            {
                Ogre::CompositorManager2* compositorManager = WorkspaceModule::getInstance()->getCompositorManager();

                compositorManager->removeWorkspace(this->workspace);

                // Note: Each time addWorkspaceDefinition is called, an "AutoGen Hash..." node is internally created by Ogre, this one must also be removed, else a crash occurs (bug in Ogre?)
                if (true == compositorManager->hasNodeDefinition(this->minimapNodeName))
                {
                    compositorManager->removeNodeDefinition(this->minimapNodeName);
                }
                if (true == compositorManager->hasWorkspaceDefinition(this->minimapWorkspaceName))
                {
                    compositorManager->removeWorkspaceDefinition(this->minimapWorkspaceName);
                }

                this->workspace = nullptr;
                this->externalChannels.clear();
            }

            if (nullptr != this->targetMarkerWidget)
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->targetMarkerWidget);
                this->targetMarkerWidget = nullptr;
            }

            for (size_t i = 0; i < this->compassObjectWidgets.size(); i++)
            {
                if (nullptr != this->compassObjectWidgets[i])
                {
                    MyGUI::Gui::getInstancePtr()->destroyWidget(this->compassObjectWidgets[i]);
                    this->compassObjectWidgets[i] = nullptr;
                }
            }
            this->compassObjectWidgets.clear();

            for (size_t i = 0; i < this->compassObjectDistanceTexts.size(); i++)
            {
                if (nullptr != this->compassObjectDistanceTexts[i])
                {
                    MyGUI::Gui::getInstancePtr()->destroyWidget(this->compassObjectDistanceTexts[i]);
                    this->compassObjectDistanceTexts[i] = nullptr;
                }
            }
            this->compassObjectDistanceTexts.clear();

            if (nullptr != this->maskWidget)
            {
                // Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
                auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->minimapMask->getString());
                if (nullptr != myGuiTexture)
                {
                    static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
                }
                this->maskWidget->setImageTexture("");
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->maskWidget);
                this->maskWidget = nullptr;
            }
            if (nullptr != this->minimapWidget)
            {
                // Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
                if (nullptr != this->minimapTexture)
                {
                    auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->minimapTexture->getNameStr());
                    if (nullptr != myGuiTexture)
                    {
                        static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
                    }
                }
                this->minimapWidget->setImageTexture("");
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->minimapWidget);
                this->minimapWidget = nullptr;
            }
            if (nullptr != this->fogOfWarWidget)
            {
                // Must be done, because mygui also holds a reference to the Ogre::TextureGpu texture.
                if (nullptr != this->fogOfWarTexture)
                {
                    auto myGuiTexture = MyGUI::RenderManager::getInstancePtr()->getTexture(this->fogOfWarTexture->getNameStr());
                    if (nullptr != myGuiTexture)
                    {
                        static_cast<MyGUI::Ogre2RenderManager*>(MyGUI::RenderManager::getInstancePtr())->removeTexture(myGuiTexture);
                    }
                }
                this->fogOfWarWidget->setImageTexture("");
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->fogOfWarWidget);
                this->fogOfWarWidget = nullptr;
            }

            if (nullptr != this->fogOfWarStagingTexture)
            {
                this->textureManager->removeStagingTexture(this->fogOfWarStagingTexture);
                this->fogOfWarStagingTexture = nullptr;
            }
            if (nullptr != this->minimapStagingTexture)
            {
                this->textureManager->removeStagingTexture(this->minimapStagingTexture);
                this->minimapStagingTexture = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::removeWorkspace");
    }

    Ogre::Vector2 PlanetMinimapComponent::worldDirectionToUV(const Ogre::Vector3& direction) const
    {
        Ogre::Vector3 dir = direction;

        // PlanetTerraComponent::mousePressed transforms the world-space hit position into the planet's LOCAL space
        // (nodeInv * hitPos) before deriving theta/phi for painting -- so this must too, or the bake/markers would
        // drift out of alignment with the actually-painted layers as soon as a planet has any non-trivial rotation
        // (a tilted axis, or simply spinning). Direction vectors aren't affected by translation, only rotation, so
        // the inverse of the planet's world ORIENTATION is sufficient here (scale is assumed uniform/1, matching
        // PlanetTerraComponent's own assumption).
        if (nullptr != this->planetGameObject)
        {
            dir = this->planetGameObject->getOrientation().Inverse() * dir;
        }
        dir.normalise();

        // Same inverse mapping PlanetTerra::sampleHeightAndNormalAtDirection uses internally, kept identical on
        // purpose so this always lines up with PlanetTerra's own UV parameterization (see
        // PlanetTerra::generateBaseSphere): dir.y = cos(phi) -> phi = acos(dir.y); dir.x = sin(phi)*cos(theta),
        // dir.z = sin(phi)*sin(theta) -> theta = atan2(dir.z, dir.x), wrapped into [0, 2*PI).
        const float phi = Ogre::Math::ACos(Ogre::Math::Clamp(dir.y, -1.0f, 1.0f)).valueRadians();
        float theta = Ogre::Math::ATan2(dir.z, dir.x).valueRadians();
        if (theta < 0.0f)
        {
            theta += Ogre::Math::TWO_PI;
        }

        Ogre::Vector2 uv;
        uv.x = theta / Ogre::Math::TWO_PI;
        uv.y = phi / Ogre::Math::PI;
        return uv;
    }

    Ogre::Vector2 PlanetMinimapComponent::uvToPixelCoordinates(const Ogre::Vector2& uv, unsigned int textureWidth, unsigned int textureHeight) const
    {
        Ogre::Vector2 pixel;
        pixel.x = uv.x * static_cast<float>(textureWidth);
        pixel.y = uv.y * static_cast<float>(textureHeight);
        return pixel;
    }

    void PlanetMinimapComponent::bakePlanetOverviewTexture(void)
    {
        // Threadsafe from the outside, same convention as updateFogOfWarTexture/clearFogOfWar -- call only from
        // inside a GraphicsModule render command, since this writes via a staging texture.
        if (nullptr == this->minimapTexture || nullptr == this->planetTerraComponent || nullptr == this->planetGameObject)
        {
            return;
        }

        const unsigned int texSize = this->textureSize->getUInt();
        const Ogre::Vector3 planetCenter = this->planetGameObject->getPosition();
        const Ogre::Real baseRadius = this->planetTerraComponent->getRadius();

        // Pulls the same CPU blend weight data PlanetTerra paints with (R/G/B/A = layer 0..3 weight, see
        // PlanetTerra::applyPaintBrush), so the overview reflects what is ACTUALLY painted on the planet instead of
        // an arbitrary height-based tint. Requires PlanetTerraComponent::getPlanetTerra() to expose the underlying
        // PlanetTerra instance -- if that accessor does not exist yet, add it as a one-line public getter:
        //   PlanetTerra* getPlanetTerra(void) const { return this->planet; }
        // (member name assumed from PlanetTerraComponent.cpp's own internal usage, e.g. this->planet->applyPaintBrush).
        PlanetTerra* planetTerra = this->planetTerraComponent->getPlanetTerra();
        std::vector<uint8_t> blendData;
        int blendTexSize = 0;
        bool hasBlendData = false;
        if (nullptr != planetTerra)
        {
            blendTexSize = planetTerra->getBlendTexSize();
            if (blendTexSize > 0)
            {
                blendData = planetTerra->getBlendDataCopy();
                hasBlendData = (false == blendData.empty()) && (blendData.size() >= static_cast<size_t>(blendTexSize) * blendTexSize * 4u);
            }
        }
        if (false == hasBlendData)
        {
            if (true == this->bShowDebugData)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() +
                        "' bakePlanetOverviewTexture: no usable blend weight data from PlanetTerra (getPlanetTerra() missing/null, or blend texture not yet created) -- falling back to BaseTerrainColor only, with no layer painting shown.");
            }
        }
        else if (true == this->bShowDebugData)
        {
            // Direct proof of what actually arrived: the center pixel's raw RGBA bytes, plus a count of how many
            // bytes in the WHOLE array are non-zero. If everything is 0 (R=G=B=A=0 everywhere, default per
            // PlanetTerra::createBlendWeightTexture's "no detail layer active" initial state), the planet was
            // simply never painted -- that's a content/workflow question, not a bug in this bake function. If the
            // center sample or the non-zero count show real data but the minimap still renders white, the problem
            // is downstream of this point (GPU upload/display), not the CPU-side data itself.
            const size_t centerX = static_cast<size_t>(blendTexSize) / 2u;
            const size_t centerY = static_cast<size_t>(blendTexSize) / 2u;
            const size_t centerIdx = (centerY * static_cast<size_t>(blendTexSize) + centerX) * 4u;

            size_t nonZeroByteCount = 0u;
            for (size_t i = 0u; i < blendData.size(); ++i)
            {
                if (0u != blendData[i])
                {
                    ++nonZeroByteCount;
                }
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' bakePlanetOverviewTexture: blendData.size()=" + Ogre::StringConverter::toString(blendData.size()) + " blendTexSize=" + Ogre::StringConverter::toString(blendTexSize) +
                    " centerPixel(R,G,B,A)=" + Ogre::StringConverter::toString(static_cast<int>(blendData[centerIdx + 0u])) + "," + Ogre::StringConverter::toString(static_cast<int>(blendData[centerIdx + 1u])) + "," +
                    Ogre::StringConverter::toString(static_cast<int>(blendData[centerIdx + 2u])) + "," + Ogre::StringConverter::toString(static_cast<int>(blendData[centerIdx + 3u])) +
                    " nonZeroByteCount=" + Ogre::StringConverter::toString(nonZeroByteCount) + " / " + Ogre::StringConverter::toString(blendData.size()) +
                    (0u == nonZeroByteCount ? " (ALL ZERO -- planet has never been painted with any layer, this is expected base-diffuse-only content, not a bug)" : " (real paint data present)"));
        }

        // First pass: inverse-map every pixel's (u,v) to a world direction and sample its height via the same
        // public accessor PlanetTerraComponent already exposes for foliage placement -- this is the "unwrap" step,
        // reading PlanetTerra's existing per-direction height/normal data into a flat equirectangular grid exactly
        // like PlanetTerra's own blend weight texture is already organised. Track min/max so the second pass can
        // normalise the height-based shading regardless of how tall/deep this particular planet's terrain is.
        std::vector<float> heights(static_cast<size_t>(texSize) * texSize, 0.0f);
        float minHeight = std::numeric_limits<float>::max();
        float maxHeight = std::numeric_limits<float>::lowest();

        for (unsigned int y = 0u; y < texSize; ++y)
        {
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(texSize);
            const float phi = v * Ogre::Math::PI;
            for (unsigned int x = 0u; x < texSize; ++x)
            {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(texSize);
                const float theta = u * Ogre::Math::TWO_PI;

                Ogre::Vector3 dir;
                dir.x = Ogre::Math::Sin(phi) * Ogre::Math::Cos(theta);
                dir.y = Ogre::Math::Cos(phi);
                dir.z = Ogre::Math::Sin(phi) * Ogre::Math::Sin(theta);

                Ogre::Vector3 worldPos = Ogre::Vector3::ZERO;
                Ogre::Vector3 worldNormal = Ogre::Vector3::ZERO;
                float height = 0.0f;
                if (true == this->planetTerraComponent->sampleHeightAndNormalAtDirection(dir, worldPos, worldNormal))
                {
                    height = (worldPos - planetCenter).length() - baseRadius;
                }

                heights[static_cast<size_t>(y) * texSize + x] = height;
                if (height < minHeight)
                {
                    minHeight = height;
                }
                if (height > maxHeight)
                {
                    maxHeight = height;
                }
            }
        }

        Ogre::Real heightRange = maxHeight - minHeight;
        if (heightRange < 0.0001f)
        {
            heightRange = 0.0001f;
        }

        const bool round = this->useRoundMinimap->getBool();
        const float padding = (false == this->minimapMask->getString().empty()) ? 2.0f : 0.0f;

        const Ogre::Vector3 baseColorVec = this->baseTerrainColor->getVector3();
        const Ogre::Vector3 layerColorVec[4] = {this->layer0Color->getVector3(), this->layer1Color->getVector3(), this->layer2Color->getVector3(), this->layer3Color->getVector3()};

        // Same robustness guards PlanetTerra::uploadBlendData uses, which this code was missing: re-check residency
        // (a texture can theoretically drop residency between creation and this bake call) and validate the staging
        // texture's actual byte size against what this exact upload needs, recreating it if Ogre's staging pool
        // handed back a differently-sized one than requested. A silent size mismatch here would write an
        // incomplete/garbled region without any error, which can present as a blank/white result.
        if (this->minimapTexture->getResidencyStatus() != Ogre::GpuResidency::Resident)
        {
            this->minimapTexture->_transitionTo(Ogre::GpuResidency::Resident, reinterpret_cast<Ogre::uint8*>(0));
            this->minimapTexture->_setNextResidencyStatus(Ogre::GpuResidency::Resident);
        }

        if (nullptr == this->minimapStagingTexture)
        {
            this->minimapStagingTexture = this->textureManager->getStagingTexture(this->minimapTexture->getWidth(), this->minimapTexture->getHeight(), 1u, 1u, this->minimapTexture->getPixelFormat());
        }

        this->minimapStagingTexture->startMapRegion();
        Ogre::TextureBox texBox = this->minimapStagingTexture->mapRegion(this->minimapTexture->getWidth(), this->minimapTexture->getHeight(), 1u, 1u, this->minimapTexture->getPixelFormat());
        const size_t bytesPerPixel = texBox.bytesPerPixel;

        // Second pass: write the blend-layer-coloured, height-shaded, optionally round-masked equirectangular image.
        for (unsigned int y = 0u; y < texSize; ++y)
        {
            Ogre::uint8* RESTRICT_ALIAS pixBoxData = reinterpret_cast<Ogre::uint8 * RESTRICT_ALIAS>(texBox.at(0, y, 0));
            for (unsigned int x = 0u; x < texSize; ++x)
            {
                // Same (u,v) the first pass used to derive the sample direction -- re-derived here rather than
                // stored, since it is cheap and keeps the height array single-purpose.
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(texSize);
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(texSize);

                float r = baseColorVec.x;
                float g = baseColorVec.y;
                float b = baseColorVec.z;

                if (true == hasBlendData)
                {
                    // blendTexSize is independent of texSize (the minimap's own resolution) -- (u,v) is
                    // resolution-agnostic, so it is rescaled separately into blend-pixel-space here. Indexing
                    // matches PlanetTerraComponent::mousePressed's hitUV -> blendData lookup exactly (verified
                    // against PlanetTerra::applyPaintBrush: cx = hitUV.x * blendTexSize, cy = hitUV.y * blendTexSize),
                    // so this lines up pixel-for-pixel with what was actually painted.
                    int bx = static_cast<int>(u * static_cast<float>(blendTexSize));
                    int by = static_cast<int>(v * static_cast<float>(blendTexSize));
                    if (bx < 0)
                    {
                        bx = 0;
                    }
                    if (bx >= blendTexSize)
                    {
                        bx = blendTexSize - 1;
                    }
                    if (by < 0)
                    {
                        by = 0;
                    }
                    if (by >= blendTexSize)
                    {
                        by = blendTexSize - 1;
                    }

                    const size_t blendIdx = (static_cast<size_t>(by) * static_cast<size_t>(blendTexSize) + static_cast<size_t>(bx)) * 4u;
                    const float w0 = static_cast<float>(blendData[blendIdx + 0u]) / 255.0f;
                    const float w1 = static_cast<float>(blendData[blendIdx + 1u]) / 255.0f;
                    const float w2 = static_cast<float>(blendData[blendIdx + 2u]) / 255.0f;
                    const float w3 = static_cast<float>(blendData[blendIdx + 3u]) / 255.0f;

                    const float totalWeight = w0 + w1 + w2 + w3;
                    if (totalWeight > 0.0001f)
                    {
                        const float invTotal = 1.0f / totalWeight;
                        const float blendR = (w0 * layerColorVec[0].x + w1 * layerColorVec[1].x + w2 * layerColorVec[2].x + w3 * layerColorVec[3].x) * invTotal;
                        const float blendG = (w0 * layerColorVec[0].y + w1 * layerColorVec[1].y + w2 * layerColorVec[2].y + w3 * layerColorVec[3].y) * invTotal;
                        const float blendB = (w0 * layerColorVec[0].z + w1 * layerColorVec[1].z + w2 * layerColorVec[2].z + w3 * layerColorVec[3].z) * invTotal;

                        // Where totalWeight is small, the base diffuse still dominates (PlanetTerra shows base where
                        // no layer is painted); where it approaches/exceeds 1, the layer-mix colour fully replaces
                        // the base, matching how the blend weight texture drives the actual material on the planet.
                        const float totalWeightClamped = (totalWeight < 1.0f) ? totalWeight : 1.0f;
                        r = Ogre::Math::lerp(baseColorVec.x, blendR, totalWeightClamped);
                        g = Ogre::Math::lerp(baseColorVec.y, blendG, totalWeightClamped);
                        b = Ogre::Math::lerp(baseColorVec.z, blendB, totalWeightClamped);
                    }
                }

                // Mild height-based brightness modulation on top of the layer colour, purely for terrain relief
                // readability (valleys slightly darker, peaks slightly brighter) -- secondary to the blend-layer
                // colour now, not the primary signal anymore.
                const float normalizedHeight = (heights[static_cast<size_t>(y) * texSize + x] - minHeight) / heightRange;
                const float brightness = 0.85f + 0.3f * normalizedHeight;
                r *= brightness;
                g *= brightness;
                b *= brightness;

                float alpha = 1.0f;
                if (true == round)
                {
                    const float rx = static_cast<float>(x) - (static_cast<float>(texSize) * 0.5f);
                    const float ry = static_cast<float>(y) - (static_cast<float>(texSize) * 0.5f);
                    const float distanceRound = std::sqrt(rx * rx + ry * ry);
                    if (distanceRound >= (static_cast<float>(texSize) * 0.5f) - padding)
                    {
                        alpha = 0.0f;
                    }
                }

                const size_t dstIdx = x * bytesPerPixel;
                // NOTE: packColour expects [0,1] floats, not [0,255] bytes -- this is the convention
                // PlanetTerra::createBlendWeightTexture/applyPaintBrush themselves use. An earlier version of this
                // function multiplied by 255 here, which saturated every channel toward the format's maximum during
                // packing -- the actual root cause of every "white background" report against this bake function,
                // independent of blend data, residency, or upload() signature (all of which were red herrings).
                float rgba[4] = {Ogre::Math::Clamp(r, 0.0f, 1.0f), Ogre::Math::Clamp(g, 0.0f, 1.0f), Ogre::Math::Clamp(b, 0.0f, 1.0f), alpha};
                Ogre::PixelFormatGpuUtils::packColour(rgba, this->minimapTexture->getPixelFormat(), &pixBoxData[dstIdx]);
            }
        }

        this->minimapStagingTexture->stopMapRegion();
        this->minimapStagingTexture->upload(texBox, this->minimapTexture, 0u);

        if (true == this->bShowDebugData)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' baked planet overview: planet='" + this->planetGameObject->getName() +
                                                                                    "' texSize=" + Ogre::StringConverter::toString(texSize) + " baseRadius=" + Ogre::StringConverter::toString(baseRadius) +
                                                                                    " minHeight=" + Ogre::StringConverter::toString(minHeight) + " maxHeight=" + Ogre::StringConverter::toString(maxHeight) +
                                                                                    " hasBlendData=" + Ogre::StringConverter::toString(hasBlendData) + " blendTexSize=" + Ogre::StringConverter::toString(blendTexSize));
        }
    }

    void PlanetMinimapComponent::rebakePlanetOverview(void)
    {
        if (nullptr == this->minimapTexture || false == this->wholeSceneVisible->getBool())
        {
            return;
        }
        // Mirrors MinimapComponent::adjustMinimapCamera, but centered on the planet's world position and sized from
        NOWA::GraphicsModule::RenderCommand cmd = [this]
        {
            this->bakePlanetOverviewTexture();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::rebakePlanetOverview");
    }

    void PlanetMinimapComponent::updateOverviewTargetMarker(const Ogre::Vector3& targetWorldPosition)
    {
        // Threadsafe from the outside -- call only from inside a GraphicsModule render command/tracked closure,
        // since this touches a MyGUI widget.
        if (nullptr == this->targetMarkerWidget || nullptr == this->planetGameObject || nullptr == this->minimapWidget)
        {
            return;
        }

        const Ogre::Vector3 planetCenter = this->planetGameObject->getPosition();
        Ogre::Vector3 direction = targetWorldPosition - planetCenter;
        if (direction.squaredLength() < 0.000001f)
        {
            // Degenerate: target sits exactly at the planet's center. Keep the marker at its previous position.
            return;
        }

        const Ogre::Vector2 uv = this->worldDirectionToUV(direction);
        const Ogre::Vector2 pixel = this->uvToPixelCoordinates(uv, this->minimapWidget->getWidth(), this->minimapWidget->getHeight());

        const int markerHalfWidth = static_cast<int>(this->targetMarkerWidget->getWidth() / 2);
        const int markerHalfHeight = static_cast<int>(this->targetMarkerWidget->getHeight() / 2);
        this->targetMarkerWidget->setPosition(static_cast<int>(pixel.x) - markerHalfWidth, static_cast<int>(pixel.y) - markerHalfHeight);
    }

    void PlanetMinimapComponent::updateCompassObjects(const Ogre::Vector3& targetWorldPosition, const Ogre::Vector3& followCameraPosition, const Ogre::Quaternion& followCameraOrientation, Ogre::Real followCameraDistance)
    {
        // Threadsafe from the outside -- call only from inside a GraphicsModule render command/tracked closure,
        // since this touches MyGUI widgets.
        for (unsigned int i = 0; i < this->compassObjectCount->getUInt(); i++)
        {
            this->updateSingleCompassObject(i, targetWorldPosition, followCameraPosition, followCameraOrientation, followCameraDistance);
        }
    }

    void PlanetMinimapComponent::updateSingleCompassObject(unsigned int index, const Ogre::Vector3& targetWorldPosition, const Ogre::Vector3& followCameraPosition, const Ogre::Quaternion& followCameraOrientation, Ogre::Real followCameraDistance)
    {
        if (index >= this->compassObjectWidgets.size() || nullptr == this->compassObjectWidgets[index] || nullptr == this->planetGameObject)
        {
            return;
        }

        MyGUI::ImageBox* markerWidget = this->compassObjectWidgets[index];
        MyGUI::TextBox* distanceWidget = (index < this->compassObjectDistanceTexts.size()) ? this->compassObjectDistanceTexts[index] : nullptr;

        unsigned long compassId = (index < this->compassGameObjectIds.size() && nullptr != this->compassGameObjectIds[index]) ? this->compassGameObjectIds[index]->getULong() : 0ul;
        if (0 == compassId)
        {
            markerWidget->setVisible(false);
            if (nullptr != distanceWidget)
            {
                distanceWidget->setVisible(false);
            }
            return;
        }

        const auto& compassGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(compassId);
        if (nullptr == compassGameObjectPtr)
        {
            // Not currently spawned/already destroyed -- hide rather than show a stale position.
            markerWidget->setVisible(false);
            if (nullptr != distanceWidget)
            {
                distanceWidget->setVisible(false);
            }
            return;
        }

        const Ogre::Vector3 compassWorldPosition = compassGameObjectPtr->getPosition();
        const Ogre::Vector3 planetCenter = this->planetGameObject->getPosition();

        // True distance from the tracked player/target to this compass object, independent of the follow camera's
        // CameraHeight offset -- this is what the player actually cares about ("how far is it"), not the distance
        // from wherever the minimap camera happens to be floating.
        const Ogre::Real distanceMeters = (compassWorldPosition - targetWorldPosition).length();
        const Ogre::String distanceText = this->formatDistanceMeters(distanceMeters);

        if (true == this->wholeSceneVisible->getBool())
        {
            // Overview mode: the bake covers the ENTIRE sphere, so this object is always representable as a true
            // position marker, exactly like updateOverviewTargetMarker -- no "off the edge" case can occur here,
            // unlike follow mode below.
            if (nullptr == this->minimapWidget)
            {
                return;
            }

            Ogre::Vector3 direction = compassWorldPosition - planetCenter;
            if (direction.squaredLength() < 0.000001f)
            {
                markerWidget->setVisible(false);
                if (nullptr != distanceWidget)
                {
                    distanceWidget->setVisible(false);
                }
                return;
            }

            const Ogre::Vector2 uv = this->worldDirectionToUV(direction);
            const Ogre::Vector2 pixel = this->uvToPixelCoordinates(uv, this->minimapWidget->getWidth(), this->minimapWidget->getHeight());

            const int markerHalfWidth = static_cast<int>(markerWidget->getWidth() / 2);
            const int markerHalfHeight = static_cast<int>(markerWidget->getHeight() / 2);
            markerWidget->setVisible(true);
            markerWidget->setPosition(static_cast<int>(pixel.x) - markerHalfWidth, static_cast<int>(pixel.y) - markerHalfHeight);

            if (nullptr != distanceWidget)
            {
                distanceWidget->setCaption(distanceText);
                distanceWidget->setVisible(true);
                const int textHalfWidth = static_cast<int>(distanceWidget->getWidth() / 2);
                distanceWidget->setPosition(static_cast<int>(pixel.x) - textHalfWidth, static_cast<int>(pixel.y) + markerHalfHeight + 2);
            }
        }
        else
        {
            // Follow mode: TRUE position when this object is within the local view's footprint, clamped onto the
            // rim ring (at the correct bearing) once it would fall outside it -- the standard "off-screen indicator"
            // pattern. Rather than re-deriving the camera's heading analytically (which would have to separately
            // account for RotateMinimap's twist AND buildSurfaceLookOrientation's pole-roll ambiguity), this
            // transforms the RAW (non-normalized) world-space offset through the minimap camera's CURRENT actual
            // orientation (camOrientation.Inverse() * toObjectWorld). That guarantees this always matches whatever
            // the camera is actually showing right now, with no separate logic to keep in sync if either changes.
            if (nullptr == this->minimapWidget || nullptr == this->cameraComponent)
            {
                return;
            }

            // IMPORTANT: do NOT normalize this -- the magnitude (world-space distance) is exactly what lets the
            // same calculation place the marker at its TRUE position when nearby and clamp it onto the rim ring
            // only once it's actually out of view, instead of always snapping to the ring regardless of distance.
            // Uses the camera position freshly computed by updateFollowCamera THIS SAME TICK (passed in as
            // followCameraPosition), not read back from the Camera object -- see updateFollowCamera's own comment
            // for why that read-back was unreliable.
            Ogre::Vector3 toObjectWorld = compassWorldPosition - followCameraPosition;
            if (toObjectWorld.squaredLength() < 0.000001f)
            {
                markerWidget->setVisible(false);
                if (nullptr != distanceWidget)
                {
                    distanceWidget->setVisible(false);
                }
                return;
            }

            // Camera-local space: by Ogre/NOWA convention the camera looks down -Z, +X is right, +Y is up. Still in
            // world units at this point (no normalisation), so .x/.y below carry real distance information.
            Ogre::Vector3 toObjectLocal = followCameraOrientation.Inverse() * toObjectWorld;

            // World-unit offset on the camera's local screen plane: local X (right) and local Y (up) -- NOT local Z
            // (the camera's forward/depth axis).
            Ogre::Real worldOffsetX = toObjectLocal.x;
            Ogre::Real worldOffsetY = toObjectLocal.y;

            // Convert world units to pixels via the same view-footprint formula updateFollowCamera uses for the
            // orthographic window size, evaluated at CameraHeight -- i.e. "how many world units does the local
            // minimap actually show edge-to-edge". fovY/aspectRatio come live from the Camera (intrinsic projection
            // parameters), cameraDistance uses the value updateFollowCamera computed THIS tick (followCameraDistance).
            Ogre::Real fovY = this->cameraComponent->getCamera()->getFOVy().valueRadians();
            Ogre::Real aspectRatio = this->cameraComponent->getCamera()->getAspectRatio();
            Ogre::Real cameraDistance = followCameraDistance;
            Ogre::Real viewHeightWorld = 2.0f * cameraDistance * tan(fovY / 2.0f);
            Ogre::Real viewWidthWorld = viewHeightWorld * aspectRatio;

            if (viewWidthWorld < 0.0001f || viewHeightWorld < 0.0001f)
            {
                return;
            }

            Ogre::Real pixelsPerWorldUnitX = static_cast<Ogre::Real>(this->minimapWidget->getWidth()) / viewWidthWorld;
            Ogre::Real pixelsPerWorldUnitY = static_cast<Ogre::Real>(this->minimapWidget->getHeight()) / viewHeightWorld;

            Ogre::Real pixelOffsetX = worldOffsetX * pixelsPerWorldUnitX;
            Ogre::Real pixelOffsetY = worldOffsetY * pixelsPerWorldUnitY;

            // Elliptical rim, matching the widget's actual (often non-square) aspect ratio -- e.g. a 512x206 wide
            // minimap needs a wide ellipse, not a circle capped at the SMALLER of the two half-dimensions.
            const Ogre::Real halfWidth = static_cast<Ogre::Real>(this->minimapWidget->getWidth()) * 0.5f;
            const Ogre::Real halfHeight = static_cast<Ogre::Real>(this->minimapWidget->getHeight()) * 0.5f;
            const Ogre::Real ringMarginX = static_cast<Ogre::Real>(markerWidget->getWidth()) * 0.5f + 2.0f;
            const Ogre::Real ringMarginY = static_cast<Ogre::Real>(markerWidget->getHeight()) * 0.5f + 2.0f;
            const Ogre::Real ringRadiusX = halfWidth - ringMarginX;
            const Ogre::Real ringRadiusY = halfHeight - ringMarginY;

            // Clamp the TRUE pixel offset onto the ellipse only once it would actually exceed it -- this single
            // calculation covers both cases: object in view -> true position; object out of view -> rim, at the
            // correct angle. Normalising each axis by its own ellipse radius before measuring magnitude is what
            // makes the clamp elliptical instead of circular.
            Ogre::Real normalizedX = (ringRadiusX > 0.0001f) ? (pixelOffsetX / ringRadiusX) : 0.0f;
            Ogre::Real normalizedY = (ringRadiusY > 0.0001f) ? (pixelOffsetY / ringRadiusY) : 0.0f;
            Ogre::Real normalizedMagnitude = std::sqrt(normalizedX * normalizedX + normalizedY * normalizedY);
            Ogre::Real offsetMagnitude = std::sqrt(pixelOffsetX * pixelOffsetX + pixelOffsetY * pixelOffsetY);
            if (normalizedMagnitude > 1.0f)
            {
                pixelOffsetX /= normalizedMagnitude;
                pixelOffsetY /= normalizedMagnitude;
            }

            const int markerHalfWidth = static_cast<int>(markerWidget->getWidth() / 2);
            const int markerHalfHeight = static_cast<int>(markerWidget->getHeight() / 2);
            markerWidget->setVisible(true);
            const int markerCenterX = static_cast<int>(halfWidth + pixelOffsetX);
            const int markerCenterY = static_cast<int>(halfHeight - pixelOffsetY);
            markerWidget->setPosition(markerCenterX - markerHalfWidth, markerCenterY - markerHalfHeight);

            if (nullptr != distanceWidget)
            {
                distanceWidget->setCaption(distanceText);
                distanceWidget->setVisible(true);
                const int textHalfWidth = static_cast<int>(distanceWidget->getWidth() / 2);
                distanceWidget->setPosition(markerCenterX - textHalfWidth, markerCenterY + markerHalfHeight + 2);
            }

            // Throttled diagnostic (roughly once a second at 60 closure calls/sec, per object), only when bDebugData
            // is enabled -- prints every value this calculation depends on, so a wrong result can be traced to the
            // exact step that produced it.
            this->compassLogCounter++;
            if (true == this->bShowDebugData && 0u == (this->compassLogCounter % 60u))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' compass[" + Ogre::StringConverter::toString(index) + "](follow): compassWorldPosition=" + Ogre::StringConverter::toString(compassWorldPosition) +
                        " followCameraPosition=" + Ogre::StringConverter::toString(followCameraPosition) + " followCameraOrientation=" + Ogre::StringConverter::toString(followCameraOrientation) +
                        " followCameraDistance=" + Ogre::StringConverter::toString(followCameraDistance) + " toObjectWorld=" + Ogre::StringConverter::toString(toObjectWorld) + " toObjectLocal=" + Ogre::StringConverter::toString(toObjectLocal) +
                        " worldOffsetX=" + Ogre::StringConverter::toString(worldOffsetX) + " worldOffsetY=" + Ogre::StringConverter::toString(worldOffsetY) + " fovY(deg)=" + Ogre::StringConverter::toString(Ogre::Radian(fovY).valueDegrees()) +
                        " aspectRatio=" + Ogre::StringConverter::toString(aspectRatio) + " viewWidthWorld=" + Ogre::StringConverter::toString(viewWidthWorld) + " viewHeightWorld=" + Ogre::StringConverter::toString(viewHeightWorld) +
                        " pixelOffsetX(final)=" + Ogre::StringConverter::toString(pixelOffsetX) + " pixelOffsetY(final)=" + Ogre::StringConverter::toString(pixelOffsetY) + " offsetMagnitude=" + Ogre::StringConverter::toString(offsetMagnitude) +
                        " normalizedMagnitude=" + Ogre::StringConverter::toString(normalizedMagnitude) + " ringRadiusX=" + Ogre::StringConverter::toString(ringRadiusX) + " ringRadiusY=" + Ogre::StringConverter::toString(ringRadiusY) +
                        " minimapWidgetSize=" + Ogre::StringConverter::toString(this->minimapWidget->getWidth()) + "x" + Ogre::StringConverter::toString(this->minimapWidget->getHeight()) +
                        " finalWidgetPos=" + Ogre::StringConverter::toString(markerCenterX - markerHalfWidth) + "," + Ogre::StringConverter::toString(markerCenterY - markerHalfHeight));
            }
        }
    }

    void PlanetMinimapComponent::generateCompassObjects(void)
    {
        // Threadsafe from the outside -- call only from inside a GraphicsModule render command (matches the rest
        // of this file's convention, e.g. createMinimapTexture/createFogOfWarTexture).
        this->destroyCompassObjects();

        if (nullptr == this->minimapWidget)
        {
            return;
        }

        for (unsigned int i = 0; i < this->compassObjectCount->getUInt(); i++)
        {
            Ogre::String image = (i < this->compassImages.size() && nullptr != this->compassImages[i]) ? this->compassImages[i]->getString() : "";
            if (true == image.empty())
            {
                this->compassObjectWidgets.push_back(nullptr);
                this->compassObjectDistanceTexts.push_back(nullptr);
                continue;
            }

            // Marker icon -- created as a child of minimapWidget so it always shares the exact same pixel rect as
            // the map texture/other markers, regardless of which mode (overview/follow) is active.
            MyGUI::ImageBox* markerWidget = this->minimapWidget->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, 32, 32), MyGUI::Align::Default);
            markerWidget->setImageTexture(image);
            markerWidget->setNeedMouseFocus(false);
            markerWidget->setDepth(3);
            markerWidget->setVisible(false);

            Ogre::String toolTip = (i < this->compassToolTipTexts.size() && nullptr != this->compassToolTipTexts[i]) ? this->compassToolTipTexts[i]->getString() : "";
            if (false == toolTip.empty())
            {
                // Standard MyGUI tooltip hook: skins with an auto-tooltip popup configured read this UserString.
                // If your project does not have that popup wired up yet, this is a harmless no-op for now -- the
                // text is still available via getCompassToolTipText(index) for a custom tooltip implementation.
                markerWidget->setNeedMouseFocus(true);
                markerWidget->setUserString("ToolTip", toolTip);
            }

            this->compassObjectWidgets.push_back(markerWidget);

            // Live distance readout (e.g. "342 m" / "1.2 km"), repositioned alongside the marker every tick in
            // updateSingleCompassObject. Created as a sibling (not a child of markerWidget) so its own position can
            // be offset independently (placed just below the marker icon).
            MyGUI::TextBox* distanceWidget = this->minimapWidget->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(0, 0, 90, 16), MyGUI::Align::Default);
            distanceWidget->setTextAlign(MyGUI::Align::Center);
            distanceWidget->setFontHeight(12);
            distanceWidget->setTextColour(MyGUI::Colour::White);
            distanceWidget->setNeedMouseFocus(false);
            distanceWidget->setDepth(4);
            distanceWidget->setVisible(false);
            this->compassObjectDistanceTexts.push_back(distanceWidget);
        }
    }

    void PlanetMinimapComponent::destroyCompassObjects(void)
    {
        for (size_t i = 0; i < this->compassObjectWidgets.size(); i++)
        {
            if (nullptr != this->compassObjectWidgets[i])
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->compassObjectWidgets[i]);
            }
        }
        this->compassObjectWidgets.clear();

        for (size_t i = 0; i < this->compassObjectDistanceTexts.size(); i++)
        {
            if (nullptr != this->compassObjectDistanceTexts[i])
            {
                MyGUI::Gui::getInstancePtr()->destroyWidget(this->compassObjectDistanceTexts[i]);
            }
        }
        this->compassObjectDistanceTexts.clear();
    }

    Ogre::Quaternion PlanetMinimapComponent::buildSurfaceLookOrientation(const Ogre::Vector3& outwardNormal) const
    {
        // Same fixed camera-local pose MinimapComponent bakes via setCameraDegreeOrientation(-90, 90, -90) for its
        // "look straight down world -Y" overhead pose, expressed here as a quaternion so it can be re-targeted from
        // world -Y onto an arbitrary outward surface normal, while keeping the same camera-local convention.
        Ogre::Degree yDeg(90.0f);
        Ogre::Degree pDeg(-90.0f);
        Ogre::Degree rDeg(-90.0f);
        Ogre::Matrix3 fixedPoseMat;
        fixedPoseMat.FromEulerAnglesYXZ(Ogre::Radian(yDeg.valueRadians()), Ogre::Radian(pDeg.valueRadians()), Ogre::Radian(rDeg.valueRadians()));
        Ogre::Quaternion fixedPoseQuat;
        fixedPoseQuat.FromRotationMatrix(fixedPoseMat);

        Ogre::Vector3 targetDownAxis = outwardNormal * -1.0f;
        Ogre::Quaternion alignQuat = Ogre::Vector3::NEGATIVE_UNIT_Y.getRotationTo(targetDownAxis);

        return alignQuat * fixedPoseQuat;
    }

    Ogre::Real PlanetMinimapComponent::computeTangentHeadingDegrees(const Ogre::Vector3& outwardNormal, const Ogre::Vector3& worldForward) const
    {
        // Projects world +Y onto the tangent plane at outwardNormal to obtain a local "north" reference, with a
        // world +Z fallback near the poles (where world +Y is (anti-)parallel to the surface normal and the
        // projection becomes degenerate). This is the planet-surface analogue of the world yaw MinimapComponent
        // uses for RotateMinimap on a flat scene.
        Ogre::Vector3 localNorth = Ogre::Vector3::UNIT_Y - outwardNormal * Ogre::Vector3::UNIT_Y.dotProduct(outwardNormal);
        if (localNorth.squaredLength() < 0.000001f)
        {
            localNorth = Ogre::Vector3::UNIT_Z - outwardNormal * Ogre::Vector3::UNIT_Z.dotProduct(outwardNormal);
        }
        localNorth.normalise();

        Ogre::Vector3 localEast = localNorth.crossProduct(outwardNormal);
        localEast.normalise();

        Ogre::Vector3 forwardTangent = worldForward - outwardNormal * worldForward.dotProduct(outwardNormal);
        if (forwardTangent.squaredLength() < 0.000001f)
        {
            // The target is looking straight along the surface normal here; keep the previous heading reference
            // rather than producing an unstable angle from a near-zero vector.
            forwardTangent = localNorth;
        }
        forwardTangent.normalise();

        Ogre::Radian headingRad = Ogre::Math::ATan2(forwardTangent.dotProduct(localEast), forwardTangent.dotProduct(localNorth));
        return headingRad.valueDegrees();
    }

    Ogre::String PlanetMinimapComponent::formatDistanceMeters(Ogre::Real distanceMeters) const
    {
        if (distanceMeters < 0.0f)
        {
            distanceMeters = 0.0f;
        }

        if (distanceMeters >= 1000.0f)
        {
            Ogre::Real km = distanceMeters / 1000.0f;
            // One decimal place for km (e.g. "1.2 km"), rounded rather than truncated.
            Ogre::Real kmRounded = std::round(km * 10.0f) / 10.0f;
            return Ogre::StringConverter::toString(kmRounded, 1) + " km";
        }

        int meters = static_cast<int>(distanceMeters + 0.5f);
        return Ogre::StringConverter::toString(meters) + " m";
    }

    void PlanetMinimapComponent::updateFollowCamera(const Ogre::Vector3& targetWorldPosition, const Ogre::Quaternion& targetOrientation, const Ogre::Vector3& targetDefaultDirection, Ogre::Vector3& outCameraPosition,
        Ogre::Quaternion& outCameraOrientation, Ogre::Real& outCameraDistance)
    {
        if (nullptr == this->planetGameObject || nullptr == this->cameraComponent)
        {
            return;
        }

        Ogre::Vector3 planetCenter = this->planetGameObject->getPosition();
        Ogre::Vector3 outwardNormal = targetWorldPosition - planetCenter;
        if (outwardNormal.squaredLength() < 0.000001f)
        {
            // Degenerate case: the target sits exactly at the planet's center. Keep the previous camera transform
            // instead of normalising a zero-length vector. The out-parameters still get the current actual camera
            // transform so a caller reading them (e.g. for the compass) has a sane fallback rather than garbage.
            outCameraPosition = this->cameraComponent->getCamera()->getPosition();
            outCameraOrientation = this->cameraComponent->getCamera()->getOrientation();
            outCameraDistance = this->cameraHeight->getReal();
            return;
        }
        outwardNormal.normalise();

        Ogre::Real cameraDistance = this->cameraHeight->getReal();
        Ogre::Vector3 camPos = targetWorldPosition + outwardNormal * cameraDistance;
        this->cameraComponent->setCameraPosition(camPos);

        Ogre::Quaternion baseOrientation = this->buildSurfaceLookOrientation(outwardNormal);
        Ogre::Quaternion finalOrientation = baseOrientation;

        if (true == this->rotateMinimap->getBool())
        {
            Ogre::Vector3 worldForward = targetOrientation * targetDefaultDirection;
            Ogre::Real headingDeg = this->computeTangentHeadingDegrees(outwardNormal, worldForward);
            Ogre::Real smoothedAngle = this->filter.update(headingDeg);
            Ogre::Quaternion twistQuat(Ogre::Degree(smoothedAngle), outwardNormal);
            finalOrientation = twistQuat * baseOrientation;
            this->cameraComponent->setCameraOrientation(finalOrientation);
        }
        else
        {
            this->cameraComponent->setCameraOrientation(finalOrientation);
        }

        // IMPORTANT: these are the exact values just computed and handed to setCameraPosition/setCameraOrientation
        // above -- NOT read back via this->cameraComponent->getCamera()->getPosition()/getOrientation(). Reading
        // those back could return a stale (not-yet-applied) or differently-spaced (local-to-parent-node rather than
        // world, a classic Ogre::Camera::getPosition() vs _getDerivedPosition() gotcha) value depending on
        // CameraComponent's internal threading/parenting, neither of which this function can verify from here.
        // Handing the authoritative values out directly sidesteps both possibilities entirely.
        outCameraPosition = camPos;
        outCameraOrientation = finalOrientation;
        outCameraDistance = cameraDistance;

        if (true == this->cameraComponent->getIsOrthographic())
        {
            Ogre::Real fovY = this->cameraComponent->getCamera()->getFOVy().valueRadians();
            Ogre::Real aspectRatio = this->cameraComponent->getCamera()->getAspectRatio();

            Ogre::Real viewHeight = 2.0f * cameraDistance * tan(fovY / 2.0f);
            Ogre::Real viewWidth = viewHeight * aspectRatio;

            this->cameraComponent->setOrthoWindowSize(Ogre::Vector2(viewWidth, viewHeight));
        }

        // Same far clip safety net as adjustPlanetOverviewCamera -- cameraDistance here is just CameraHeight, so this
        // is normally a non-issue in follow mode, but a tiny CameraHeight combined with a very large existing near
        // clip (e.g. left over from WholeSceneVisible mode's far clip change without a matching near clip change)
        // could still clip the ground plane right under the target.
        Ogre::Real requiredNearClip = std::max(0.1f, cameraDistance * 0.001f);
        if (this->cameraComponent->getCamera()->getNearClipDistance() > requiredNearClip)
        {
            this->cameraComponent->getCamera()->setNearClipDistance(requiredNearClip);
        }

        if (false == this->followCameraLogged && true == this->bShowDebugData)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[PlanetMinimapComponent] '" + this->gameObjectPtr->getName() + "' follow camera (first tick only): targetWorldPosition=" + Ogre::StringConverter::toString(targetWorldPosition) +
                    " planetCenter=" + Ogre::StringConverter::toString(planetCenter) + " outwardNormal=" + Ogre::StringConverter::toString(outwardNormal) + " cameraDistance=" + Ogre::StringConverter::toString(cameraDistance) +
                    " resultingCameraPos=" + Ogre::StringConverter::toString(camPos) + " actualCameraPos(afterSet)=" + Ogre::StringConverter::toString(this->cameraComponent->getCamera()->getPosition()) + " actualCameraPos_derived(afterSet)=" +
                    Ogre::StringConverter::toString(nullptr != this->cameraComponent->getCamera()->getParentSceneNode() ? this->cameraComponent->getCamera()->getParentSceneNode()->_getDerivedPosition() : Ogre::Vector3::ZERO) +
                    " nearClip=" + Ogre::StringConverter::toString(this->cameraComponent->getCamera()->getNearClipDistance()) + " farClip=" + Ogre::StringConverter::toString(this->cameraComponent->getCamera()->getFarClipDistance()));
            this->followCameraLogged = true;
        }
    }

    bool PlanetMinimapComponent::saveDiscoveryState(void)
    {
        Ogre::String existingMinimapDiscoveryFilePathName =
            Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_" + Ogre::StringConverter::toString(this->planetId->getULong()) + "_pmData.bin";
        std::ofstream outFile(existingMinimapDiscoveryFilePathName, std::ios::binary);
        if (!outFile)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetMinimapComponent] Failed to open file for writing: " + existingMinimapDiscoveryFilePathName + " for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        // Save dimensions
        int rows = this->discoveryState.size();
        int cols = this->discoveryState.empty() ? 0 : this->discoveryState[0].size();
        outFile.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        outFile.write(reinterpret_cast<const char*>(&cols), sizeof(cols));

        for (const auto& row : this->discoveryState)
        {
            for (bool state : row)
            {
                outFile.write(reinterpret_cast<const char*>(&state), sizeof(state));
            }
        }

        outFile.close();
        return true;
    }

    bool PlanetMinimapComponent::loadDiscoveryState(void)
    {
        Ogre::String existingMinimapDiscoveryFilePathName =
            Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + "_" + Ogre::StringConverter::toString(this->planetId->getULong()) + "_pmData.bin";
        std::ifstream inFile(existingMinimapDiscoveryFilePathName, std::ios::binary);
        if (!inFile)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetMinimapComponent] Failed to open file for reading: " + existingMinimapDiscoveryFilePathName + " for game object: " + this->gameObjectPtr->getName());
            return false;
        }

        // Load dimensions
        int rows, cols;
        inFile.read(reinterpret_cast<char*>(&rows), sizeof(rows));
        inFile.read(reinterpret_cast<char*>(&cols), sizeof(cols));

        // Resize the discovery state array
        this->discoveryState.resize(rows, std::vector<bool>(cols, false));

        // Load data
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                bool state;
                inFile.read(reinterpret_cast<char*>(&state), sizeof(state));
                this->discoveryState[i][j] = state;
            }
        }

        inFile.close();

        return true;
    }

    Ogre::TextureGpu* PlanetMinimapComponent::getMinimapTexture(void) const
    {
        return this->minimapTexture;
    }

    void PlanetMinimapComponent::deleteGameObjectDelegate(EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
        unsigned long id = castEventData->getGameObjectId();

        if (nullptr != this->planetGameObject && id == this->planetGameObject->getId())
        {
            this->planetGameObject = nullptr;
            this->planetTerraComponent = nullptr;
        }
    }

    void PlanetMinimapComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlanetMinimapComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrPlanetId() == attribute->getName())
        {
            this->setPlanetId(attribute->getULong());
        }
        else if (PlanetMinimapComponent::AttrTargetId() == attribute->getName())
        {
            this->setTargetId(attribute->getULong());
        }
        else if (PlanetMinimapComponent::AttrTextureSize() == attribute->getName())
        {
            this->setTextureSize(attribute->getUInt());
        }
        else if (PlanetMinimapComponent::AttrMinimapGeometry() == attribute->getName())
        {
            this->setMinimapGeometry(attribute->getVector4());
        }
        else if (PlanetMinimapComponent::AttrTransparent() == attribute->getName())
        {
            this->setTransparent(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrUseFogOfWar() == attribute->getName())
        {
            this->setUseFogOfWar(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrUseDiscovery() == attribute->getName())
        {
            this->setUseDiscovery(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrPersistDiscovery() == attribute->getName())
        {
            this->setPersistDiscovery(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrVisibilityRadius() == attribute->getName())
        {
            this->setVisibilityRadius(attribute->getReal());
        }
        else if (PlanetMinimapComponent::AttrUseVisibilitySpotlight() == attribute->getName())
        {
            this->setUseVisibilitySpotlight(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrWholeSceneVisible() == attribute->getName())
        {
            this->setWholeSceneVisible(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrCameraHeight() == attribute->getName())
        {
            this->setCameraHeight(attribute->getReal());
        }
        else if (PlanetMinimapComponent::AttrMinimapMask() == attribute->getName())
        {
            this->setMinimapMask(attribute->getString());
        }
        else if (PlanetMinimapComponent::AttrUseRoundMinimap() == attribute->getName())
        {
            this->setUseRoundMinimap(attribute->getBool());
        }
        else if (PlanetMinimapComponent::AttrTargetMarkerImage() == attribute->getName())
        {
            this->setTargetMarkerImage(attribute->getString());
        }
        else if (PlanetMinimapComponent::AttrCompassObjectCount() == attribute->getName())
        {
            this->setCompassObjectCount(attribute->getUInt());
        }
        else if (PlanetMinimapComponent::AttrBaseTerrainColor() == attribute->getName())
        {
            this->setBaseTerrainColor(attribute->getVector3());
        }
        else if (PlanetMinimapComponent::AttrLayer0Color() == attribute->getName())
        {
            this->setLayer0Color(attribute->getVector3());
        }
        else if (PlanetMinimapComponent::AttrLayer1Color() == attribute->getName())
        {
            this->setLayer1Color(attribute->getVector3());
        }
        else if (PlanetMinimapComponent::AttrLayer2Color() == attribute->getName())
        {
            this->setLayer2Color(attribute->getVector3());
        }
        else if (PlanetMinimapComponent::AttrLayer3Color() == attribute->getName())
        {
            this->setLayer3Color(attribute->getVector3());
        }
        else if (PlanetMinimapComponent::AttrRotateMinimap() == attribute->getName())
        {
            this->setRotateMinimap(attribute->getBool());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->compassGameObjectIds.size()); i++)
            {
                if (PlanetMinimapComponent::AttrCompassGameObjectId() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setCompassGameObjectId(i, attribute->getULong());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->compassImages.size()); i++)
            {
                if (PlanetMinimapComponent::AttrCompassImage() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setCompassImage(i, attribute->getString());
                }
            }
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->compassToolTipTexts.size()); i++)
            {
                if (PlanetMinimapComponent::AttrCompassToolTipText() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setCompassToolTipText(i, attribute->getString());
                }
            }
        }
    }

    void PlanetMinimapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "PlanetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->planetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TextureSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textureSize->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MinimapGeometry"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minimapGeometry->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Transparent"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->transparent->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseFogOfWar"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useFogOfWar->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseDiscovery"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useDiscovery->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "PersistDiscovery"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->persistDiscovery->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "VisibilityRadius"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->visibilityRadius->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseVisibilitySpotlight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useVisibilitySpotlight->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "WholeSceneVisible"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wholeSceneVisible->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CameraHeight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MinimapMask"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minimapMask->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "UseRoundMinimap"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useRoundMinimap->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetMarkerImage"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetMarkerImage->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CompassObjectCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->compassObjectCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->compassGameObjectIds.size(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CompassGameObjectId" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->compassGameObjectIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CompassImage" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->compassImages[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CompassToolTipText" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->compassToolTipTexts[i]->getString())));
            propertiesXML->append_node(propertyXML);
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BaseTerrainColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->baseTerrainColor->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer0Color"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer0Color->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer1Color"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer1Color->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer2Color"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer2Color->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer3Color"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer3Color->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RotateMinimap"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotateMinimap->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String PlanetMinimapComponent::getClassName(void) const
    {
        return "PlanetMinimapComponent";
    }

    Ogre::String PlanetMinimapComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void PlanetMinimapComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
        if (true == activated)
        {
            // Tear down any previously created workspace/textures/widgets first. Without this, calling
            // setActivated(true) a second time (e.g. after changing PlanetId/TargetId from Lua) would stack a
            // second compositor workspace onto the same RT and leak the previous minimapWidget/fogOfWarWidget,
            // since setupPlanetMinimap() below unconditionally creates new ones.
            this->removeWorkspace();

            const auto& planetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->planetId->getULong());
            if (nullptr != planetGameObjectPtr)
            {
                this->planetGameObject = planetGameObjectPtr.get();

                const auto& planetTerraCompPtr = NOWA::makeStrongPtr(this->planetGameObject->getComponent<PlanetTerraComponent>());
                if (nullptr != planetTerraCompPtr)
                {
                    this->planetTerraComponent = planetTerraCompPtr.get();
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[PlanetMinimapComponent] Error: Planet game object '" + this->planetGameObject->getName() + "' referenced by game object: " + this->gameObjectPtr->getName() + " does not have a PlanetTerraComponent!");
                    this->planetGameObject = nullptr;
                    this->planetTerraComponent = nullptr;
                }
            }
            else
            {
                this->planetGameObject = nullptr;
                this->planetTerraComponent = nullptr;
            }

            this->setupPlanetMinimap();
        }
        else
        {
            // Persist discovery for the CURRENT planet before tearing anything down, otherwise progress made since
            // the last 0.25s throttle tick (or since the last manual save) would be lost on deactivation.
            if (true == this->persistDiscovery->getBool())
            {
                this->saveDiscoveryState();
            }

            this->removeWorkspace();

            this->planetGameObject = nullptr;
            this->planetTerraComponent = nullptr;
            this->cameraComponent = nullptr;
        }
    }

    void PlanetMinimapComponent::switchPlanet(unsigned long newPlanetId, unsigned long newTargetId)
    {

        //    before PlanetId is changed below, since saveDiscoveryState()/loadDiscoveryState() key the filename off it.
        if (true == this->persistDiscovery->getBool())
        {
            this->saveDiscoveryState();
        }

        // 2. Full teardown of the previous planet's workspace/textures/widgets.
        this->removeWorkspace();

        // 3. Switch the ids.
        this->planetId->setValue(newPlanetId);
        if (0 != newTargetId)
        {
            this->targetId->setValue(newTargetId);
        }

        // 4. Re-setup, exactly like activating from scratch -- this re-resolves planetGameObject/planetTerraComponent/
        //    targetGameObject from the new ids and, if PersistDiscovery is set, loads the discovery file for the NEW
        //    PlanetId (creating a fresh blank one if this planet has never been visited before).
        this->setActivated(true);
    }

    bool PlanetMinimapComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void PlanetMinimapComponent::setPlanetId(unsigned long planetId)
    {
        this->planetId->setValue(planetId);
    }

    unsigned long PlanetMinimapComponent::getPlanetId(void) const
    {
        return this->planetId->getULong();
    }

    GameObject* PlanetMinimapComponent::getPlanetGameObject(void) const
    {
        return this->planetGameObject;
    }

    void PlanetMinimapComponent::setTargetId(unsigned long targetId)
    {
        this->targetId->setValue(targetId);
    }

    unsigned long PlanetMinimapComponent::getTargetId(void) const
    {
        return this->targetId->getULong();
    }

    void PlanetMinimapComponent::setTextureSize(unsigned int textureSize)
    {
        this->textureSize->setValue(textureSize);
    }

    unsigned int PlanetMinimapComponent::getTextureSize(void) const
    {
        return this->textureSize->getUInt();
    }

    void PlanetMinimapComponent::setMinimapGeometry(const Ogre::Vector4& minimapGeometry)
    {
        this->minimapGeometry->setValue(minimapGeometry);
    }

    Ogre::Vector4 PlanetMinimapComponent::getMinimapGeometry(void) const
    {
        return this->minimapGeometry->getVector4();
    }

    void PlanetMinimapComponent::setTransparent(bool transparent)
    {
        this->transparent->setValue(transparent);
    }

    bool PlanetMinimapComponent::getTransparent(void) const
    {
        return this->transparent->getBool();
    }

    void PlanetMinimapComponent::setUseFogOfWar(bool useFogOfWar)
    {
        if (true == this->wholeSceneVisible->getBool())
        {
            this->useFogOfWar->setValue(useFogOfWar);
        }

        this->wholeSceneVisible->setVisible(false == this->useFogOfWar->getBool());
        this->useDiscovery->setVisible(true == this->useFogOfWar->getBool());
        this->persistDiscovery->setVisible(true == this->useFogOfWar->getBool());
        this->visibilityRadius->setVisible(true == this->useFogOfWar->getBool());
        this->useVisibilitySpotlight->setVisible(true == this->useFogOfWar->getBool());
        this->cameraHeight->setVisible(false == this->wholeSceneVisible->getBool());
        this->rotateMinimap->setVisible(false == this->wholeSceneVisible->getBool());

        if (false == useFogOfWar)
        {
            this->useDiscovery->setValue(false);
            this->persistDiscovery->setValue(false);
        }
    }

    bool PlanetMinimapComponent::getUseFogOfWar(void) const
    {
        return this->useFogOfWar->getBool();
    }

    void PlanetMinimapComponent::setUseDiscovery(bool useDiscovery)
    {
        if (true == this->useFogOfWar->getBool())
        {
            this->useDiscovery->setValue(useDiscovery);
        }
    }

    bool PlanetMinimapComponent::getUseDiscovery(void) const
    {
        return this->useDiscovery->getBool();
    }

    void PlanetMinimapComponent::setPersistDiscovery(bool persistDiscovery)
    {
        if (true == this->useFogOfWar->getBool() && this->useDiscovery->getBool())
        {
            this->persistDiscovery->setValue(persistDiscovery);
        }
    }

    bool PlanetMinimapComponent::getPersistDiscovery(void) const
    {
        return this->persistDiscovery->getBool();
    }

    void PlanetMinimapComponent::setVisibilityRadius(Ogre::Real visibilityRadius)
    {
        if (visibilityRadius < 0.0f)
        {
            visibilityRadius = 0.0f;
        }
        if (true == this->useFogOfWar->getBool())
        {
            this->visibilityRadius->setValue(visibilityRadius);
        }
    }

    Ogre::Real PlanetMinimapComponent::getVisibilityRadius(void) const
    {
        return this->visibilityRadius->getReal();
    }

    void PlanetMinimapComponent::setUseVisibilitySpotlight(bool useVisibilitySpotlight)
    {
        if (true == this->useFogOfWar->getBool())
        {
            this->useVisibilitySpotlight->setValue(useVisibilitySpotlight);
        }
    }

    bool PlanetMinimapComponent::getUseVisibilitySpotlight(void) const
    {
        return this->useVisibilitySpotlight->getBool();
    }

    void PlanetMinimapComponent::setWholeSceneVisible(bool wholeSceneVisible)
    {
        this->wholeSceneVisible->setValue(wholeSceneVisible);

        this->useFogOfWar->setVisible(true == this->wholeSceneVisible->getBool());
        this->useDiscovery->setVisible(true == this->useFogOfWar->getBool());
        this->persistDiscovery->setVisible(true == this->useFogOfWar->getBool());
        this->visibilityRadius->setVisible(true == this->useFogOfWar->getBool());
        this->useVisibilitySpotlight->setVisible(true == this->useFogOfWar->getBool());
        this->cameraHeight->setVisible(false == this->wholeSceneVisible->getBool());
        this->rotateMinimap->setVisible(false == this->wholeSceneVisible->getBool());

        if (false == wholeSceneVisible)
        {
            this->useFogOfWar->setValue(false);
            this->useDiscovery->setValue(false);
            this->persistDiscovery->setValue(false);
            this->visibilityRadius->setValue(false);
            this->useVisibilitySpotlight->setValue(false);
        }
    }

    bool PlanetMinimapComponent::getWholeSceneVisible(void) const
    {
        return this->wholeSceneVisible->getBool();
    }

    void PlanetMinimapComponent::setCameraHeight(Ogre::Real cameraHeight)
    {
        if (cameraHeight < 2.0f)
        {
            cameraHeight = 2.0f;
        }

        this->cameraHeight->setValue(cameraHeight);
    }

    Ogre::Real PlanetMinimapComponent::getCameraHeight(void) const
    {
        return this->cameraHeight->getReal();
    }

    void PlanetMinimapComponent::setMinimapMask(const Ogre::String& minimapMask)
    {
        this->minimapMask->setValue(minimapMask);
    }

    Ogre::String PlanetMinimapComponent::getMinimapMask(void) const
    {
        return minimapMask->getString();
    }

    void PlanetMinimapComponent::setTargetMarkerImage(const Ogre::String& targetMarkerImage)
    {
        this->targetMarkerImage->setValue(targetMarkerImage);
    }

    Ogre::String PlanetMinimapComponent::getTargetMarkerImage(void) const
    {
        return this->targetMarkerImage->getString();
    }

    void PlanetMinimapComponent::setCompassObjectCount(unsigned int compassObjectCount)
    {
        this->compassObjectCount->setValue(compassObjectCount);

        unsigned int oldSize = static_cast<unsigned int>(this->compassGameObjectIds.size());

        bool changed = (compassObjectCount != oldSize);

        if (compassObjectCount > oldSize)
        {
            this->compassGameObjectIds.resize(compassObjectCount);
            this->compassImages.resize(compassObjectCount);
            this->compassToolTipTexts.resize(compassObjectCount);

            for (unsigned int i = oldSize; i < compassObjectCount; i++)
            {
                this->compassGameObjectIds[i] = new Variant(PlanetMinimapComponent::AttrCompassGameObjectId() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
                this->compassImages[i] = new Variant(PlanetMinimapComponent::AttrCompassImage() + Ogre::StringConverter::toString(i), "", this->attributes);
                this->compassImages[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Minimap");
                this->compassToolTipTexts[i] = new Variant(PlanetMinimapComponent::AttrCompassToolTipText() + Ogre::StringConverter::toString(i), "", this->attributes);
            }
        }
        else if (compassObjectCount < oldSize)
        {
            this->eraseVariants(this->compassGameObjectIds, compassObjectCount);
            this->eraseVariants(this->compassImages, compassObjectCount);
            this->eraseVariants(this->compassToolTipTexts, compassObjectCount);
        }

        if (true == changed && nullptr != this->minimapWidget)
        {
            // The widget COUNT itself changed, so a full regenerate is needed here (unlike a single id/image/tooltip
            // edit on an existing slot, which the per-index setters below patch in place for efficiency).
            NOWA::GraphicsModule::RenderCommand cmd = [this]
            {
                this->generateCompassObjects();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::setCompassObjectCount regenerate");
        }
    }

    unsigned int PlanetMinimapComponent::getCompassObjectCount(void) const
    {
        return this->compassObjectCount->getUInt();
    }

    void PlanetMinimapComponent::setCompassGameObjectId(unsigned int index, unsigned long compassGameObjectId)
    {
        if (index >= this->compassGameObjectIds.size())
        {
            return;
        }
        this->compassGameObjectIds[index]->setValue(compassGameObjectId);

        if (0 == compassGameObjectId && index < this->compassObjectWidgets.size() && nullptr != this->compassObjectWidgets[index])
        {
            // Cheap immediate feedback so the marker disappears right away rather than waiting up to one tick for
            // updateSingleCompassObject()'s own 0-check to catch up.
            NOWA::GraphicsModule::RenderCommand cmd = [this, index]
            {
                if (index < this->compassObjectWidgets.size() && nullptr != this->compassObjectWidgets[index])
                {
                    this->compassObjectWidgets[index]->setVisible(false);
                }
                if (index < this->compassObjectDistanceTexts.size() && nullptr != this->compassObjectDistanceTexts[index])
                {
                    this->compassObjectDistanceTexts[index]->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::setCompassGameObjectId hide");
        }
    }

    unsigned long PlanetMinimapComponent::getCompassGameObjectId(unsigned int index) const
    {
        if (index >= this->compassGameObjectIds.size())
        {
            return 0ul;
        }
        return this->compassGameObjectIds[index]->getULong();
    }

    void PlanetMinimapComponent::setCompassImage(unsigned int index, const Ogre::String& compassImage)
    {
        if (index >= this->compassImages.size())
        {
            return;
        }
        this->compassImages[index]->setValue(compassImage);

        NOWA::GraphicsModule::RenderCommand cmd = [this, index, compassImage]
        {
            if (index < this->compassObjectWidgets.size() && nullptr != this->compassObjectWidgets[index])
            {
                if (false == compassImage.empty())
                {
                    // Widget already exists for this slot -- patch the texture in place, no need to regenerate
                    // every compass object's widgets just for one image change.
                    this->compassObjectWidgets[index]->setImageTexture(compassImage);
                    this->compassObjectWidgets[index]->setImageRect(MyGUI::IntRect(0, 0, this->compassObjectWidgets[index]->getImageSize().width, this->compassObjectWidgets[index]->getImageSize().height));
                }
                else
                {
                    // Image cleared -- regenerate so this slot's now-superfluous widgets get torn down cleanly
                    // (mirrors how generateCompassObjects() skips creating widgets for empty-image slots).
                    this->generateCompassObjects();
                }
            }
            else if (false == compassImage.empty())
            {
                // No widget existed yet for this slot (it was empty before) -- regenerate so it gets created now.
                this->generateCompassObjects();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::setCompassImage");
    }

    Ogre::String PlanetMinimapComponent::getCompassImage(unsigned int index) const
    {
        if (index >= this->compassImages.size())
        {
            return "";
        }
        return this->compassImages[index]->getString();
    }

    void PlanetMinimapComponent::setCompassToolTipText(unsigned int index, const Ogre::String& compassToolTipText)
    {
        if (index >= this->compassToolTipTexts.size())
        {
            return;
        }
        this->compassToolTipTexts[index]->setValue(compassToolTipText);

        if (index < this->compassObjectWidgets.size() && nullptr != this->compassObjectWidgets[index])
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, index, compassToolTipText]
            {
                if (index < this->compassObjectWidgets.size() && nullptr != this->compassObjectWidgets[index])
                {
                    this->compassObjectWidgets[index]->setNeedMouseFocus(false == compassToolTipText.empty());
                    this->compassObjectWidgets[index]->setUserString("ToolTip", compassToolTipText);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetMinimapComponent::setCompassToolTipText");
        }
    }

    Ogre::String PlanetMinimapComponent::getCompassToolTipText(unsigned int index) const
    {
        if (index >= this->compassToolTipTexts.size())
        {
            return "";
        }
        return this->compassToolTipTexts[index]->getString();
    }

    void PlanetMinimapComponent::setBaseTerrainColor(const Ogre::Vector3& baseTerrainColor)
    {
        this->baseTerrainColor->setValue(baseTerrainColor);
        // Live-read every bake; no GPU resource recreation needed, just call rebakePlanetOverview() to see the change.
    }

    Ogre::Vector3 PlanetMinimapComponent::getBaseTerrainColor(void) const
    {
        return this->baseTerrainColor->getVector3();
    }

    void PlanetMinimapComponent::setLayer0Color(const Ogre::Vector3& layer0Color)
    {
        this->layer0Color->setValue(layer0Color);
        // Live-read every bake; no GPU resource recreation needed, just call rebakePlanetOverview() to see the change.
    }

    Ogre::Vector3 PlanetMinimapComponent::getLayer0Color(void) const
    {
        return this->layer0Color->getVector3();
    }

    void PlanetMinimapComponent::setLayer1Color(const Ogre::Vector3& layer1Color)
    {
        this->layer1Color->setValue(layer1Color);
        // Live-read every bake; no GPU resource recreation needed, just call rebakePlanetOverview() to see the change.
    }

    Ogre::Vector3 PlanetMinimapComponent::getLayer1Color(void) const
    {
        return this->layer1Color->getVector3();
    }

    void PlanetMinimapComponent::setLayer2Color(const Ogre::Vector3& layer2Color)
    {
        this->layer2Color->setValue(layer2Color);
        // Live-read every bake; no GPU resource recreation needed, just call rebakePlanetOverview() to see the change.
    }

    Ogre::Vector3 PlanetMinimapComponent::getLayer2Color(void) const
    {
        return this->layer2Color->getVector3();
    }

    void PlanetMinimapComponent::setLayer3Color(const Ogre::Vector3& layer3Color)
    {
        this->layer3Color->setValue(layer3Color);
        // Live-read every bake; no GPU resource recreation needed, just call rebakePlanetOverview() to see the change.
    }

    Ogre::Vector3 PlanetMinimapComponent::getLayer3Color(void) const
    {
        return this->layer3Color->getVector3();
    }

    void PlanetMinimapComponent::setUseRoundMinimap(bool useRoundMinimap)
    {
        this->useRoundMinimap->setValue(useRoundMinimap);
    }

    bool PlanetMinimapComponent::getUseRoundMinimap(void) const
    {
        return this->useRoundMinimap->getBool();
    }

    void PlanetMinimapComponent::setRotateMinimap(bool rotateMinimap)
    {
        if (false == this->wholeSceneVisible->getBool())
        {
            this->rotateMinimap->setValue(rotateMinimap);
        }
    }

    bool PlanetMinimapComponent::getRotateMinimap(void) const
    {
        return this->rotateMinimap->getBool();
    }

    void PlanetMinimapComponent::clearDiscoveryState(void)
    {
        this->discoveryState.clear();
        this->discoveryState.resize((this->textureSize->getUInt()), std::vector<bool>(this->textureSize->getUInt(), false));

        if (true == this->persistDiscovery->getBool())
        {
            this->saveDiscoveryState();
        }
    }

    // Lua registration part

    PlanetMinimapComponent* getPlanetMinimapComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PlanetMinimapComponent>(gameObject->getComponentWithOccurrence<PlanetMinimapComponent>(occurrenceIndex)).get();
    }

    PlanetMinimapComponent* getPlanetMinimapComponent(GameObject* gameObject)
    {
        return makeStrongPtr<PlanetMinimapComponent>(gameObject->getComponent<PlanetMinimapComponent>()).get();
    }

    PlanetMinimapComponent* getPlanetMinimapComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PlanetMinimapComponent>(gameObject->getComponentFromName<PlanetMinimapComponent>(name)).get();
    }

    void setLuaTargetId(PlanetMinimapComponent* instance, const Ogre::String& targetId)
    {
        instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
    }

    Ogre::String getLuaTargetId(PlanetMinimapComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getTargetId());
    }

    void setLuaPlanetId(PlanetMinimapComponent* instance, const Ogre::String& targetId)
    {
        instance->setPlanetId(Ogre::StringConverter::parseUnsignedLong(targetId));
    }

    Ogre::String getLuaPlanetId(PlanetMinimapComponent* instance)
    {
        return Ogre::StringConverter::toString(instance->getPlanetId());
    }

    void setLuaCompassGameObjectId(PlanetMinimapComponent* instance, unsigned int index, const Ogre::String& compassGameObjectId)
    {
        instance->setCompassGameObjectId(index, Ogre::StringConverter::parseUnsignedLong(compassGameObjectId));
    }

    Ogre::String getLuaCompassGameObjectId(PlanetMinimapComponent* instance, unsigned int index)
    {
        return Ogre::StringConverter::toString(instance->getCompassGameObjectId(index));
    }

    void PlanetMinimapComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<PlanetMinimapComponent, GameObjectComponent>("PlanetMinimapComponent")
                .def("setActivated", &PlanetMinimapComponent::setActivated)
                .def("isActivated", &PlanetMinimapComponent::isActivated)
                .def("setPlanetId", &setLuaPlanetId)
                .def("getPlanetId", &getLuaPlanetId)
                .def("setTargetId", &setLuaTargetId)
                .def("getTargetId", &getLuaTargetId)
                .def("setVisibilityRadius", &PlanetMinimapComponent::setVisibilityRadius)
                .def("getVisibilityRadius", &PlanetMinimapComponent::getVisibilityRadius)
                .def("clearDiscoveryState", &PlanetMinimapComponent::clearDiscoveryState)
                .def("saveDiscoveryState", &PlanetMinimapComponent::saveDiscoveryState)
                .def("loadDiscoveryState", &PlanetMinimapComponent::loadDiscoveryState)
                .def("setCameraHeight", &PlanetMinimapComponent::setCameraHeight)
                .def("getCameraHeight", &PlanetMinimapComponent::getCameraHeight)
                .def("rebakePlanetOverview", &PlanetMinimapComponent::rebakePlanetOverview)
                .def("setTargetMarkerImage", &PlanetMinimapComponent::setTargetMarkerImage)
                .def("getTargetMarkerImage", &PlanetMinimapComponent::getTargetMarkerImage)
                .def("setCompassObjectCount", &PlanetMinimapComponent::setCompassObjectCount)
                .def("getCompassObjectCount", &PlanetMinimapComponent::getCompassObjectCount)
                .def("setCompassGameObjectId", &setLuaCompassGameObjectId)
                .def("getCompassGameObjectId", &getLuaCompassGameObjectId)
                .def("setCompassImage", &PlanetMinimapComponent::setCompassImage)
                .def("getCompassImage", &PlanetMinimapComponent::getCompassImage)
                .def("setCompassToolTipText", &PlanetMinimapComponent::setCompassToolTipText)
                .def("getCompassToolTipText", &PlanetMinimapComponent::getCompassToolTipText)
                .def("setBaseTerrainColor", &PlanetMinimapComponent::setBaseTerrainColor)
                .def("getBaseTerrainColor", &PlanetMinimapComponent::getBaseTerrainColor)
                .def("setLayer0Color", &PlanetMinimapComponent::setLayer0Color)
                .def("getLayer0Color", &PlanetMinimapComponent::getLayer0Color)
                .def("setLayer1Color", &PlanetMinimapComponent::setLayer1Color)
                .def("getLayer1Color", &PlanetMinimapComponent::getLayer1Color)
                .def("setLayer2Color", &PlanetMinimapComponent::setLayer2Color)
                .def("getLayer2Color", &PlanetMinimapComponent::getLayer2Color)
                .def("setLayer3Color", &PlanetMinimapComponent::setLayer3Color)
                .def("getLayer3Color", &PlanetMinimapComponent::getLayer3Color)
                .def("switchPlanet", &PlanetMinimapComponent::switchPlanet)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "class inherits GameObjectComponent", PlanetMinimapComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "bool isActivated()", "Gets whether this component is activated.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setPlanetId(String planetId)", "Sets the game object id of the planet (must hold a PlanetTerraComponent), which shall be displayed on the minimap.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getPlanetId()", "Gets the game object id of the planet, which is displayed on the minimap.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setTargetId(String targetId)", "Sets the game object id of the target object that the minimap camera should follow.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getTargetId()", "Gets the game object id of the target object that the minimap camera is following.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setVisibilityRadius(number visibilityRadius)", "If fog of war is used, sets the visibilty radius which discovers the fog of war.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "number getVisibilityRadius()", "Gets the visibilty radius which discovers the fog of war.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void clearDiscoveryState()", "Clears the area which has been already discovered and saves the state.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void saveDiscoveryState()", "Saves the area which has been already discovered.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void loadDiscoveryState()", "Loads the area which has been already discovered.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setCameraHeight(number cameraHeight)",
            "If whole scene visible is set to false, sets the camera height, which is added along the local outward direction at the top of the target game object's position.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "number getCameraHeight()",
            "If whole scene visible is set to false, gets the camera height, which is added along the local outward direction at the top of the target game object's position.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void rebakePlanetOverview()",
            "Re-bakes the equirectangular planet overview texture from the planet's current height data. Call after sculpting has changed the terrain, in WholeSceneVisible mode.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setTargetMarkerImage(String image)",
            "If whole scene visible is true, sets the icon image shown at the tracked target's position on the overview map (same folder as MinimapMask). Left empty, no marker is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getTargetMarkerImage()", "Gets the icon image shown at the tracked target's position on the overview map.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setCompassObjectCount(number count)",
            "Sets the count of generic compass objects to find/track (e.g. the player's ship, a quest objective, an NPC). Each gets its own CompassGameObjectId/CompassImage/CompassToolTipText.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "number getCompassObjectCount()", "Gets the count of generic compass objects.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setCompassGameObjectId(number index, String id)",
            "Sets the game object id to track for the compass object at the given index. Shown as a true-position marker on the baked overview in WholeSceneVisible mode, or as a marker wandering the rim of the local minimap in follow mode. 0 "
            "disables that slot.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getCompassGameObjectId(number index)", "Gets the game object id tracked for the compass object at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setCompassImage(number index, String image)",
            "Sets the marker icon image for the compass object at the given index (same folder as MinimapMask). Left empty, no marker is shown for that slot even if CompassGameObjectId is set.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getCompassImage(number index)", "Gets the marker icon image for the compass object at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setCompassToolTipText(number index, String text)",
            "Sets the tooltip text shown when hovering the compass object's marker at the given index. Left empty, no tooltip is shown for that slot.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "String getCompassToolTipText(number index)", "Gets the tooltip text for the compass object at the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setBaseTerrainColor(Vector3 color)",
            "If whole scene visible is true, sets the colour shown where none of the 4 paint layers are active. Call rebakePlanetOverview() afterwards to see the change.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "Vector3 getBaseTerrainColor()", "Gets the colour shown where none of the 4 paint layers are active.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setLayer0Color(Vector3 color)",
            "If whole scene visible is true, sets the colour for paint layer 0 (blend weight R channel). Call rebakePlanetOverview() afterwards to see the change.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "Vector3 getLayer0Color()", "Gets the colour for paint layer 0.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setLayer1Color(Vector3 color)",
            "If whole scene visible is true, sets the colour for paint layer 1 (blend weight G channel). Call rebakePlanetOverview() afterwards to see the change.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "Vector3 getLayer1Color()", "Gets the colour for paint layer 1.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setLayer2Color(Vector3 color)",
            "If whole scene visible is true, sets the colour for paint layer 2 (blend weight B channel). Call rebakePlanetOverview() afterwards to see the change.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "Vector3 getLayer2Color()", "Gets the colour for paint layer 2.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void setLayer3Color(Vector3 color)",
            "If whole scene visible is true, sets the colour for paint layer 3 (blend weight A channel). Call rebakePlanetOverview() afterwards to see the change.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "Vector3 getLayer3Color()", "Gets the colour for paint layer 3.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetMinimapComponent", "void switchPlanet(number newPlanetId, number newTargetId)",
            "Saves discovery for the current planet, tears down the workspace, switches to the given planet (and target, if non-zero) and sets the minimap back up, loading that planet's discovery state.");

        gameObjectClass.def("getPlanetMinimapComponentFromName", &getPlanetMinimapComponentFromName);
        gameObjectClass.def("getPlanetMinimapComponent", (PlanetMinimapComponent * (*)(GameObject*)) & getPlanetMinimapComponent);
        // If its desired to create several of this components for one game object
        gameObjectClass.def("getPlanetMinimapComponentFromIndex", (PlanetMinimapComponent * (*)(GameObject*, unsigned int)) & getPlanetMinimapComponentFromIndex);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetMinimapComponent getPlanetMinimapComponentFromIndex(unsigned int occurrenceIndex)",
            "Gets the component by the given occurence index, since a game object may this component maybe several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetMinimapComponent getPlanetMinimapComponent()", "Gets the component. This can be used if the game object this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetMinimapComponent getPlanetMinimapComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castPlanetMinimapComponent", &GameObjectController::cast<PlanetMinimapComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PlanetMinimapComponent castPlanetMinimapComponent(PlanetMinimapComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    std::optional<NOWA::GameObjectTypeDescriptor> PlanetMinimapComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "Planet Minimap";
        desc.meshToDisplay = "Camera.mesh";
        desc.needsMeshItem = true;
        desc.forceZeroTransform = false;
        desc.autoComponents = {PlanetMinimapComponent::getStaticClassName(), CameraComponent::getStaticClassName()};
        return desc;
    }

    bool PlanetMinimapComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

}; // namespace end