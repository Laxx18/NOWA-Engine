#ifndef MATH_HELPER_H
#define MATH_HELPER_H

#include "defines.h"
#include <random>

class Ogre::Camera;
class Ogre::SceneNode;
class Ogre::Viewport;
class Ogre::v1::Entity;
class Ogre::ManualObject;
class Ogre::RaySceneQuery;

namespace NOWA
{

	class EXPORTED MathHelper
	{
	public:
		/**
		* @brief		Maps an 3D-object to the graphics scene from 2D-mouse coordinates
		* @param[in]	node			The reference point, for example the node of the player
		* @param[in]	camera			The current view
		* @param[in]	mousePosition	The current mouse position
		* @param[in]	offset			The distance of the object (the higher the offset the further afar the object gets mapped to the mouse coordinates
		* @return		position		The resulting position, of the 3D-object that is mapped to the 2D mouse coordinates in the 3d world
		*/
		Ogre::Vector3 calibratedFromScreenSpaceToWorldSpace(Ogre::SceneNode* node, Ogre::Camera* camera, const Ogre::Vector2& mousePosition, const Ogre::Real& offset);

		/**
		* @brief		Gets the relative position to the window size. For example mapping an crosshair overlay object and conrolling it via mouse or Wii controller.
		* @param[in]	x				The x coordinate
		* @param[in]	y				The y coordinate
		* @param[in]	width			When using an overlay object with its width, to map the center of the object.
		* @param[in]	height			When using an overlay object with its height, to map the center of the object.
		* @param[in]	viewport		The reference viewport
		* @return		position		The resulting window position
		* @note		X, y coordinates are the origin point and width and height the right/bottom edge.
		*/
		Ogre::Vector4 getCalibratedWindowCoordinates(Ogre::Real x, Ogre::Real y, Ogre::Real width, Ogre::Real height, Ogre::Viewport* viewport);

		/**
		* @brief		Gets relative position to the overlay coordinate system. For example mapping the infrared camera of the WII controller.
		* @param[in]	left			The left corner
		* @param[in]	top				The top corner
		* @param[in]	right			The right corner
		* @param[in]	bottom			The bottom corner
		* @return		position		The resulting relative position to the overlay coordinate system
		*/
		Ogre::Vector4 getCalibratedOverlayCoordinates(Ogre::Real left, Ogre::Real top, Ogre::Real right, Ogre::Real bottom);

		/**
		* @see			Core::getCalibratedOverlayCoordinates()
		*/
		Ogre::Vector4 getCalibratedOverlayCoordinates(Ogre::Vector4 border);

		/**
		* @brief		Builds a custom projected orthogonal matrix for camera for zooming
		* @param[in]	left				The left corner
		* @param[in]	right				The right corner
		* @param[in]	bottom				The bottom corner
		* @param[in]	top					The top corner
		* @param[in]	nearClipDistance	The near clip distance corner
		* @param[in]	farClipDistance		The near clip distance corner
		* @return		position			The resulting matrix for custom projection
		*/
		Ogre::Matrix4 buildScaledOrthoMatrix(Ogre::Real left, Ogre::Real right, Ogre::Real bottom, Ogre::Real top, Ogre::Real nearClipDistance, Ogre::Real farClipDistance);

		/**
		* @brief		A low pass filter in order to smooth chaotic values.
		* @param[in]	value			The current value of the device
		* @param[in]	oldValue		The old value of the device (value in the previous cycle)
		* @return		new value		The smoothed value
		*/
		Ogre::Real lowPassFilter(Ogre::Real value, Ogre::Real oldValue, Ogre::Real scale = 0.01f);

		/**
		* @brief		A low pass filter in order to smooth chaotic vector3 values.
		* @param[in]	value			The current value of the device
		* @param[in]	oldValue		The old value of the device (value in the previous cycle)
		* @return		new value		The smoothed vector3 value
		*/
		Ogre::Vector3 lowPassFilter(const Ogre::Vector3& value, const Ogre::Vector3& oldValue, Ogre::Real scale = 0.01f);

		/**
		 * @brief Clamps the value between min and max.
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real clamp(Ogre::Real value, Ogre::Real min, Ogre::Real max);

		/**
		* @brief	Gradually changes a value towards a desired goal over time.
		* @param[in]		current				The current position
		* @param[in|out]	target				The target position to reach
		* @param[in|out]	currentVelocity		The current velocity, this value is modified by the function every time the function is called
		* @param[in|out]	smoothTime			Approximately the time it will take to reach the target. A smaller value will reach the target faster.
		* @param[in]		maxSpeed			Optionally allowing to clamp the maximum speed.
		* @param[in]		deltaTime			The time since the last call to this function. By default Time.deltaTime.
		* @return		smoothValue		The smoothed value
		*/
		Ogre::Real smoothDamp(Ogre::Real current, Ogre::Real& target, Ogre::Real& currentVelocity, Ogre::Real& smoothTime, Ogre::Real maxSpeed = Ogre::Math::POS_INFINITY, Ogre::Real deltaTime = 0.016f);

