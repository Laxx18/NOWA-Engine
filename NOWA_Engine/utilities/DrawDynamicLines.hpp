#ifndef __DRAWDYNAMICLINES_HPP__
#define __DRAWDYNAMICLINES_HPP__

#include "defines.h"
#include <vector>

class ObjectMemoryManager;
class Vector3;


namespace NOWA
{
	class GraphicsManager;
	class Scene;
	class DynamicLines;

	/*
		Wrapper class for render dynamic lines
	*/
	class EXPORTED DrawDynamicLines
	{

	public:
		DrawDynamicLines(Ogre::SceneManager* sceneManager);

		virtual ~DrawDynamicLines();

		/// Add a point to the point list
		void addPoint(const Ogre::Vector3& p);
		/// Add a point to the point list
		void addPoint(float x, float y, float z);

		/// Change the location of an existing point in the point list
		void setPoint(unsigned short index, const Ogre::Vector3& value);

		/// Remove all points from the point list
		void clear();

		/// Call this to update the hardware buffer after making changes.
		void update();

	private:
		bool rendererisDone;
		Ogre::SceneManager* sceneManager;
		//modify only in renderthread!
		DynamicLines* dynamicLines;
		Ogre::ObjectMemoryManager* objectMemoryManager;
		bool swapBuffer;
		std::vector<Ogre::Vector3> pointsBuf1;
		std::vector<Ogre::Vector3> pointsBuf2;
	};

} // namespace end

#endif





