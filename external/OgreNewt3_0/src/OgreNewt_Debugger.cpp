#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Joint.h"
#include "OgreNewt_Collision.h"
#include <dMatrix.h>

#include <dCustomJoint.h>

#include "OgreHlmsUnlitDatablock.h"

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
		showText(true)
	{

	}

	Debugger::~Debugger()
	{
		deInit();
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
	}

	void Debugger::nodeDestroyed(const Ogre::Node *node)
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
				// delete mo;
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
		for (Body* body = m_world->getFirstBody(); body && body->getNewtonBody(); body = body->getNext())
		{
			if (OgreNewt::NullPrimitiveType != body->getCollisionPrimitiveType())
			{
				processBody(body);
			}
		}

		// display any joint debug information
		NewtonWorldForEachJointDo(m_world->getNewtonWorld(), newtonprocessJoints, this);

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
					// delete mo;
					m_sceneManager->destroyManualObject(mo);
					mo = nullptr;
				}
			}
		}
		m_cachemap.swap(newbodymap);
	}

	void Debugger::hideDebugInformation()
	{
		clearBodyDebugDataCache();
		deInit();
		// erase any existing lines!
		if (m_debugnode)
			m_debugnode->removeAllChildren();
	}

	void Debugger::setMaterialColor(const MaterialID* mat, Ogre::ColourValue col)
	{
		m_materialcolors[mat->getID()] = col;
	}

	void Debugger::setDefaultColor(Ogre::ColourValue col)
	{
		m_defaultcolor = col;
	}

	void _CDECL Debugger::newtonprocessJoints(const NewtonJoint* newtonJoint, void* userData)
	{
		Debugger* me = (Debugger*)userData;
		dCustomJoint* customJoint = (dCustomJoint*)NewtonJointGetUserData(newtonJoint);
		me->processJoint((Joint*)customJoint->GetUserData());
	}

	void Debugger::processJoint(Joint* joint)
	{
		// show joint info
		joint->showDebugData(m_sceneManager, m_debugnode);
	}

	void Debugger::buildDebugObjectFromCollision(Ogre::ManualObject* object, Ogre::ColourValue colour, OgreNewt::Body* body, NewtonMesh* mesh) const
	{
		if (!object || !body || !mesh)
			return;

		// Clear existing object content
		object->clear();

		Ogre::HlmsUnlitDatablock* sourceDataBlock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablock("RedNoLighting"));
		if (nullptr != sourceDataBlock)
		{
			sourceDataBlock->setUseColour(true);
			sourceDataBlock->setColour(colour);
			// Ogre::HlmsUnlitDatablock* datablock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(this->originalDatablock->clone(originalDataBlockName
			// 	+ "__" + Ogre::StringConverter::toString(this->gameObjectPtr->getId())));
		}

		object->begin("WhiteNoLighting", Ogre::OperationType::OT_LINE_LIST);

		const NewtonBody* newtonBody = body->getNewtonBody();
		const auto it = m_materialcolors.find(NewtonBodyGetMaterialGroupID(newtonBody));

		// Triangulate the mesh for debugging
		NewtonMeshTriangulate(mesh);

		// Get position, orientation, and scale of the body
		Ogre::Vector3 pos, vel, omega;
		Ogre::Quaternion ori;
		body->getPositionOrientation(pos, ori);

		Ogre::Vector3 scale = body->getOgreNode()->_getDerivedScaleUpdated();

		// Extract vertex data from the mesh
		int vertexCount = NewtonMeshGetPointCount(mesh);
		dFloat* vertexArray = new dFloat[3 * vertexCount];
		memset(vertexArray, 0, 3 * vertexCount * sizeof(dFloat));
		NewtonMeshGetVertexChannel(mesh, 3 * sizeof(dFloat), (dFloat*)vertexArray);

		// Handle material indices and draw the lines
		void* const geometryHandle = NewtonMeshBeginHandle(mesh);
		for (int handle = NewtonMeshFirstMaterial(mesh, geometryHandle); handle != -1; handle = NewtonMeshNextMaterial(mesh, geometryHandle, handle))
		{
			int material = NewtonMeshMaterialGetMaterial(mesh, geometryHandle, handle);
			int indexCount = NewtonMeshMaterialGetIndexCount(mesh, geometryHandle, handle);
			object->estimateIndexCount(indexCount);

			unsigned int* indexArray = new unsigned[indexCount];
			NewtonMeshMaterialGetIndexStream(mesh, geometryHandle, handle, (int*)indexArray);

			unsigned int lineIndex = 0;
			for (int i = 0; i < indexCount; i += 3)
			{
				int idx0 = indexArray[i] * 3;
				int idx1 = indexArray[i + 1] * 3;
				int idx2 = indexArray[i + 2] * 3;

				// Calculate vertex positions considering scale
				Ogre::Vector3 v0(
					vertexArray[idx0] / scale.x,
					vertexArray[idx0 + 1] / scale.y,
					vertexArray[idx0 + 2] / scale.z
				);
				Ogre::Vector3 v1(
					vertexArray[idx1] / scale.x,
					vertexArray[idx1 + 1] / scale.y,
					vertexArray[idx1 + 2] / scale.z
				);
				Ogre::Vector3 v2(
					vertexArray[idx2] / scale.x,
					vertexArray[idx2 + 1] / scale.y,
					vertexArray[idx2 + 2] / scale.z
				);

				Ogre::ColourValue colour;
				// Set the material color
				if (it != m_materialcolors.end())
					colour = it->second;
				else
					colour = m_defaultcolor;

				// Draw the edges of the triangles
				object->position(v0);
				object->colour(colour);
				object->index(lineIndex++);
				object->position(v1);
				object->colour(colour);
				object->index(lineIndex++);

				object->position(v1);
				object->colour(colour);
				object->index(lineIndex++);
				object->position(v2);
				object->colour(colour);
				object->index(lineIndex++);

				object->position(v2);
				object->colour(colour);
				object->index(lineIndex++);
				object->position(v0);
				object->colour(colour);
				object->index(lineIndex++);
			}

			delete[] indexArray;
		}

		// Finalize the mesh handling
		NewtonMeshEndHandle(mesh, geometryHandle);
		NewtonMeshDestroy(mesh);

		// Clean up vertex array
		delete[] vertexArray;
		object->end();
	}

	void _CDECL Debugger::newtonPerPoly(void* userData, int vertexCount, const dFloat* faceVertec, int id)
	{
		if (vertexCount < 3 || !userData)
			return;

		// Cast the user data back to a std::pair<ManualObject*, Ogre::ColourValue>
		std::pair<Ogre::ManualObject*, Ogre::ColourValue>* data =
			static_cast<std::pair<Ogre::ManualObject*, Ogre::ColourValue>*>(userData);

		// Access ManualObject* and Ogre::ColourValue
		Ogre::ManualObject* lines = data->first;
		Ogre::ColourValue colour = data->second;

		unsigned int index = 0;

		// Fetch scale from parent scene node
		Ogre::Vector3 scale = Ogre::Vector3::UNIT_SCALE;
		if (lines->getParentSceneNode())
		{
			scale = lines->getParentSceneNode()->_getDerivedScaleUpdated();
		}

		// Iterate through each triangle and draw the edges
		for (int i = 0; i < vertexCount; i += 3)
		{
			Ogre::Vector3 v0(
				faceVertec[i * 3 + 0] / scale.x,
				faceVertec[i * 3 + 1] / scale.y,
				faceVertec[i * 3 + 2] / scale.z
			);

			Ogre::Vector3 v1(
				faceVertec[(i + 1) * 3 + 0] / scale.x,
				faceVertec[(i + 1) * 3 + 1] / scale.y,
				faceVertec[(i + 1) * 3 + 2] / scale.z
			);

			Ogre::Vector3 v2(
				faceVertec[(i + 2) * 3 + 0] / scale.x,
				faceVertec[(i + 2) * 3 + 1] / scale.y,
				faceVertec[(i + 2) * 3 + 2] / scale.z
			);

			// Draw edges
			lines->position(v0);
			lines->colour(colour);
			lines->index(index++);
			lines->position(v1);
			lines->colour(colour);
			lines->index(index++);

			lines->position(v1);
			lines->colour(colour);
			lines->index(index++);
			lines->position(v2);
			lines->colour(colour);
			lines->index(index++);

			lines->position(v2);
			lines->colour(colour);
			lines->index(index++);
			lines->position(v0);
			lines->colour(colour);
			lines->index(index++);
		}
	}

	void Debugger::processBody(OgreNewt::Body* bod)
	{
		NewtonBody* newtonBody = bod->getNewtonBody();
		MaterialIdColorMap::iterator it = m_materialcolors.find(NewtonBodyGetMaterialGroupID(newtonBody));

		Ogre::Vector3 pos, vel, omega;
		Ogre::Quaternion ori;
		// bod->getVisualPositionOrientation(pos, ori);
		bod->getPositionOrientation(pos, ori);

		vel = bod->getVelocity();
		omega = bod->getOmega();

		// ----------- create debug-text ------------
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
		// ----------- ------------------ ------------

		// look for cached data
		BodyDebugData* data = &m_cachemap[bod];

		if (data->m_lastcol == bod->getNewtonCollision()) // use cached data
		{
			// set new position...
			data->m_node->setPosition(pos);
			data->m_node->setOrientation(ori);
			data->m_updated = 1;
			m_debugnode->addChild(data->m_node);
		}
		else
		{
			data->m_lastcol = bod->getNewtonCollision();
			
			data->m_updated = 1;

			if (data->m_node)
			{
				data->m_node->detachAllObjects();
				data->m_node->setPosition(pos);
				data->m_node->setOrientation(ori);
			}
			else
			{
				data->m_node = m_debugnode->createChildSceneNode(Ogre::SCENE_DYNAMIC, pos, ori);
				data->m_mesh = NewtonMeshCreateFromCollision(data->m_lastcol);
			}

			if (data->m_lines)
			{
				data->m_lines->clear();
			}
			else
			{
				std::ostringstream oss;
				oss << "__OgreNewt__Debugger__Lines__" << bod << "__";
				// Note Ogre::ManualObject does not work without indices, but in OgreNewt the collision hulls do not have indices due to performance reasons
				// data->m_lines = new Ogre::ManualObject(0, &m_sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_sceneManager);
				data->m_lines = m_sceneManager->createManualObject();
				data->m_lines->setRenderQueueGroup(10);
				data->m_lines->setCastShadows(false);
				data->m_lines->setName(oss.str());
			}

			Ogre::ColourValue colour;
			// set color
			if (it != m_materialcolors.end())
				colour = it->second;
			else
				colour = m_defaultcolor;

			Ogre::HlmsUnlitDatablock* sourceDataBlock = dynamic_cast<Ogre::HlmsUnlitDatablock*>(Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablock("RedNoLighting"));
			if (nullptr != sourceDataBlock)
			{
				sourceDataBlock->setUseColour(true);
				sourceDataBlock->setColour(Ogre::ColourValue::Blue);
			}

			data->m_lines->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);

			float matrix[16];
			Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, &matrix[0]);

			NewtonCollisionForEachPolygonDo( NewtonBodyGetCollision(newtonBody), &matrix[0], newtonPerPoly, &std::make_pair(data->m_lines, colour));
			data->m_lines->end();
			
			buildDebugObjectFromCollision(data->m_lines, Ogre::ColourValue::Blue, bod, data->m_mesh);
			data->m_node->attachObject(data->m_lines);
		}
	}

	void Debugger::addDiscardedBody(const OgreNewt::Body* body)
	{
		if (!m_raycastsnode)
			return;

		static int i = 0;
		float matrix[16];
		Ogre::Vector3 pos;
		Ogre::Quaternion ori;

		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__DiscardedBody__" << i++ << "__";
		Ogre::ManualObject *line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		// Ogre::ManualObject *line = new Ogre::ManualObject(0, &m_sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_sceneManager);
		line->setName(oss.str());

		line->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
		// line->colour(m_prefilterdiscardedcol);

		body->getVisualPositionOrientation(pos, ori);
		pos = body->getAABB().getCenter() - pos;
		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);

		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);

		NewtonCollisionForEachPolygonDo(body->getNewtonCollision(), &matrix[0], newtonPerPoly, &std::make_pair(line, m_prefilterdiscardedcol));

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
		if (!m_raycastsnode)
			return;

		static int i = 0;
		float matrix[16];
		Ogre::Vector3 pos;
		Ogre::Quaternion ori;

		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__HitBody__" << i++ << "__";
		Ogre::ManualObject *line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		// Ogre::ManualObject *line = new Ogre::ManualObject(0, &m_sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_sceneManager);
		line->setName(oss.str());

		line->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
		// line->colour(m_hitbodycol);

		body->getVisualPositionOrientation(pos, ori);

		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);
		
		NewtonCollisionForEachPolygonDo(body->getNewtonCollision(), &matrix[0], newtonPerPoly, &std::make_pair(line, m_hitbodycol));

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
			// delete (*it);
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

	void Debugger::addRay(const Ogre::Vector3 &startpt, const Ogre::Vector3 &endpt)
	{
		if (!m_raycastsnode)
			return;

		static int i = 0;
		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__Raycastline__" << i++ << "__";
		Ogre::ManualObject *line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		// Ogre::ManualObject *line = new Ogre::ManualObject(0, &m_sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_sceneManager);
		line->setName(oss.str());

		line->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
		// line->colour(m_raycol);
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

	void Debugger::addConvexRay(const NewtonCollision* col, const Ogre::Vector3 &startpt, const Ogre::Quaternion &colori, const Ogre::Vector3 &endpt)
	{
		if (!m_raycastsnode)
			return;

		static int i = 0;
		// lines from aab
		std::ostringstream oss;
		oss << "__OgreNewt__Raycast_Debugger__Lines__Convexcastlines__" << i++ << "__";
		Ogre::ManualObject *line = m_sceneManager->createManualObject();
		line->setRenderQueueGroup(10);
		line->setCastShadows(false);
		// Ogre::ManualObject *line = new Ogre::ManualObject(0, &m_sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), m_sceneManager);
		line->setName(oss.str());

		line->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
		// line->colour(m_convexcol);

		// bodies
		float matrix[16];
		
		Converters::QuatPosToMatrix(colori, startpt, &matrix[0]);
		NewtonCollisionForEachPolygonDo(col, &matrix[0], newtonPerPoly, &std::make_pair(line, m_convexcol));

		if (endpt != startpt)
		{
			Converters::QuatPosToMatrix(colori, endpt, &matrix[0]);
			NewtonCollisionForEachPolygonDo(col, &matrix[0], newtonPerPoly, &std::make_pair(line, m_convexcol));
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