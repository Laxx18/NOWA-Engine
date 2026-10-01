#include "NOWAPrecompiled.h"
#include "DeployResourceModule.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/rapidxml.hpp"

#include "OgreArchive.h"
#include "OgreHlms.h"
#include "OgreHlmsDatablock.h"
#include "OgreHlmsManager.h"
#include "OgreMesh.h"
#include "OgreMesh2.h"
#include "OgreMeshManager.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh.h"
#include "OgreSubMesh2.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <cctype>
#include <deque>
#include <filesystem>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <unordered_set>

namespace fs = std::filesystem;

namespace
{
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

    // ------------------------------------------------------------------------------------------------
    // Deploy configuration
    // ------------------------------------------------------------------------------------------------

    // Folders (relative to the media root) that are copied completely, because their content is not referenced by name from scenes:
    // shaders, Hlms templates, compositor scripts, MyGUI skins/layouts/fonts, engine Lua libraries.
    // Their text files (compositors, materials, json, particle scripts, MyGUI xml) are still scanned, so that e.g. textures
    // referenced by a compositor are deployed too.
    const char* const fullCopyFolders[] = {"Hlms", "2.0/scripts/materials/Common", "Compute", "NOWA/PriorScripts", "NOWA/Scripts", "MyGUI_Media", "fonts", "lua"};

    // Editor only folders, never deployed
    const char* const excludedFolders[] = {"MyGUI_Media/NOWA_Design", "MyGUI_Media/brushes"};

    // Resource groups (sections in the resources cfg) that only the editor needs
    const char* const editorOnlySections[] = {"NOWA_Design", "Brushes"};

    // Resources the engine code uses by name (not referenced from any scene)
    const char* const engineSeedNames[] = {"Missing.mesh", "Node.mesh", "Camera.mesh", "Procedural.mesh", "RedNoLighting", "GreenNoLighting", "BlueNoLighting", "BaseYellowLine", "circleRed.png"};

    // Text files whose content is scanned for further references
    const char* const scannedTextExtensions[] = {".lua", ".particle", ".particle2", ".material", ".program", ".compositor", ".xml", ".layout", ".json", ".txt"};

    // Optional file in the project folder: one entry per line (file name, datablock name, particle template, or a folder relative to the media root,
    // which is then copied completely). Lines starting with '#' are comments.
    const char* const additionalDeployFileName = "DeployAdditional.txt";

    const size_t maxListedWarnings = 50;

    // ------------------------------------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------------------------------------

    Ogre::String toLower(Ogre::String text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return text;
    }

    Ogre::String trim(const Ogre::String& text)
    {
        const size_t first = text.find_first_not_of(" \t\r\n");
        if (Ogre::String::npos == first)
        {
            return Ogre::String();
        }
        const size_t last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    Ogre::String getLowerExtension(const Ogre::String& fileName)
    {
        return toLower(fs::path(fileName).extension().string());
    }

    bool hasScannedTextExtension(const Ogre::String& fileName)
    {
        const Ogre::String extension = getLowerExtension(fileName);
        for (const char* scannedExtension : scannedTextExtensions)
        {
            if (extension == scannedExtension)
            {
                return true;
            }
        }
        return false;
    }

    // Case-insensitive check whether relativePath (generic, '/' separated) is the folder itself or lies inside it
    bool isInsideFolder(const Ogre::String& relativePath, const Ogre::String& folder)
    {
        const Ogre::String lowerPath = toLower(relativePath);
        const Ogre::String lowerFolder = toLower(folder);
        if (lowerPath == lowerFolder)
        {
            return true;
        }
        if (lowerPath.size() > lowerFolder.size() && 0 == lowerPath.compare(0, lowerFolder.size(), lowerFolder) && '/' == lowerPath[lowerFolder.size()])
        {
            return true;
        }
        return false;
    }

    bool isInFullCopyFolder(const Ogre::String& relativePath)
    {
        for (const char* folder : fullCopyFolders)
        {
            if (true == isInsideFolder(relativePath, folder))
            {
                return true;
            }
        }
        return false;
    }

    bool isExcluded(const Ogre::String& relativePath)
    {
        for (const char* folder : excludedFolders)
        {
            if (true == isInsideFolder(relativePath, folder))
            {
                return true;
            }
        }
        return false;
    }

    // Relative path of 'path' below 'root' in generic form ('/' separated), or empty if 'path' is not below 'root'
    Ogre::String getRelativePath(const fs::path& path, const fs::path& root)
    {
        std::error_code errorCode;
        const fs::path relative = fs::relative(path, root, errorCode);
        if (errorCode || true == relative.empty())
        {
            return Ogre::String();
        }
        const Ogre::String relativeString = relative.generic_string();
        if (0 == relativeString.compare(0, 2, ".."))
        {
            return Ogre::String();
        }
        return relativeString;
    }

    bool readTextFile(const fs::path& filePath, Ogre::String& content)
    {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (false == file.is_open())
        {
            return false;
        }
        content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return true;
    }

    bool readProjectTextFile(const fs::path& filePath, bool expectXml, Ogre::String& content)
    {
        if (false == readTextFile(filePath, content))
        {
            return false;
        }
        // Encrypted project files are detected by the content
        content = NOWA::Core::getSingletonPtr()->decryptContent(content);
        return true;
    }

    // Collects "..." and '...' literals (single line, reasonable length)
    void collectQuotedStrings(const Ogre::String& text, std::vector<Ogre::String>& tokens)
    {
        size_t position = 0;
        while (position < text.size())
        {
            const char c = text[position];
            if ('"' == c || '\'' == c)
            {
                const size_t end = text.find(c, position + 1);
                if (Ogre::String::npos == end)
                {
                    break;
                }
                const Ogre::String literal = text.substr(position + 1, end - position - 1);
                if (false == literal.empty() && literal.size() < 260 && Ogre::String::npos == literal.find('\n'))
                {
                    tokens.emplace_back(literal);
                }
                position = end + 1;
            }
            else
            {
                position++;
            }
        }
    }

    // Collects all quoted literals plus all words (for script formats like particle scripts: "material MyMaterial")
    void collectTokensFromText(const Ogre::String& text, std::vector<Ogre::String>& tokens)
    {
        collectQuotedStrings(text, tokens);

        const Ogre::String delimiters = " \t\r\n\"',;:=(){}[]<>";
        size_t start = text.find_first_not_of(delimiters);
        while (Ogre::String::npos != start)
        {
            const size_t end = text.find_first_of(delimiters, start);
            if (Ogre::String::npos == end)
            {
                tokens.emplace_back(text.substr(start));
                break;
            }
            tokens.emplace_back(text.substr(start, end - start));
            start = text.find_first_not_of(delimiters, end);
        }
    }

    void collectTokensFromXmlNode(rapidxml::xml_node<>* node, std::vector<Ogre::String>& tokens)
    {
        for (rapidxml::xml_node<>* child = node; nullptr != child; child = child->next_sibling())
        {
            for (rapidxml::xml_attribute<>* attribute = child->first_attribute(); nullptr != attribute; attribute = attribute->next_attribute())
            {
                const Ogre::String value = attribute->value();
                if (false == value.empty())
                {
                    // The complete value (names with spaces like "Level 1 - Awakening Moon Overworld.ogg")
                    tokens.emplace_back(value);

                    // Lists like "a.png;b.png" are split too, but not values with spaces (they are names or texts, and their fragments would only produce false warnings)
                    if (Ogre::String::npos == value.find_first_of(" \t"))
                    {
                        collectTokensFromText(value, tokens);
                    }
                }
            }
            if (nullptr != child->first_node())
            {
                collectTokensFromXmlNode(child->first_node(), tokens);
            }
        }
    }

    bool copyFileOverwrite(const fs::path& from, const fs::path& to, uintmax_t& copiedBytes)
    {
        std::error_code errorCode;
        fs::create_directories(to.parent_path(), errorCode);
        errorCode.clear();
        fs::copy_file(from, to, fs::copy_options::overwrite_existing, errorCode);
        if (errorCode)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not copy: '" + from.string() + "' to: '" + to.string() + "': " + errorCode.message());
            return false;
        }
        copiedBytes += fs::file_size(from, errorCode);
        return true;
    }

    struct IndexedFile
    {
        fs::path absolutePath;
        Ogre::String relativePath; // Relative to the media root, '/' separated
    };

    struct DeployContext
    {
        fs::path mediaRoot;
        fs::path deployRoot;
        fs::path deployMediaRoot;

        // Lower case file name -> all files with that name (the same name may exist in several folders/resource groups)
        std::map<Ogre::String, std::vector<IndexedFile>> fileIndex;
        // Datablock name -> absolute path of the json file that defines it
        std::map<Ogre::String, fs::path> datablockToJson;
        // Particle system template name -> absolute path of the script that defines it
        std::map<Ogre::String, fs::path> particleTemplateToFile;
        // Absolute paths of json files, that define datablocks (they are pruned instead of copied)
        std::set<Ogre::String> materialJsonFiles;

        std::deque<Ogre::String> pendingTokens;
        std::set<Ogre::String> seenTokens;

        std::map<Ogre::String, IndexedFile> usedFiles; // Key: absolute path
        std::set<Ogre::String> usedDatablocks;
        std::map<Ogre::String, std::set<Ogre::String>> jsonToUsedDatablocks; // Key: absolute json path
        std::map<Ogre::String, std::unique_ptr<rapidjson::Document>> jsonCache;