		/**
		* @brief	Gradually changes a value towards a desired goal over time for vector3.
		* @param[in]		current				The current position
		* @param[in|out]	target				The target position to reach
		* @param[in|out]	currentVelocity		The current velocity, this value is modified by the function every time the function is called
		* @param[in|out]	smoothTime			Approximately the time it will take to reach the target. A smaller value will reach the target faster.
		* @param[in]		maxSpeed			Optionally allowing to clamp the maximum speed.
		* @param[in]		deltaTime			The time since the last call to this function. By default Time.deltaTime.
		* @return		smoothValue		The smoothed value
		*/
		Ogre::Vector3 smoothDamp(const Ogre::Vector3& current, Ogre::Vector3& target, Ogre::Vector3& currentVelocity, Ogre::Real& smoothTime, Ogre::Real maxSpeed = Ogre::Math::POS_INFINITY, Ogre::Real deltaTime = 0.016f);
	
		/**
		* @brief		Gets the mouse coordinates relative to the viewport.
		* @param[in]	mx				The current mouse x coordinate
		* @param[in]	my				The current mouse y coordinate
		* @param[out]	x				The resulting mouse x coordinate
		* @param[out]	y				The resulting mouse y coordinate
		* @param[in]	window	The reference renderWindow
		*/
		void mouseToViewPort(int mx, int my, Ogre::Real& x, Ogre::Real& y, Ogre::Window* renderWindow);

		/**
		* @brief		Rounds a number according to the number of places. E.g. 0.5 will be 1.0 and 4.443 will be 4.44 when number of places is set to 2.
		* @param[in]	num				The number to round
		* @param[in]	places			The the places behind the dot for precision
		* @return		roundedNumber	The rounded number
		*/
		Ogre::Real round(Ogre::Real number, unsigned int places);

		/**
		* @brief		Rounds a number
		* @param[in]	num				The number to round
		* @return		roundedNumber	The rounded number
		*/
		Ogre::Real round(Ogre::Real number);

		/**
		 * @brief		Rounds a vector according to the number of places.
		 * @param[in]	vec				The vec to round (x, y)
		 * @param[in]	places			The the places behind the dot for precision
		 * @return		roundedVector	The rounded vector
		 */
		Ogre::Vector2 round(Ogre::Vector2 vec, unsigned int places);

		/**
		 * @brief		Rounds a vector according to the number of places.
		 * @param[in]	vec				The vec to round (x, y, z)
		 * @param[in]	places			The the places behind the dot for precision
		 * @return		roundedVector	The rounded vector
		 */
		Ogre::Vector3 round(Ogre::Vector3 vec, unsigned int places);

		/**
		 * @brief		Rounds a vector according to the number of places.
		 * @param[in]	vec				The vec to round (x, y, z, w)
		 * @param[in]	places			The the places behind the dot for precision
		 * @return		roundedVector	The rounded vector
		 */
		Ogre::Vector4 MathHelper::round(Ogre::Vector4 vec, unsigned int places);

		/**
		* @brief		Gets the difference pitch of two quaternions
		* @param[in]	dest			The destination quaternion
		* @param[in]	src				The source quaternion
		* @return		pitchQuaternion	The quaternion that holds the pitch difference
		*/
		Ogre::Quaternion diffPitch(Ogre::Quaternion dest, Ogre::Quaternion src);

		/**
		* @brief		Gets the difference yaw of two quaternions
		* @param[in]	dest			The destination quaternion
		* @param[in]	src				The source quaternion
		* @return		yawQuaternion	The quaternion that holds the yaw difference
		*/
		Ogre::Quaternion diffYaw(Ogre::Quaternion dest, Ogre::Quaternion src);

		/**
		* @brief		Gets the difference roll of two quaternions
		* @param[in]	dest			The destination quaternion
		* @param[in]	src				The source quaternion
		* @return		rollQuaternion	The quaternion that holds the roll difference
		*/
		Ogre::Quaternion diffRoll(Ogre::Quaternion dest, Ogre::Quaternion src);

		/**
		* @brief		Gets the difference of two quaternions
		* @param[in]	dest			The destination quaternion
		* @param[in]	src				The source quaternion
		* @param[in]	mode			The mode, 1: pitch quaternion, 2: yaw quaternion, 3: roll quaternion will be calculated
		* @return		rollQuaternion	The quaternion that holds the roll difference
		*/
		Ogre::Quaternion diffDegree(Ogre::Quaternion dest, Ogre::Quaternion src, int mode);

		/**
		* @brief		Extracts the yaw (heading), pitch and roll out of the quaternion.
		* @param[in]	quat			The source direction.
		* @return		Vector3(heading, pitch, roll)	The Vector3 holding pitch angle in radians for x, the heading angle in radians for y and the roll angle in radians for z.
		*/
		Ogre::Vector3 extractEuler(const Ogre::Quaternion& quat);

		/**
		* @brief		Gets the new orientation out from the current orientation and its steering angle in radians and pitch angle in radians, taking the default model direction into account.
		*/
		Ogre::Quaternion getOrientationFromHeadingPitch(const Ogre::Quaternion& orientation, Ogre::Real steeringAngle, Ogre::Real pitchAngle, const Ogre::Vector3& defaultModelDirection);

		/**
		* @brief		Gets the orientation in order to face a target, this orientation can be set directly.
		* @param[in]	source			The source scene node
		* @param[in]	dest			The dest scene node as target
		* @return		orientation		The quaternion that holds the orientation in order to face the target
		*/
		Ogre::Quaternion faceTarget(Ogre::SceneNode* source, Ogre::SceneNode* dest);

