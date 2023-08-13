/*
 * File: GpuParticleSystemJsonManager.h
 * Author: Przemysław Bągard
 * Created: 2021-6-12
 *
 */

#ifndef GpuParticleSystemJsonManager_H
#define GpuParticleSystemJsonManager_H

#if !OGRE_NO_JSON

#include "OgreStringVector.h"
#include "OgreIdString.h"
#include "GpuParticleEmitter.h"
#include "GpuParticleAffectorCommon.h"
#include <OgreCommon.h>
#include <ogrestd/map.h>

#include <OgreScriptLoader.h>
#include <OgreSingleton.h>

class __declspec( dllexport ) GpuParticleSystem;
class __declspec( dllexport ) GpuParticleEmitter;

// Forward declaration for |Document|.
namespace rapidjson
{
    class __declspec( dllexport ) CrtAllocator;
    template <typename> class __declspec( dllexport ) MemoryPoolAllocator;
    template <typename> struct UTF8;
    //template <typename, typename, typename> class __declspec( dllexport ) GenericDocument;
    //typedef GenericDocument< UTF8<char>, MemoryPoolAllocator<CrtAllocator>, CrtAllocator > Document;

    template <typename BaseAllocator> class __declspec( dllexport ) MemoryPoolAllocator;
    template <typename Encoding, typename>  class __declspec( dllexport ) GenericValue;
    typedef GenericValue<UTF8<char>, MemoryPoolAllocator<CrtAllocator> > Value;
}

/// Reads scripts in '*.gpuparticle.json' files
class GpuParticleSystemJsonManager
        : public Ogre::Singleton<GpuParticleSystemJsonManager>
        , public Ogre::ScriptLoader
{
public:

    __declspec(dllexport) GpuParticleSystemJsonManager();

    __declspec(dllexport) virtual const Ogre::StringVector& getScriptPatterns(void) const override;
    __declspec(dllexport) virtual void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName) override;
    __declspec(dllexport) virtual Ogre::Real getLoadingOrder(void) const override;

    __declspec(dllexport) void loadGpuParticleSystems( const Ogre::String &filename,
                                 const Ogre::String &resourceGroup,
                                 const char *jsonString,
                                 const Ogre::String &additionalTextureExtension );

    static __declspec(dllexport) void loadGpuParticleSystem( const rapidjson::Value &json, GpuParticleSystem *gpuParticleSystem );
    static __declspec(dllexport) void loadGpuParticleEmitter(const rapidjson::Value &json, GpuParticleEmitter *gpuParticleEmitter );

    static __declspec(dllexport) void saveGpuParticleSystem( const GpuParticleSystem *gpuParticleSystem, Ogre::String &outString );
    static __declspec(dllexport) void saveGpuParticleEmitter( const GpuParticleEmitter *gpuParticleEmitter, Ogre::String &outString );

public:
    typedef Ogre::map<Ogre::String, Ogre::String>::type ResourceToTexExtensionMap;
    ResourceToTexExtensionMap mAdditionalTextureExtensionsPerGroup;

private:
    Ogre::StringVector mScriptPatterns;

    /// Writes '\n\t\t"var": '
    static void writeGpuEmitterVariable( const Ogre::String& var,  Ogre::String& outString);

public:
    static __declspec(dllexport) void readVector3Value(const rapidjson::Value &json, Ogre::Vector3& value);
    static __declspec(dllexport) void readVector2Value(const rapidjson::Value &json, Ogre::Vector2& value);
    static __declspec(dllexport) void readQuaternionValue(const rapidjson::Value &json, Ogre::Quaternion& value);
    static __declspec(dllexport) void readColourValue(const rapidjson::Value &json, Ogre::ColourValue& value);
    static __declspec(dllexport) void readMinMaxFloatValue(const rapidjson::Value &json, float& valueMin, float& valueMax);
    static __declspec(dllexport) void readFloatTrack(const rapidjson::Value &array, GpuParticleAffectorCommon::FloatTrack& valueMax);
    static __declspec(dllexport) void readVector2Track(const rapidjson::Value &array, GpuParticleAffectorCommon::Vector2Track& valueMax);
    static __declspec(dllexport) void readVector3Track(const rapidjson::Value &array, GpuParticleAffectorCommon::Vector3Track& valueMax);

    static __declspec(dllexport) void toStr( const Ogre::ColourValue &value, Ogre::String &outString );
    static __declspec(dllexport) void toStr( const Ogre::Vector2 &value, Ogre::String &outString );
    static __declspec(dllexport) void toStr( const Ogre::Vector3 &value, Ogre::String &outString );
    static __declspec(dllexport) void toStr( const Ogre::Vector4 &value, Ogre::String &outString );
    static __declspec(dllexport) void toStr( const Ogre::Quaternion &value, Ogre::String &outString );
    static __declspec(dllexport) void toStr( const bool &value, Ogre::String &outString );
    static __declspec(dllexport) void toStrMinMax( float valueMin, float valueMax, Ogre::String &outString );

    /// writeGpuAffectorVariable differs from writeGpuEmitterVariable with number indentations ('\t')
    static __declspec(dllexport) void writeGpuAffectorVariable( const Ogre::String& var,  Ogre::String& outString);
    static __declspec(dllexport) Ogre::String quote( const Ogre::String& value );

    static __declspec(dllexport) void writeFloatTrack(const GpuParticleAffectorCommon::FloatTrack& valueTrack, Ogre::String& outString);
    static __declspec(dllexport) void writeVector2Track(const GpuParticleAffectorCommon::Vector2Track& valueTrack, Ogre::String& outString);
    static __declspec(dllexport) void writeVector3Track(const GpuParticleAffectorCommon::Vector3Track& valueTrack, Ogre::String& outString);
};

#endif

#endif