        std::vector<Ogre::String> additionalFullCopyFolders;
        std::set<Ogre::String> missingFileReferences;
        // Tokens that may legitimately not exist (engine seeds), no warning for them
        std::set<Ogre::String> optionalTokens;

        size_t meshCount = 0;
    };

    bool isFullCopyPath(const DeployContext& context, const Ogre::String& relativePath)
    {
        if (true == isInFullCopyFolder(relativePath))
        {
            return true;
        }
        for (const Ogre::String& folder : context.additionalFullCopyFolders)
        {
            if (true == isInsideFolder(relativePath, folder))
            {
                return true;
            }
        }
        return false;
    }

    void addToken(DeployContext& context, const Ogre::String& rawToken)
    {
        const Ogre::String token = trim(rawToken);
        if (true == token.empty() || token.size() > 260)
        {
            return;
        }
        if (false == context.seenTokens.insert(token).second)
        {
            return;
        }
        context.pendingTokens.push_back(token);
    }

    void addTokens(DeployContext& context, const std::vector<Ogre::String>& tokens)
    {
        for (const Ogre::String& token : tokens)
        {
            addToken(context, token);
        }
    }

    rapidjson::Document* getJsonDocument(DeployContext& context, const fs::path& jsonPath)
    {
        const Ogre::String key = jsonPath.string();
        auto found = context.jsonCache.find(key);
        if (found != context.jsonCache.end())
        {
            return found->second.get();
        }

        std::unique_ptr<rapidjson::Document> document(new rapidjson::Document());
        Ogre::String content;
        if (true == readTextFile(jsonPath, content))
        {
            // Default flags only: the rapidjson version bundled with Ogre knows neither kParseCommentsFlag nor kParseTrailingCommasFlag
            document->Parse(content.c_str());
        }
        if (true == document->HasParseError() || false == document->IsObject())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not parse material json: '" + key + "'");
            document.reset();
        }

        rapidjson::Document* result = document.get();
        context.jsonCache[key] = std::move(document);
        return result;
    }

    bool isBlockSection(const Ogre::String& sectionName)
    {
        return "samplers" == sectionName || "macroblocks" == sectionName || "blendblocks" == sectionName;
    }

    // Gets the textures etc. a datablock references, by serializing its json object
    void collectDatablockTokens(DeployContext& context, const Ogre::String& datablockName, const fs::path& jsonPath)
    {
        rapidjson::Document* document = getJsonDocument(context, jsonPath);
        if (nullptr == document)
        {
            return;
        }

        for (auto section = document->MemberBegin(); section != document->MemberEnd(); ++section)
        {
            if (true == isBlockSection(section->name.GetString()) || false == section->value.IsObject())
            {
                continue;
            }

            auto datablockMember = section->value.FindMember(datablockName.c_str());
            if (datablockMember != section->value.MemberEnd())
            {
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                datablockMember->value.Accept(writer);

                std::vector<Ogre::String> tokens;
                collectTokensFromText(buffer.GetString(), tokens);
                addTokens(context, tokens);
            }
        }
    }

    // Loads (if necessary) the mesh on the render thread and delivers its skeleton and sub mesh material names
    void collectMeshTokens(DeployContext& context, const Ogre::String& meshName)
    {
        std::vector<Ogre::String> tokens;

        NOWA::GraphicsModule::RenderCommand renderCommand = [&tokens, meshName]()
        {
            const Ogre::String group = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
            try
            {
                // Note: The implicit bool conversion works for Ogre's SharedPtr and for std::shared_ptr
                Ogre::MeshPtr existingMesh = Ogre::MeshManager::getSingleton().getByName(meshName, group);
                bool wasLoaded = false;
                if (existingMesh)
                {
                    wasLoaded = existingMesh->isLoaded();
                }

                Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().load(meshName, group);
                if (mesh)
                {
                    tokens.emplace_back(mesh->getSkeletonName());
                    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); i++)
                    {
                        tokens.emplace_back(mesh->getSubMesh(i)->mMaterialName);
                    }

                    // Do not keep meshes in the editor, which were only loaded for the deploy analysis
                    if (false == wasLoaded)
                    {
                        Ogre::MeshManager::getSingleton().remove(mesh->getHandle());
                    }
                }
                return;
            }
            catch (Ogre::Exception&)
            {
                // Maybe a v1 mesh, see below
            }

            try
            {
                Ogre::v1::MeshPtr existingMesh = Ogre::v1::MeshManager::getSingleton().getByName(meshName, group);
                bool wasLoaded = false;
                if (existingMesh)
                {
                    wasLoaded = existingMesh->isLoaded();
                }

                Ogre::v1::MeshPtr mesh = Ogre::v1::MeshManager::getSingleton().load(meshName, group);
                if (mesh)
                {
                    tokens.emplace_back(mesh->getSkeletonName());
                    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); i++)
                    {
                        tokens.emplace_back(mesh->getSubMesh(i)->getMaterialName());
                    }

                    if (false == wasLoaded)
                    {
                        Ogre::v1::MeshManager::getSingleton().remove(mesh->getHandle());
                    }
                }
            }
            catch (Ogre::Exception& exception)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not analyze mesh: '" + meshName + "': " + exception.getDescription());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DeployResourceModule::collectMeshTokens");

        context.meshCount++;
        addTokens(context, tokens);
    }

    void markFileUsed(DeployContext& context, const IndexedFile& indexedFile)
    {
        const Ogre::String key = indexedFile.absolutePath.string();

        // Material json files are pruned to the used datablocks, never copied as a whole
        if (context.materialJsonFiles.end() != context.materialJsonFiles.find(key) && false == isFullCopyPath(context, indexedFile.relativePath))
        {
            return;
        }

        if (false == context.usedFiles.emplace(key, indexedFile).second)
        {
            return;
        }

        const Ogre::String fileName = indexedFile.absolutePath.filename().string();
        const Ogre::String extension = getLowerExtension(fileName);

        if (".mesh" == extension)
        {
            collectMeshTokens(context, fileName);
        }
        else if (true == hasScannedTextExtension(fileName))
        {
            Ogre::String content;
            if (true == readProjectTextFile(indexedFile.absolutePath, ".xml" == extension || ".layout" == extension, content))
            {
                std::vector<Ogre::String> tokens;
                if (".lua" == extension)
                {
                    collectQuotedStrings(content, tokens);
                }
                else
                {
                    collectTokensFromText(content, tokens);
                }
                addTokens(context, tokens);
            }
        }
    }

    void markDatablockUsed(DeployContext& context, const Ogre::String& datablockName)
    {
        if (false == context.usedDatablocks.insert(datablockName).second)
        {
            return;
        }

        const auto found = context.datablockToJson.find(datablockName);
        if (found == context.datablockToJson.end())
        {
            return;
        }

        context.jsonToUsedDatablocks[found->second.string()].insert(datablockName);
        collectDatablockTokens(context, datablockName, found->second);
    }

    void resolvePendingTokens(DeployContext& context)
    {
        while (false == context.pendingTokens.empty())
        {
            const Ogre::String token = context.pendingTokens.front();
            context.pendingTokens.pop_front();

            bool resolved = false;

            // Tokens may also contain a path ("textures/Rock.png"), only the file name is relevant for Ogre resources
            const auto foundFiles = context.fileIndex.find(toLower(fs::path(token).filename().string()));
            if (foundFiles != context.fileIndex.end())
            {
                // Several files with the same name may exist in different folders; which one Ogre resolves depends on the group, so deploy all
                for (const IndexedFile& indexedFile : foundFiles->second)
                {
                    markFileUsed(context, indexedFile);
                }
                resolved = true;
            }

            if (context.datablockToJson.end() != context.datablockToJson.find(token))
            {
                markDatablockUsed(context, token);
                resolved = true;
            }

            const auto foundTemplate = context.particleTemplateToFile.find(token);
            if (foundTemplate != context.particleTemplateToFile.end())
            {
                const auto foundTemplateFiles = context.fileIndex.find(toLower(foundTemplate->second.filename().string()));
                if (foundTemplateFiles != context.fileIndex.end())
                {
                    for (const IndexedFile& indexedFile : foundTemplateFiles->second)
                    {
                        if (indexedFile.absolutePath == foundTemplate->second)
                        {
                            markFileUsed(context, indexedFile);
                        }
                    }
                }
                resolved = true;
            }

            // A reference that looks like a file (has a known resource extension), but does not exist: worth a warning
            if (false == resolved && context.optionalTokens.end() == context.optionalTokens.find(token))
            {
                const Ogre::String extension = getLowerExtension(token);
                if (".mesh" == extension || ".skeleton" == extension || ".png" == extension || ".dds" == extension || ".jpg" == extension || ".tga" == extension || ".wav" == extension || ".ogg" == extension || ".rag" == extension ||
                    ".lua" == extension)
                {
                    context.missingFileReferences.insert(token);
                }
            }
        }
    }
}

namespace
{
    bool isEditorOnlySection(const Ogre::String& sectionName)
    {
        for (const char* section : editorOnlySections)
        {
            if (sectionName == section)
            {
                return true;
            }
        }
        return false;
    }

