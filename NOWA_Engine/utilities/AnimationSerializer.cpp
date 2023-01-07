#include "NOWAPrecompiled.h"
#include "AnimationSerializer.h"

namespace NOWA
{

	/// stream overhead = ID + size
	const long STREAM_OVERHEAD_SIZE = sizeof(Ogre::uint16) + sizeof(Ogre::uint32);

	AnimationSerializer::AnimationSerializer()
	{
		mVersion = "[AnimationSerializer.10]";
	}

	AnimationSerializer::~AnimationSerializer()
	{

	}

	void AnimationSerializer::exportAnimation(const Ogre::v1::Animation* pAnimation, const Ogre::String& filename, Endian endianMode)
	{
		// Decide on endian mode
		determineEndianness(endianMode);

		Ogre::String msg;
		FILE* mpfFile = fopen(filename.c_str(), "wb");
		if (!mpfFile)
		{
			OGRE_EXCEPT(Ogre::Exception::ERR_CANNOT_WRITE_TO_FILE,
				"Unable to open file " + filename + " for writing",
				"AnimationSerializer::exportAnimation");
		}

		writeFileHeader();

		Ogre::LogManager::getSingleton().logMessage("Exporting animation: " + pAnimation->getName());
		writeAnimation(pAnimation);

		Ogre::LogManager::getSingleton().logMessage("Animation exported.");

		fclose(mpfFile);

	}

