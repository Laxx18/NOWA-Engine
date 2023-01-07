/*
-----------------------------------------------------------------------------
Filename:    AnimationSerializer.h
Description: A serializer for keyframe animations
-----------------------------------------------------------------------------
*/

#ifndef ANIMATION_SERIALIZER_H
#define ANIMATION_SERIALIZER_H

#include "defines.h"

namespace NOWA
{

	enum AnimationChunkID
	{
		AC_HEADER = 0x1000,
		AC_ANIMATION = 0x2000,
		AC_ANIMATION_TRACK = 0x3000,
		AC_ANIMATION_TRACK_KEYFRAME = 0x4000,
	};

	class EXPORTED AnimationSerializer : public Ogre::Serializer
	{
	public:
		AnimationSerializer();
		virtual ~AnimationSerializer();

		void exportAnimation(const Ogre::v1::Animation* pAnimation, const Ogre::String& filename,
			Endian endianMode = ENDIAN_NATIVE);

		Ogre::v1::Animation* importAnimation(const Ogre::String& filename);

		Ogre::v1::Animation* importAnimation(Ogre::DataStreamPtr& stream);

	protected:
		void writeAnimation(const Ogre::v1::Animation* anim);
		void writeAnimationTrack(const Ogre::v1::NodeAnimationTrack* track);
		void writeKeyFrame(const Ogre::v1::TransformKeyFrame* key);
		Ogre::v1::Animation* readAnimation(Ogre::DataStreamPtr& stream);
		void readAnimationTrack(Ogre::DataStreamPtr& stream, Ogre::v1::Animation* anim);
		void readKeyFrame(Ogre::DataStreamPtr& stream, Ogre::v1::NodeAnimationTrack* track);
		size_t calcAnimationSize(const Ogre::v1::Animation* pAnim);
		size_t calcAnimationTrackSize(const Ogre::v1::NodeAnimationTrack* pTrack);
		size_t calcKeyFrameSize(const Ogre::v1::TransformKeyFrame* pKey);
	};

}; // namespace end


#endif