		/**
		* @brief		Gets the orientation in order to face a target, this orientation can be set directly.
		* @param[in]	source				The source scene node
		* @param[in]	dest				The dest scene node as target
		* @param[in]	defaultDirection	The default direction
		* @return		orientation			The quaternion that holds the orientation in order to face the target
		*/
		Ogre::Quaternion faceTarget(Ogre::SceneNode* source, Ogre::SceneNode* dest, const Ogre::Vector3& defaultDirection);

		/**
		* @brief		Gets the orientation in order to face a direction vector.
		* @param[in]	source			The source scene node
		* @param[in]	direction		The direction vector face
		* @return		orientation		The quaternion that holds the orientation in order to face the direction vector
		*/
		Ogre::Quaternion faceDirection(Ogre::SceneNode* source, const Ogre::Vector3& direction);

		/**
		* @brief		Gets the orientation in order to face a direction vector and also uses local direction vector to constrain one axis (e.g. avoid roll)
		* @param[in]	source					The source scene node
		* @param[in]	direction				The direction vector to face
		* @param[in]	localDirectionVector	The local direction vector to constrain that axis
		* @return		orientation		The quaternion that holds the orientation in order to face the direction vector
		*/
		Ogre::Quaternion faceDirection(Ogre::SceneNode* source, const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector);

		/**
		* @brief		Gets the orientation in order to face a direction vector and also uses local direction vector to constrain one axis (e.g. avoid roll)
		* @param[in]	sourceOrientation		The source orientation
		* @param[in]	direction				The direction vector to face
		* @param[in]	localDirectionVector	The local direction vector to constrain that axis
		* @return		orientation		The quaternion that holds the orientation in order to face the direction vector
		*/
		Ogre::Quaternion faceDirection(const Ogre::Quaternion& sourceOrientation, const Ogre::Vector3& direction, const Ogre::Vector3& localDirectionVector);

		/**
		* @brief		Gets the orientation slerp steps in order to face towards a direction vector and also uses the default direction of the source.
		* @param[in]	sourceOrientation		The source orientation
		* @param[in]	direction				The direction vector to face
		* @param[in]	defaultDirection		The default direction vector of the source
		* @param[in]	dt						The delta time between two frames. Use it in an update function and use the dt from there.
		* @param[in]	rotationSpeed			The rotation speed. Default is 10, which slows down the rotation a little bit.
		* @return		orientation		The quaternion that holds the orientation slerp steps in order to face towards direction vector.
		*/
		Ogre::Quaternion faceDirectionSlerp(const Ogre::Quaternion& sourceOrientation, const Ogre::Vector3& direction, const Ogre::Vector3& defaultDirection, Ogre::Real dt, Ogre::Real rotationSpeed = 10.0f);

		/**
		* @brief		Gets the orientation in order to look a the target direction with a fixed axis.
		* @param[in]	unnormalizedTargetDirection		The direction vector to face
		* @param[in]	fixedAxis						The fixed axis, which is used for orientation. Default is y, so that the result quaternion will not pitch or roll.
		* @return		orientation		The quaternion that holds the orientation in order to look the direction vector
		*/
		Ogre::Quaternion lookAt(const Ogre::Vector3& unnormalizedTargetDirection, const Ogre::Vector3 fixedAxis = Ogre::Vector3::UNIT_Y);

		/**
		* @brief		Reimplementation of Ogre::Camera lookAt
		*/
		Ogre::Quaternion computeLookAtQuaternion(const Ogre::Vector3& position, const Ogre::Vector3& target, const Ogre::Vector3& worldUp = Ogre::Vector3::UNIT_Y);

		/**
		* @brief		Reimplementation of Ogre::Camera setDirection
		*/
		Ogre::Quaternion computeDirectionQuaternion(const Ogre::Vector3& direction, const Ogre::Vector3& fallbackUp = Ogre::Vector3::UNIT_Y);

		/**
		 * @brief		Gets angle between two vectors. The normal of the two vectors is used in conjunction with the signed angle parameter, to determine
		 *				whether the angle between the two vectors is negative or positive.
		 * @info		This function can be use e.g. when using a gizmo or a grabber to rotate objects via mouse in the correct direction.
		 * @param[in]	dir1			The first direction vector
		 * @param[in]	dir2			The second direction vector
		 * @param[in]	norm			The normal of the vectors
		 * @param[in]	signedAngle		Whether to calculate a signed angle or not, see @info.
		 * @return		radian			The result radian angle.
		 */
		Ogre::Radian getAngle(const Ogre::Vector3& dir1, const Ogre::Vector3& dir2, const Ogre::Vector3& norm, bool signedAngle);

		/**
		* @brief		Normalizes the degree angle, e.g. a value bigger 180 would normally be set to -180, so after normalization even 181 degree are possible
		* @param[in]	degree				The degree to normalize
		* @return		normalizedDegree	The normalized degree angle
		*/
		Ogre::Real normalizeDegreeAngle(Ogre::Real degree);

		/**
		* @brief		Normalizes the radian angle, e.g. a value bigger pi would normally be set to -pi, so after normalization even pi + 0.03 degree are possible
		* @param[in]	radian				The radian to normalize
		* @return		normalizedRadian	The normalized radian angle
		*/
		Ogre::Real normalizeRadianAngle(Ogre::Real radian);

