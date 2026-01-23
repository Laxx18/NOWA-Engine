#if !OGRE_NO_JSON
#ifndef _OgreHlmsJsonOcean_H_
#define _OgreHlmsJsonOcean_H_

#include "OgreHlmsOceanPrerequisites.h"
#include "OgreHlmsOceanDatablock.h"
#include "OgreHlmsJson.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

    class HlmsJsonOcean
    {
        HlmsManager* mHlmsManager;
        TextureGpuManager* mTextureManager;

        static OceanBrdf::OceanBrdf parseBrdf(const char* value);

        static inline Vector3 parseVector3Array(const rapidjson::Value& jsonArray);

        void loadTexture(const rapidjson::Value& json, const HlmsJson::NamedBlocks& blocks,
            OceanTextureTypes textureType, HlmsOceanDatablock* datablock,
            const String& resourceGroup);

        void saveTexture(const char* blockName,
            OceanTextureTypes textureType,
            const HlmsOceanDatablock* datablock, String& outString,
            bool writeTexture = true);

    public:
        HlmsJsonOcean(HlmsManager* hlmsManager, TextureGpuManager* textureManager);

        void loadMaterial(const rapidjson::Value& json, const HlmsJson::NamedBlocks& blocks,
            HlmsDatablock* datablock, const String& resourceGroup);
        void saveMaterial(const HlmsDatablock* datablock, String& outString);

        static void collectSamplerblocks(const HlmsDatablock* datablock,
            set<const HlmsSamplerblock*>::type& outSamplerblocks);
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
#endif