	Ogre::v1::Animation* AnimationSerializer::importAnimation(const Ogre::String& filename)
	{
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename);
		return importAnimation(stream);

	}

	Ogre::v1::Animation* AnimationSerializer::importAnimation(Ogre::DataStreamPtr& stream)
	{
		// Determine endianness (must be the first thing we do!)
		determineEndianness(stream);

		// Check header
		readFileHeader(stream);

		Ogre::v1::Animation* anim;

		unsigned short streamID;
		while (!stream->eof())
		{
			streamID = readChunk(stream);
			switch (streamID)
			{
			case AC_ANIMATION:
				anim = readAnimation(stream);
				break;
			}
		}
		return anim;
	}

	void AnimationSerializer::writeAnimation(const Ogre::v1::Animation* anim)
	{
		writeChunkHeader(AC_ANIMATION, calcAnimationSize(anim));

		// char* name                       : Name of the animation
		writeString(anim->getName());
		// float length                      : Length of the animation in seconds
		float len = anim->getLength();
		writeFloats(&len, 1);

		// Write all tracks
		Ogre::v1::Animation::NodeTrackIterator trackIt = anim->getNodeTrackIterator();
		while (trackIt.hasMoreElements())
		{
			writeAnimationTrack(trackIt.getNext());
		}
	}

	void AnimationSerializer::writeAnimationTrack(const Ogre::v1::NodeAnimationTrack* track)
	{
		static unsigned short trackId = 0;
		writeChunkHeader(AC_ANIMATION_TRACK, calcAnimationTrackSize(track));

		writeShorts(&trackId, 1);

		// Write all keyframes
		for (unsigned short i = 0; i < track->getNumKeyFrames(); ++i)
		{
			writeKeyFrame(track->getNodeKeyFrame(i));
		}
		trackId++;
	}

	void AnimationSerializer::writeKeyFrame(const Ogre::v1::TransformKeyFrame* key)
	{
		writeChunkHeader(AC_ANIMATION_TRACK_KEYFRAME,
			calcKeyFrameSize(key));

		// float time                    : The time position (seconds)
		float time = key->getTime();
		writeFloats(&time, 1);
		// Quaternion rotate            : Rotation to apply at this keyframe
		writeObject(key->getRotation());
		// Vector3 translate            : Translation to apply at this keyframe
		writeObject(key->getTranslate());
	}

	Ogre::v1::Animation* AnimationSerializer::readAnimation(Ogre::DataStreamPtr& stream)
	{
		Ogre::String name;
		name = readString(stream);
		// float length                      : Length of the animation in seconds
		float len;
		readFloats(stream, &len, 1);

		Ogre::v1::Animation* anim = new Ogre::v1::Animation(name, len);

		// Read all tracks
		if (!stream->eof())
		{
			unsigned short streamID = readChunk(stream);
			while (streamID == AC_ANIMATION_TRACK && !stream->eof())
			{
				readAnimationTrack(stream, anim);

				if (!stream->eof())
				{
					// Get next stream
					streamID = readChunk(stream);
				}
			}
			if (!stream->eof())
			{
				// Backpedal back to start of this stream if we've found a non-track
				stream->skip(-STREAM_OVERHEAD_SIZE);
			}

		}
		return anim;
	}

	void AnimationSerializer::readAnimationTrack(Ogre::DataStreamPtr& stream, Ogre::v1::Animation* anim)
	{
		unsigned short trackId;
		readShorts(stream, &trackId, 1);
		// Create track
		// Ogre::v1::NodeAnimationTrack* pTrack = anim->createNodeTrack(trackId);
//Attention: maybe corrupt, because its not possible to set track id, and not sure if the track must be attachted to node
		Ogre::v1::NodeAnimationTrack* pTrack = anim->createNodeTrack();
		// Keep looking for nested keyframes
		if (!stream->eof())
		{
			unsigned short streamID = readChunk(stream);
			while (streamID == AC_ANIMATION_TRACK_KEYFRAME && !stream->eof())
			{
				readKeyFrame(stream, pTrack);

				if (!stream->eof())
				{
					// Get next stream
					streamID = readChunk(stream);
				}
			}
			if (!stream->eof())
			{
				// Backpedal back to start of this stream if we've found a non-keyframe
				stream->skip(-STREAM_OVERHEAD_SIZE);
			}

		}
	}

	void AnimationSerializer::readKeyFrame(Ogre::DataStreamPtr& stream, Ogre::v1::NodeAnimationTrack* track)
	{
		// float time                    : The time position (seconds)
		float time;
		readFloats(stream, &time, 1);

		Ogre::v1::TransformKeyFrame *kf = track->createNodeKeyFrame(time);

		// Quaternion rotate            : Rotation to apply at this keyframe
		Ogre::Quaternion rot;
		readObject(stream, rot);
		kf->setRotation(rot);
		// Vector3 translate            : Translation to apply at this keyframe
		Ogre::Vector3 trans;
		readObject(stream, trans);
		kf->setTranslate(trans);
	}

	size_t AnimationSerializer::calcAnimationSize(const Ogre::v1::Animation* pAnim)
	{
		size_t size = STREAM_OVERHEAD_SIZE;

		// Name, including terminator
		size += pAnim->getName().length() + 1;
		// length
		size += sizeof(float);

		// Nested animation tracks
		Ogre::v1::Animation::NodeTrackIterator trackIt = pAnim->getNodeTrackIterator();
		while (trackIt.hasMoreElements())
		{
			size += calcAnimationTrackSize(trackIt.getNext());
		}

		return size;
	}

	size_t AnimationSerializer::calcAnimationTrackSize(const Ogre::v1::NodeAnimationTrack* pTrack)
	{
		size_t size = STREAM_OVERHEAD_SIZE;

		// unsigned short boneIndex     : Index of bone to apply to
		size += sizeof(unsigned short);

		// Nested keyframes
		for (unsigned short i = 0; i < pTrack->getNumKeyFrames(); ++i)
		{
			size += calcKeyFrameSize(pTrack->getNodeKeyFrame(i));
		}

		return size;
	}

	size_t AnimationSerializer::calcKeyFrameSize(const Ogre::v1::TransformKeyFrame* pKey)
	{
		size_t size = STREAM_OVERHEAD_SIZE;

		// float time                    : The time position (seconds)
		size += sizeof(float);
		// Quaternion rotate            : Rotation to apply at this keyframe
		size += sizeof(float) * 4;
		// Vector3 translate            : Translation to apply at this keyframe
		size += sizeof(float) * 3;

		return size;
	}

}; //namespace end