		/**
		* @brief		Checks whether the given degree angles are equal. Internally the angles are normalized.
		* @param[in]	degree0				The degree0 to compare
		* @param[in]	degree1				The degree1 to compare
		* @return		success	true, When degree1 does equal degree0, else false
		*/
		bool degreeAngleEquals(Ogre::Real degree0, Ogre::Real degree1);

		/**
		* @brief		Checks whether the given radian angles are equal. Internally the radians are normalized.
		* @param[in]	radian0				The radian0 to compare
		* @param[in]	radian1				The radian1 to compare
		* @return		success	true, When radian1 does equal radian0, else false
		*/
		bool radianAngleEquals(Ogre::Real radian0, Ogre::Real radian1);

		/**
		* @brief		Compares two real objects, using tolerance for inaccuracies.
		* @param[in]	first				The first real to compare
		* @param[in]	second				The second real to compare
		* @return		success	true, When first real does equal second real considering given tolerance
		*/
		bool realEquals(Ogre::Real first, Ogre::Real second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon());

		/**
		* @brief		Compares two vector2 objects, using tolerance for inaccuracies.
		* @param[in]	first				The first vector to compare
		* @param[in]	second				The second vector to compare
		* @return		success	true, When first vector does equal second vector considering given tolerance
		*/
		bool vector2Equals(const Ogre::Vector2& first, const Ogre::Vector2& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon());

		/**
		* @brief		Compares two vector3 objects, using tolerance for inaccuracies.
		* @param[in]	first				The first vector to compare
		* @param[in]	second				The second vector to compare
		* @return		success	true, When first vector does equal second vector considering given tolerance
		*/
		bool vector3Equals(const Ogre::Vector3& first, const Ogre::Vector3& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon());

		/**
		* @brief		Compares two vector4 objects, using tolerance for inaccuracies.
		* @param[in]	first				The first vector to compare
		* @param[in]	second				The second vector to compare
		* @return		success	true, When first vector does equal second vector considering given tolerance
		*/
		bool vector4Equals(const Ogre::Vector4& first, const Ogre::Vector4& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon());

		/**
		* @brief		Compares two quaternion objects, using tolerance for inaccuracies.
		* @param[in]	first				The first quaternion to compare
		* @param[in]	second				The second quaternion to compare
		* @return		success	true, When first quaternion does equal second quaternion considering given tolerance
		*/
		bool quaternionEquals(const Ogre::Quaternion& first, const Ogre::Quaternion& second, Ogre::Real tolerance = std::numeric_limits<Ogre::Real>::epsilon());

		/**
		* @brief		Converts degree angle to radian
		* @param[in]	degree	The degree to convert
		* @return		radian	The converted radian
		*/
		Ogre::Real degreeAngleToRadian(Ogre::Real degree);

		/**
		* @brief		Converts radian angle to degree
		* @param[in]	degree	The degree to convert
		* @return		degree	The converted degree
		*/
		Ogre::Real radianAngleToDegree(Ogre::Real radian);

		/**
		* @brief		Builds the orientation in the form of: degree, x-axis, y-axis, z-axis for better usability
		* @param[in]	quat	The quaternion to convert
		* @return		values	The degree, x-axis, y-axis, z-axis as 4 components in a vector
		*/
		Ogre::Vector4 quatToDegreeAngleAxis(const Ogre::Quaternion& quat);

		/**
		* @brief		Builds the orientation in the form of: degreeX, degreeY, degreeZ for better usability
		* @param[in]	quat	The quaternion to convert
		* @return		values	The degree, degreeX, degreeY, degreeZ as 3 components in a vector
		*/
		Ogre::Vector3 quatToDegrees(const Ogre::Quaternion& quat);

		/**
		* @brief		Builds the rounded orientation in the form of: degreeX, degreeY, degreeZ for better usability
		* @param[in]	quat	The quaternion to convert
		* @return		values	The degree, degreeX, degreeY, degreeZ as 3 components in a vector
		*/
		Ogre::Vector3 quatToDegreesRounded(const Ogre::Quaternion& quat);

		/**
		 * @brief		Builds the	quaternion from degreeX, degreeY, degreeZ
		 * @param[in]	degrees	The degrees to convert
		 * @return		values	The quaternion
		 */
		Ogre::Quaternion degreesToQuat(const Ogre::Vector3& degrees);

		/**
		 * @brief		Gets the angle, for the given target location
		 * @param[in]	currentLocation		The current location of the object
		 * @param[in]	currentOrientation	The current orientation of the object
		 * @param[in]	targetLocation		The target location at which the object should be rotated
		 * @param[in]	defaultOrientation	The default orientation axis, at which the object is orientated (normally NEGATIVE_UNIT_Z)

		 * @return		angle	The angle that is required to rotate the object based on its current location and orientation to the target location
		 */
		Ogre::Radian getAngleForTargetLocation(const Ogre::Vector3& currentLocation, const Ogre::Quaternion& currentOrientation, const Ogre::Vector3& targetLocation, const Ogre::Vector3& defaultOrientation);

		/**
		 * @brief		Computes a quaternion between UNIT_Z and direction. It keeps the "up" vector to UNIT_Y.
		 * @param[in]	direction			The direction vector
		 * @param[in]	angle				The up vector
		 * @return		result				The result quaternion.
		 */
		Ogre::Quaternion computeQuaternion(const Ogre::Vector3& direction, const Ogre::Vector3& upVector = Ogre::Vector3::UNIT_Y);

