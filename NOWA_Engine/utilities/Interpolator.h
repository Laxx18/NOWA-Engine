#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include "defines.h"
#include <random>
#include <array>

class Ogre::SceneNode;

namespace NOWA
{

	class EXPORTED Interpolator
	{
	public:

		enum EaseFunctions
		{
			Linear,
			EaseInSine,
			EaseOutSine,
			EaseInOutSine,
			EaseInQuad,
			EaseOutQuad,
			EaseInOutQuad,
			EaseInCubic,
			EaseOutCubic,
			EaseInOutCubic,
			EaseInQuart,
			EaseOutQuart,
			EaseInOutQuart,
			EaseInQuint,
			EaseOutQuint,
			EaseInOutQuint,
			EaseInExpo,
			EaseOutExpo,
			EaseInOutExpo,
			EaseInCirc,
			EaseOutCirc,
			EaseInOutCirc,
			EaseInBack,
			EaseOutBack,
			EaseInOutBack,
			EaseInElastic,
			EaseOutElastic,
			EaseInOutElastic,
			EaseInBounce,
			EaseOutBounce,
			EaseInOutBounce
		};
	public:

		/**
		* @brief		Linear interpolation is a method useful for curve fitting using linear polynomials.
		*				It helps in building new data points within the range of a discrete set of already known data points.
		*				Therefore, the Linear interpolation is the simplest method for estimating a channel from the vector of the given channels estimates.
		*				It is very useful for data prediction and many other mathematical and scientific applications.
		* @param[in]	value				The point to perform the interpolation
		* @param[in]	xLowBorder			The x low border from which the interpolation shall start.
		* @param[in]	xHighBorder			The x high border from which the interpolation shall end.
		* @param[in]	yLowBorder			The y low border from which the interpolation shall start.
		* @param[in]	yHighBorder			The y high border from which the interpolation shall end.
		* @note			All those values are forming a range, in which the given value will be interpolated.
		* @details		E.g. xLowBorder = 12; yLowBorder = 0.5; xHighBorder = 21; yHighBorder = 1.0; if value = 12; result will be: 0.5 (yLowBorder); if value = 21; result will be 1.0 (yHighBorder); if value = 15 result will be: 0.61325
		* @return		interpolatedValue	The result interpolated value.
		*/
		Ogre::Real linearInterpolation(Ogre::Real value, Ogre::Real xLowBorder, Ogre::Real xHighBorder, Ogre::Real yLowBorder, Ogre::Real yHighBorder);

		/**
		 * @brief		Interpolates between two vectors at the given time t.
		 */
		Ogre::Vector3 interpolate(const Ogre::Vector3& v1, const Ogre::Vector3& v2, Ogre::Real t);

		/**
		 * @brief		Interpolates between two vectors at the given time t.
		 */
		Ogre::Real interpolate(Ogre::Real v1, Ogre::Real v2, Ogre::Real t);

		EaseFunctions mapStringToEaseFunctions(const Ogre::String& strEaseFunction);

		Ogre::String mapEaseFunctionsToString(EaseFunctions easeFunctions);

		Ogre::Vector3 applyEaseFunction(const Ogre::Vector3& startPosition, const Ogre::Vector3& targetPosition, EaseFunctions easeFunctions, Ogre::Real time);

		Ogre::Real applyEaseFunction(Ogre::Real v1, Ogre::Real v2, EaseFunctions easeFunctions, Ogre::Real time);

		std::vector<Ogre::String> getAllEaseFunctionNames(void) const;
	
		/**
		 * @brief Calculates the bezier y at the given time t.
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real calculateBezierYT(Ogre::Real t, bool invert);

		/**
		 * @brief Calculates the bezier y points at the given time t.
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real calcBezierYTPoints(Ogre::Real t, bool invert);

		/**
		 * @brief Slight acceleration from zero to full speed
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInSine(Ogre::Real t);

		/**
		 * @brief Slight deceleration at the end
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutSine(Ogre::Real t);

		/**
		 * @brief Slight acceleration at beginning and slight deceleration at end
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutSine(Ogre::Real t);

		/**
		 * @brief Accelerating from zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInQuad(Ogre::Real t);

		/**
		 * @brief Decelerating to zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutQuad(Ogre::Real t);

		/**
		 * @brief Acceleration until halfway, then deceleration
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutQuad(Ogre::Real t);

		/**
		 * @brief Accelerating from zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInCubic(Ogre::Real t);

		/**
		 * @brief Decelerating to zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutCubic(Ogre::Real t);

		/**
		 * @brief Acceleration until halfway, then deceleration
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutCubic(Ogre::Real t);

		/**
		 * @brief Accelerating from zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInQuart(Ogre::Real t);

		/**
		 * @brief Decelerating to zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutQuart(Ogre::Real t);

		/**
		 * @brief Acceleration until halfway, then deceleration
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutQuart(Ogre::Real t);

		/**
		 * @brief Accelerating from zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInQuint(Ogre::Real t);

		/**
		 * @brief Decelerating to zero velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutQuint(Ogre::Real t);

		/**
		 * @brief Acceleration until halfway, then deceleration
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutQuint(Ogre::Real t);

		/**
		 * @brief Accelerate exponentially until finish
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInExpo(Ogre::Real t);

		/**
		 * @brief Initial exponential acceleration slowing to stop
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutExpo(Ogre::Real t);

		/**
		 * @brief Exponential acceleration and deceleration
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutExpo(Ogre::Real t);

		/**
		 * @brief Increasing velocity until stop
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInCirc(Ogre::Real t);

		/**
		 * @brief Start fast, decreasing velocity until stop
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutCirc(Ogre::Real t);

		/**
		 * @brief Fast increase in velocity, fast decrease in velocity
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutCirc(Ogre::Real t);

		/**
		 * @brief Slow movement backwards then fast snap to finish
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInBack(Ogre::Real t, Ogre::Real magnitude = 1.70158f);

		/**
		 * @brief Fast snap to backwards point then slow resolve to finish
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutBack(Ogre::Real t, Ogre::Real magnitude = 1.70158f);

		/**
		 * @brief Slow movement backwards, fast snap to past finish, slow resolve to finish
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutBack(Ogre::Real t, Ogre::Real magnitude = 1.70158f);

		/**
		 * @brief Bounces slowly then quickly to finish
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInElastic(Ogre::Real t, Ogre::Real magnitude = 0.7f);

		/**
		 * @brief Fast acceleration, bounces to zero
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutElastic(Ogre::Real t, Ogre::Real magnitude = 0.7f);

		/**
		 * @brief Slow start and end, two bounces sandwich a fast motion
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutElastic(Ogre::Real t, Ogre::Real magnitude = 0.65f);

		/**
		 * @brief Bounce to completion
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeOutBounce(Ogre::Real t);

		/**
		 * @brief Bounce increasing in velocity until completion
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInBounce(Ogre::Real t);

		/**
		 * Bounce in and bounce out
		 * @param[in] t		The time to set e.g. t = (time - animationStart) / duration;
		 */
		Ogre::Real easeInOutBounce(Ogre::Real t);

	public:
		static Interpolator* getInstance();
	};

}; // namespace end


#endif