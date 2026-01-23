namespace Ogre
{
    void HlmsOceanDatablock::cloneImpl( HlmsDatablock* datablock ) const
    {
        // Clone base HLMS + texture base class state
        HlmsOceanBaseTextureDatablock::cloneImpl( datablock );

        HlmsOceanDatablock* dst =
            static_cast<HlmsOceanDatablock*>( datablock );

        // ===== Copy GPU material struct (must match shader exactly) =====
        dst->mDeepColour     = mDeepColour;
        dst->mShallowColour  = mShallowColour;
        dst->mkD             = mkD;
        dst->mRoughness      = mRoughness;
        dst->mMetalness      = mMetalness;

        for( int i = 0; i < 4; ++i )
            dst->mDetailOffsetScale[i] = mDetailOffsetScale[i];

        memcpy( dst->mIndices0_7,   mIndices0_7,   sizeof( mIndices0_7 ) );
        memcpy( dst->mIndices8_15,  mIndices8_15,  sizeof( mIndices8_15 ) );
        memcpy( dst->mIndices16_24, mIndices16_24, sizeof( mIndices16_24 ) );

        // ===== Logical state =====
        dst->mBrdf = mBrdf;
    }
}
