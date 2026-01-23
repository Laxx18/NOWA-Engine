#include "NOWAPrecompiled.h"

#if !OGRE_NO_JSON

#include "OgreStringConverter.h"

#include "OgreHlmsJsonOcean.h"
#include "OgreHlmsManager.h"
#include "OgreTextureGpuManager.h"

#    if defined( __clang__ )
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#        pragma clang diagnostic ignored "-Wdeprecated-copy"
#    endif
#    include "rapidjson/document.h"
#    if defined( __clang__ )
#        pragma clang diagnostic pop
#    endif

namespace Ogre
{
    HlmsJsonOcean::HlmsJsonOcean(HlmsManager* hlmsManager, TextureGpuManager* textureManager) :
        mHlmsManager(hlmsManager),
        mTextureManager(textureManager)
    {
    }
    //-----------------------------------------------------------------------------------
    OceanBrdf::OceanBrdf HlmsJsonOcean::parseBrdf(const char* value)
    {
        if (!strcmp(value, "default"))
            return OceanBrdf::Default;
        if (!strcmp(value, "cook_torrance"))
            return OceanBrdf::CookTorrance;
        if (!strcmp(value, "blinn_phong"))
            return OceanBrdf::BlinnPhong;
        if (!strcmp(value, "default_uncorrelated"))
            return OceanBrdf::DefaultUncorrelated;
        if (!strcmp(value, "default_separate_diffuse_fresnel"))
            return OceanBrdf::DefaultSeparateDiffuseFresnel;
        if (!strcmp(value, "cook_torrance_separate_diffuse_fresnel"))
            return OceanBrdf::CookTorranceSeparateDiffuseFresnel;
        if (!strcmp(value, "blinn_phong_separate_diffuse_fresnel"))
            return OceanBrdf::BlinnPhongSeparateDiffuseFresnel;

        return OceanBrdf::Default;
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonOcean::loadTexture(const rapidjson::Value& json, const HlmsJson::NamedBlocks& blocks,
        OceanTextureTypes textureType, HlmsOceanDatablock* datablock,
        const String& resourceGroup)
    {
        TextureGpu* texture = 0;
        HlmsSamplerblock const* samplerblock = 0;

        const CommonTextureTypes::CommonTextureTypes texMapTypes[NUM_OCEAN_TEXTURE_TYPES] = {
            CommonTextureTypes::EnvMap  // OCEAN_REFLECTION
        };

        rapidjson::Value::ConstMemberIterator itor = json.FindMember("texture");
        if (itor != json.MemberEnd() && itor->value.IsString())
        {
            const char* textureName = itor->value.GetString();

            // For env maps, use TypeCube
            uint32 textureFlags = TextureFlags::PrefersLoadingFromFileAsSRGB;
            textureFlags &= ~TextureFlags::AutomaticBatching;

            texture = mTextureManager->createOrRetrieveTexture(
                textureName,
                GpuPageOutStrategy::SaveToSystemRam,
                textureFlags,
                TextureTypes::TypeCube,
                resourceGroup);
        }

        itor = json.FindMember("sampler");
        if (itor != json.MemberEnd() && itor->value.IsString())
        {
            map<LwConstString, const HlmsSamplerblock*>::type::const_iterator it =
                blocks.samplerblocks.find(LwConstString::FromUnsafeCStr(itor->value.GetString()));
            if (it != blocks.samplerblocks.end())
            {
                samplerblock = it->second;
                mHlmsManager->addReference(samplerblock);
            }
        }

        if (texture)
        {
            if (!samplerblock)
                samplerblock = mHlmsManager->getSamplerblock(HlmsSamplerblock());
            datablock->_setTexture(textureType, texture, samplerblock);
        }
        else if (samplerblock)
            datablock->_setSamplerblock(textureType, samplerblock);
    }
    //-----------------------------------------------------------------------------------
    inline Vector3 HlmsJsonOcean::parseVector3Array(const rapidjson::Value& jsonArray)
    {
        Vector3 retVal(Vector3::ZERO);

        const rapidjson::SizeType arraySize = std::min(3u, jsonArray.Size());
        for (rapidjson::SizeType i = 0; i < arraySize; ++i)
        {
            if (jsonArray[i].IsNumber())
                retVal[i] = static_cast<float>(jsonArray[i].GetDouble());
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonOcean::loadMaterial(const rapidjson::Value& json, const HlmsJson::NamedBlocks& blocks,
        HlmsDatablock* datablock, const String& resourceGroup)
    {
        assert(dynamic_cast<HlmsOceanDatablock*>(datablock));
        HlmsOceanDatablock* oceanDatablock = static_cast<HlmsOceanDatablock*>(datablock);

        rapidjson::Value::ConstMemberIterator itor = json.FindMember("brdf");
        if (itor != json.MemberEnd() && itor->value.IsString())
            oceanDatablock->setBrdf(parseBrdf(itor->value.GetString()));

        // ===== COLORS =====
        itor = json.FindMember("deep_colour");
        if (itor != json.MemberEnd() && itor->value.IsArray())
            oceanDatablock->setDeepColour(parseVector3Array(itor->value));

        itor = json.FindMember("shallow_colour");
        if (itor != json.MemberEnd() && itor->value.IsArray())
            oceanDatablock->setShallowColour(parseVector3Array(itor->value));

        // ===== WAVES =====
        itor = json.FindMember("waves_scale");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setWavesScale(static_cast<float>(itor->value.GetDouble()));

        // ===== REFLECTION =====
        itor = json.FindMember("reflection_strength");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setReflectionStrength(static_cast<float>(itor->value.GetDouble()));

        // ===== ROUGHNESS =====
        itor = json.FindMember("base_roughness");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setBaseRoughness(static_cast<float>(itor->value.GetDouble()));

        itor = json.FindMember("foam_roughness");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setFoamRoughness(static_cast<float>(itor->value.GetDouble()));

        // ===== AMBIENT & DIFFUSE =====
        itor = json.FindMember("ambient_reduction");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setAmbientReduction(static_cast<float>(itor->value.GetDouble()));

        itor = json.FindMember("diffuse_scale");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setDiffuseScale(static_cast<float>(itor->value.GetDouble()));

        // ===== FOAM =====
        itor = json.FindMember("foam_intensity");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setFoamIntensity(static_cast<float>(itor->value.GetDouble()));

        // ===== TRANSPARENCY ===== (NEW)
        itor = json.FindMember("transparency");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setTransparency(static_cast<float>(itor->value.GetDouble()));

        // ===== SHADOW =====
        itor = json.FindMember("shadow_constant_bias");
        if (itor != json.MemberEnd() && itor->value.IsNumber())
            oceanDatablock->setShadowConstantBias(static_cast<float>(itor->value.GetDouble()));

        // ===== TEXTURES =====
        itor = json.FindMember("reflection");
        if (itor != json.MemberEnd() && itor->value.IsObject())
        {
            const rapidjson::Value& subobj = itor->value;
            loadTexture(subobj, blocks, OCEAN_REFLECTION, oceanDatablock, resourceGroup);
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonOcean::saveTexture(const char* blockName,
        OceanTextureTypes textureType,
        const HlmsOceanDatablock* datablock, String& outString,
        bool writeTexture)
    {
        outString += ",\n\t\t\t\"";
        outString += blockName;
        outString += "\" :\n\t\t\t{\n";

        const size_t currentOffset = outString.size();

        if (writeTexture)
        {
            const TextureGpu* texture = datablock->getTexture(textureType);
            if (texture)
            {
                outString += "\t\t\t\t\"texture\" : \"";
                outString += texture->getNameStr();
                outString += "\"";
            }
        }

        const HlmsSamplerblock* samplerblock = datablock->getSamplerblock(textureType);
        if (samplerblock)
        {
            if (currentOffset != outString.size())
                outString += ",\n";
            outString += "\t\t\t\t\"sampler\" : ";
            outString += HlmsJson::getName(samplerblock);
        }

        if (currentOffset != outString.size())
        {
            outString += "\n\t\t\t}";
        }
        else
        {
            // Remove the block if empty
            outString.resize(currentOffset - strlen(",\n\t\t\t\"") - strlen(blockName) - strlen("\" :\n\t\t\t{\n"));
        }
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonOcean::saveMaterial(const HlmsDatablock* datablock, String& outString)
    {
        assert(dynamic_cast<const HlmsOceanDatablock*>(datablock));
        const HlmsOceanDatablock* oceanDatablock = static_cast<const HlmsOceanDatablock*>(datablock);

        // ===== BRDF =====
        outString += ",\n\t\t\t\"brdf\" : ";
        switch (oceanDatablock->getBrdf())
        {
        case OceanBrdf::Default:
            outString += "\"default\""; break;
        case OceanBrdf::CookTorrance:
            outString += "\"cook_torrance\""; break;
        case OceanBrdf::BlinnPhong:
            outString += "\"blinn_phong\""; break;
        case OceanBrdf::DefaultUncorrelated:
            outString += "\"default_uncorrelated\""; break;
        case OceanBrdf::DefaultSeparateDiffuseFresnel:
            outString += "\"default_separate_diffuse_fresnel\""; break;
        case OceanBrdf::CookTorranceSeparateDiffuseFresnel:
            outString += "\"cook_torrance_separate_diffuse_fresnel\""; break;
        case OceanBrdf::BlinnPhongSeparateDiffuseFresnel:
            outString += "\"blinn_phong_separate_diffuse_fresnel\""; break;
        default:
            outString += "\"default\""; break;
        }

        // ===== COLORS =====
        outString += ",\n\t\t\t\"deep_colour\" : ";
        HlmsJson::toStr(oceanDatablock->getDeepColour(), outString);

        outString += ",\n\t\t\t\"shallow_colour\" : ";
        HlmsJson::toStr(oceanDatablock->getShallowColour(), outString);

        // ===== WAVES =====
        outString += ",\n\t\t\t\"waves_scale\" : ";
        outString += StringConverter::toString(oceanDatablock->getWavesScale());

        // ===== REFLECTION =====
        outString += ",\n\t\t\t\"reflection_strength\" : ";
        outString += StringConverter::toString(oceanDatablock->getReflectionStrength());

        // ===== ROUGHNESS =====
        outString += ",\n\t\t\t\"base_roughness\" : ";
        outString += StringConverter::toString(oceanDatablock->getBaseRoughness());

        outString += ",\n\t\t\t\"foam_roughness\" : ";
        outString += StringConverter::toString(oceanDatablock->getFoamRoughness());

        // ===== AMBIENT & DIFFUSE =====
        outString += ",\n\t\t\t\"ambient_reduction\" : ";
        outString += StringConverter::toString(oceanDatablock->getAmbientReduction());

        outString += ",\n\t\t\t\"diffuse_scale\" : ";
        outString += StringConverter::toString(oceanDatablock->getDiffuseScale());

        // ===== FOAM =====
        outString += ",\n\t\t\t\"foam_intensity\" : ";
        outString += StringConverter::toString(oceanDatablock->getFoamIntensity());

        // ===== TRANSPARENCY ===== (NEW)
        outString += ",\n\t\t\t\"transparency\" : ";
        outString += StringConverter::toString(oceanDatablock->getTransparency());

        // ===== SHADOW =====
        outString += ",\n\t\t\t\"shadow_constant_bias\" : ";
        outString += StringConverter::toString(oceanDatablock->getShadowConstantBias());

        // ===== TEXTURES =====
        if (oceanDatablock->getTexture(OCEAN_REFLECTION))
            saveTexture("reflection", OCEAN_REFLECTION, oceanDatablock, outString, true);
    }
    //-----------------------------------------------------------------------------------
    void HlmsJsonOcean::collectSamplerblocks(const HlmsDatablock* datablock, set<const HlmsSamplerblock*>::type& outSamplerblocks)
    {
        assert(dynamic_cast<const HlmsOceanDatablock*>(datablock));
        const HlmsOceanDatablock* oceanDatablock = static_cast<const HlmsOceanDatablock*>(datablock);

        for (int i = 0; i < NUM_OCEAN_TEXTURE_TYPES; ++i)
        {
            const OceanTextureTypes textureType = static_cast<OceanTextureTypes>(i);
            const HlmsSamplerblock* samplerblock = oceanDatablock->getSamplerblock(textureType);
            if (samplerblock)
                outSamplerblocks.insert(samplerblock);
        }
    }
}  // namespace Ogre

#endif