    // Index of all files in all file system resource locations below the media root. Must run on the render thread (Ogre resource system).
    void buildFileIndex(DeployContext& context)
    {
        Ogre::ResourceGroupManager& resourceGroupManager = Ogre::ResourceGroupManager::getSingleton();
        const Ogre::StringVector groups = resourceGroupManager.getResourceGroups();

        for (const Ogre::String& group : groups)
        {
            if (true == isEditorOnlySection(group))
            {
                continue;
            }

            const Ogre::ResourceGroupManager::LocationList& locations = resourceGroupManager.getResourceLocationList(group);
            for (auto locationIt = locations.cbegin(); locationIt != locations.cend(); ++locationIt)
            {
                Ogre::Archive* archive = (*locationIt)->archive;
                if (nullptr == archive || "FileSystem" != archive->getType())
                {
                    continue;
                }

                std::error_code errorCode;
                const fs::path basePath = fs::weakly_canonical(fs::path(archive->getName()), errorCode);
                if (errorCode)
                {
                    continue;
                }

                const Ogre::String baseRelativePath = getRelativePath(basePath, context.mediaRoot);
                // Outside of the media root (e.g. external tools) or editor only: not part of a game
                if ((true == baseRelativePath.empty() && basePath != context.mediaRoot) || true == isExcluded(baseRelativePath))
                {
                    continue;
                }

                Ogre::StringVectorPtr fileNames = archive->list((*locationIt)->recursive, false);
                for (const Ogre::String& fileName : *fileNames)
                {
                    const fs::path absolutePath = basePath / fs::path(fileName);
                    const Ogre::String relativePath = getRelativePath(absolutePath, context.mediaRoot);
                    if (true == relativePath.empty() || true == isExcluded(relativePath))
                    {
                        continue;
                    }

                    std::vector<IndexedFile>& entries = context.fileIndex[toLower(absolutePath.filename().string())];
                    bool alreadyKnown = false;
                    for (const IndexedFile& entry : entries)
                    {
                        if (entry.absolutePath == absolutePath)
                        {
                            alreadyKnown = true;
                            break;
                        }
                    }
                    if (false == alreadyKnown)
                    {
                        IndexedFile indexedFile;
                        indexedFile.absolutePath = absolutePath;
                        indexedFile.relativePath = relativePath;
                        entries.emplace_back(indexedFile);
                    }
                }
            }
        }
    }

    // Maps every datablock that was loaded from a json file to that file. Must run on the render thread (Hlms).
    void buildDatablockIndex(DeployContext& context)
    {
        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();

        for (size_t i = Ogre::HLMS_LOW_LEVEL + 1u; i < Ogre::HLMS_MAX; ++i)
        {
            Ogre::Hlms* hlms = hlmsManager->getHlms(static_cast<Ogre::HlmsTypes>(i));
            if (nullptr == hlms)
            {
                continue;
            }

            const Ogre::Hlms::HlmsDatablockMap& datablocks = hlms->getDatablockMap();
            for (auto it = datablocks.cbegin(); it != datablocks.cend(); ++it)
            {
                Ogre::HlmsDatablock* datablock = it->second.datablock;
                if (nullptr == datablock)
                {
                    continue;
                }

                const Ogre::String* namePtr = datablock->getNameStr();
                const Ogre::String* fileNamePtr = nullptr;
                const Ogre::String* resourceGroupPtr = nullptr;
                datablock->getFilenameAndResourceGroup(&fileNamePtr, &resourceGroupPtr);

                // Datablocks created by code (no source file) are not deployed, the code creates them again in the game
                if (nullptr == namePtr || nullptr == fileNamePtr || true == fileNamePtr->empty())
                {
                    continue;
                }

                const auto foundFiles = context.fileIndex.find(toLower(fs::path(*fileNamePtr).filename().string()));
                if (foundFiles == context.fileIndex.end() || true == foundFiles->second.empty())
                {
                    continue;
                }

                const fs::path& jsonPath = foundFiles->second.front().absolutePath;
                context.datablockToJson[*namePtr] = jsonPath;
                context.materialJsonFiles.insert(jsonPath.string());
            }
        }
    }

    // Maps particle system templates ("particle_system HitImpact") to their script files
    void buildParticleTemplateIndex(DeployContext& context)
    {
        for (const auto& entry : context.fileIndex)
        {
            for (const IndexedFile& indexedFile : entry.second)
            {
                const Ogre::String extension = getLowerExtension(indexedFile.absolutePath.filename().string());
                if (".particle" != extension && ".particle2" != extension)
                {
                    continue;
                }

                Ogre::String content;
                if (false == readTextFile(indexedFile.absolutePath, content))
                {
                    continue;
                }

                std::vector<Ogre::String> words;
                collectTokensFromText(content, words);
                for (size_t i = 0; i + 1 < words.size(); i++)
                {
                    if ("particle_system" == words[i])
                    {
                        context.particleTemplateToFile[words[i + 1]] = indexedFile.absolutePath;
                    }
                }
            }
        }
    }

    void seedFromXmlFile(DeployContext& context, const fs::path& filePath)
    {
        Ogre::String content;
        if (false == readProjectTextFile(filePath, true, content))
        {
            return;
        }

        std::vector<char> buffer(content.begin(), content.end());
        buffer.push_back('\0');

        rapidxml::xml_document<> document;
        try
        {
            document.parse<0>(buffer.data());
        }
        catch (rapidxml::parse_error& error)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not parse: '" + filePath.string() + "': " + Ogre::String(error.what()));
            return;
        }

