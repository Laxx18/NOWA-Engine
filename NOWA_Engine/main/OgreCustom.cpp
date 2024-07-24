#include "NOWAPrecompiled.h"
#include "main/Core.h"
#include "OgreCustom.h"

namespace NOWA
{
	TransformableSceneNode::TransformableSceneNode(Ogre::IdType id, Ogre::SceneManager* creator, Ogre::NodeMemoryManager* nodeMemoryManager, Ogre::SceneNode* parent)
		: Ogre::SceneNode(id, creator, nodeMemoryManager, parent),
		firstTimePositionSet(true),
		firstTimeOrientationSet(true),
		firstTimeScaleSet(true),
		oldAlpha(0.0f),
		currentAlpha(0.0f)
	{
		this->transformState.currentPosition = getPosition();
		this->transformState.currentOrientation = getOrientation();
		this->transformState.currentScale = getScale();
	}

	void TransformableSceneNode::setPosition(const Ogre::Vector3& pos)
	{
		this->transformState.previousPosition = this->transformState.currentPosition;
		Ogre::SceneNode::setPosition(pos);
		this->transformState.currentPosition = pos;
		if (true == this->firstTimePositionSet)
		{
			this->transformState.previousPosition = pos;
			this->firstTimePositionSet = false;
		}
	}