		/**
		 * @brief		Rotates a Vector2 by a given oriented angle
		 * @param[in]	in					The source vector
		 * @param[in]	angle				The angle for rotation
		 * @return		out					The rotated target vector.
		 */
		Ogre::Vector2 rotateVector2(const Ogre::Vector2& in, const Ogre::Radian& angle);

		/// Gets the min of the coordinates between 2 vectors

		/**
		 * @brief		Gets the min of the coordinates between 2 numbers
		 * @param[in]	v1					The first number
		 * @param[in]	v2					The second number
		 * @return		result				The min number.
		 */
		Ogre::Real min(Ogre::Real v1, Ogre::Real v2);

		/**
		 * @brief		Gets the max of the coordinates between 2 numbers
		 * @param[in]	v1					The first number
		 * @param[in]	v2					The second number
		 * @return		result				The max number.
		 */
		Ogre::Real max(Ogre::Real v1, Ogre::Real v2);

		/**
		 * @brief		Gets the min of the coordinates between 2 vectors
		 * @param[in]	v1					The first vector
		 * @param[in]	v2					The second vector
		 * @return		result				The min coordinates vector.
		 */
		Ogre::Vector3 min(const Ogre::Vector3& v1, const Ogre::Vector3& v2);

		/**
		 * @brief		Gets the max of the coordinates between 2 vectors
		 * @param[in]	v1					The first vector
		 * @param[in]	v2					The second vector
		 * @return		result				The max coordinates vector.
		 */
		Ogre::Vector3 max(const Ogre::Vector3& v1, const Ogre::Vector3& v2);

		/**
		 * @brief		Gets the min of the coordinates between 2 vectors
		 * @param[in]	v1					The first vector
		 * @param[in]	v2					The second vector
		 * @return		result				The min coordinates vector.
		 */
		Ogre::Vector2 min(const Ogre::Vector2& v1, const Ogre::Vector2& v2);

		/**
		 * @brief		Gets the max of the coordinates between 2 vectors
		 * @param[in]	v1					The first vector
		 * @param[in]	v2					The second vector
		 * @return		result				The max coordinates vector.
		 */
		Ogre::Vector2 max(const Ogre::Vector2& v1, const Ogre::Vector2& v2);

		/**
		 * @brief		Gets a random angle.
		 * @return		rndAngle				The random angle to get.
		 */
		Ogre::Degree getRandomAngle();

		/**
		 * @brief		Gets a random vector3.
		 * @return		rndVector				The random vector3 to get.
		 */
		Ogre::Vector3 getRandomVector();

		/**
		 * @brief		Gets a random direction.
		 * @return		rndDirection			The random direction to get.
		 */
		Ogre::Quaternion getRandomDirection();

		/**
		 * @brief		Adds to a given random vector a position multiplied by an offset.
		 * @param[in]	pos					The pos to add.
		 * @param[in]	offset				The offset to multiply.
		 * @return		rndVector			The random vector3 to get.
		 */
		Ogre::Vector3 addRandomVectorOffset(const Ogre::Vector3& pos, Ogre::Real offset);

		/**
		 * @brief		An extend version of the standard modulo, in that int values are "wrapped"
		 *				in both directions, whereas with standard modulo, (-1)%2 == -1
		 *				Always return an int between 0 and cap-1
		 * @param[in]	n					The first number
		 * @param[in]	cap					The modulo number
		 * @return		result				The wrapped modulo result.
		 */
		int wrappedModulo(int n, int cap);

		/**
		* @brief		Calculates the rotation grid value
		* @param[in]	step	The grid step for rotation
		* @param[in]	angle	The source angle
		* @return		gridValue	The calculated rotation grid value
		* @note			This function can be called during rotation progress to rotate an object in steps. E.g. step = 2.0 the given angle remains the same after 2 degress have passed.
		*/
		Ogre::Real calculateRotationGridValue(Ogre::Real step, Ogre::Real angle);

		Ogre::Real calculateRotationGridValue(const Ogre::Quaternion& orientation, Ogre::Real step, Ogre::Real angle);

		/**
		* @brief		Calculates the grid value. E.g. step = 2 would calculate a grid that is 2x2 meters in x and z
		* @param[in]	step			The grid step
		* @param[in]	sourcePosition	The source position vector
		* @return		gridValue		The calculated grid value
		* @note			A object will be only moved if it passes the critical distance.
		*				Do it without the y coordinate because its for placing an object and would cause that the objects gains high
		*/
		Ogre::Vector3 calculateGridValue(Ogre::Real step, const Ogre::Vector3& sourcePosition);

		/**
		* @brief		Calculates the grid value for the given movable object's size and also takes the movable object orientation into account.
		* @param[in]	gridFactor			The gridFactor to multiply the movable object's size with.
		* @param[in]	targetWorldPosition	The target world position vector
		* @param[in]	movableObject		The target movable object to calculate the grid value for
		* @return		result				The result grid snapped vector
		* @note			A object will be only moved if it passes the critical distance.
		*/
		Ogre::Vector3 calculateGridTranslation(const Ogre::Vector3& gridFactor, const Ogre::Vector3& targetWorldPosition, Ogre::MovableObject* movableObject);
		