        std::vector<Ogre::String> tokens;
        if (nullptr != document.first_node())
        {
            collectTokensFromXmlNode(document.first_node(), tokens);
        }
        addTokens(context, tokens);
    }

    void seedFromScriptFile(DeployContext& context, const fs::path& filePath)
    {
        Ogre::String content;
        if (false == readProjectTextFile(filePath, false, content))
        {
            return;
        }

        std::vector<Ogre::String> tokens;
        collectQuotedStrings(content, tokens);
        addTokens(context, tokens);
    }

    // Scenes and Lua scripts of the project (the old "media" sub folder of former deploys is skipped)
    void seedFromProjectFolder(DeployContext& context, const fs::path& projectFolder)
    {
        std::error_code errorCode;
        for (fs::recursive_directory_iterator it(projectFolder, errorCode), end; it != end; it.increment(errorCode))
        {
            if (errorCode)
            {
                break;
            }

            if (true == it->is_directory() && 0 == it.depth() && "media" == toLower(it->path().filename().string()))
            {
                it.disable_recursion_pending();
                continue;
            }

            if (false == it->is_regular_file())
            {
                continue;
            }

            const Ogre::String extension = getLowerExtension(it->path().filename().string());
            if (".scene" == extension)
            {
                seedFromXmlFile(context, it->path());
            }
            else if (".lua" == extension)
            {
                seedFromScriptFile(context, it->path());
            }
        }
    }

    // String literals in the C++ code of the game (e.g. menu music, background images of the states)
    void seedFromGameSources(DeployContext& context, const fs::path& sourceFolder)
    {
        std::error_code errorCode;
        if (false == fs::exists(sourceFolder, errorCode))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[DeployResourceModule] No game source folder found at: '" + sourceFolder.string() + "', resources used only by C++ code must be listed in " + Ogre::String(additionalDeployFileName));
            return;
        }

        for (fs::recursive_directory_iterator it(sourceFolder, errorCode), end; it != end; it.increment(errorCode))
        {
            if (errorCode)
            {
                break;
            }

            if (true == it->is_directory())
            {
                const Ogre::String folderName = toLower(it->path().filename().string());
                if ("x64" == folderName || "debug" == folderName || "release" == folderName || ".vs" == folderName || "bin" == folderName || "obj" == folderName || "build" == folderName)
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            const Ogre::String extension = getLowerExtension(it->path().filename().string());
            if (".cpp" == extension || ".h" == extension || ".hpp" == extension)
            {
                seedFromScriptFile(context, it->path());
            }
        }
    }

    void seedFromAdditionalFile(DeployContext& context, const fs::path& projectFolder)
    {
        Ogre::String content;
        if (false == readTextFile(projectFolder / additionalDeployFileName, content))
        {
            return;
        }

        std::istringstream stream(content);
        Ogre::String line;
        while (std::getline(stream, line))
        {
            line = trim(line);
            if (true == line.empty() || '#' == line[0])
            {
                continue;
            }

            std::error_code errorCode;
            if (true == fs::is_directory(context.mediaRoot / fs::path(line), errorCode))
            {
                context.additionalFullCopyFolders.emplace_back(fs::path(line).generic_string());
            }
            else
            {
                addToken(context, line);
            }
        }
    }

    // Text files inside the completely copied folders may reference further resources (e.g. a compositor its textures)
    void seedFromFullCopyFolder(DeployContext& context, const Ogre::String& relativeFolder)
    {
        std::error_code errorCode;
        const fs::path folder = context.mediaRoot / fs::path(relativeFolder);
        if (false == fs::exists(folder, errorCode))
        {
            return;
        }

        for (fs::recursive_directory_iterator it(folder, errorCode), end; it != end; it.increment(errorCode))
        {
            if (errorCode)
            {
                break;
            }
            if (false == it->is_regular_file())
            {
                continue;
            }

            const Ogre::String relativePath = getRelativePath(it->path(), context.mediaRoot);
            if (true == isExcluded(relativePath) || false == hasScannedTextExtension(it->path().filename().string()))
            {
                continue;
            }

            Ogre::String content;
            if (true == readTextFile(it->path(), content))
            {
                std::vector<Ogre::String> tokens;
                collectTokensFromText(content, tokens);
                addTokens(context, tokens);
            }
        }
    }

    void copyFolder(const DeployContext& context, const fs::path& sourceFolder, const fs::path& destinationFolder, bool skipTopLevelMediaFolder, uintmax_t& copiedBytes, size_t& copiedFiles)
    {
        std::error_code errorCode;
        if (false == fs::exists(sourceFolder, errorCode))
        {
            return;
        }

        for (fs::recursive_directory_iterator it(sourceFolder, errorCode), end; it != end; it.increment(errorCode))
        {
            if (errorCode)
            {
                break;
            }

            if (true == it->is_directory())
            {
                const Ogre::String relativePath = getRelativePath(it->path(), context.mediaRoot);
                if ((true == skipTopLevelMediaFolder && 0 == it.depth() && "media" == toLower(it->path().filename().string())) || true == isExcluded(relativePath))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (false == it->is_regular_file())
            {
                continue;
            }

            const fs::path relative = fs::relative(it->path(), sourceFolder, errorCode);
            if (true == copyFileOverwrite(it->path(), destinationFolder / relative, copiedBytes))
            {
                copiedFiles++;
            }
        }
    }

    // Encrypts all Lua scripts and scene files (incl. global.scene and init.lua) in the deployed project folder against casual cheating.
    // The development project stays readable. The engine detects encrypted content automatically when loading.
    size_t encryptDeployedProjectFiles(const fs::path& deployedProjectFolder)
    {
        size_t encryptedFiles = 0;
        std::error_code errorCode;
        for (fs::recursive_directory_iterator it(deployedProjectFolder, errorCode), end; it != end; it.increment(errorCode))
        {
            if (errorCode)
            {
                break;
            }
            if (false == it->is_regular_file())
            {
                continue;
            }

            const Ogre::String extension = toLower(it->path().extension().string());
            if (".lua" != extension && ".scene" != extension)
            {
                continue;
            }

            Ogre::String content;
            if (false == readTextFile(it->path(), content))
            {
                continue;
            }

            // Already encrypted files are decrypted first, so that nothing is encrypted twice
            content = NOWA::Core::getSingletonPtr()->decryptContent(content);
            if (true == content.empty())
            {
                continue;
            }

            // Normalize line endings, the files are read binary at runtime after decryption
            content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());

            const Ogre::String encryptedContent = NOWA::Core::getSingletonPtr()->encryptContent(content);
            std::ofstream outFile(it->path(), std::ios::out | std::ios::binary | std::ios::trunc);
            if (false == outFile.is_open())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Warning: Could not write encrypted file: '" + it->path().string() + "'");
                continue;
            }
            outFile.write(encryptedContent.data(), static_cast<std::streamsize>(encryptedContent.size()));
            outFile.close();
            encryptedFiles++;
        }
        return encryptedFiles;
    }

    bool writePrunedMaterialJson(DeployContext& context, const fs::path& jsonPath, const std::set<Ogre::String>& usedDatablocks, const fs::path& destinationPath)
    {
        // Parse a fresh document to prune (the cached one stays untouched). Note: no Document::CopyFrom, the rapidjson version bundled with Ogre may not have it.
        Ogre::String content;
        if (false == readTextFile(jsonPath, content))
        {
            return false;
        }

        rapidjson::Document document;
        document.Parse(content.c_str());
        if (true == document.HasParseError() || false == document.IsObject())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not parse material json: '" + jsonPath.string() + "'");
            return false;
        }

        // Keep samplers, macroblocks and blendblocks as they are: their names are only valid inside this file.
        // Previously they were de-duplicated across all files by name, although every export numbers them anew,
        // so materials of one file ended up with the (different) sampler of another file.
        for (auto section = document.MemberBegin(); section != document.MemberEnd(); ++section)
        {
            if (true == isBlockSection(section->name.GetString()) || false == section->value.IsObject())
            {
                continue;
            }

            for (auto member = section->value.MemberBegin(); member != section->value.MemberEnd();)
            {
                if (usedDatablocks.end() == usedDatablocks.find(member->name.GetString()))
                {
                    member = section->value.EraseMember(member);
                }
                else
                {
                    ++member;
                }
            }
        }

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        document.Accept(writer);

        std::error_code errorCode;
        fs::create_directories(destinationPath.parent_path(), errorCode);
        std::ofstream outputFile(destinationPath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (false == outputFile.is_open())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not write: '" + destinationPath.string() + "'");
            return false;
        }
        outputFile << buffer.GetString();
        return true;
    }

    // exe, dlls, plugins and plugins.cfg of the Release build
    void copyBinaries(const DeployContext& context, const Ogre::String& projectName, uintmax_t& copiedBytes, size_t& copiedFiles)
    {
        std::error_code errorCode;
        const fs::path releaseFolder = fs::weakly_canonical(fs::path("../Release"), errorCode);
        const fs::path destinationFolder = context.deployRoot / "bin" / "Release";

        const fs::path executablePath = releaseFolder / (projectName + ".exe");
        if (false == fs::exists(executablePath, errorCode))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Warning: '" + executablePath.string() + "' does not exist. Build the game in Release first, then deploy again.");
        }
        else if (true == copyFileOverwrite(executablePath, destinationFolder / executablePath.filename(), copiedBytes))
        {
            copiedFiles++;
        }

        auto copyDlls = [&](const fs::path& sourceFolder, const fs::path& targetFolder)
        {
            std::error_code iteratorErrorCode;
            if (false == fs::exists(sourceFolder, iteratorErrorCode))
            {
                return;
            }
            for (fs::directory_iterator it(sourceFolder, iteratorErrorCode), end; it != end; it.increment(iteratorErrorCode))
            {
                if (iteratorErrorCode)
                {
                    break;
                }
                if (false == it->is_regular_file())
                {
                    continue;
                }

                const Ogre::String fileName = it->path().filename().string();
                const Ogre::String lowerFileName = toLower(fileName);
                // Release dlls only (debug dlls end with "_d.dll")
                if (".dll" != getLowerExtension(fileName) || (lowerFileName.size() > 6 && "_d.dll" == lowerFileName.substr(lowerFileName.size() - 6)))
                {
                    continue;
                }
                if (true == copyFileOverwrite(it->path(), targetFolder / fileName, copiedBytes))
                {
                    copiedFiles++;
                }
            }
        };

        copyDlls(releaseFolder, destinationFolder);
        copyDlls(releaseFolder / "plugins", destinationFolder / "plugins");

        const fs::path pluginsCfgPath = releaseFolder / "plugins.cfg";
        if (true == fs::exists(pluginsCfgPath, errorCode) && true == copyFileOverwrite(pluginsCfgPath, destinationFolder / "plugins.cfg", copiedBytes))
        {
            copiedFiles++;
        }
    }
}

