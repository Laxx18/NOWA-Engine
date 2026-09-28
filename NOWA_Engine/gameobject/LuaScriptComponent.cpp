#include "NOWAPrecompiled.h"
#include "LuaScriptComponent.h"
#include "GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/ProcessManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    class EXPORTED LuaMethodProcess : public Process
    {
    public:
        explicit LuaMethodProcess(luabind::object closureFunction) : closureFunction(closureFunction)
        {
        }

    protected:
        virtual void onInit(void) override
        {
            this->succeed();
            if (this->closureFunction.is_valid())
            {
                // Copy the closure into the command instead of capturing 'this'. The command runs
                // later on the logic thread, and the observer may already be gone by then - a raw
                // 'this' capture then dereferences freed memory.
                luabind::object callback = this->closureFunction;

                NOWA::AppStateManager::LogicCommand logicCommand = [callback]()
                {
                    try
                    {
                        luabind::call_function<void>(callback);
                    }
                    catch (luabind::error& error)
                    {
                        luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                        std::stringstream msg;
                        msg << errorMsg;

                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'LuaMethodProcess' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                    }
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }
        }

        virtual void onUpdate(float dt) override
        {
            this->succeed();
        }

    private:
        luabind::object closureFunction;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    using namespace rapidxml;
    using namespace luabind;

    LuaScriptComponent::LuaScriptComponent() :
        GameObjectComponent(),
        luaScript(nullptr),
        hasUpdateFunction(false),
        hasLateUpdateFunction(false),
        componentCloned(false),
        // Attention: true on purpose. It means "the lua 'disconnect' must not run now" and is only set
        // to false once the lua 'connect' really ran (see setActivated()). onRemoveComponent() now calls
        // disconnect() as well, and in the editor (component or game object removed, simulation never
        // started) that must not execute the lua 'disconnect' of a script that was never connected.
        alreadyDisconnected(true),
        activated(new Variant(LuaScriptComponent::AttrActivated(), true, this->attributes)),
        scriptFile(new Variant(LuaScriptComponent::AttrScriptFile(), Ogre::String(""), this->attributes)),
        cloneScript(new Variant(LuaScriptComponent::AttrCloneScript(), false, this->attributes)),
        commonScript(new Variant(LuaScriptComponent::AttrHasCommonScript(), false, this->attributes)),
        scriptGlobal(new Variant(LuaScriptComponent::AttrScriptGlobal(), false, this->attributes)),
        orderIndex(new Variant(LuaScriptComponent::AttrOrderIndex(), static_cast<int>(-1), this->attributes))
    {
        this->scriptFile->setDescription("Name of the script file, e.g. 'Explosion.lua'.");
        this->scriptFile->addUserData(GameObject::AttrActionNeedRefresh());
        this->scriptFile->addUserData(GameObject::AttrActionLuaScript());
        this->cloneScript->addUserData(GameObject::AttrActionNeedRefresh());
        this->commonScript->addUserData(GameObject::AttrActionNeedRefresh());
        this->scriptGlobal->addUserData(GameObject::AttrActionNeedRefresh());
        this->cloneScript->setDescription("If activated, a copy of the original script will be made for the new game object with the new cloned name. Else the cloned components will have no lua script component."
                                          "Attention: Cloning a lua script may be dangerous, because its content of this original component will also be cloned and executed.");
        this->commonScript->setDescription("Several game object may use the same script, if the script name for the lua script components is the same. This is useful if they should behave the same. This also increases the performance."
                                           "Also for example when the game object is cloned, the lua script component is cloned too, but referencing the original lua script file.");
        this->scriptGlobal
            ->setDescription("Only relevant if 'Has Common Script' is activated. A game object can be local to just one scene, but its (common) lua script can still be made global via this flag. "
                             "This lets hundreds of local, non-global game objects, spread across many different scenes, share and execute the exact same global lua script instance, "
                             "instead of every scene creating and compiling its own local copy of it. Has no effect while 'Has Common Script' is false; the script's global state then depends "
                             "solely on whether the game object itself is global. "
                             "Note: If the game object itself is already global, the script is unavoidably global too - this switch is then automatically shown as activated and locked (read-only), since toggling it would have no effect.");
        this->scriptGlobal->setVisible(false);
        this->orderIndex->setDescription("The order index, at which this lua script is executed at.");
        this->orderIndex->setReadOnly(true);

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &LuaScriptComponent::handleGroupLoaded), EventDataGroupLoaded::getStaticEventType());
    }

    LuaScriptComponent::~LuaScriptComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LuaScriptComponent] Destructor lua script component component for game object: " + this->gameObjectPtr->getName());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &LuaScriptComponent::handleGroupLoaded), EventDataGroupLoaded::getStaticEventType());

        if (nullptr != this->luaScript)
        {
            AppStateManager::getSingletonPtr()->getLuaScriptModule()->destroyScript(this->luaScript->getName());
            this->luaScript = nullptr;
        }
    }

    bool LuaScriptComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScriptFile")
        {
            this->scriptFile->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CloneScript")
        {
            this->setCloneScript(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HasCommonScript")
        {
            this->setHasCommonScript(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ScriptGlobal")
        {
            this->setScriptGlobal(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OrderIndex")
        {
            this->setOrderIndex(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    void LuaScriptComponent::maybeCopyScriptFromGroups(void)
    {
        // Check if in projects/Groups the corresponding lua file does exist and copy to target location, if there is no such lua file
        Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Projects");
        Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
        Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

        Ogre::String filePath;
        Ogre::String scriptSourceFilePathName;
        for (; it != itEnd; ++it)
        {
            filePath = (*it)->archive->getName() + "/Groups";
            scriptSourceFilePathName = filePath + "/" + this->scriptFile->getString();
            break;
        }

        Ogre::String scriptDestFilePathName;
        if (false == this->determineIsLuaScriptGlobal())
        {
            scriptDestFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptFile->getString();
        }
        else
        {
            scriptDestFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptFile->getString();
        }

        // Copy script to location, if it does not exist there
        std::ifstream scriptFile(scriptDestFilePathName, std::ios::in);
        if (false == scriptFile.good())
        {
            CopyFile(scriptSourceFilePathName.c_str(), scriptDestFilePathName.c_str(), TRUE);
        }
    }

    void LuaScriptComponent::setOrderIndex(int orderIndex)
    {
        this->orderIndex->setReadOnly(false);
        this->orderIndex->setValue(orderIndex);
        this->orderIndex->setReadOnly(true);
    }

    int LuaScriptComponent::getOrderIndex(void) const
    {
        return this->orderIndex->getUInt();
    }

    GameObjectCompPtr LuaScriptComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        if (false == this->cloneScript->getBool() && false == this->commonScript->getBool())
        {
            return GameObjectCompPtr();
        }

        LuaScriptCompPtr clonedCompPtr(boost::make_shared<LuaScriptComponent>());

        // Flag set to determine if component has been cloned, so that in post init, script will not be created again
        clonedCompPtr->componentCloned = true;

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setCloneScript(this->cloneScript->getBool());
        clonedCompPtr->setHasCommonScript(this->commonScript->getBool());
        clonedCompPtr->setScriptGlobal(this->scriptGlobal->getBool());
        clonedCompPtr->setOrderIndex(this->orderIndex->getUInt());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        // Owner is available now: re-sync in case the cloned game object's global state differs from the original one
        clonedCompPtr->syncScriptGlobalWithGameObject();

        // New game object must already be available
        if (false == this->cloneScript->getBool() && true == this->commonScript->getBool() && true == this->scriptFile->getString().empty())
        {
            // Use original name, so that cloned game object will automatically use the same script
            clonedCompPtr->setScriptFile(this->gameObjectPtr->getName() + ".lua", eScriptAction::LOAD);
        }
        else if (true == this->cloneScript->getBool())
        {
            // Set the original name
            clonedCompPtr->scriptFile->setValue(this->scriptFile->getString());
            clonedCompPtr->setScriptFile(clonedGameObjectPtr->getName() + ".lua", eScriptAction::CLONE);
            this->componentCloned = true;
        }
        else
        {
            clonedCompPtr->setScriptFile(this->scriptFile->getString(), eScriptAction::LOAD);
        }

        clonedCompPtr->compileScript();

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool LuaScriptComponent::postInit(void)
    {
        // Owner is guaranteed to be set by now: keep the "Script Global" switch in sync with it (e.g. after loading a
        // scene, so a game object that is global from XML correctly shows/locks the switch, even before any editing).
        this->syncScriptGlobalWithGameObject();

        // Do not set script a second time
        if (true == this->componentCloned || nullptr != this->luaScript)
        {
            return true;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LuaScriptComponent] Init lua script component for game object: " + this->gameObjectPtr->getName());

        if (true == this->scriptFile->getString().empty())
        {
            // this->scriptFile->setValue(this->gameObjectPtr->getName() + ".lua");
            this->setScriptFile("", eScriptAction::NEW);
        }
        else
        {
            this->setScriptFile(this->scriptFile->getString(), eScriptAction::LOAD);
        }
        return true;
    }

    void LuaScriptComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        // Attention: a game object deleted WHILE the simulation runs (e.g. an enemy killed and removed
        // via deleteDelayedGameObject) never got its lua 'disconnect' called - GameObject::destroy()
        // only calls onRemoveComponent(). The script's module ("Rhino_<id>") however stays alive in the
        // lua state, together with every event listener it registered and every game object reference
        // it holds. The next event (e.g. EnemyDeadEvent of the NEXT rhino) then ran the handler of the
        // deleted one, which called getId() on its already freed game object -> crash.
        //
        // disconnect() runs the lua 'disconnect' only if the lua 'connect' really ran and it was not
        // disconnected yet (alreadyDisconnected), so this is a no-op in the editor and after a normal
        // simulation stop. The script's 'disconnect' is expected to remove its event listeners and to
        // drop its game object references.
        this->disconnect();

        boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(this->gameObjectPtr->getId(), ""));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);
    }

    bool LuaScriptComponent::connect(void)
    {
        this->setActivated(this->activated->getBool());
        return true;
    }

    bool LuaScriptComponent::disconnect(void)
    {
        this->executedOnceKeys.clear();

        if (true == this->alreadyDisconnected)
        {
            return true;
        }

        if (nullptr != this->luaScript)
        {
            if (true == this->activated->getBool())
            {
                // this->luaScript->callFunction("disconnect");
                // Lots faster
                this->luaScript->callTableFunction("disconnect");
                this->luaScript->decompile();
                this->alreadyDisconnected = true;
            }

            // Reset for the lua script any runtime error, if connect is called. So that any external editor has the chance to clean up errors
            boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->luaScript->getScriptName(), this->luaScript->getScriptFilePathName(), -1, ""));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
        }
        return true;
    }

    void LuaScriptComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (nullptr != this->luaScript && false == notSimulating)
        {
            // Only call update permanently in lua when the function has been programmed in lua script
            if (this->activated->getBool() && true == this->hasUpdateFunction)
            {
                // this->luaScript->callFunction("update", dt);
                // Lots faster
                this->luaScript->callTableFunction("update", dt);
            }
        }
    }

    void LuaScriptComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (LuaScriptComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (LuaScriptComponent::AttrScriptFile() == attribute->getName())
        {
            this->setScriptFile(attribute->getString(), eScriptAction::RENAME);
        }
        else if (LuaScriptComponent::AttrCloneScript() == attribute->getName())
        {
            this->setCloneScript(attribute->getBool());
        }
        else if (LuaScriptComponent::AttrHasCommonScript() == attribute->getName())
        {
            this->setHasCommonScript(attribute->getBool());
        }
        else if (LuaScriptComponent::AttrScriptGlobal() == attribute->getName())
        {
            this->setScriptGlobal(attribute->getBool());
        }
        else if (LuaScriptComponent::AttrOrderIndex() == attribute->getName())
        {
            this->setOrderIndex(attribute->getUInt());
        }
    }

    void LuaScriptComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

        Ogre::String filePathName = Core::getSingletonPtr()->getCurrentProjectPath();

        // Copy script to location, if it does not exist in a group file path
        size_t foundGroup = filePathName.find("Groups");
        if (Ogre::String::npos != foundGroup)
        {
            std::ifstream scriptFileInGroup(filePathName + "/" + Core::getSingletonPtr()->getProjectName() + "/" + this->scriptFile->getString(), std::ios::in);
            // If its a common script, overwrite an existing file
            if (false == scriptFileInGroup.good() || true == this->commonScript->getBool())
            {
                Ogre::String scriptSourceFilePathName;
                if (false == this->determineIsLuaScriptGlobal())
                {
                    scriptSourceFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptFile->getString();
                }
                else
                {
                    scriptSourceFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptFile->getString();
                }

                Ogre::String scriptDestFilePathName = filePathName + "/" + this->scriptFile->getString();
                AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(scriptSourceFilePathName, scriptDestFilePathName, false, this->determineIsLuaScriptGlobal());
            }
            else
            {
                Ogre::String scriptSourceFilePathName;
                if (false == this->determineIsLuaScriptGlobal())
                {
                    scriptSourceFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptFile->getString();
                }
                else
                {
                    scriptSourceFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptFile->getString();
                }

                Ogre::String tempScriptFileName = AppStateManager::getSingletonPtr()->getLuaScriptModule()->getValidatedLuaScriptName(this->scriptFile->getString(), this->determineIsLuaScriptGlobal());
                Ogre::String scriptDestFilePathName = filePathName + "/" + Core::getSingletonPtr()->getProjectName() + "/" + tempScriptFileName;
                this->scriptFile->setValue(tempScriptFileName);
                AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(scriptSourceFilePathName.c_str(), scriptDestFilePathName.c_str(), false, this->determineIsLuaScriptGlobal());
            }
        }

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ScriptFile"));

        if (true == this->differentScriptNameForXML.empty())
        {
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scriptFile->getString())));
        }
        else
        {
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->differentScriptNameForXML)));
            this->differentScriptNameForXML.clear();
        }
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CloneScript"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cloneScript->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "HasCommonScript"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->commonScript->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ScriptGlobal"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scriptGlobal->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OrderIndex"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orderIndex->getUInt())));
        propertiesXML->append_node(propertyXML);
    }

    void LuaScriptComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (nullptr == this->luaScript)
        {
            return;
        }

        if (true == activated)
        {
            // Everything below touches the Lua state (compile, createLuaEnvironmentForTable,
            // callTableFunction) and MUST run on the main/logic thread - Lua itself is not
            // thread-safe, and calling into it from another thread (e.g. a worker thread
            // via AreaOfInterestComponent::update() -> ... -> restoreComponentActivationStates())
            // corrupts the Lua call stack. This used to run directly on whatever thread called
            // setActivated(), which worked by accident as long as everyone happened to call it
            // from the main thread - until AreaOfInterestComponent's worker-thread callback path
            // called it too, causing sporadic (and only reliably visible in Debug, due to timing)
            // crashes deep inside Lua's own error handling.
            NOWA::AppStateManager::LogicCommand logicCommand = [this]()
            {
                this->alreadyDisconnected = false;

                if (nullptr != this->luaScript && false == this->luaScript->isCompiled())
                {
                    this->luaScript->compile();
                }

                this->hasUpdateFunction = AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaFunctionAvailable(this->luaScript->getName(), "update");
                this->hasLateUpdateFunction = AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaFunctionAvailable(this->luaScript->getName(), "lateUpdate");

                Ogre::String scriptTableName = this->scriptFile->getString();
                scriptTableName.erase(scriptTableName.find_last_of("."), Ogre::String::npos);

                if (true == this->luaScript->createLuaEnvironmentForTable(scriptTableName))
                {
                    if (true == this->componentCloned)
                    {
                        this->luaScript->callTableFunction("cloned", this->gameObjectPtr.get());
                    }
                    this->luaScript->callTableFunction("connect", this->gameObjectPtr.get());

                    boost::shared_ptr<EventDataLuaScriptConnected> eventDataLuaScriptConnected(new EventDataLuaScriptConnected(this->gameObjectPtr->getId()));
                    NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptConnected);
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        }
    }

    bool LuaScriptComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    Ogre::String LuaScriptComponent::getClassName(void) const
    {
        return "LuaScriptComponent";
    }

    Ogre::String LuaScriptComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void LuaScriptComponent::setScriptFile(const Ogre::String& scriptFile, eScriptAction scriptAction)
    {
        // Special case: When copying scene, a different script name may be required for writing out to xml, but this lua script must be untouched, so just use this
        // differentScriptNameForXML one times for writing xml and erase afterwards.
        if (eScriptAction::WRITE_XML == scriptAction)
        {
            this->differentScriptNameForXML = scriptFile;
            return;
        }

        Ogre::String oldScriptFile = this->scriptFile->getString();
        Ogre::String tempScriptFileName = scriptFile;
        if (false == this->commonScript->getBool() && (eScriptAction::NEW == scriptAction || eScriptAction::RENAME == scriptAction))
        {
            if (true == tempScriptFileName.empty())
            {
                tempScriptFileName = this->gameObjectPtr->getName() + ".lua";
            }
            // Script name may exist and must be replaced, before a copy is made
            tempScriptFileName = AppStateManager::getSingletonPtr()->getLuaScriptModule()->getValidatedLuaScriptName(tempScriptFileName, this->determineIsLuaScriptGlobal());
        }
        else if (eScriptAction::CLONE == scriptAction)
        {
            // Script name may exist and must be replaced, before a copy is made
            Ogre::String newScriptName = AppStateManager::getSingletonPtr()->getLuaScriptModule()->getValidatedLuaScriptName(tempScriptFileName, this->determineIsLuaScriptGlobal());
            AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScript(this->scriptFile->getString(), tempScriptFileName, false, this->determineIsLuaScriptGlobal());

            boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);

            boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(this->gameObjectPtr->getId(), newScriptName));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);
        }

        size_t found = tempScriptFileName.find(".lua");
        if (Ogre::String::npos == found)
        {
            tempScriptFileName += ".lua";
        }

        this->scriptFile->setValue(tempScriptFileName);

        // Get unique script name without .lua extension
        size_t lastindex = tempScriptFileName.find_last_of(".");
        Ogre::String unqiueScriptName = tempScriptFileName.substr(0, lastindex);
        Ogre::String tempScriptName = unqiueScriptName;
        unqiueScriptName += "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        if (eScriptAction::RENAME == scriptAction)
        {
            if (nullptr != this->luaScript && scriptFile != this->luaScript->getName())
            {
                AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScript(this->luaScript->getScriptName(), tempScriptFileName, true, this->determineIsLuaScriptGlobal());
                AppStateManager::getSingletonPtr()->getLuaScriptModule()->destroyScript(this->luaScript);

                boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);

                boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(this->gameObjectPtr->getId(), tempScriptFileName));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);
            }
        }

        this->luaScript = AppStateManager::getSingletonPtr()->getLuaScriptModule()->createScript(unqiueScriptName, tempScriptFileName, this->determineIsLuaScriptGlobal());

        if (nullptr == this->luaScript)
        {
            // Script does exist already
            return;
        }
        // Set the lua script also for the game object
        this->gameObjectPtr->luaScript = this->luaScript;

        if (false == AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaScriptFileExists(tempScriptFileName, this->determineIsLuaScriptGlobal()))
        {
            Ogre::String firstLetter = tempScriptName.substr(0, 1);
            Ogre::StringUtil::toLowerCase(firstLetter);
            Ogre::String tempScriptVariable = firstLetter + tempScriptName.substr(1, tempScriptName.length() - 1);

            if (tempScriptVariable == tempScriptName)
            {
                tempScriptVariable += "_";
            }

            this->luaScript->setInterfaceFunctionsTemplate(+"-- Scene: " + Core::getSingletonPtr()->getSceneName() + "\n\n" +
                                                           "require("
                                                           "\"init"
                                                           "\");\n\n" +
                                                           "local " + tempScriptVariable +
                                                           " = nil\n\n"
                                                           "-- local physicsActiveComponent = nil;\n\n" +
                                                           tempScriptName + " = {}\n\n" + tempScriptName +
                                                           "["
                                                           "\"connect"
                                                           "\"] = function(gameObject)\n"
                                                           "\t--" +
                                                           tempScriptVariable +
                                                           " = AppStateManager:getGameObjectController():castGameObject(gameObject);\n"
                                                           "\t--physicsActiveComponent = " +
                                                           tempScriptVariable +
                                                           ":getPhysicsActiveComponent();\nend"
                                                           "\n\n" +
                                                           tempScriptName +
                                                           "["
                                                           "\"disconnect"
                                                           "\"] = function()\n\nend"
                                                           "\n\n" +
                                                           "--" + tempScriptName +
                                                           "["
                                                           "\"update"
                                                           "\"] = function(dt)\n"
                                                           "\t--physicsActiveComponent:applyOmegaForce(Vector3(0, 10, 0));\n--end");

            // http://blog.nuclex-games.com/tutorials/cxx/luabind-introduction/

            // https://forums.ogre3d.org/viewtopic.php?t=13837
            // Tells lua that there is a global variable called "game"
            // luabind::globals(LuaScriptApi::getInstance()->getLua())[this->getClassName()] = this;

            //// Saves the reference to the global variable "game" so that
            //// we can refer to it from c++
            // this->thisClassObject = luabind::globals(LuaScriptApi::getInstance()->getLua())[this->getClassName()];

            if (eScriptAction::NEW == scriptAction || eScriptAction::LOAD == scriptAction)
            {
                this->compileScript();
                boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(this->gameObjectPtr->getId(), tempScriptFileName));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);
            }
        }
        else if (eScriptAction::RENAME == scriptAction)
        {
            if (true == this->commonScript->getBool())
            {
                // If a common script has been renamed, maybe there are other game objects, which have a lua script component, that points to the same script
                // Hence adapt also the script name for the other one
                const auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
                for (auto it = gameObjects->begin(); it != gameObjects->end(); it++)
                {
                    auto gameObject = it->second;
                    auto luaScriptComponent = NOWA::makeStrongPtr(gameObject->getComponent<LuaScriptComponent>());
                    if (nullptr != luaScriptComponent)
                    {
                        if (luaScriptComponent->getScriptFile() == oldScriptFile)
                        {
                            AppStateManager::getSingletonPtr()->getLuaScriptModule()->destroyScript(luaScriptComponent->getLuaScript());
                            luaScriptComponent->setScriptFile(tempScriptFileName, eScriptAction::SET);

                            boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(this->gameObjectPtr->getId(), tempScriptFileName));
                            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);
                        }
                    }
                }
            }
        }

        Ogre::String relativeLuaScriptFilePathName;
        if (false == this->determineIsLuaScriptGlobal())
        {
            relativeLuaScriptFilePathName = NOWA::Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + tempScriptFileName;
        }
        else
        {
            relativeLuaScriptFilePathName = NOWA::Core::getSingletonPtr()->getCurrentProjectPath() + "/" + tempScriptFileName;
        }
        Ogre::String luaScriptFilePathName = NOWA::Core::getSingletonPtr()->getAbsolutePath(relativeLuaScriptFilePathName);

        this->luaScript->setScriptFile(tempScriptFileName);
        this->luaScript->setScriptFilePathName(luaScriptFilePathName);
        this->scriptFile->addUserData(GameObject::AttrActionLuaScript(), luaScriptFilePathName);

        AppStateManager::getSingletonPtr()->getGameObjectController()->addLuaScript(boost::dynamic_pointer_cast<LuaScriptComponent>(shared_from_this()));
    }

    void LuaScriptComponent::handleGroupLoaded(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataGroupLoaded> castEventData = boost::static_pointer_cast<EventDataGroupLoaded>(eventData);

        if (castEventData->getGameObjectId() != this->gameObjectPtr->getId())
        {
            return;
        }

        this->maybeCopyScriptFromGroups();
    }

    Ogre::String LuaScriptComponent::getScriptFile(void) const
    {
        return this->scriptFile->getString();
    }

    void LuaScriptComponent::setCloneScript(bool cloneScript)
    {
        this->cloneScript->setValue(cloneScript);
        if (true == cloneScript)
        {
            // Both is not possible, either clone a script or use a common script!
            this->commonScript->setVisible(false);
            this->commonScript->setValue(false);
        }
        else
        {
            this->commonScript->setVisible(true);
        }
    }

    bool LuaScriptComponent::getCloneScript(void) const
    {
        return this->cloneScript->getBool();
    }

    void LuaScriptComponent::setHasCommonScript(bool commonScript)
    {
        this->commonScript->setValue(commonScript);
        if (true == commonScript)
        {
            // Both is not possible, either clone a script or use a common script!
            this->cloneScript->setVisible(false);
            this->cloneScript->setValue(false);

            // ScriptGlobal only makes sense in combination with a common script
            this->scriptGlobal->setVisible(true);
        }
        else
        {
            this->cloneScript->setVisible(true);

            // Without a common script there is nothing to make independently global; fall back to the game object's own global state
            this->scriptGlobal->setVisible(false);
            this->scriptGlobal->setValue(false);
        }

        // If the owner is already global, the switch above must be overridden again: it always shows activated + locked in that case
        this->syncScriptGlobalWithGameObject();
    }

    bool LuaScriptComponent::getHasCommonScript(void) const
    {
        return this->commonScript->getBool();
    }

    void LuaScriptComponent::setScriptGlobal(bool scriptGlobal)
    {
        // If the game object itself is already global, the script cannot be made non-global again - it stays forced to true (see @syncScriptGlobalWithGameObject).
        if (nullptr != this->gameObjectPtr && true == this->gameObjectPtr->getGlobal())
        {
            scriptGlobal = true;
        }

        this->scriptGlobal->setValue(scriptGlobal);

        // Propagate to the actual lua script instance (if it already exists), so its internal global state
        // (used for e.g. resource group / storage path resolution) stays in sync with the component's setting.
        // NOTE: verify "setGlobal" is the correct LuaScript setter name for your LuaScript.h - adjust if it differs.
        if (nullptr != this->luaScript)
        {
            this->luaScript->setIsGlobal(this->determineIsLuaScriptGlobal());
        }
    }

    bool LuaScriptComponent::getIsScriptGlobal(void) const
    {
        return this->scriptGlobal->getBool();
    }

    bool LuaScriptComponent::determineIsLuaScriptGlobal(void) const
    {
        // A script is global either because the whole game object is global (legacy behavior, affects everything on that
        // game object including this script), or - new - because the game object is local but this component has a common
        // script that has explicitly been marked global. This lets hundreds of local game objects, spread across many
        // scenes, share and execute the exact same global lua script instance.
        return this->gameObjectPtr->getGlobal() || (true == this->commonScript->getBool() && true == this->scriptGlobal->getBool());
    }

    void LuaScriptComponent::syncScriptGlobalWithGameObject(void)
    {
        // Owner not assigned yet (e.g. during clone(), before setOwner() has been called) - nothing to sync against yet.
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        if (true == this->gameObjectPtr->getGlobal())
        {
            // The game object itself is global, hence the script is unavoidably global too (@determineIsLuaScriptGlobal).
            // Show and lock the switch accordingly, instead of leaving it editable and possibly showing "false" while the
            // effective state is actually "true".
            this->scriptGlobal->setVisible(true);
            this->scriptGlobal->setValue(true);
            this->scriptGlobal->setReadOnly(true);
        }
        else
        {
            // Game object is local again: unlock the switch and fall back to the normal visibility rule (only relevant
            // together with a common script).
            this->scriptGlobal->setReadOnly(false);
            this->scriptGlobal->setVisible(this->commonScript->getBool());
        }
    }

    LuaScript* LuaScriptComponent::getLuaScript(void) const
    {
        return this->luaScript;
    }

    void LuaScriptComponent::callDelayedMethod(luabind::object closureFunction, Ogre::Real delaySec)
    {
        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(delaySec));
        delayProcess->attachChild(NOWA::ProcessPtr(new LuaMethodProcess(closureFunction)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
    }

    void LuaScriptComponent::callMethodOnce(const Ogre::String& key, luabind::object closureFunction)
    {
        Ogre::String uniqueKey = this->gameObjectPtr->getName() + "_" + key;
        // Lua creates a FRESH closure object every time the calling line executes (e.g. from
        // within update()), so tracking "already called" via closure identity would never work -
        // it would just fire every time. An explicit, caller-chosen key is the only reliable way
        // to identify "this particular call site" across repeated invocations.
        if (this->executedOnceKeys.find(uniqueKey) != this->executedOnceKeys.end())
        {
            return;
        }

        this->executedOnceKeys.insert(uniqueKey);

        // Re-use the exact same invocation path as callDelayedMethod (LuaMethodProcess handles the
        // actual luabind call + error handling there already) - just without the DelayProcess
        // parent, so it fires as soon as the ProcessManager next ticks, instead of after a delay.
        NOWA::ProcessPtr methodProcess(new LuaMethodProcess(closureFunction));
        NOWA::ProcessManager::getInstance()->attachProcess(methodProcess);
    }

    bool LuaScriptComponent::compileScript(void)
    {
        // Just compile but do not call any connect function
        if (true == this->activated->getBool() && nullptr != this->luaScript)
        {
            this->luaScript->compile();
            return this->luaScript->isCompiled();
        }
        else
        {
            return true;
        }
    }

    void LuaScriptComponent::setComponentCloned(bool componentCloned)
    {
        this->componentCloned = componentCloned;
    }

}; // namespace end