		/**
		 * @brief		Gets the bottom center point of the given movable object mesh
		 * @param[in]	sceneNode	The scene node (for taking scale into account)
		 * @param[in]	movableObject	The movable object to get the bounding box for calcuation
		 * @return		bottomCenter	The bottom center point to get
		 */
		Ogre::Vector3 getBottomCenterOfMesh(Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject) const;

		/**
		* @brief			Gets geometry information about a given mesh from entity
		* @param[in]		entity		The entity to get the mesh and geometry data from
		* @param[in|out]	vertexCount	The vertex count of the model
		* @param[in|out]	vertices	The vertices array of the model
		* @param[in|out]	indexCount	The index count of the model
		* @param[in|out]	indices		The indices array of the model
		* @param[in]		position	The position as offset (can be the given entity->getParentNode()->_getDerivedPosition())
		* @param[in]		orientation	The orientation as offset (can be the given entity->getParentNode()->_getDerivedOrientation())
		* @param[in]		scale		The scale as offset (can be the given entity->getParentNode()->_getDerivedScale())
		* @note				The vertices-, indices-data can be used to perform raycast on the model. See @raycastFromPoint
		*/
		void getMeshInformationWithSkeleton(Ogre::v1::Entity* entity, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices,
											const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale);

		void getMeshInformation(const Ogre::v1::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertexBuffer, size_t& indexCount, unsigned long*& indexBuffer,
								const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale);

		void getMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices,
								 const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale);

