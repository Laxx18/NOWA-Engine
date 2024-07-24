#ifndef OGRE_CUSTOM_H
#define OGRE_CUSTOM_H

#include "defines.h"
#include "OgreSceneNode.h"
#include "OgreSceneManager.h"
#include "OgreSceneManagerEnumerator.h"

namespace NOWA
{
	class TransformableSceneNode : public Ogre::SceneNode
	{
	public:
		TransformableSceneNode(Ogre::IdType id, Ogre::SceneManager* creator, Ogre::NodeMemoryManager* nodeMemoryManager, Ogre::SceneNode* parent);

		virtual void setPosition(const Ogre::Vector3& pos) override;

		virtual void setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void setOrientation(Ogre::Quaternion q) override;

		virtual void setOrientation(Ogre::Real w, Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void resetOrientation() override;

		virtual void lookAt(const Ogre::Vector3& targetPoint, Ogre::Node::TransformSpace relativeTo, const Ogre::Vector3& localDirectionVector = Ogre::Vector3::NEGATIVE_UNIT_Z) override;

		virtual void setScale(const Ogre::Vector3& scale) override;

		virtual void setScale(Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void scale(const Ogre::Vector3& scale) override;

		virtual void scale(Ogre::Real x, Ogre::Real y, Ogre::Real z) override;

		virtual void translate(const Ogre::Vector3& d, Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_PARENT) override;
       
		virtual void translate(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_PARENT) override;
       
		virtual void translate(const Ogre::Matrix3& axes, const Ogre::Vector3& move,
								  Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_PARENT) override;
      
		virtual void translate(const Ogre::Matrix3& axes, Ogre::Real x, Ogre::Real y, Ogre::Real z,
								  Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_PARENT) override;
       
		virtual void roll(const Ogre::Radian& angle, TransformSpace relativeTo = TS_LOCAL) override;

		virtual void pitch(const Ogre::Radian& angle, TransformSpace relativeTo = TS_LOCAL) override;

		virtual void yaw(const Ogre::Radian& angle, TransformSpace relativeTo = TS_LOCAL) override;

		virtual void rotate(const Ogre::Vector3& axis, const Ogre::Radian& angle,
                               Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_LOCAL) override;

		virtual void rotate(const Ogre::Quaternion& q, Ogre::Node::TransformSpace relativeTo = TS_LOCAL) override;

		virtual void setDirection(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_LOCAL,
								  const Ogre::Vector3& localDirectionVector = Ogre::Vector3::NEGATIVE_UNIT_Z) override;

		virtual void setDirection(const Ogre::Vector3& vec, Ogre::Node::TransformSpace relativeTo = Ogre::Node::TS_LOCAL,
								  const Ogre::Vector3& localDirectionVector = Ogre::Vector3::NEGATIVE_UNIT_Z) override;

		virtual void setAutoTracking(bool enabled, Ogre::SceneNode* const target = 0,
									 const Ogre::Vector3& localDirectionVector = Ogre::Vector3::NEGATIVE_UNIT_Z,
									 const Ogre::Vector3& offset = Ogre::Vector3::ZERO) override;

		virtual void _setDerivedPosition(const Ogre::Vector3& pos) override;

		virtual void _setDerivedOrientation(const Ogre::Quaternion& q) override;

		void interpolate(Ogre::Real alpha);
	private:
		struct TransformState
		{
			Ogre::Vector3 previousPosition;
			Ogre::Vector3 currentPosition;

			Ogre::Quaternion previousOrientation;
			Ogre::Quaternion currentOrientation;

			Ogre::Vector3 previousScale;
			Ogre::Vector3 currentScale;

			TransformState()
				: previousPosition(Ogre::Vector3::ZERO),
				currentPosition(Ogre::Vector3::ZERO),
				previousOrientation(Ogre::Quaternion::IDENTITY),
				currentOrientation(Ogre::Quaternion::IDENTITY),
				previousScale(Ogre::Vector3::UNIT_SCALE),
				currentScale(Ogre::Vector3::UNIT_SCALE)
			{
			}
		};

	private:
		Ogre::Vector3 interpolateVector(const Ogre::Vector3& start, const Ogre::Vector3& end, Ogre::Real alpha);

		Ogre::Quaternion interpolateQuaternion(const Ogre::Quaternion& start, const Ogre::Quaternion& end, Ogre::Real alpha);
	private:
		TransformState transformState;
		bool firstTimePositionSet;
		bool firstTimeOrientationSet;
		bool firstTimeScaleSet;
		Ogre::Real oldAlpha;
		Ogre::Real currentAlpha;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class TransformableSceneManager : public Ogre::SceneManager
	{
	public:
		TransformableSceneManager(const Ogre::String& instanceName, size_t numWorkerThreads);

		virtual Ogre::SceneNode* createSceneNodeImpl(Ogre::SceneNode* parent, Ogre::NodeMemoryManager* nodeMemoryManager) override;

		const Ogre::String& getTypeName() const override;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class TransformableSceneManagerFactory : public Ogre::SceneManagerFactory
	{
	public:
		virtual void destroyInstance(Ogre::SceneManager* instance) override;

		virtual void initMetaData() const override;

		virtual Ogre::SceneManager* createInstance(const Ogre::String& instanceName, size_t numWorkerThreads) override;
	};

}; // namespace end

#endif