#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Joint.h"
#include "OgreNewt_Collision.h"

#include "OgreHlmsUnlitDatablock.h"
#include "OgreHlmsUnlit.h"

#include <sstream>

#ifdef __APPLE__
#   include <Ogre/OgreSceneNode.h>
#   include <Ogre/OgreSceneManager.h>
#   include <Ogre/OgreManualObject.h>
#else
#   include <OgreSceneNode.h>
#   include <OgreSceneManager.h>
#   include <OgreManualObject.h>
#endif

namespace OgreNewt
{
	std::vector<Ogre::String> Debugger::unlitDatablockNames;

	//////////////////////////////////////////////////////////
	// DEUBBER FUNCTIONS
	//////////////////////////////////////////////////////////
	Debugger::Debugger(const OgreNewt::World* world)
		: m_world(world),
		m_debugnode(nullptr),
		m_raycastsnode(nullptr),
		m_defaultcolor(Ogre::ColourValue::White),
		m_recordraycasts(false),
		m_markhitbodies(true),
		m_raycol(Ogre::ColourValue::Green),
		m_convexcol(Ogre::ColourValue::Blue),
		m_hitbodycol(Ogre::ColourValue::Red),
		m_prefilterdiscardedcol(Ogre::ColourValue::Black),
		showText(true),
		index(0)
	{

	}

	Debugger::~Debugger()
	{
		this->deInit();
	}

	void Debugger::init(Ogre::SceneManager* smgr, bool showText)
	{
		this->showText = showText;
		if (!m_debugnode)
		{
			m_debugnode = smgr->getRootSceneNode()->createChildSceneNode();
			m_debugnode->setName("__OgreNewt__Debugger__Node__");
			m_debugnode->setListener(this);
		}

		if (!m_raycastsnode)
		{
			m_raycastsnode = smgr->getRootSceneNode()->createChildSceneNode();
			m_raycastsnode->setName("__OgreNewt__Raycasts_Debugger__Node__");
			m_raycastsnode->setListener(this);
		}
		m_sceneManager = smgr;
	}

	void Debugger::deInit()
	{
		this->index = 0;

		this->clearBodyDebugDataCache();

		if (m_debugnode)
		{
			m_debugnode->setListener(nullptr);
			m_debugnode->removeAndDestroyAllChildren();
			if (m_debugnode && m_debugnode->getParentSceneNode())
			{
				m_debugnode->getParentSceneNode()->removeAndDestroyChild(m_debugnode);
			}
			m_debugnode = nullptr;
		}

		clearRaycastsRecorded();
		if (m_raycastsnode)
		{
			m_raycastsnode->setListener(nullptr);
			m_raycastsnode->removeAndDestroyAllChildren();
			if (m_raycastsnode && m_raycastsnode->getParentSceneNode())
			{
				m_raycastsnode->getParentSceneNode()->removeAndDestroyChild(m_raycastsnode);
			}
			m_raycastsnode = nullptr;
		}

		Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);

