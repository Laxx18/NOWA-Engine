/*
 * =====================================================================================
 *
 *       Filename:  BtOgreExtras.h
 *
 *    Description:  Contains the Ogre Mesh to Bullet Shape converters.
 *
 *        Version:  1.0
 *        Created:  27/12/2008 01:45:56 PM
 *
 *         Author:  Nikhilesh (nikki)
 modified: Nuke
 *
 * =====================================================================================
 */

#ifndef __DYNAMICLINES_HPP__
#define __DYNAMICLINES_HPP__

#include "defines.h"
#include "OgreManualObject2.h"
#include "DynamicRenderable.hpp"

class Vector3;

namespace NOWA
{

	class EXPORTED DynamicLines : public Ogre::ManualObject
	{
		typedef Ogre::Quaternion Quaternion;
		typedef Ogre::Camera Camera;
		typedef Ogre::OperationType OperationType;

	public:
		DynamicLines(Ogre::IdType id, Ogre::ObjectMemoryManager* objManager, Ogre::SceneManager* sceneManager, OperationType opType = Ogre::OperationType::OT_LINE_STRIP);
		virtual ~DynamicLines();

		/// Add a point to the point list
		void addPoint(const Ogre::Vector3& p);
		/// Add a point to the point list
		void addPoint(Ogre::Real x, Ogre::Real y, Ogre::Real z);

		/// Change the location of an existing point in the point list
		void setPoint(unsigned short index, const Ogre::Vector3& value);

		/// Return the location of an existing point in the point list
		const Ogre::Vector3& getPoint(unsigned short index) const;

		/// Return the total number of points in the point list
		unsigned short getNumPoints(void) const;

		/// Remove all points from the point list
		void clear();

		/// Call this to update the hardware buffer after making changes.
		void update();

		/** Set the type of operation to draw with.
		 * @param opType Can be one of
		 *    - RenderOperation::OT_LINE_STRIP
		 *    - RenderOperation::OT_LINE_LIST
		 *    - RenderOperation::OT_POINT_LIST
		 *    - RenderOperation::OT_TRIANGLE_LIST
		 *    - RenderOperation::OT_TRIANGLE_STRIP
		 *    - RenderOperation::OT_TRIANGLE_FAN
		 *    The default is OT_LINE_STRIP.
		 */
		void setOperationType(OperationType opType);
		OperationType getOperationType() const;

		std::vector<Ogre::Vector3> points;

	protected:
		/// Implementation DynamicRenderable, creates a simple vertex-only decl
		virtual void createVertexDeclaration();
		/// Implementation DynamicRenderable, pushes point list out to hardware memory
		virtual void fillHardwareBuffers();
	private:
		Ogre::OperationType operationType;
		bool dirty;
	};

} // namespace end

#endif