	void TransformableSceneNode::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->setPosition(Ogre::Vector3(x, y, z));
	}

	void TransformableSceneNode::setOrientation(Ogre::Quaternion q)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::setOrientation(q);
		this->transformState.currentOrientation = q;

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = q;
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::setOrientation(Ogre::Real w, Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->setOrientation(Ogre::Quaternion(w, x, y, z));
	}

	void TransformableSceneNode::TransformableSceneNode::resetOrientation()
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::resetOrientation();
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::lookAt(const Ogre::Vector3& targetPoint, Ogre::Node::TransformSpace relativeTo, const Ogre::Vector3& localDirectionVector)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::lookAt(targetPoint, relativeTo, localDirectionVector);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::setScale(const Ogre::Vector3& scale)
	{
		this->transformState.previousScale = this->transformState.currentScale;
		this->transformState.currentScale = scale;
		Ogre::SceneNode::setScale(scale);

		if (true == this->firstTimeScaleSet)
		{
			this->transformState.previousScale = scale;
			this->firstTimeScaleSet = false;
		}
	}

	void TransformableSceneNode::setScale(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->setScale(Ogre::Vector3(x, y, z));
	}

	void TransformableSceneNode::scale(const Ogre::Vector3& scale)
	{
		this->transformState.previousScale = this->transformState.currentScale;
		Ogre::SceneNode::scale(scale);
		this->transformState.currentScale = Ogre::SceneNode::getScale();

		if (true == this->firstTimeScaleSet)
		{
			this->transformState.previousScale = scale;
			this->firstTimeScaleSet = false;
		}
	}

	void TransformableSceneNode::scale(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->scale(Ogre::Vector3(x, y, z));
	}

	void TransformableSceneNode::translate(const Ogre::Vector3& d, Ogre::Node::TransformSpace relativeTo)
	{
		this->transformState.previousPosition = this->transformState.currentPosition;
		Ogre::SceneNode::translate(d, relativeTo);
		this->transformState.currentPosition = Ogre::SceneNode::getPosition();

		if (true == this->firstTimePositionSet)
		{
			this->transformState.previousPosition = Ogre::SceneNode::getPosition();
			this->firstTimePositionSet = false;
		}
	}

	void TransformableSceneNode::translate(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Node::TransformSpace relativeTo)
	{
		this->translate(Ogre::Vector3(x, y, z), relativeTo);
	}

	void TransformableSceneNode::translate(const Ogre::Matrix3& axes, const Ogre::Vector3& move, Ogre::Node::TransformSpace relativeTo)
	{
		this->transformState.previousPosition = this->transformState.currentPosition;
		this->transformState.currentPosition = move;
		Ogre::SceneNode::translate(axes, move, relativeTo);

		if (true == this->firstTimePositionSet)
		{
			this->transformState.previousPosition = Ogre::SceneNode::getPosition();
			this->firstTimePositionSet = false;
		}
	}

	void TransformableSceneNode::translate(const Ogre::Matrix3& axes, Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Node::TransformSpace relativeTo)
	{
		this->translate(axes, Ogre::Vector3(x, y, z), relativeTo);
	}

	void TransformableSceneNode::roll(const Ogre::Radian& angle, TransformSpace relativeTo)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::roll(angle, relativeTo);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::pitch(const Ogre::Radian& angle, TransformSpace relativeTo)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::pitch(angle, relativeTo);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::yaw(const Ogre::Radian& angle, TransformSpace relativeTo)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::yaw(angle, relativeTo);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::rotate(const Ogre::Vector3& axis, const Ogre::Radian& angle, Ogre::Node::TransformSpace relativeTo)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::rotate(axis, angle, relativeTo);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::rotate(const Ogre::Quaternion& q, TransformSpace relativeTo)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::rotate(q, relativeTo);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::setDirection(Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Node::TransformSpace relativeTo, const Ogre::Vector3& localDirectionVector)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::setDirection(x, y, z, relativeTo, localDirectionVector);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::setDirection(const Ogre::Vector3& vec, Ogre::Node::TransformSpace relativeTo, const Ogre::Vector3& localDirectionVector)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::setDirection(vec, relativeTo, localDirectionVector);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::setAutoTracking(bool enabled, SceneNode* const target, const Ogre::Vector3& localDirectionVector, const Ogre::Vector3& offset)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::setAutoTracking(enabled, target, localDirectionVector, offset);
		this->transformState.currentOrientation = Ogre::SceneNode::getOrientation();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousPosition = Ogre::SceneNode::getPosition();
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::_setDerivedPosition(const Ogre::Vector3& pos)
	{
		this->transformState.previousPosition = this->transformState.currentPosition;
		Ogre::SceneNode::_setDerivedPosition(pos);
		this->transformState.currentPosition = Ogre::SceneNode::_getDerivedPositionUpdated();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::_setDerivedOrientation(const Ogre::Quaternion& q)
	{
		this->transformState.previousOrientation = this->transformState.currentOrientation;
		Ogre::SceneNode::_setDerivedOrientation(q);
		this->transformState.currentOrientation = Ogre::SceneNode::_getDerivedOrientationUpdated();

		if (true == this->firstTimeOrientationSet)
		{
			this->transformState.previousOrientation = Ogre::SceneNode::getOrientation();
			this->firstTimeOrientationSet = false;
		}
	}

	void TransformableSceneNode::interpolate(Ogre::Real alpha)
	{
		Ogre::String name = this->getName();

		if (true == this->isStatic())
		{
			return;
		}

		this->oldAlpha = this->currentAlpha;
		this->currentAlpha = alpha;

		if (this->transformState.previousPosition != this->transformState.currentPosition)
		{
			// Interpolate position
			Ogre::Vector3 interpolatedPosition = interpolateVector(this->transformState.previousPosition, this->transformState.currentPosition, alpha);
			Ogre::SceneNode::setPosition(interpolatedPosition);
		}

		if (this->transformState.previousOrientation != this->transformState.currentOrientation)
		{
			// Interpolate orientation
			Ogre::Quaternion interpolatedOrientation = interpolateQuaternion(this->transformState.previousOrientation, this->transformState.currentOrientation, alpha);
			Ogre::SceneNode::setOrientation(interpolatedOrientation);
		}

		if (this->transformState.previousScale != this->transformState.currentScale)
		{
			// Interpolate scale
			Ogre::Vector3 interpolatedScale = interpolateVector(this->transformState.previousScale, this->transformState.currentScale, alpha);
			Ogre::SceneNode::setScale(interpolatedScale);
		}

		// Next interpolation round, update previous data to be present
		if (this->oldAlpha > alpha)
		{
			this->transformState.previousPosition = this->transformState.currentPosition;
			this->transformState.previousOrientation = this->transformState.currentOrientation;
			this->transformState.previousScale = this->transformState.previousScale;
		}
	}

	Ogre::Vector3 TransformableSceneNode::interpolateVector(const Ogre::Vector3& start, const Ogre::Vector3& end, Ogre::Real alpha)
	{
		return start * (1.0f - alpha) + end * alpha;
	}

	Ogre::Quaternion TransformableSceneNode::interpolateQuaternion(const Ogre::Quaternion& start, const Ogre::Quaternion& end, Ogre::Real alpha)
	{
		return Ogre::Quaternion::nlerp(alpha, start, end, true);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TransformableSceneManager::TransformableSceneManager(const Ogre::String& instanceName, size_t numWorkerThreads)
		: Ogre::SceneManager(instanceName, numWorkerThreads)
	{

	}

	Ogre::SceneNode* TransformableSceneManager::createSceneNodeImpl(Ogre::SceneNode* parent, Ogre::NodeMemoryManager* nodeMemoryManager)
	{
		return new TransformableSceneNode(Ogre::Id::generateNewId<Ogre::SceneNode>(), static_cast<Ogre::SceneManager*>(this), nodeMemoryManager, parent);
	}

	const Ogre::String& TransformableSceneManager::getTypeName() const
	{
		return "TransformableSceneManager";
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Ogre::SceneManager* TransformableSceneManagerFactory::createInstance(const Ogre::String& instanceName, size_t numWorkerThreads)
	{
		return new TransformableSceneManager(instanceName, numWorkerThreads);
	}

	void TransformableSceneManagerFactory::destroyInstance(Ogre::SceneManager* instance)
	{
		delete instance;
	}

	void TransformableSceneManagerFactory::initMetaData() const
	{
		mMetaData.typeName = "TransformableSceneManager";
		mMetaData.description = "Custom SceneManager for using interpolations for transforms.";
		mMetaData.worldGeometrySupported = false;
		mMetaData.sceneTypeMask = Ogre::ST_GENERIC;
	}

}; //namespace end