namespace NOWA
{
    DeployResourceModule::DeployResourceModule()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DeployResourceModule] Module created");

        this->hwndNOWALuaScript = 0;

        this->deleteLuaRuntimeErrorXmlFiles("../../external/NOWALuaScript/bin");

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DeployResourceModule::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
    }

    DeployResourceModule::~DeployResourceModule()
    {
        this->taggedResourceMap.clear();
    }

    void DeployResourceModule::destroyContent(void)
    {
        this->taggedResourceMap.clear();
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DeployResourceModule::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
    }

    Ogre::String DeployResourceModule::getCurrentComponentPluginFolder(void) const
    {
        return this->currentComponentPluginFolder;
    }

    bool DeployResourceModule::checkIfInstanceRunning(void)
    {
// #if defined(_WIN32)
#if 0
		// Try to create a named mutex
		this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");
		if (0 != this->hwndNOWALuaScript)
		{
			return true;
		}
#else
        Ogre::String filePath = "../../external/NOWALuaScript/bin/NOWALuaScript.running";

        struct stat buffer;
        bool fileExists = (stat(filePath.c_str(), &buffer) == 0);

        return fileExists;
#endif
        return false;
    }

    bool DeployResourceModule::sendFilePathToRunningInstance(const Ogre::String& filePathName)
    {
// #if defined(_WIN32)
#if 0
		this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");  // Replace with the correct window title
		if (this->hwndNOWALuaScript)
		{
			SetForegroundWindow(this->hwndNOWALuaScript);
			SetForegroundWindow(this->hwndNOWALuaScript);

			// Prepare a custom message identifier
			std::string messageId = "LuaScriptPath";
			std::string message = messageId + "|" + filePathName;  // Combine message ID and file path

			COPYDATASTRUCT cds;
			cds.dwData = 1;  // Optional identifier
			cds.cbData = message.size() + 1;  // Size of the message (including null terminator)
			cds.lpData = (void*)message.c_str();  // Pointer to the message

			SendMessage(this->hwndNOWALuaScript, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
			return true;
		}
		else
		{
			// If somehow mutex does exist, even it should not, release the mutex and create process again
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to find running instance of NOWALuaScript.");
			return false;
		}
#else
#if defined(_WIN32)
        this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript"); // Replace with the correct window title
        if (this->hwndNOWALuaScript)
        {
            SetForegroundWindow(this->hwndNOWALuaScript);
            SetForegroundWindow(this->hwndNOWALuaScript);
        }
#endif

        bool success = false;
        // Sends the file path name to the to be opened lua script file
        // Create an XML document
        rapidxml::xml_document<> doc;

        // Create and append the root node
        rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Message");
        doc.append_node(root);

        // Create and append the message ID node
        char* messageId = doc.allocate_string("LuaScriptPath");
        rapidxml::xml_node<>* idNode = doc.allocate_node(rapidxml::node_element, "MessageId", messageId);
        root->append_node(idNode);

        // Create and append the file path node
        char* filePath = doc.allocate_string(filePathName.c_str());
        rapidxml::xml_node<>* pathNode = doc.allocate_node(rapidxml::node_element, "FilePath", filePath);
        root->append_node(pathNode);

        // Write the XML to a file
        std::ofstream outFile("../../external/NOWALuaScript/bin/lua_script_data.xml"); // Use a platform-specific path if necessary
        if (outFile.is_open())
        {
            outFile << doc; // Print the XML content to the file
            outFile.close();
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: XML file created successfully at ../../external/NOWALuaScript/bin/lua_script_data.xml");
            success = true;
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to write to ../../external/NOWALuaScript/bin/lua_script_data.xml");
            success = false;
        }

        // Clear the document (optional)
        doc.clear();
        return success;
#endif
    }

    void DeployResourceModule::handleLuaError(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataPrintLuaError> castEventData = boost::static_pointer_cast<NOWA::EventDataPrintLuaError>(eventData);
        // Create an XML document
        rapidxml::xml_document<> doc;

        // Create and append the root node <Message>
        rapidxml::xml_node<>* root = doc.allocate_node(rapidxml::node_element, "Message");
        doc.append_node(root);

        // Create and append the <MessageId> node with the value "LuaRuntimeErrors"
        Ogre::String id = "LuaRuntimeErrors" + Ogre::StringConverter::toString(castEventData->getLine());
        char* messageId = doc.allocate_string("LuaRuntimeErrors");
        rapidxml::xml_node<>* idNode = doc.allocate_node(rapidxml::node_element, "MessageId", messageId);
        root->append_node(idNode);

        // Allocate and append the <FilePath> node with the script file path from castEventData
        Ogre::String filePathStr = castEventData->getScriptFilePathName();
        char* filePath = doc.allocate_string(filePathStr.c_str());
        rapidxml::xml_node<>* pathNode = doc.allocate_node(rapidxml::node_element, "FilePath", filePath);
        root->append_node(pathNode);

        // Format the line and start/end attributes for the <error> node
        int line = castEventData->getLine();
        Ogre::String lineStr = std::to_string(line);
        char* lineAttr = doc.allocate_string(lineStr.c_str());

        // Set the error message from castEventData
        Ogre::String errorMsg = castEventData->getErrorMessage();
        char* errorMsgText = doc.allocate_string(errorMsg.c_str());

        if (true == errorMsg.empty())
        {
            if (false == this->checkIfInstanceRunning())
            {
                return;
            }
        }

        // Create and append the <error> node with line attribute and error message text
        rapidxml::xml_node<>* errorNode = doc.allocate_node(rapidxml::node_element, "error", errorMsgText);
        errorNode->append_attribute(doc.allocate_attribute("line", lineAttr));
        errorNode->append_attribute(doc.allocate_attribute("start", "-1"));
        errorNode->append_attribute(doc.allocate_attribute("end", "-1"));
        root->append_node(errorNode);

        std::hash<Ogre::String> hash;
        unsigned int hashNumber = static_cast<unsigned int>(hash(filePathStr));
        Ogre::String outFilePathName = "../../external/NOWALuaScript/bin/lua_script_data" + Ogre::StringConverter::toString(hashNumber) + ".xml";

        // Write the XML document to a file
        std::ofstream outFile(outFilePathName); // Adjust the file path as needed
        if (outFile.is_open())
        {
            outFile << doc;
            outFile.close();
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[NOWA-Engine]: XML file created successfully at: " + outFilePathName);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[NOWA-Engine]: Failed to write to: " + outFilePathName);
        }

        // Clear the document to free memory
        doc.clear();
    }

    bool DeployResourceModule::openNOWALuaScriptEditor(const Ogre::String& filePathName)
    {
        // Check if the process is already running (pseudo-implementation)
        bool instanceRunning = this->checkIfInstanceRunning(); // Implement platform-specific instance check

        if (instanceRunning)
        {
            return this->sendFilePathToRunningInstance(filePathName);
        }

#if defined(_WIN32)
        // No instance running, so start NOWALuaScript.exe
        STARTUPINFOA si = {sizeof(STARTUPINFOA)};
        PROCESS_INFORMATION pi;

        std::string command = "../../external/NOWALuaScript/bin/NOWALuaScript.exe \"" + filePathName + "\"";

        if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            // Get the window handle of NOWALuaScript
            this->hwndNOWALuaScript = FindWindow(NULL, "NOWALuaScript");

            // Start a thread to monitor the process
            std::thread(
                [this, processHandle = pi.hProcess]()
                {
                    this->monitorProcess(processHandle);
                })
                .detach();

            if (this->hwndNOWALuaScript != NULL)
            {
                // Bring the window to the foreground
                SetForegroundWindow(this->hwndNOWALuaScript);
                SetForegroundWindow(this->hwndNOWALuaScript);
            }
            // No need to wait for the process here; we handle it in the thread
            CloseHandle(pi.hThread); // We can close the thread handle, we don't need it

            return true; // Return true when the process is started
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to start NOWALuaScript.exe.");
        }
#else
        std::string command = "../../external/NOWALuaScript/bin/NOWALuaScript \"" + filePathName + "\" &";
        int result = std::system(command.c_str());
        if (result == 0)
        {
            return true;
        }
        else
        {
            std::cerr << "Failed to start NOWALuaScript on POSIX system." << std::endl;
            return false;
        }