		void getDetailedMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices,
			size_t& indexCount, unsigned long*& indices,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale,
			bool& isVET_HALF4, bool& isIndices32);

		void getDetailedMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, Ogre::Vector3*& normals,
			Ogre::Vector2*& textureCoords, // Added parameter for texture coordinates
			size_t& indexCount, unsigned long*& indices,
			const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale,
			bool& isVET_HALF4, bool& isIndices32);

		/**
		* @brief			Gets geometry information about a given manual object
		* @param[in]		manualObject	The manual object to get the mesh and geometry data from
		* @param[in|out]	vertexCount		The vertex count of the model
		* @param[in|out]	vertices		The vertices array of the model
		* @param[in|out]	indexCount		The index count of the model
		* @param[in|out]	indices			The indices array of the model
		* @param[in]		position		The position as offset (can be the given manualObject->getParentNode()->_getDerivedPosition())
		* @param[in]		orientation		The orientation as offset (can be the given manualObject->getParentNode()->_getDerivedOrientation())
		* @param[in]		scale			The scale as offset (can be the given manualObject->getParentNode()->_getDerivedScale())
		* @note				The vertices-, indices-data can be used to perform raycast on the model. See @raycastFromPoint
		*/
		void getManualMeshInformation(const Ogre::v1::ManualObject* manualObject, size_t& vertexCount, Ogre::Vector3*& vertices,
									  size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale);

		/**
		* @see	getManualMeshInformation
		*/
		void getManualMeshInformation2(const Ogre::ManualObject* manualObject, size_t& vertexCount, Ogre::Vector3*& vertices,
									   size_t& indexCount, unsigned long*& indices, const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale);

		/**
		* @brief			Performs a raycast on model base to get a position on the model
		* @param[in]		raySceneQuery				The ray scene query, which must already have a valid !ray! and maybe a filter, on what objects to perform the raycast and which objects to skip.
		* @param[in]		camera						The camera for further data to get of the camera
		* @param[in|out]	resultPostionOnModel		The result position on the model
		* @param[in]		excludeMovableObjects		Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @return			True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastFromPoint(Ogre::RaySceneQuery* raySceneQuery, Ogre::Camera* camera, Ogre::Vector3& resultPostionOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr);

		/**
		* @brief			Performs a raycast on model based on current mouse position. (More convenient function as the other raycastFromPoint function)
		* @param[in]		X				The absolute mouse x position
		* @param[in]		Y				The absolute mouse y position
		* @param[in]		camera			The camera to through the ray from current camera position and orientation to the mouse x and y position
		* @param[in]		window	The renderWindow to calculate the mouse position in relation to the renderWindow size
		* @param[in]		raySceneQuery				The ray scene query, to check if the ray cast should be performed for the given object category
		* @param[in|out]	resultPositionOnModel		The result position on the model
		* @param[in]		excludeMovableObjects		Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @return		True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastFromPoint(int mouseX, int mouseY, Ogre::Camera* camera, Ogre::Window* renderWindow, Ogre::RaySceneQuery* raySceneQuery,
								 Ogre::Vector3& resultPositionOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr);

		/**
		* @brief			Performs a raycast on model from the given x and z coordinate
		* @param[in]		raySceneQuery	The ray scene query, to check if the ray cast should be performed for the given object category
		* @param[in|out]	height			The height of the model mesh for the given x and z coordinate
		* @param[in]		excludeMovableObjects		Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @return		True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastHeight(Ogre::Real x, Ogre::Real z, Ogre::RaySceneQuery* raySceneQuery, Ogre::Real& height, std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr);

		/**
		* @brief			Performs a raycast from a given point in a given direction
		* @param[in]		origin					The ray origin
		* @param[in]		direction				The ray direction
		* @param[in]		raySceneQuery			The ray scene query, to check if the ray cast should be performed for the given object category
		* @param[in|out]	result					The result vector
		* @param[in|out]	targetMovableObject		The target movable object that has been hit
		* @param[in]		excludeMovableObjects	Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @return		True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastResult(const Ogre::Vector3& origin, const Ogre::Vector3& direction, Ogre::RaySceneQuery* raySceneQuery, Ogre::Vector3& result, size_t& targetMovableObject,
							  std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr);

		bool getRaycastDetailsResult(const Ogre::Ray& ray, Ogre::RaySceneQuery* raySceneQuery, Ogre::Vector3& result, size_t& targetMovableObject,
			Ogre::Vector3*& outVertices, size_t& outVertexCount, unsigned long*& outIndices, size_t& outIndexCount);

		/**
		* @brief			Performs a raycast on model from the given x and z coordinate
		* @param[in]		raySceneQuery	The ray scene query, to check if the ray cast should be performed for the given object category
		* @param[in|out]	height			The height of the model mesh for the given x and z coordinate
		* @param[in|out]	normalOnModel				The normal of the hit model data
		* @return		True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastHeightAndNormal(Ogre::Real x, Ogre::Real z, Ogre::RaySceneQuery* raySceneQuery, Ogre::Real& height, Ogre::Vector3& normalOnModel);

		/**
		* @brief			Performs a raycast on model base to get a position on the model (Most complex and powerful raycast function)
		* @param[in]		raySceneQuery				The ray scene query, which must already have a valid !ray! and maybe a filter, on what objects to perform the raycast and which objects to skip.
		* @param[in]		camera						The camera for further data to get of the camera
		* @param[in|out]	resultPositionOnModel		The result position on the model
		* @param[in|out]	targetMovableObject			The target movable object that has been hit
		* @param[in|out]	closestDistance				The closest distance at which the ray hit the model
		* @param[in|out]	normalOnModel				The normal of the hit model data
		* @param[in]		excludeMovableObjects		Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @param[in]		forGizmo				Optional flag, if the query should only match the gizmo, so no query flags will be wasted
		* @note				Getting information about the normal of the hit model data allows e.g. to snap other objects and rotate them according to the normal
		* @return			True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastFromPoint(Ogre::RaySceneQuery* raySceneQuery, Ogre::Camera* camera, Ogre::Vector3& resultPositionOnModel, size_t& targetMovableObject, float& closestDistance,
								 Ogre::Vector3& normalOnModel, std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr, bool forGizmo = false);

		/**
		* @brief			Performs a raycast on model based on current mouse position. (More convenient function as the other raycastFromPoint function, but as powerful and complex)
		* @param[in]		X						The absolute mouse x position
		* @param[in]		Y						The absolute mouse y position
		* @param[in]		camera					The camera to through the ray from current camera position and orientation to the mouse x and y position
		* @param[in]		window			The render window to calculate the mouse position in relation to the render window size
		* @param[in]		raySceneQuery			The ray scene query, to check if the ray cast should be performed for the given object category
		* @param[in|out]	resultPositionOnModel	The result position on the model
		* @param[in|out]	targetMovableObject		The target movable object that has been hit
		* @param[in|out]	closestDistance			The closest distance at which the ray hit the model
		* @param[in|out]	normalOnModel			The normal of the hit model data
		* @param[in]		excludeMovableObjects		Optional list of movable objects (entity, item, etc.), that should be excluded from the ray cast
		* @param[in]		forGizmo				Optional flag, if the query should only match the gizmo, so no query flags will be wasted
		* @note				Getting information about the normal of the hit model data allows e.g. to snap other objects and rotate them according to the normal
		* @return			True if the raycast could be performed or hit the object, else false if e.g. the object category was not in the ray scene query filter
		*/
		bool getRaycastFromPoint(int mouseX, int mouseY, Ogre::Camera* camera, Ogre::Window* renderWindow, Ogre::RaySceneQuery* raySceneQuery,
								 Ogre::Vector3& resultPositionOnModel, size_t& targetMovableObject, float& closestDistance, Ogre::Vector3& normalOnModel,
								 std::vector<Ogre::MovableObject*>* excludeMovableObjects = nullptr, bool forGizmo = false);

		/// Ogre Matrix inverse really sucks (it use a full Gaussian pivoting when a simple transpose follow by vector rotation will do.
		inline Ogre::Matrix4 matrixTransposeInverse(const Ogre::Matrix4& matrix)
		{
			Ogre::Matrix4 tmp(matrix.transpose());
			Ogre::Vector4 posit(tmp[3][0], tmp[3][1], tmp[3][2], 1.0f);
			tmp[3][0] = 0.0f;
			tmp[3][1] = 0.0f;
			tmp[3][2] = 0.0f;

			posit = tmp * posit;
			tmp[0][3] = -posit.x;
			tmp[1][3] = -posit.y;
			tmp[2][3] = -posit.z;
			return tmp;
		}

		template <typename T>
		T getRandomNumber(T min, T max)
		{
			// Seed with a real random value, if available
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<T> uniformDist(min, max);
			return uniformDist(mt);
		}

		/**
		* @brief			Gets the world axis aligned bounding box for the scene node and its childs.
		* @param[in]		sceneNode		The scene node to get the world axis aligned bounding box for
		* @return			The world axis aligned bounding box
		*/
		Ogre::Aabb getWorldAABB(Ogre::SceneNode* sceneNode);

		/**
		* @brief			Remaps a normalized value (between 0 and 1.0f) to a target range.
		* @param[in]		valueToMap		The value to map
		* @param[in]		targetMin		The target minimum
		* @param[in]		targetMax		The target maximum
		* @return			The mapped value
		*/
		Ogre::Real mapValue(Ogre::Real valueToMap, Ogre::Real targetMin, Ogre::Real targetMax);

		/**
		* @brief			Remaps a value from a source range to a target range.
		* @param[in]		valueToMap		The value to map
		* @param[in]		sourceMin		The source minimum
		* @param[in]		sourceMax		The source maximum
		* @param[in]		targetMin		The target minimum
		* @param[in]		targetMax		The target maximum
		* @return			The mapped value
		*/
		Ogre::Real mapValue2(Ogre::Real valueToMap, Ogre::Real sourceMin, Ogre::Real sourceMax, Ogre::Real targetMin, Ogre::Real targetMax);

		/**
		* @brief			Limits the value by the given borders
		* @param[in]		lowerValue		The lower value limit
		* @param[in]		upperValue		The upper value limit
		* @param[in]		limitValue		The value to limit
		* @return			The mapped value
		*/
		Ogre::Real limit(Ogre::Real lowerValue, Ogre::Real upperValue, Ogre::Real limitValue)
		{
			return limitValue < lowerValue ? lowerValue : (upperValue < limitValue ? upperValue : limitValue);
		}

		/**
		* @brief			Converts the given amplitude to decibels
		* @param[in]		amplitude		The amplitude to convert
		* @return			The decibel value
		*/
		inline Ogre::Real amplitudeToDecibels(Ogre::Real amplitude)
		{
			// return 20.0f * log10(amplitude);
			return amplitude > Ogre::Real() ? std::max(Ogre::Math::NEG_INFINITY, static_cast<Ogre::Real> (std::log10(amplitude)) * Ogre::Real(20.0f)) : Ogre::Math::NEG_INFINITY;
		}

		/**
		* @brief			Converts the given decibel value to amplitude value
		* @param[in]		decibels		The decibels to convert
		* @return			The decibel value
		*/
		inline Ogre::Real decibelsToAmplitude(Ogre::Real decibels)
		{
			return pow(10.0f, decibels / 20.0f);
		}

		inline Ogre::Real divf(Ogre::Real val1, Ogre::Real val2)
		{
			return (val1 == 0 || val2 == 0) ? 0 : val1 / val2;
		}

		inline Ogre::Vector3 pointToLocalSpace(const Ogre::Vector3& point, Ogre::Vector3& heading, Ogre::Vector3& side, Ogre::Vector3& position);

		inline Ogre::Vector3 vectorToWorldSpace(const Ogre::Vector3& vector, const Ogre::Vector3& heading, const Ogre::Vector3& side);

		inline Ogre::Vector3 pointToWorldSpace(const Ogre::Vector3& point, const Ogre::Vector3& heading, const Ogre::Vector3& side, const Ogre::Vector3& position);

		bool hasNoTangentsAndCanGenerate(Ogre::v1::VertexDeclaration* vertexDecl);

		void ensureHasTangents(Ogre::v1::MeshPtr mesh);

		void substractOutTangentsForShader(Ogre::v1::Entity* entity);

		template <typename T>
		void localToGlobal(T* node, const Ogre::Quaternion& localOrient, const Ogre::Vector3& localPos, Ogre::Quaternion& globalOrient, Ogre::Vector3& globalPos)
		{
			globalPos = (node->_getDerivedOrientation() * localPos) + node->_getDerivedPosition();
			globalOrient = node->_getDerivedOrientation() * localOrient;
		}

		template <typename T>
		void globalToLocal(T* node, const Ogre::Quaternion& globalOrient, const Ogre::Vector3& globalPos, Ogre::Quaternion& localOrient, Ogre::Vector3& localPos)
		{
			Ogre::Quaternion orientInv = node->_getDerivedOrientation().Inverse();

			localOrient = orientInv * globalOrient;
			localPos = orientInv * (globalPos - node->_getDerivedPosition());
		}

		template <typename T>
		void findMinAndMax(T arr[], T low, T high, T& min, T& max)
		{
			// Divide & Conquer solution to find minimum and maximum number in an array
			// if array contains only one element

			if (low == high)            // common comparison
			{
				if (max < arr[low])     // comparison 1
					max = arr[low];

				if (min > arr[high])    // comparison 2
					min = arr[high];

				return;
			}

			// if array contains only two elements

			if (high - low == 1)            // common comparison
			{
				if (arr[low] < arr[high])    // comparison 1
				{
					if (min > arr[low])     // comparison 2
						min = arr[low];

					if (max < arr[high])    // comparison 3
						max = arr[high];
				}
				else
				{
					if (min > arr[high])    // comparison 2
						min = arr[high];

					if (max < arr[low])     // comparison 3
						max = arr[low];
				}
				return;
			}

			// find mid element
			T mid = (low + high) / 2;

			// recur for left sub-array
			findMinAndMax(arr, low, mid, min, max);

			// recur for right sub-array
			findMinAndMax(arr, mid + 1, high, min, max);
		}

		/**
		 * @brief			Tweaks the given unlit datablock, so that its not always rendered in foreground
		 * @param[in]		datablockName		The datablock name to tweak
		 */
		void tweakUnlitDatablock(const Ogre::String& datablockName);
	public:
		static MathHelper* getInstance();
	};

}; // namespace end


#endif