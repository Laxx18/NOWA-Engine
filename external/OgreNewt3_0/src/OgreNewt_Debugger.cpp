#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Joint.h"
#include "OgreNewt_Collision.h"
#include <dMatrix.h>

#include <dCustomJoint.h>

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
		clearBodyDebugDataCache();
		if (m_debugnode)
		{
			m_debugnode->setListener(nullptr);
			m_debugnode->removeAndDestroyAllChildren();
			m_debugnode->getParentSceneNode()->removeAndDestroyChild(m_debugnode);
			m_debugnode = nullptr;
		}


		clearRaycastsRecorded();
		if (m_raycastsnode)
		{
			m_raycastsnode->setListener(nullptr);
			m_raycastsnode->removeAndDestroyAllChildren();
			m_raycastsnode->getParentSceneNode()->removeAndDestroyChild(m_raycastsnode);
			m_raycastsnode = nullptr;
		}
	}

	void Debugger::nodeDestroyed(const Ogre::Node *node)
	{
		if (node == m_debugnode)
		{
			m_debugnode = nullptr;
			clearBodyDebugDataCache();
		}

		if (node == m_raycastsnode)
		{
			m_raycastsnode = nullptr;
			clearRaycastsRecorded();
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
			//Ogre::ManualObject* mo = it->second.m_lines;
			//if (mo)
			//{
			//	// delete mo;
			//	m_sceneManager->destroyManualObject(mo);
			//	mo = nullptr;
			//}
			/*OgreNewt::OgreAddons::MovableText *text = it->second.m_text;
			if (text)
			{
				delete text;
			}*/
		}
		m_cachemap.clear();
	}



	void Debugger::showDebugInformation()
	{
		if (!m_debugnode)
			return;

		m_debugnode->removeAllChildren();

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
				/* OgreNewt::OgreAddons::MovableText *text = it->second.m_text;
				 if( text )
					 delete text;*/
			}
		}
		m_cachemap.swap(newbodymap);
	}

	void Debugger::hideDebugInformation()
	{
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
		object->begin("BlueNoLighting", Ogre::OperationType::OT_LINE_LIST);

		// set color
	//	if( it != m_materialcolors.end() )
	//		object->colour(it->second);
	//	else
	//		object->colour(m_defaultcolor);

		object->colour(colour);

		NewtonMeshTriangulate(mesh);
		// NewtonMeshPolygonize(mesh);

#if 1
		Ogre::Vector3 pos, vel, omega;
		Ogre::Quaternion ori;
		// bod->getVisualPositionOrientation(pos, ori);
		body->getPositionOrientation(pos, ori);

		Ogre::Vector3 center = body->getAABB().getCenter();
		Ogre::Vector3 offset(pos - center);

		Ogre::Vector3 min = body->getAABB().getMinimum();
		Ogre::Vector3 max = body->getAABB().getMaximum();
		Ogre::Vector3 diff = max - min;
		Ogre::Vector3 half = (max - min) / 2.0f;

		// Why the heck is there a debugging position offset??
		// pos += (ori * (body->getAABB().getCenter()));
		// ori = ori * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X);

		///*Ogre::AxisAlignedBox boundingBox = bod->getAABB();
		//Ogre::Vector3 padding = (boundingBox.getMaximum() - boundingBox.getMinimum()) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
		//Ogre::Vector3 size = ((boundingBox.getMaximum() - boundingBox.getMinimum()) - padding * 2.0f) * bod->getOgreNode()->getScale();
		//Ogre::Vector3 centerOffset = boundingBox.getMinimum() + padding + (size / 2.0f);*/
		// pos = pos + bod->getAABB().getCenter();

		// float matrix[16];
		// Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, &matrix[0]);

		int vertexCount = NewtonMeshGetPointCount(mesh);
		object->estimateVertexCount(vertexCount);
		dFloat* vertexArray = new dFloat[3 * vertexCount];
		memset(vertexArray, 0, 3 * vertexCount * sizeof(dFloat));

		NewtonMeshGetVertexChannel(mesh, 3 * sizeof(dFloat), (dFloat*)vertexArray);
		
		// NewtonMeshGetNormalChannel(mesh, 3 * sizeof(dFloat), (dFloat*)m_normal);
		// NewtonMeshGetUV0Channel(mesh, 2 * sizeof(dFloat), (dFloat*)m_uv);

		/*dMatrix matrix;
		matrix.m_posit = dVector(offset.x, offset.y, offset.z, 1.0f);
		matrix.TransformTriplex(vertexArray, 3 * sizeof(dFloat), vertexArray, 3 * sizeof(dFloat), vertexCount);*/
		// matrix.m_posit = dVector(0.0f, 0.0f, 0.0f, 1.0f);
		
		// matrix = (matrix.Inverse4x4()).Transpose();
		// matrix.TransformTriplex(m_normal, 3 * sizeof(dFloat), m_normal, 3 * sizeof(dFloat), m_vertexCount);*/

		// extract the materials index array for mesh
		void* const geometryHandle = NewtonMeshBeginHandle(mesh);
		for (int handle = NewtonMeshFirstMaterial(mesh, geometryHandle); handle != -1; handle = NewtonMeshNextMaterial(mesh, geometryHandle, handle))
		{
			int material = NewtonMeshMaterialGetMaterial(mesh, geometryHandle, handle);
			int indexCount = NewtonMeshMaterialGetIndexCount(mesh, geometryHandle, handle);
			object->estimateIndexCount(indexCount);
			unsigned int* indexArray = new unsigned[indexCount];

			NewtonMeshMaterialGetIndexStream(mesh, geometryHandle, handle, (int*)indexArray);

			// object->getUserObjectBindings().setUserAny(Ogre::Any(indexArray));

			/*float matrix[16];
			Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, &matrix[0]);
			NewtonCollisionForEachPolygonDo(body->getNewtonCollision(), &matrix[0], newtonPerPoly, object);*/

			

			for (int vert = 0; vert < vertexCount; vert++)
			{
				object->position(vertexArray[(vert * 3) + 0], vertexArray[(vert * 3) + 1], vertexArray[(vert * 3) + 2]);
			}
			for (int index = 0; index < indexCount; index++)
			{
				//could use object->line here but all that does is call index twice
				object->index(indexArray[index]);
			}

			//int j = 0;
			//for (int vert = 0; vert < vertexCount; vert++)
			//{
			//	//could use object->line here but all that does is call index twice
			//	int index0 = indexArray[(vert * 3) + 0];
			//	int index1 = indexArray[(vert * 3) + 1];
			//	int index2 = indexArray[(vert * 3) + 2];
			//	object->position(vertexArray[index0], vertexArray[index1], vertexArray[index2]);
			//	// object->index(index);
			//}
			//for (int index = 0; index < indexCount; index++)
			//{
			//	//could use object->line here but all that does is call index twice
			//	object->index(indexArray[index]);
			// }



			delete[] indexArray;
		}

		delete[] vertexArray;

#endif

#if 0
		int vertexCount = NewtonMeshGetPointCount(mesh);
		object->estimateVertexCount(vertexCount);
		dFloat* vertexArray = new dFloat[3 * vertexCount];
		memset(vertexArray, 0, 3 * vertexCount * sizeof(dFloat));

		NewtonMeshGetVertexChannel(mesh, 3 * sizeof(dFloat), (dFloat*)vertexArray);

		for (int i = 0; i < vertexCount; i++)
		{
			object->position(vertexArray[(i * 3) + 0], vertexArray[(i * 3) + 1], vertexArray[(i * 3) + 2]);
		}

		int j = 0;
		// int indexCount = NewtonMeshGetTotalIndexCount(mesh);
		// object->estimateIndexCount(indexCount);
		for (void* faceNode = NewtonMeshGetFirstFace(mesh); faceNode; faceNode = NewtonMeshGetNextFace(mesh, faceNode))
		{
			if (!NewtonMeshIsFaceOpen(mesh, faceNode))
			{
				int indexArray[8];
				int indexCount = NewtonMeshGetFaceIndexCount(mesh, faceNode);
				NewtonMeshGetFaceIndices(mesh, faceNode, indexArray);
				for (int i = 0; i < indexCount; i++)
				{
					object->index(j++);
				}
			}
		}

#endif

		NewtonMeshDestroy(mesh);
		object->end();
	}


	void Debugger::processBody(OgreNewt::Body* bod)
	{
		NewtonBody* newtonBody = bod->getNewtonBody();
		MaterialIdColorMap::iterator it = m_materialcolors.find(NewtonBodyGetMaterialGroupID(newtonBody));


		Ogre::Vector3 pos, vel, omega;
		Ogre::Quaternion ori;
		// bod->getVisualPositionOrientation(pos, ori);
		bod->getPositionOrientation(pos, ori);

		// Why the heck is there a debugging position offset??
				// pos += (ori * (bod->getCollision()->getAABB().getCenter() + Ogre::Vector3(0.0f, 0.0f, -0.5f)));
				// ori = ori * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::NEGATIVE_UNIT_X);

		/*Ogre::AxisAlignedBox boundingBox = bod->getAABB();
		Ogre::Vector3 padding = (boundingBox.getMaximum() - boundingBox.getMinimum()) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
		Ogre::Vector3 size = ((boundingBox.getMaximum() - boundingBox.getMinimum()) - padding * 2.0f) * bod->getOgreNode()->getScale();
		Ogre::Vector3 centerOffset = boundingBox.getMinimum() + padding + (size / 2.0f);*/
		// pos = pos + bod->getAABB().getCenter();


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
			if (this->showText)
			{
				// data->m_text->setCaption(oss_info.str());
				// data->m_text->setLocalTranslation(bod->getAABB().getSize().y * 1.1f * Ogre::Vector3::UNIT_Y);
			}
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

			if (this->showText)
			{
				/*if (data->m_text)
				{
					data->m_text->setCaption(oss_info.str());
					data->m_text->setLocalTranslation(bod->getAABB().getMaximum().y * 1.1f * Ogre::Vector3::UNIT_Y);
				}
				else
				{
					data->m_text = new OgreNewt::OgreAddons::MovableText(oss_name.str(), oss_info.str(), "BlueHighway-10", 0.5);
					Ogre::Vector3 max = bod->getAABB().getMaximum().y / 2.0f * Ogre::Vector3::UNIT_Y;
					data->m_text->setLocalTranslation(max + Ogre::Vector3::UNIT_Y * 0.1f);
					data->m_text->setTextAlignment(OgreNewt::OgreAddons::MovableText::H_LEFT, OgreNewt::OgreAddons::MovableText::V_ABOVE);
				}

				data->m_node->attachObject(data->m_text);*/
			}

			/*
					data->m_lines->begin("RedNoLighting", Ogre::v1::RenderOperation::OT_LINE_LIST );

					// set color
					if( it != m_materialcolors.end() )
						data->m_lines->colour(it->second);
					else
						data->m_lines->colour(m_defaultcolor);

					float matrix[16];
					Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, &matrix[0]);

					NewtonCollisionForEachPolygonDo( NewtonBodyGetCollision(newtonBody), &matrix[0], newtonPerPoly, data->m_lines );
					data->m_lines->end();
			*/
			buildDebugObjectFromCollision(data->m_lines, m_defaultcolor, bod, data->m_mesh);
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
		line->colour(m_prefilterdiscardedcol);

		body->getVisualPositionOrientation(pos, ori);
		pos = body->getAABB().getCenter() - pos;
		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);

		/*Ogre::AxisAlignedBox boundingBox = body->getAABB();
		Ogre::Vector3 padding = (boundingBox.getMaximum() - boundingBox.getMinimum()) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
		Ogre::Vector3 size = ((boundingBox.getMaximum() - boundingBox.getMinimum()) - padding * 2.0f) * body->getOgreNode()->getScale();
		Ogre::Vector3 centerOffset = boundingBox.getMinimum() + padding + (size / 2.0f);
		pos = centerOffset - pos;*/
		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);

		NewtonCollisionForEachPolygonDo(body->getNewtonCollision(), &matrix[0], newtonPerPoly, line);

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
		line->colour(m_hitbodycol);

		body->getVisualPositionOrientation(pos, ori);

		/*Ogre::AxisAlignedBox boundingBox = body->getAABB();
		Ogre::Vector3 padding = (boundingBox.getMaximum() - boundingBox.getMinimum()) * Ogre::v1::MeshManager::getSingleton().getBoundsPaddingFactor();
		Ogre::Vector3 size = ((boundingBox.getMaximum() - boundingBox.getMinimum()) - padding * 2.0f) * body->getOgreNode()->getScale();
		Ogre::Vector3 centerOffset = boundingBox.getMinimum() + padding + (size / 2.0f);
		pos = centerOffset - pos;*/
		Converters::QuatPosToMatrix(ori, pos, &matrix[0]);
		
		NewtonCollisionForEachPolygonDo(body->getNewtonCollision(), &matrix[0], newtonPerPoly, line);

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

	void _CDECL Debugger::newtonPerPoly(void* userData, int vertexCount, const dFloat* faceVertec, int id)
	{
		/*Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		Ogre::Vector3 p0, p1;

		if (vertexCount < 2)
			return;

		
		int i = vertexCount - 1;
		p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);


		for (i = 0; i < vertexCount; i++)
		{
			p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

			lines->position(p0);
			lines->position(p1);
			if (i < vertexCount - 1)
			{
				lines->line(id, id + 1);
			}

			p0 = p1;
		}*/

		// lines->index(id);
		
		
		//Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		//Ogre::Vector3 p0, p1;

		//if (vertexCount < 2)
		//	return;

		//int i = vertexCount - 1;
		//p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);


		//for (i = 0; i < vertexCount; i++)
		//{
		//	p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

		//	lines->position(p0);
		//	lines->position(p1);
		//	/*if (i < vertexCount - 1)
		//	{
		//		lines->line(i, i + 1);
		//	}*/

		//	p0 = p1;
		//}

		//Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		//Ogre::Vector3 p0, p1;
		//if (vertexCount < 2)
		//	return;
		//int idx1 = 0;
		//int idx2 = 0;
		//int i = vertexCount - 1;
		//p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);
		//// static int index = 0;
		//for (i = 0; i < vertexCount; i++)
		//{
		//	p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

		//	idx1 = lines->getCurrentVertexCount();
		//	if (i > 0)
		//	{
		//		idx1--;
		//	}
		//	lines->position(p0);
		//	idx2 = lines->getCurrentVertexCount();
		//	if (i == vertexCount - 1)
		//	{
		//		idx2--;
		//	}
		//	lines->position(p1);

		//	lines->line(idx1, idx2);

		//	// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "P" + Ogre::StringConverter::toString(index++) + ": " + Ogre::StringConverter::toString(p0));
		//	// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "P" + Ogre::StringConverter::toString(index++) + ": " + Ogre::StringConverter::toString(p1));

		//	// index++;
		//	p0 = p1;
		//	
		//}
		
		/* short indices[] = { 0, 2, 1,
                0, 3, 2,

                1,2,6,
                6,5,1,

                4,5,6,
                6,7,4,

                2,3,6,
                6,3,7,

                0,7,3,
                0,4,7,

                0,1,5,
                0,5,4
                         };*/

		
		//static std::vector<short> indices = { 0, 1, 2, 0, 2, 3, //front
		//	4, 5, 6, 4, 6, 7, //right
		//	8, 9, 10, 8, 10, 11, //back
		//	12, 13, 14, 12, 14, 15, //left
		//	16, 17, 18, 16, 18, 19, //upper
		//	20, 21, 22, 20, 22, 23 }; //bottom

		//Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		//Ogre::Vector3 p0, p1;
		//if (vertexCount < 2)
		//	return;
		//static int index = 0;
		//int idx1 = 0;
		//int idx2 = 0;
		//int i = vertexCount - 1;
		//p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

		//for (i = 0; i < vertexCount; i++)
		//{
		//	p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);
		//	lines->position(p0);
		//	lines->index(indices[index]);
		//	index++;
		//	lines->position(p1);
		//	lines->index(indices[index]);
		//	index++;


		//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "P0: " + Ogre::StringConverter::toString(p0) + " i0: " + Ogre::StringConverter::toString(index)
		//		+ " P1: " + Ogre::StringConverter::toString(p1) + " i1: " + Ogre::StringConverter::toString(index+1));
		//	p0 = p1;

		//	

		//}
		static int j = 0;
		
		Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		unsigned int* indices = Ogre::any_cast<unsigned int*>((lines)->getUserObjectBindings().getUserAny());

		Ogre::Vector3 p0, p1;

		if (vertexCount < 2)
			return;

		int i = vertexCount - 1;
		p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);


		for (i = 0; i < vertexCount; i++)
		{
			p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

			lines->position(p0);
			lines->position(p1);
			lines->line(indices[j++], indices[j++]);
			p0 = p1;
		}

		// Attention: indices are not deleted!
		// sizeof(indeices) / sizeof(int), if j >= that, then delete


		//Ogre::ManualObject* lines = (Ogre::ManualObject*)userData;
		//Ogre::Vector3 p0, p1;
		//if (vertexCount < 2)
		//	return;

		///*std::vector<int>* indices = Ogre::any_cast<std::vector<int>*>((lines)->getUserObjectBindings().getUserAny());
		//static int j = 0;*/

		///*if (j >= indices->size() - 1)
		//{
		//	j = 0;
		//	delete indices;
		//}*/

		//int i = vertexCount - 1;
		//p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);

		//for (int i = 0; i < vertexCount; i++)
		//{
		//	p1 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);
		//	lines->position(p1);

		//	// int index = indices->at(j);
		//	// lines->index(index);
		//	// j++;
		//}

		/*int idx1 = 0;
		int idx2 = 0;
		for (int i = 0; i < vertexCount - 1; i += 2)
		{
			
			p0 = Ogre::Vector3(faceVertec[(i * 3) + 0], faceVertec[(i * 3) + 1], faceVertec[(i * 3) + 2]);
			p1 = Ogre::Vector3(faceVertec[((i + 1) * 3) + 0], faceVertec[((i + 1) * 3) + 1], faceVertec[((i + 1) * 3) + 2]);

			idx1 = lines->getCurrentVertexCount();
			lines->position(p0);
			idx2 = lines->getCurrentVertexCount();
			lines->position(p1);
			lines->line(idx1, idx2);
		}*/
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
			/*
					while( m_raycastsnode->numAttachedObjects() > 0 )
					{
						delete m_raycastsnode->detachObject((unsigned short)0);
					}
			*/
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
		line->colour(m_raycol);
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

	void Debugger::addConvexRay(const OgreNewt::ConvexCollisionPtr& col, const Ogre::Vector3 &startpt, const Ogre::Quaternion &colori, const Ogre::Vector3 &endpt)
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
		line->colour(m_convexcol);

		/*
		// aab1
		Ogre::AxisAlignedBox aab1 = col->getAABB(colori, startpt);
		const Ogre::Vector3* corners1 = aab1.getAllCorners();
		Ogre::AxisAlignedBox aab2 = col->getAABB(colori, endpt);
		const Ogre::Vector3* corners2 = aab2.getAllCorners();
		for(int i = 0; i < 4; i++)
		{
			line->position(corners1[i]); line->position(corners1[(i+1)%4]);
			line->position(corners1[i+4]); line->position(corners1[(i+1)%4+4]);
			line->position(corners2[i]); line->position(corners2[(i+1)%4]);
			line->position(corners2[i+4]); line->position(corners2[(i+1)%4+4]);
			line->position(corners1[i]); line->position(corners2[i]);
			line->position(corners1[i+4]); line->position(corners2[i+4]);
		}
		line->position(corners1[0]); line->position(corners1[6]);
		line->position(corners1[1]); line->position(corners1[5]);
		line->position(corners1[2]); line->position(corners1[4]);
		line->position(corners1[3]); line->position(corners1[7]);
		line->position(corners2[0]); line->position(corners2[6]);
		line->position(corners2[1]); line->position(corners2[5]);
		line->position(corners2[2]); line->position(corners2[4]);
		line->position(corners2[3]); line->position(corners2[7]);
		*/

		// bodies
		float matrix[16];
		
		Converters::QuatPosToMatrix(colori, startpt, &matrix[0]);
		NewtonCollisionForEachPolygonDo(col->getNewtonCollision(), &matrix[0], newtonPerPoly, line);

		if (endpt != startpt)
		{
			Converters::QuatPosToMatrix(colori, endpt, &matrix[0]);
			NewtonCollisionForEachPolygonDo(col->getNewtonCollision(), &matrix[0], newtonPerPoly, line);
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