#endif

        return false;
    }

    void DeployResourceModule::monitorProcess(HANDLE processHandle)
    {
        // Wait for the process to exit
        WaitForSingleObject(processHandle, INFINITE);

        // Cleanup actions or notifications can go here
        this->hwndNOWALuaScript = 0;
    }

    void DeployResourceModule::deleteLuaRuntimeErrorXmlFiles(const Ogre::String& directoryPath)
    {
        namespace fs = std::filesystem;

        try
        {
            // Iterate through the directory
            for (const auto& entry : fs::directory_iterator(directoryPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".xml")
                {
                    DeleteFile(entry.path().string().c_str()); // Delete the file
                }
            }
        }
        catch (const fs::filesystem_error&)
        {
        }
        catch (const std::exception&)
        {
        }
    }

    DeployResourceModule* DeployResourceModule::getInstance()
    {
        static DeployResourceModule instance;

        return &instance;
    }

    void DeployResourceModule::removeResource(const Ogre::String& name)
    {
        const auto it = this->taggedResourceMap.find(name);
        if (this->taggedResourceMap.cend() != it)
        {
            this->taggedResourceMap.erase(it);
        }
    }

    std::pair<Ogre::String, Ogre::String> DeployResourceModule::getPathAndResourceGroupFromDatablock(const Ogre::String& datablockName, Ogre::HlmsTypes type)
    {
        const Ogre::String* fileNamePtr = nullptr;
        const Ogre::String* resourceGroupPtr = nullptr;
        Ogre::Hlms* hlms = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(type);
        if (nullptr != hlms)
        {
            // Works for every Hlms type (previously only unlit datablocks were supported)
            Ogre::HlmsDatablock* datablock = hlms->getDatablock(datablockName);
            if (nullptr != datablock)
            {
                datablock->getFilenameAndResourceGroup(&fileNamePtr, &resourceGroupPtr);
            }
        }

        Ogre::String resourceGroup;
        if (nullptr != resourceGroupPtr)
        {
            resourceGroup = *resourceGroupPtr;
        }

        Ogre::String path;
        if (nullptr != fileNamePtr)
        {
            path = *fileNamePtr;
        }

        return std::make_pair(resourceGroup, path);
    }

    Ogre::String DeployResourceModule::getResourceGroupName(const Ogre::String& name) const
    {
        const auto it = this->taggedResourceMap.find(name);
        if (this->taggedResourceMap.cend() != it)
        {
            return it->second.first;
        }
        return Ogre::String();
    }

    Ogre::String DeployResourceModule::getResourcePath(const Ogre::String& name) const
    {
        const auto it = this->taggedResourceMap.find(name);
        if (this->taggedResourceMap.cend() != it)
        {
            return it->second.second;
        }
        return Ogre::String();
    }

    void DeployResourceModule::createConfigFile(const Ogre::String& configurationFilePathName, const Ogre::String& applicationName)
    {
        // Fallback template, only used if the game has no own resources cfg (<resources folder>/<ProjectName>.cfg) to derive the deployed cfg from
        std::ofstream configFile(configurationFilePathName.c_str());

        if (configFile.is_open())
        {
            configFile << "# Resources required by the sample browser and most samples.\n\n";
            configFile << "[Essential]\n";
            configFile << "FileSystem=../../media/MyGUI_Media\n";
            configFile << "FileSystem=../../media/MyGUI_Media/images\n\n";

            configFile << "[General]\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common/Any\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common/GLSL\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common/GLSLES\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common/HLSL\n";
            configFile << "FileSystem=../../media/2.0/scripts/materials/Common/Metal\n";
            configFile << "FileSystem=../../media/Hlms/Common/Any\n";
            configFile << "FileSystem=../../media/Hlms/Common/GLSL\n";
            configFile << "FileSystem=../../media/Hlms/Common/HLSL\n";
            configFile << "FileSystem=../../media/Hlms/Common/Metal\n";
            configFile << "FileSystem=../../media/Hlms/Compute\n";
            configFile << "FileSystem=../../media/Compute/Algorithms/IBL\n";
            configFile << "FileSystem=../../media/Compute/Tools/Any\n";
            configFile << "FileSystem=../../media/NOWA/PriorScripts\n";
            configFile << "FileSystem=../../media/NOWA/Scripts\n";
            configFile << "# Custom scripts for compositor effects etc.\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/Postprocessing/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/terra\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/terra/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/terra/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/terra/Metal\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/planetterra\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/planetocean\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/planetsun\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/planetatmosphere\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/ocean\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/ocean/textures\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/ocean/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/ocean/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/ocean/Metal\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SMAA\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SMAA/Metal\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SSAO\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/SSAO/Metal\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/Distortion\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/Watervolume\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/HDR\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/HDR/GLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/HDR/HLSL\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/HDR/Metal\n";
            configFile << "FileSystem=../../media/materials/textures\n\n";

            configFile << "[Audio]\n";
            configFile << "FileSystem=../../media/Audio\n";
            configFile << "FileSystem=../../media/Audio/machinery\n";
            configFile << "FileSystem=../../media/Audio/music\n";
            configFile << "FileSystem=../../media/Audio/heavy_object\n";
            configFile << "FileSystem=../../media/Audio/rumble\n\n";

            configFile << "[Models]\n";
            configFile << "FileSystem=../../media/models\n";
            configFile << "[Backgrounds]\n";
            configFile << "FileSystem=../../media/Backgrounds\n\n";
            configFile << "[Effects]\n";
            configFile << "FileSystem=../../media/models/Effects\n\n";

            configFile << "[Projects]\n";
            configFile << "FileSystem=../../media/projects\n\n";

            configFile << "[Project]\n";
            configFile << "FileSystem=../../media/Projects/" << applicationName << "\n";
            configFile << "FileSystem=../../media/Projects/" << applicationName << "/media" << "\n\n";

            configFile << "[TerrainTextures]\n";
            configFile << "FileSystem=../../media/TerrainTextures\n";
            configFile << "[Skies]\n";
            configFile << "FileSystem=../../media/Skies\n\n";

            configFile << "[NOWA]\n";
            configFile << "FileSystem=../../media/fonts\n";
            configFile << "FileSystem=../../media/NOWA\n\n";

            configFile << "[Lua]\n";
            configFile << "FileSystem=../../media/lua\n\n";

            configFile << "[ParticleFX2]\n";
            configFile << "FileSystem=../../media/ParticleFX2\n";

            configFile << "[Unlit]\n";
            configFile << "FileSystem=../../media/unlit\n\n";

            configFile << "[IES]\n";
            configFile << "FileSystem=../../media/NOWA/Scripts/IesProfiles\n";

            configFile << "# Do not load this as a resource. It's here merely to tell the code where\n";
            configFile << "# the Hlms templates are located\n";
            configFile << "[Hlms]\n";
            configFile << "DoNotUseAsResource=../../media\n";

            configFile.close();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Configuration file created at: " + configurationFilePathName);
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Failed to create the configuration file: " + configurationFilePathName);
        }
    }

    bool DeployResourceModule::writeDeployedResourcesConfig(const Ogre::String& projectName, const Ogre::String& deployRootPathName)
    {
        const fs::path deployRoot(deployRootPathName);
        const fs::path destinationPath = deployRoot / "bin" / "resources" / (projectName + ".cfg");
        const fs::path sourcePath = fs::path(Core::getSingletonPtr()->getResourcesFilePathName()) / (projectName + ".cfg");

        std::error_code errorCode;
        fs::create_directories(destinationPath.parent_path(), errorCode);

        Ogre::String content;
        if (true == readTextFile(sourcePath, content))
        {
            // Same cfg as in development, only without the editor sections. Because the deploy folder mirrors the media folder structure,
            // all paths stay valid, and the game keeps its normal cfg name (no switch to a "Deployed.cfg" in MainApplication necessary).
            std::istringstream stream(content);
            std::ostringstream output;
            Ogre::String line;
            bool skipSection = false;
            while (std::getline(stream, line))
            {
                if (false == line.empty() && '\r' == line.back())
                {
                    line.pop_back();
                }

                const Ogre::String trimmedLine = trim(line);
                if (false == trimmedLine.empty() && '[' == trimmedLine[0])
                {
                    const size_t sectionEnd = trimmedLine.find(']');
                    Ogre::String sectionName;
                    if (Ogre::String::npos != sectionEnd)
                    {
                        sectionName = trimmedLine.substr(1, sectionEnd - 1);
                    }
                    skipSection = isEditorOnlySection(sectionName);
                }

                if (false == skipSection)
                {
                    output << line << "\n";
                }
            }

            std::ofstream configFile(destinationPath, std::ios::out | std::ios::trunc);
            if (false == configFile.is_open())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Could not write: '" + destinationPath.string() + "'");
                return false;
            }
            configFile << output.str();
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DeployResourceModule] No game resources cfg found at: '" + sourcePath.string() + "', using the default template.");
            this->createConfigFile(destinationPath.string(), projectName);
        }

        // Every resource location must exist, else Ogre throws at start. Locations, of which nothing was used, are created empty.
        if (false == readTextFile(destinationPath, content))
        {
            return false;
        }

        const fs::path executableFolder = deployRoot / "bin" / "Release";
        std::istringstream stream(content);
        Ogre::String line;
        while (std::getline(stream, line))
        {
            line = trim(line);
            const size_t separator = line.find('=');
            if (true == line.empty() || '#' == line[0] || Ogre::String::npos == separator)
            {
                continue;
            }

            const Ogre::String type = trim(line.substr(0, separator));
            if ("FileSystem" == type || "DoNotUseAsResource" == type)
            {
                fs::create_directories(executableFolder / fs::path(trim(line.substr(separator + 1))), errorCode);
            }
        }

        return true;
    }

    bool DeployResourceModule::deployProject(const Ogre::String& projectName, const Ogre::String& projectFilePathName)
    {
        // Deploys the game into "<NOWA root>/deploy/<ProjectName>/" (e.g. for a Steam depot):
        //   bin/Release/                   exe, dlls, plugins, plugins.cfg of the Release build
        //   bin/resources/<Project>.cfg    the game's resources cfg without editor sections
        //   media/...                      same folder structure as the development media folder, but only with used files
        // "Used" is determined from ALL scenes of the project (no scene needs to be loaded), their Lua scripts, the game's C++ sources,
        // engine resources and the optional DeployAdditional.txt, and then resolved transitively: mesh -> skeleton + materials,
        // datablock -> textures, particle template -> script -> its materials.
        if (true == projectName.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Deploy failed: no project name.");
            return false;
        }

        DeployContext context;
        std::error_code errorCode;
        context.mediaRoot = fs::weakly_canonical(fs::path("../../media"), errorCode);
        context.deployRoot = fs::weakly_canonical(fs::path("../../deploy"), errorCode) / projectName;
        context.deployMediaRoot = context.deployRoot / "media";

        const fs::path projectFolder = fs::weakly_canonical(fs::path(projectFilePathName), errorCode);
        if (false == fs::is_directory(projectFolder, errorCode))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Deploy failed: project folder: '" + projectFilePathName + "' does not exist.");
            return false;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Deploying project: '" + projectName + "' to: '" + context.deployRoot.string() + "'");

        // 1) Remove the previous deploy output, so that no outdated files remain. Attention: only the two known sub folders of the project's deploy folder.
        fs::remove_all(context.deployRoot / "media", errorCode);
        fs::remove_all(context.deployRoot / "bin", errorCode);

        // 2) Indices (Ogre resource system and Hlms, therefore on the render thread)
        NOWA::GraphicsModule::RenderCommand indexCommand = [&context]()
        {
            buildFileIndex(context);
            buildDatablockIndex(context);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(indexCommand), "DeployResourceModule::deployProject::index");

        buildParticleTemplateIndex(context);

        // 3) Seeds
        for (const char* engineSeedName : engineSeedNames)
        {
            context.optionalTokens.insert(engineSeedName);
            addToken(context, engineSeedName);
        }
        seedFromAdditionalFile(context, projectFolder);
        seedFromProjectFolder(context, projectFolder);
        seedFromGameSources(context, fs::weakly_canonical(fs::path("../../" + projectName), errorCode));
        for (const char* folder : fullCopyFolders)
        {
            seedFromFullCopyFolder(context, folder);
        }
        for (const Ogre::String& folder : context.additionalFullCopyFolders)
        {
            seedFromFullCopyFolder(context, folder);
        }

        // 4) Resolve everything that is referenced, transitively
        resolvePendingTokens(context);

        // 5) Copy
        uintmax_t copiedBytes = 0;
        size_t copiedFiles = 0;

        for (const char* folder : fullCopyFolders)
        {
            copyFolder(context, context.mediaRoot / fs::path(folder), context.deployMediaRoot / fs::path(folder), false, copiedBytes, copiedFiles);
        }
        for (const Ogre::String& folder : context.additionalFullCopyFolders)
        {
            copyFolder(context, context.mediaRoot / fs::path(folder), context.deployMediaRoot / fs::path(folder), false, copiedBytes, copiedFiles);
        }

        // The project itself (scenes, Lua scripts, init.lua ...), without the "media" sub folder of former deploys
        Ogre::String projectRelativePath = getRelativePath(projectFolder, context.mediaRoot);
        if (true == projectRelativePath.empty())
        {
            projectRelativePath = "Projects/" + projectName;
        }
        copyFolder(context, projectFolder, context.deployMediaRoot / fs::path(projectRelativePath), true, copiedBytes, copiedFiles);

        // Lua scripts and scenes of the deployed project are encrypted (the development project stays readable)
        const size_t encryptedFiles = encryptDeployedProjectFiles(context.deployMediaRoot / fs::path(projectRelativePath));

        for (const auto& usedFile : context.usedFiles)
        {
            const IndexedFile& indexedFile = usedFile.second;
            if (true == isFullCopyPath(context, indexedFile.relativePath))
            {
                continue;
            }
            if (true == isInsideFolder(indexedFile.relativePath, projectRelativePath) && false == isInsideFolder(indexedFile.relativePath, projectRelativePath + "/media"))
            {
                continue;
            }

            if (true == copyFileOverwrite(indexedFile.absolutePath, context.deployMediaRoot / fs::path(indexedFile.relativePath), copiedBytes))
            {
                copiedFiles++;
            }
        }

        size_t prunedJsonFiles = 0;
        for (const auto& jsonEntry : context.jsonToUsedDatablocks)
        {
            const fs::path jsonPath(jsonEntry.first);
            const Ogre::String relativePath = getRelativePath(jsonPath, context.mediaRoot);
            if (true == relativePath.empty() || true == isFullCopyPath(context, relativePath))
            {
                continue;
            }

            if (true == writePrunedMaterialJson(context, jsonPath, jsonEntry.second, context.deployMediaRoot / fs::path(relativePath)))
            {
                prunedJsonFiles++;
                copiedFiles++;
            }
        }

        // 6) Binaries and resources cfg
        copyBinaries(context, projectName, copiedBytes, copiedFiles);
        this->writeDeployedResourcesConfig(projectName, context.deployRoot.string());

        // 7) Summary
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[DeployResourceModule] Deploy finished: " + Ogre::StringConverter::toString(copiedFiles) + " files, " + Ogre::StringConverter::toString(static_cast<Ogre::Real>(copiedBytes / (1024.0 * 1024.0))) + " MB, " +
                Ogre::StringConverter::toString(context.usedDatablocks.size()) + " datablocks in " + Ogre::StringConverter::toString(prunedJsonFiles) + " material files, " + Ogre::StringConverter::toString(context.meshCount) +
                " meshes analyzed. Indexed files: " + Ogre::StringConverter::toString(context.fileIndex.size()) + ", particle templates: " + Ogre::StringConverter::toString(context.particleTemplateToFile.size()) +
                ", encrypted Lua/scene files: " + Ogre::StringConverter::toString(encryptedFiles));

        size_t listed = 0;
        for (const Ogre::String& missingReference : context.missingFileReferences)
        {
            if (listed >= maxListedWarnings)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] ... and " + Ogre::StringConverter::toString(context.missingFileReferences.size() - listed) + " more missing references.");
                break;
            }
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Warning: referenced file not found in any resource location: '" + missingReference + "'");
            listed++;
        }

        return true;
    }

    void DeployResourceModule::deploy(const Ogre::String& applicationName, const Ogre::String& sceneName, const Ogre::String& projectFilePathName, bool isLastScene)
    {
        // Compatibility wrapper for the former per scene deploy: the new deploy analyzes all scenes of the project at once, so only the last call does the work.
        if (true == isLastScene)
        {
            this->deployProject(applicationName, projectFilePathName);
        }
    }

    bool DeployResourceModule::createCPlusPlusProject(const Ogre::String& projectName, const Ogre::String& sceneName)
    {
        {
            // Check if project already exists
            std::ifstream ifs("../../" + projectName + "/" + projectName + ".vcxproj");
            if (true == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule]: Warning: Project: '" + projectName + "/" + projectName + "' does already exist. Please specify a different name, or delete the project.");
                ifs.close();
                return true;
            }
        }

        ////////////////////////////Copy to destination/////////////////////////////////////
        Core::getSingletonPtr()->createFolder("../../" + projectName);
        Ogre::String destinationFolder = "../../" + projectName + "/";
        Ogre::String sourceFolder = "../../media/NOWA/ProjectTemplate/";

        Ogre::String destinationFilePathName = destinationFolder + projectName + ".vcxproj";
        Ogre::String sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "ProjectTemplate" with project name
            content = replaceAll(content, "ProjectTemplate", projectName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + projectName + ".vcxproj.user";
        sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj.user";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "ProjectTemplate" with project name
            content = replaceAll(content, "ProjectTemplate", projectName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + projectName + ".vcxproj.filters";
        sourceFilePathName = sourceFolder + "ProjectTemplate.vcxproj.filters";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "ProjectTemplate" with project name
            content = replaceAll(content, "ProjectTemplate", projectName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        ////////////////////////////code/////////////////////////////////////
        destinationFolder = "../../" + projectName + "/code";

        sourceFolder = "../../media/NOWA/ProjectTemplate/code";
        Core::getSingletonPtr()->createFolder(destinationFolder);

        destinationFilePathName = destinationFolder + "/GameState.cpp";
        sourceFilePathName = sourceFolder + "/GameState.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "ProjectTemplate", projectName);
            content = replaceAll(content, "Scene1", sceneName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + "/GameState.h";
        sourceFilePathName = sourceFolder + "/GameState.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/main.cpp";
        sourceFilePathName = sourceFolder + "/main.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/MainApplication.cpp";
        sourceFilePathName = sourceFolder + "/MainApplication.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "ProjectTemplate", projectName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + "/MainApplication.h";
        sourceFilePathName = sourceFolder + "/MainApplication.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/NOWAPrecompiled.cpp";
        sourceFilePathName = sourceFolder + "/NOWAPrecompiled.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/NOWAPrecompiled.h";
        sourceFilePathName = sourceFolder + "/NOWAPrecompiled.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        ////////////////////////////res/////////////////////////////////////
        destinationFolder = "../../" + projectName + "/res";
        sourceFolder = "../../media/NOWA/ProjectTemplate/res";
        Core::getSingletonPtr()->createFolder(destinationFolder);

        destinationFilePathName = destinationFolder + "/NOWA.ico";
        sourceFilePathName = sourceFolder + "/NOWA.ico";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/Resource.aps";
        sourceFilePathName = sourceFolder + "/Resource.aps";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/resource.h";
        sourceFilePathName = sourceFolder + "/resource.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/Resource.rc";
        sourceFilePathName = sourceFolder + "/Resource.rc";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        ////////////////////////config file/////////////////////////////////////

        destinationFilePathName = "../resources/" + projectName + ".cfg";
        sourceFolder = "../../media/NOWA/ProjectTemplate";
        sourceFilePathName = sourceFolder + "/ProjectTemplate.cfg";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        // TODO: Check if visual studio is available

        return true;
    }

    bool DeployResourceModule::createCPlusPlusComponentPluginProject(const Ogre::String& componentName)
    {
        {
            // Check if project already exists
            std::ifstream ifs("../../NOWA_Engine/plugins" + componentName + "/" + componentName + ".vcxproj");
            if (true == ifs.good())
            {
                this->currentComponentPluginFolder = "../../NOWA_Engine/plugins/" + componentName + "/code";
                ;

                ifs.close();
                return true;
            }
        }

        ////////////////////////////Copy to destination/////////////////////////////////////

        Core::getSingletonPtr()->createFolder("../../NOWA_Engine/plugins/" + componentName);
        Ogre::String destinationFolder = "../../NOWA_Engine/plugins/" + componentName + "/";
        Ogre::String sourceFolder = "../../media/NOWA/PluginTemplate/";

        Ogre::String destinationFilePathName = destinationFolder + componentName + ".vcxproj";
        Ogre::String sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "PluginTemplate" with component name
            content = replaceAll(content, "PluginTemplate", componentName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + componentName + ".vcxproj.user";
        sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj.user";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "PluginTemplate" with project name
            content = replaceAll(content, "PluginTemplate", componentName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + componentName + ".vcxproj.filters";
        sourceFilePathName = sourceFolder + "PluginTemplate.vcxproj.filters";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            // Replace "PluginTemplate" with project name
            content = replaceAll(content, "PluginTemplate", componentName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        ////////////////////////////code/////////////////////////////////////
        destinationFolder = "../../NOWA_Engine/plugins/" + componentName + "/code";

        this->currentComponentPluginFolder = destinationFolder;

        sourceFolder = "../../media/NOWA/PluginTemplate/code";
        Core::getSingletonPtr()->createFolder(destinationFolder);

        destinationFilePathName = destinationFolder + "/" + componentName + ".cpp";
        sourceFilePathName = sourceFolder + "/PluginTemplate.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "PluginTemplate", componentName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + "/" + componentName + ".h";
        sourceFilePathName = sourceFolder + "/PluginTemplate.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "PluginTemplate", componentName);

            Ogre::String upperComponentName = componentName;
            std::transform(upperComponentName.begin(), upperComponentName.end(), upperComponentName.begin(), ::toupper);
            content = replaceAll(content, "PLUGIN_TEMPLATE_H", upperComponentName + "_H");

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + "/startDll.cpp";
        sourceFilePathName = sourceFolder + "/startDll.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "PluginTemplate", componentName);

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        destinationFilePathName = destinationFolder + "/NOWAPrecompiled.cpp";
        sourceFilePathName = sourceFolder + "/NOWAPrecompiled.cpp";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        destinationFilePathName = destinationFolder + "/NOWAPrecompiled.h";
        sourceFilePathName = sourceFolder + "/NOWAPrecompiled.h";
        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        // Add to plugins list (Debug)
        destinationFilePathName = "../Debug/plugins.cfg";

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            size_t foundComponentName = content.find(componentName);
            if (Ogre::String::npos == foundComponentName)
            {
                content += "\nPlugin=plugins/" + componentName + "_d";
            }

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        // Add to plugins list (Release)
        destinationFilePathName = "../Release/plugins.cfg";

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            size_t foundComponentName = content.find(componentName);
            if (Ogre::String::npos == foundComponentName)
            {
                content += "\nPlugin=plugins/" + componentName;
            }

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        // Add to all plugins list
        destinationFilePathName = "../resources/AllPlugins.cfg";

        {
            // Get the content
            std::ifstream ifs(destinationFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus component plugin project failed, because the file: " + destinationFilePathName + " cannot be opened.");
                return false;
            }

            // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create CPlusPlus project failed, because in the file: " + destinationFilePathName + " the XML Element " + marker + " cannot be found.");

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            size_t foundComponentName = content.find(componentName);
            if (Ogre::String::npos == foundComponentName)
            {
                content += "\nPlugin=plugins/" + componentName;
            }

            // Write new content
            std::ofstream ofs(destinationFilePathName);
            ofs << content;
            ofs.close();
        }

        // TODO: Check if visual studio is available

        Ogre::String executableName = componentName + ".vcxproj";

        // Does not work
        bool alreadyRunning = Core::getSingletonPtr()->isProcessRunning(executableName.data());

        if (false == alreadyRunning)
        {
            Ogre::String command = "start ../../NOWA_Engine/plugins/" + componentName + "/" + executableName;
            system(command.data());
        }

        AppStateManager::getSingletonPtr()->exitGame();

        return true;
    }

    bool DeployResourceModule::createSceneInOwnState(const Ogre::String& projectName, const Ogre::String& sceneName)
    {
        Ogre::String destinationFolder = "../../" + projectName + "/";
        Ogre::String destinationHeaderFilePathName = destinationFolder + "code/" + sceneName + "State.h";
        Ogre::String destinationImplFilePathName = destinationFolder + "code/" + sceneName + "State.cpp";

        Ogre::String sourceFolder = "../../media/NOWA/ProjectTemplate/code/";
        Ogre::String sourceHeaderFilePathName = sourceFolder + "GameState.h";
        Ogre::String sourceImplFilePathName = sourceFolder + "GameState.cpp";

        // Check if class already exists
        {
            std::ifstream ifs(destinationImplFilePathName);
            if (true == ifs.good())
            {
                ifs.close();
                return true;
            }
        }

        CopyFile(sourceHeaderFilePathName.data(), destinationHeaderFilePathName.data(), TRUE);
        CopyFile(sourceImplFilePathName.data(), destinationImplFilePathName.data(), TRUE);

        // Replace complete class name in header
        {
            // Get the content
            std::ifstream ifs(destinationHeaderFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create scene in own state failed, because the file: " + destinationHeaderFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "GameState", sceneName + "State");
            Ogre::String makroName = sceneName;
            Ogre::StringUtil::toUpperCase(makroName);
            content = replaceAll(content, "GAME_STATE_H", makroName + "_STATE_H");

            // Write new content
            std::ofstream ofs(destinationHeaderFilePathName);
            ofs << content;
            ofs.close();
        }

        // Replace complete class name in implementation
        {
            // Get the content
            std::ifstream ifs(destinationImplFilePathName);
            if (false == ifs.good())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create scene in own state failed, because the file: " + destinationImplFilePathName + " cannot be opened.");
                return false;
            }

            Ogre::String content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
            ifs.close();

            content = replaceAll(content, "GameState", sceneName + "State");
            content = replaceAll(content, "ProjectTemplate", projectName);
            content = replaceAll(content, "Scene1", sceneName);

            // Write new content
            std::ofstream ofs(destinationImplFilePathName);
            ofs << content;
            ofs.close();
        }

        return true;
    }

    void DeployResourceModule::openProject(const Ogre::String& projectName)
    {
        Ogre::String executableName = projectName + ".vcxproj";

        bool alreadyRunning = Core::getSingletonPtr()->isProcessRunning(executableName.data());

        if (false == alreadyRunning)
        {
            Ogre::String command = "start ../../" + projectName + "/" + executableName;
            system(command.data());
        }
    }

    void DeployResourceModule::openLog(void)
    {
        Ogre::String command = "start " + Core::getSingletonPtr()->getLogName();
        system(command.data());
    }

    bool DeployResourceModule::startGame(const Ogre::String& projectName)
    {
        Ogre::String executable;
        Ogre::String command;

#if _DEBUG
        executable = projectName + "_d.exe";
#else
        executable = projectName + ".exe";
#endif

        command = "start ./" + executable;
        std::ifstream ifs(executable);
        if (false == ifs.good())
        {
            return false;
        }

        system(command.data());
        return true;
    }

    bool DeployResourceModule::createAndStartExecutable(const Ogre::String& projectName, const Ogre::String& sceneName)
    {
        this->createCPlusPlusProject(projectName, sceneName);

        Ogre::String destinationFolder = "../../" + projectName;
        Ogre::String destinationFilePathName = destinationFolder + "/" + projectName + ".vcxproj";

        // TODO: check if msbuild is available
        // /WAIT
        // Ogre::String command = "start /B cmd /c  msbuild " + destinationFilePathName + "/p:configuration=release /p:platform=win64 /t:rebuild";

        // Does not work
        Ogre::String command = "CD C:/Windows/Microsoft.NET/Framework64/v4.0.30319";
        system(command.data());

        command = "msbuild " + destinationFilePathName + "/p:configuration=release /p:platform=x64 /t:rebuild";
        // /p:PlatformToolset=v110_xp
        system(command.data());

        system("pause");

        // std::remove(destinationFolder.data());

        //		command = "start " + projectName + ".exe";
        // #if _DEBUG
        //		command = "start ../Release/" + projectName + ".exe";
        // #endif
        //		system(command.data());

        return true;
    }

    bool DeployResourceModule::createLuaInitScript(const Ogre::String& projectName)
    {
        Ogre::String filePathName = Core::getSingletonPtr()->getSectionPath("Projects")[0];
        filePathName += "/" + projectName + "/" + "init.lua";
        std::ifstream ifs(filePathName);
        if (false == ifs.good())
        {
            std::ofstream ofs(filePathName);
            if (true == ofs.good())
            {
                ofs << LuaScriptApi::getInstance()->getLuaInitScriptContent();
                ofs.close();
                return true;
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create init.lua file failed, because the file: " + filePathName + " cannot be created.");
                return false;
            }
        }
        return true;
    }

    bool DeployResourceModule::createProjectBackup(const Ogre::String& projectName, const Ogre::String& sceneName)
    {
        std::list<Ogre::String> sceneBackupList;
        auto filePathNames = NOWA::Core::getSingletonPtr()->getFilePathNames("", "../../media/Projects/backup/" + projectName, "*.*");
        for (const auto& filePathName : filePathNames)
        {
            sceneBackupList.push_back(filePathName);
        }

        // TODO: Not per project but in project 4 backup files per scene!
        // Only allows up to 4 backup files per project
        const unsigned char maxElements = 10;
        while (sceneBackupList.size() >= maxElements)
        {
            Ogre::String oldestBackup = sceneBackupList.front();
            std::remove(oldestBackup.data());
            sceneBackupList.pop_front();
        }

        Ogre::String tempSceneName = sceneName;
        size_t found = tempSceneName.find(".scene");
        if (found != Ogre::String::npos)
        {
            tempSceneName = tempSceneName.substr(0, tempSceneName.size() - 6);
        }

        Ogre::String destinationFolder = "../../media/Projects/backup/" + projectName;
        Ogre::String sourceFolder = "../../media/Projects/" + projectName;

        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char strTempTime[1000];
        strftime(strTempTime, sizeof(strTempTime), "%Y_%m_%d_%H_%M_%S", timeinfo);
        Ogre::String strTime = strTempTime;

        Ogre::String destinationFilePathName = destinationFolder + "/" + tempSceneName + "/" + tempSceneName + "_backup_" + strTime + ".scene";
        Ogre::String sourceFilePathName = sourceFolder + "/" + tempSceneName + "/" + tempSceneName + ".scene";

        Core::getSingletonPtr()->createFolders(destinationFilePathName);

        CopyFile(sourceFilePathName.data(), destinationFilePathName.data(), TRUE);

        Ogre::String sourceGlobalSceneFilePathName = sourceFolder + "/" + "global.scene";
        Ogre::String destinationGlobalSceneFilePathName = destinationFolder + "/" + "global.scene";
        // FALSE: overwrite, else global.scene was only backed up once and never updated again
        CopyFile(sourceGlobalSceneFilePathName.data(), destinationGlobalSceneFilePathName.data(), FALSE);

        Ogre::String sourceLuaInitSceneFilePathName = sourceFolder + "/" + "init.lua";
        Ogre::String destinationLuaInitSceneFilePathName = destinationFolder + "/" + "init.lua";
        // FALSE: overwrite, else init.lua was only backed up once and never updated again
        CopyFile(sourceLuaInitSceneFilePathName.data(), destinationLuaInitSceneFilePathName.data(), FALSE);

        std::ifstream ifs(destinationFilePathName);
        if (false == ifs.good())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DeployResourceModule] Create project backup failed, because the file: " + destinationFilePathName + " cannot be opened.");
            return false;
        }

        return true;
    }

} // namespace end