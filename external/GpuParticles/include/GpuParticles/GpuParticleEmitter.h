/*
 * File: GpuParticleEmitter.h
 * Author: Przemysław Bągard
 * Created: 2021-5-4
 *
 */

#ifndef GPUPARTICLEEMITTERCORE_H
#define GPUPARTICLEEMITTERCORE_H

#include <OgreVector3.h>
#include <OgreBillboardSet.h>
#include <GpuParticles/Hlms/HlmsParticleDatablock.h>
#include "GpuParticleAffector.h"

/// Recipe how to create/update particles. Is tied to one datablock.
class GpuParticleEmitter
{  
public:

    enum class __declspec(dllexport) SpriteMode
    {
        None = 0,
        ChangeWithTrack,
        SetWithStart
    };

    /// Fader takes into consideration particle lifetime
    /// which may be different for each particle.
    enum class __declspec(dllexport) FaderMode
    {
        /// No fader.
        None = 0,

        /// Both colour and alpha. For additive blend mode.
        Enabled = 1,

        /// For alpha blend mode.
        AlphaOnly = 2
    };

    enum class __declspec(dllexport) SpawnShape
    {
        Point = 0,

        /// Box params: vec3d as size;
        Box,

        /// Sphere with params: r (radius)
        Sphere,

        /// Disc/Ring shape params: r1, r2 (radius range)
        Disc
    };

    typedef HlmsParticleDatablock::SpriteCoord SpriteCoord;

    static __declspec(dllexport) Ogre::String spriteModeToStr(SpriteMode value);
    static __declspec(dllexport) SpriteMode strToSpriteMode(const Ogre::String& str);

    static __declspec(dllexport) Ogre::String faderModeToStr(FaderMode value);
    static __declspec(dllexport) FaderMode strToFaderMode(const Ogre::String& str);

    static __declspec(dllexport) Ogre::String spawnShapeToStr(SpawnShape value);
    static __declspec(dllexport) SpawnShape strToSpawnShape(const Ogre::String& str);

    static __declspec(dllexport) Ogre::String billboardTypeToStr(Ogre::v1::BillboardType value);
    static __declspec(dllexport) Ogre::v1::BillboardType strToBillboardType(const Ogre::String& str);

public:

    __declspec(dllexport) GpuParticleEmitter();
    __declspec(dllexport) ~GpuParticleEmitter();
    __declspec(dllexport) GpuParticleEmitter(const GpuParticleEmitter& other);
    __declspec(dllexport) GpuParticleEmitter& operator=(const GpuParticleEmitter& other);

    __declspec(dllexport) float getMaxParticleLifetime() const { return mParticleLifetimeMax; }

    __declspec(dllexport) Ogre::uint32 getMaxParticles() const
    {
        if(mBurstMode) {
            return mBurstParticles;
        }
        return (Ogre::uint32)ceilf(mEmissionRate * getMaxParticleLifetime());
    }

    __declspec(dllexport) void setBurst(int particles, float timeToSpawnAllParticles) {
        mBurstMode = true;
        mBurstParticles = particles;
        mEmitterLifetime = timeToSpawnAllParticles;
    }

    __declspec(dllexport) void setLooped(float emissionRate) {
        mBurstMode = false;
        mEmissionRate = emissionRate;
    }

    /// Calculate time needed to spawn 1 particle.
    __declspec(dllexport) float getTimeToSpawnParticle() const;

    /// Not appliable when 'isImmediateBurst() == true' (will return 0.0 then).
    __declspec(dllexport) float getEmissionRate() const;

    /// Is burst mode and mEmitterLifetime == 0 ?
    __declspec(dllexport) bool isImmediateBurst() const;

    /// Is mEmitterLifetime == 0 ?
    __declspec(dllexport) inline bool isImmediate() const { return fabs(mEmitterLifetime) < 0.001f; }

    __declspec(dllexport) GpuParticleEmitter* clone();

public:

    /// Emitter position offset.
    Ogre::Vector3 mPos = Ogre::Vector3::ZERO;

    Ogre::Vector3 mScale = Ogre::Vector3::UNIT_SCALE;
    /// Emitter rotatation offset.
    Ogre::Quaternion mRot;

    Ogre::String mDatablockName;

    SpriteMode mSpriteMode = SpriteMode::None;

    /// For flipbook (row, col). For atlas row = 0.
    std::vector<SpriteCoord> mSpriteFlipbookCoords;
    std::vector<float> mSpriteTimes;
    static __declspec(dllexport) const int MaxSprites = 8;
    static __declspec(dllexport) const int MaxTrackValues = 8;

    /// Used when 'mBurstMode' is true.
    float mEmitterLifetime = 0.0f;

    /// Particles per second
    float mEmissionRate = 1.0f;

    Ogre::uint32  mBurstParticles = 0;
    bool mBurstMode = false;

    SpawnShape mSpawnShape = SpawnShape::Point;
    Ogre::Vector3 mSpawnShapeDimensions = Ogre::Vector3::ZERO;

    FaderMode mFaderMode = FaderMode::None;
    float mParticleFaderStartTime = 0.0f;
    float mParticleFaderEndTime = 0.0f;

    bool mUniformSize = true;

    Ogre::v1::BillboardType mBillboardType = Ogre::v1::BBT_POINT;

    Ogre::ColourValue mColourA;
    Ogre::ColourValue mColourB;

    /// SizeX. Also SizeY when mUniformSize == true.
    float mSizeMin = 1.0f;
    float mSizeMax = 1.0f;

    /// Used when mUniformSize == false.
    float mSizeYMin = 1.0f;
    float mSizeYMax = 1.0f;

    float mParticleLifetimeMin = 1.0f;
    float mParticleLifetimeMax = 1.0f;

    Ogre::Vector3 mDirection = Ogre::Vector3::UNIT_Y;
    float mSpotAngleMin = 0.0f;
    float mSpotAngleMax = 1.0f;

    float mDirectionVelocityMin = 0.0f;
    float mDirectionVelocityMax = 1.0f;

    typedef std::map<Ogre::String, GpuParticleAffector*> AffectorByNameMap;
    typedef std::map<Ogre::IdString, GpuParticleAffector*> AffectorByHashMap;
    __declspec(dllexport) const AffectorByNameMap& getAffectorByNameMap() const;
    __declspec(dllexport) const AffectorByHashMap& getAffectorByHashMap() const;
    __declspec(dllexport) const GpuParticleAffector* getAffectorNoThrow(const Ogre::String& affectorPropertyName) const;
    __declspec(dllexport) const GpuParticleAffector* getAffectorByIdStringNoThrow(const Ogre::IdString& affectorPropertyNameHash) const;
    __declspec(dllexport) void addAffector(GpuParticleAffector* affector);
    __declspec(dllexport) void removeAndDestroyAffector(const Ogre::String& affectorPropertyName);

private:
    AffectorByNameMap mAffectorByStringMap;
    AffectorByHashMap mAffectorByIdStringMap;
};

#endif