		for (size_t i = 0; i < Debugger::unlitDatablockNames.size(); i++)
		{
			auto datablock = hlms->getDatablock(Debugger::unlitDatablockNames[i]);

			// Check if datablock exists before attempting destruction
			if (nullptr != datablock)
			{
				auto& linkedRenderables = datablock->getLinkedRenderables();

				// Only destroy if the datablock is not used else where
				if (true == linkedRenderables.empty())
				{
					datablock->getCreator()->destroyDatablock(datablock->getName());
					datablock = nullptr;
				}
			}
		}
	}

	void Debugger::nodeDestroyed(const Ogre::Node* node)
	{
		if (node == m_debugnode)
		{
			m_debugnode = nullptr;
		}

		if (node == m_raycastsnode)
		{
			m_raycastsnode = nullptr;
		}
	}


	void Debugger::clearBodyDebugDataCache()
	{
		if (m_cachemap.size() == 0)
		{
			return;
		}

		for (BodyDebugDataMap::iterator it = m_cachemap.begin(); it != m_cachemap.end(); it++)
		{
			Ogre::ManualObject* mo = it->second.m_lines;
			if (mo)
			{
				m_sceneManager->destroyManualObject(mo);
				mo = nullptr;
			}
		}
		m_cachemap.clear();
	}

	void Debugger::showDebugInformation()
	{
		if (!m_debugnode)
		{
			m_debugnode = m_sceneManager->getRootSceneNode()->createChildSceneNode();
			m_debugnode->setName("__OgreNewt__Debugger__Node__");
			m_debugnode->setListener(this);
		}

		if (!m_raycastsnode)
		{
			m_raycastsnode = m_sceneManager->getRootSceneNode()->createChildSceneNode();
			m_raycastsnode->setName("__OgreNewt__Raycasts_Debugger__Node__");
			m_raycastsnode->setListener(this);
		}

		// make the new lines.
		for (Body* body = m_world->getFirstBody(); body; body = body->getNext())
		{
			if (OgreNewt::NullPrimitiveType != body->getCollisionPrimitiveType())
			{
				processBody(body);
			}
		}

		// delete old entries
		BodyDebugDataMap newbodymap;
		for (BodyDebugDataMap::iterator it = m_cachemap.begin(); it != m_cachemap.end(); it++)
		{
			if (it->second.m_updated)
				newbodymap.insert(*it);
			else
			{
				Ogre::ManualObject* mo = it->second.m_lines;
				if (mo)
				{
					m_sceneManager->destroyManualObject(mo);
					mo = nullptr;
				}
			}
		}
		m_cachemap.swap(newbodymap);
	}

	void Debugger::hideDebugInformation()
	{
		deInit();
		// erase any existing lines!
		if (m_debugnode)
			m_debugnode->removeAllChildren();
	}

	void Debugger::setMaterialColor(const MaterialID* mat, Ogre::ColourValue col)
	{
		if (nullptr == mat)
		{
			return;
		}
		m_materialcolors[mat->getID()] = col;
	}

	void Debugger::setDefaultColor(Ogre::ColourValue col)
	{
		m_defaultcolor = col;
	}

	void Debugger::createUnlitDatablock(const Ogre::String& datablockName, const Ogre::ColourValue& colour)
	{
		Ogre::Hlms* hlms = Ogre::Root::getSingleton().getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
		Ogre::HlmsUnlit* hlmsUnlit = static_cast<Ogre::HlmsUnlit*>(hlms);

		Ogre::HlmsBlendblock blendblock;
		blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA); // Example: Transparent alpha blending

		Ogre::HlmsMacroblock macroblock;
		macroblock.mCullMode = Ogre::CULL_NONE; // Example: No culling
		macroblock.mDepthCheck = true;
		macroblock.mDepthWrite = true;

		// Check if datablock already exists
		Ogre::HlmsDatablock* existingDatablock = hlmsUnlit->getDatablock(datablockName);

		Debugger::unlitDatablockNames.emplace_back(datablockName);

		if (false == existingDatablock)
		{
			Ogre::HlmsUnlitDatablock* datablock = static_cast<Ogre::HlmsUnlitDatablock*>(
				hlmsUnlit->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(macroblock), Ogre::HlmsBlendblock(blendblock), Ogre::HlmsParamVec()));

			existingDatablock = datablock;
		}
		// Set color usage
		static_cast<Ogre::HlmsUnlitDatablock*>(existingDatablock)->setUseColour(true); // Enable manual colour

		// Set a specific colour
		static_cast<Ogre::HlmsUnlitDatablock*>(existingDatablock)->setColour(colour);
	}

	void Debugger::processJoint(Joint* joint)
	{
		// show joint info
		// joint->showDebugData(m_sceneManager, m_debugnode);
	}

	Debugger::DebugCallback::DebugCallback(Ogre::ManualObject* lines, const Ogre::Vector3& scale)
		: m_lines(lines), m_scale(scale), m_index(0)
	{
	}

	void Debugger::DebugCallback::DrawPolygon(ndInt32 vertexCount, const ndVector* const faceArray, const ndEdgeType* const)
	{
		if (vertexCount < 3 || !m_lines)
			return;

		// Draw triangle edges
		for (ndInt32 i = 0; i < vertexCount; i++)
		{
			ndInt32 j = (i + 1) % vertexCount;

			Ogre::Vector3 v0(
				faceArray[i].m_x / m_scale.x,
				faceArray[i].m_y / m_scale.y,
				faceArray[i].m_z / m_scale.z
			);

			Ogre::Vector3 v1(
				faceArray[j].m_x / m_scale.x,
				faceArray[j].m_y / m_scale.y,
				faceArray[j].m_z / m_scale.z
			);

			m_lines->position(v0);
			m_lines->index(m_index++);
			m_lines->position(v1);
			m_lines->index(m_index++);
		}
	}

	void Debugger::buildDebugObjectFromCollision(Ogre::ManualObject* object, Ogre::ColourValue colour, int index, OgreNewt::Body* body) const
	{
		if (nullptr == object || nullptr == body)
		{
			return;
		}

		// Clear existing object content
		object->clear();

		const Ogre::String datablockName = "DebugObjectUnlitDatablock__" + Ogre::StringConverter::toString(index);
		Debugger::createUnlitDatablock(datablockName, colour);
		object->begin(datablockName, Ogre::OperationType::OT_LINE_LIST);

		Ogre::Vector3 scale = body->getOgreNode()->_getDerivedScaleUpdated();
		DebugCallback callback(object, scale);

		ndBodyKinematic* nativeBody = body->getNewtonBody(); // or however you store it
		ndShapeInstance& shapeInstance = nativeBody->GetCollisionShape();

		ndMatrix matrix = ndGetIdentityMatrix();
		shapeInstance.DebugShape(matrix, callback);

		object->end();
	}

	void Debugger::processBody(OgreNewt::Body* bod)
	{
		ndBodyKinematic* newtonBody = bod->getNewtonBody();
		if (!newtonBody)
			return;

		MaterialIdColorMap::iterator it = m_materialcolors.find(0); // no material group in ND4

		Ogre::Vector3 pos, vel, omega;
		Ogre::Quaternion ori;
		Ogre::Vector3 scale = bod->getOgreNode()->getScale();
		bod->getPositionOrientation(pos, ori);

		vel = bod->getVelocity();
		omega = bod->getOmega();

		// create debug-text
		std::ostringstream oss_name;
		oss_name << "__OgreNewt__Debugger__Body__" << bod << "__";
		std::ostringstream oss_info;
		oss_info.precision(2);
		oss_info.setf(std::ios::fixed, std::ios::floatfield);
		Ogre::Vector3 inertia;
		Ogre::Real mass;
		bod->getMassMatrix(mass, inertia);

		if (bod->getOgreNode())
		{
			oss_info << "[" << bod->getOgreNode()->getName() << "]" << std::endl;
		}
		oss_info << "Mass: " << mass << std::endl;
		oss_info << "Position: " << pos[0] << " x " << pos[1] << " x " << pos[2] << std::endl;
		oss_info << "Velocity: " << vel[0] << " x " << vel[1] << " x " << vel[2] << std::endl;
		oss_info << "Omega: " << omega[0] << " x " << omega[1] << " x " << omega[2] << std::endl;
		oss_info << "Inertia: " << inertia[0] << " x " << inertia[1] << " x " << inertia[2] << std::endl;

		// get native shape instance

		// look for cached data
		BodyDebugData* data = &m_cachemap[bod];

		// compare pointer to ndShape (or shapeInstance address) for caching
		if (data->m_lastcol == bod->getNewtonCollision()->GetShape())
		{
			data->m_node->setPosition(pos);
			data->m_node->setOrientation(ori);
			data->m_node->setScale(scale);
			data->m_updated = 1;
			m_debugnode->addChild(data->m_node);
		}
		else
		{
			data->m_lastcol = bod->getNewtonCollision()->GetShape();

			data->m_updated = 1;

			if (data->m_node)
			{
				data->m_node->detachAllObjects();
				data->m_node->setPosition(pos);
				data->m_node->setOrientation(ori);
				data->m_node->setScale(scale);
			}
			else
			{
				data->m_node = m_debugnode->createChildSceneNode(Ogre::SCENE_DYNAMIC, pos, ori);
			}

			if (data->m_lines)
			{
				data->m_lines->clear();
			}
			else
			{
				std::ostringstream oss;
				oss << "__OgreNewt__Debugger__Lines__" << bod << "__";
				data->m_lines = m_sceneManager->createManualObject();
				data->m_lines->setRenderQueueGroup(10);
				data->m_lines->setCastShadows(false);
				data->m_lines->setName(oss.str());
			}

			Ogre::ColourValue colour;
			if (it != m_materialcolors.end())
			{
				colour = it->second;
				this->buildDebugObjectFromCollision(data->m_lines, colour, it->first, bod);
			}
			else
			{
				colour = m_defaultcolor;
				this->buildDebugObjectFromCollision(data->m_lines, colour, 0, bod);
			}

			data->m_node->attachObject(data->m_lines);
		}
	}

	void Debugger::addDiscardedBody(const OgreNewt::Body* body)
	{
		if (nullptr == m_raycastsnode || nullptr == body)
		{
			return;
		}

		Ogre::Vector3 pos;
		Ogre::Quaternion ori;

		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__DiscardedBody__" << this->index++ << "__";
		Ogre::ManualObject* line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		line->setName(oss.str());

		const Ogre::String datablockName = "DiscardedBodyUnlitDatablock__" + Ogre::StringConverter::toString(this->index);
		Debugger::createUnlitDatablock(datablockName, m_prefilterdiscardedcol);
		line->begin(datablockName, Ogre::OperationType::OT_LINE_LIST);

		body->getVisualPositionOrientation(pos, ori);

		Ogre::Vector3 scale = body->getOgreNode()->_getDerivedScaleUpdated();
		DebugCallback callback(line, scale);

		ndMatrix matrix;
		OgreNewt::Converters::QuatPosToMatrix(ori, pos, matrix);

		ndBodyKinematic* nativeBody = body->getNewtonBody(); // or however you store it
		ndShapeInstance& shapeInstance = nativeBody->GetCollisionShape();

		shapeInstance.DebugShape(matrix, callback);

		line->end();

#ifndef WIN32
		m_world->ogreCriticalSectionLock();
#endif
		mRecordedRaycastObjects.push_back(line);
		m_raycastsnode->attachObject(line);
#ifndef WIN32
		m_world->ogreCriticalSectionUnlock();
#endif
	}

	void Debugger::addHitBody(const OgreNewt::Body* body)
	{
		if (nullptr == m_raycastsnode)
		{
			return;
		}

		Ogre::Vector3 pos;
		Ogre::Quaternion ori;

		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__HitBody__" << this->index++ << "__";
		Ogre::ManualObject* line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		line->setName(oss.str());

		const Ogre::String datablockName = "HitBodyUnlitDatablock__" + Ogre::StringConverter::toString(this->index);
		Debugger::createUnlitDatablock(datablockName, m_hitbodycol);
		line->begin(datablockName, Ogre::OperationType::OT_LINE_LIST);

		body->getVisualPositionOrientation(pos, ori);

		Ogre::Vector3 scale = body->getOgreNode()->_getDerivedScaleUpdated();
		DebugCallback callback(line, scale);

		ndMatrix matrix;
		OgreNewt::Converters::QuatPosToMatrix(ori, pos, matrix);

		ndBodyKinematic* nativeBody = body->getNewtonBody(); // or however you store it
		ndShapeInstance& shapeInstance = nativeBody->GetCollisionShape();

		shapeInstance.DebugShape(matrix, callback);

		line->end();

#ifndef WIN32
		m_world->ogreCriticalSectionLock();
#endif
		mRecordedRaycastObjects.push_back(line);
		m_raycastsnode->attachObject(line);
#ifndef WIN32
		m_world->ogreCriticalSectionUnlock();
#endif
	}

	// ----------------- raycast-debugging -----------------------
	void Debugger::startRaycastRecording(bool markhitbodies)
	{
		m_recordraycasts = true;
		m_markhitbodies = markhitbodies;
	}

	bool Debugger::isRaycastRecording()
	{
		return m_recordraycasts;
	}

	bool Debugger::isRaycastRecordingHitBodies()
	{
		return m_markhitbodies;
	}

	void Debugger::clearRaycastsRecorded()
	{
		if (m_raycastsnode)
		{
			m_raycastsnode->removeAndDestroyAllChildren();
		}

		for (ManualObjectList::iterator it = mRecordedRaycastObjects.begin(); it != mRecordedRaycastObjects.end(); it++)
		{
			this->m_sceneManager->destroyManualObject(*it);
			*it = nullptr;
		}
		mRecordedRaycastObjects.clear();
	}

	void Debugger::stopRaycastRecording()
	{
		m_recordraycasts = false;
	}

	void Debugger::setRaycastRecordingColor(Ogre::ColourValue rayCol, Ogre::ColourValue convexCol, Ogre::ColourValue hitBodyCol, Ogre::ColourValue prefilterDiscardedBodyCol)
	{
		m_raycol = rayCol;
		m_convexcol = convexCol;
		m_hitbodycol = hitBodyCol;
		m_prefilterdiscardedcol = prefilterDiscardedBodyCol;
	}

	void Debugger::addRay(const Ogre::Vector3& startpt, const Ogre::Vector3& endpt)
	{
		if (nullptr == m_raycastsnode)
		{
			return;
		}

		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__Raycastline__" << this->index++ << "__";
		Ogre::ManualObject* line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		line->setName(oss.str());

		const Ogre::String datablockName = "RayUnlitDatablock__" + Ogre::StringConverter::toString(this->index);
		Debugger::createUnlitDatablock(datablockName, m_raycol);
		line->begin(datablockName, Ogre::OperationType::OT_LINE_LIST);

		line->position(startpt);
		line->position(endpt);
		line->line(0, 1);
		line->end();

#ifndef WIN32
		m_world->ogreCriticalSectionLock();
#endif
		mRecordedRaycastObjects.push_back(line);
		m_raycastsnode->attachObject(line);
#ifndef WIN32
		m_world->ogreCriticalSectionUnlock();
#endif
	}

	void Debugger::addConvexRay(const ndShape* col, const Ogre::Vector3& startpt, const Ogre::Quaternion& colori, const Ogre::Vector3& endpt)
	{
		if (nullptr == m_raycastsnode || nullptr == col)
		{
			return;
		}

		// lines from aab
		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__Convexcastlines__" << this->index++ << "__";
		Ogre::ManualObject* line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		line->setName(oss.str());

		const Ogre::String datablockName = "ConvexRayUnlitDatablock__" + Ogre::StringConverter::toString(this->index);
		Debugger::createUnlitDatablock(datablockName, m_convexcol);
		line->begin(datablockName, Ogre::OperationType::OT_LINE_LIST);

		// bodies
		ndMatrix matrix;
		DebugCallback callback(line, Ogre::Vector3::UNIT_SCALE);

		OgreNewt::Converters::QuatPosToMatrix(colori, startpt, matrix);
		// Newton 4.0 shapes need to be wrapped in ndShapeInstance for debug rendering
		// This is a simplified version - proper implementation would need the actual shape instance
		// col->DebugShape(matrix, callback);

		if (endpt != startpt)
		{
			OgreNewt::Converters::QuatPosToMatrix(colori, endpt, matrix);
			// col->DebugShape(matrix, callback);
		}

		line->end();

#ifndef WIN32
		m_world->ogreCriticalSectionLock();
#endif
		mRecordedRaycastObjects.push_back(line);
		m_raycastsnode->attachObject(line);
#ifndef WIN32
		m_world->ogreCriticalSectionUnlock();
#endif
	}

}   // end namespace OgreNewt
