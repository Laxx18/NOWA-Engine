#include "NOWAPrecompiled.h"
#include "Interpolator.h"

#include "OgreMath.h"

namespace NOWA
{
	Interpolator* Interpolator::getInstance()
	{
		static Interpolator instance;
		return &instance;
	}

	Ogre::Real Interpolator::linearInterpolation(Ogre::Real value, Ogre::Real xLowBorder, Ogre::Real xHighBorder, Ogre::Real yLowBorder, Ogre::Real yHighBorder)
	{
		// https://www.toppr.com/guides/maths-formulas/linear-interpolation-formula/
		Ogre::Real interpolatedValue = yLowBorder + (((value - xLowBorder) * (yHighBorder - yLowBorder)) / (xHighBorder - xLowBorder));
		return interpolatedValue;
	}

	Ogre::Vector3 Interpolator::interpolate(const Ogre::Vector3& v1, const Ogre::Vector3& v2, Ogre::Real t)
	{
		return  Ogre::Vector3(
			v1.x * (1.0f - t) + v2.x * t,
			v1.y * (1.0f - t) + v2.y * t,
			v1.z * (1.0f - t) + v2.z * t
		);
	}

	Ogre::Real Interpolator::interpolate(Ogre::Real v1, Ogre::Real v2, Ogre::Real t)
	{
		return v1 + t * (v2 - v1);
	}

	Interpolator::EaseFunctions Interpolator::mapStringToEaseFunctions(const Ogre::String& strEaseFunction)
	{
		if ("linear" == strEaseFunction)
		{
			return Linear;
		}
		if ("easeInSine" == strEaseFunction)
		{
			return EaseInSine;
		}
		else if ("easeOutSine" == strEaseFunction)
		{
			return EaseOutSine;
		}
		else if ("easeInOutSine" == strEaseFunction)
		{
			return EaseInOutSine;
		}
		else if ("easeInQuad" == strEaseFunction)
		{
			return EaseInQuad;
		}
		else if ("easeOutQuad" == strEaseFunction)
		{
			return EaseOutQuad;
		}
		else if ("easeInOutQuad" == strEaseFunction)
		{
			return EaseInOutQuad;
		}
		else if ("easeInCubic" == strEaseFunction)
		{
			return EaseInCubic;
		}
		else if ("easeOutCubic" == strEaseFunction)
		{
			return EaseOutCubic;
		}
		else if ("easeInOutCubic" == strEaseFunction)
		{
			return EaseInOutCubic;
		}
		else if ("easeInQuart" == strEaseFunction)
		{
			return EaseInQuart;
		}
		else if ("easeOutQuart" == strEaseFunction)
		{
			return EaseOutQuart;
		}
		else if ("easeInOutQuart" == strEaseFunction)
		{
			return EaseInOutQuart;
		}
		else if ("easeInQuint" == strEaseFunction)
		{
			return EaseInQuint;
		}
		else if ("easeOutQuint" == strEaseFunction)
		{
			return EaseOutQuint;
		}
		else if ("easeInOutQuint" == strEaseFunction)
		{
			return EaseInOutQuint;
		}
		else if ("easeInExpo" == strEaseFunction)
		{
			return EaseInExpo;
		}
		else if ("easeOutExpo" == strEaseFunction)
		{
			return EaseOutExpo;
		}
		else if ("easeInOutExpo" == strEaseFunction)
		{
			return EaseInOutExpo;
		}
		else if ("easeInCirc" == strEaseFunction)
		{
			return EaseInCirc;
		}
		else if ("easeOutCirc" == strEaseFunction)
		{
			return EaseOutCirc;
		}
		else if ("easeInOutCirc" == strEaseFunction)
		{
			return EaseInOutCirc;
		}
		else if ("easeInBack" == strEaseFunction)
		{
			return EaseInBack;
		}
		else if ("easeOutBack" == strEaseFunction)
		{
			return EaseOutBack;
		}
		else if ("easeInOutBack" == strEaseFunction)
		{
			return EaseInOutBack;
		}
		else if ("easeInElastic" == strEaseFunction)
		{
			return EaseInElastic;
		}
		else if ("easeOutElastic" == strEaseFunction)
		{
			return EaseOutElastic;
		}
		else if ("easeInOutElastic" == strEaseFunction)
		{
			return EaseInOutElastic;
		}
		else if ("easeInBounce" == strEaseFunction)
		{
			return EaseInBounce;
		}
		else if ("easeOutBounce" == strEaseFunction)
		{
			return EaseOutBounce;
		}
		else if ("easeInOutBounce" == strEaseFunction)
		{
			return EaseInOutBounce;
		}
		return Linear;
	}

	Ogre::String Interpolator::mapEaseFunctionsToString(Interpolator::EaseFunctions easeFunctions)
	{
		switch (easeFunctions)
		{
		case Linear:
			return "linear";
		case EaseInSine:
			return "easeInSine";
		case EaseOutSine:
			return "easeOutSine";
		case EaseInOutSine:
			return "easeInOutSine";
		case EaseInQuad:
			return "easeInQuad";
		case EaseOutQuad:
			return "easeOutQuad";
		case EaseInOutQuad:
			return "easeInOutQuad";
		case EaseInCubic:
			return "easeInCubic";
		case EaseOutCubic:
			return "easeOutCubic";
		case EaseInOutCubic:
			return "easeInOutCubic";
		case EaseInQuart:
			return "easeInQuart";
		case EaseOutQuart:
			return "easeOutQuart";
		case EaseInOutQuart:
			return "easeInOutQuart";
		case EaseInQuint:
			return "easeInQuint";
		case EaseOutQuint:
			return "easeOutQuint";
		case EaseInOutQuint:
			return "easeInOutQuint";
		case EaseInExpo:
			return "easeInExpo";
		case EaseOutExpo:
			return "easeOutExpo";
		case EaseInOutExpo:
			return "easeInOutExpo";
		case EaseInCirc:
			return "easeInCirc";
		case EaseOutCirc:
			return "easeOutCirc";
		case EaseInOutCirc:
			return "easeInOutCirc";
		case EaseInBack:
			return "easeInBack";
		case EaseOutBack:
			return "easeOutBack";
		case EaseInOutBack:
			return "easeInOutBack";
		case EaseInElastic:
			return "easeInElastic";
		case EaseOutElastic:
			return "easeOutElastic";
		case EaseInOutElastic:
			return "easeInOutElastic";
		case EaseInBounce:
			return "easeInBounce";
		case EaseOutBounce:
			return "easeOutBounce";
		case EaseInOutBounce:
			return "easeInOutBounce";
		}
		return "Linear";
	}

	Ogre::Vector3 Interpolator::applyEaseFunction(const Ogre::Vector3& startPosition, const Ogre::Vector3& targetPosition, Interpolator::EaseFunctions easeFunctions, Ogre::Real time)
	{
		switch (easeFunctions)
		{
		case Linear:
			return this->interpolate(startPosition, targetPosition, time);
		case EaseInSine:
			return  this->interpolate(startPosition, targetPosition, this->easeInSine(time));
		case EaseOutSine:
			return  this->interpolate(startPosition, targetPosition, this->easeOutSine(time));
		case EaseInOutSine:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutSine(time));
		case EaseInQuad:
			return  this->interpolate(startPosition, targetPosition, this->easeInQuad(time));
		case EaseOutQuad:
			return  this->interpolate(startPosition, targetPosition, this->easeOutQuad(time));
		case EaseInOutQuad:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutQuad(time));
		case EaseInCubic:
			return  this->interpolate(startPosition, targetPosition, this->easeInCubic(time));
		case EaseOutCubic:
			return  this->interpolate(startPosition, targetPosition, this->easeOutCubic(time));
		case EaseInOutCubic:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutCubic(time));
		case EaseInQuart:
			return  this->interpolate(startPosition, targetPosition, this->easeInQuart(time));
		case EaseOutQuart:
			return  this->interpolate(startPosition, targetPosition, this->easeOutQuart(time));
		case EaseInOutQuart:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutQuart(time));
		case EaseInQuint:
			return  this->interpolate(startPosition, targetPosition, this->easeInQuint(time));
		case EaseOutQuint:
			return  this->interpolate(startPosition, targetPosition, this->easeOutQuint(time));
		case EaseInOutQuint:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutQuint(time));
		case EaseInExpo:
			return  this->interpolate(startPosition, targetPosition, this->easeInExpo(time));
		case EaseOutExpo:
			return  this->interpolate(startPosition, targetPosition, this->easeOutExpo(time));
		case EaseInOutExpo:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutExpo(time));
		case EaseInCirc:
			return  this->interpolate(startPosition, targetPosition, this->easeInCirc(time));
		case EaseOutCirc:
			return  this->interpolate(startPosition, targetPosition, this->easeOutCirc(time));
		case EaseInOutCirc:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutCirc(time));
		case EaseInBack:
			return  this->interpolate(startPosition, targetPosition, this->easeInBack(time));
		case EaseOutBack:
			return  this->interpolate(startPosition, targetPosition, this->easeOutBack(time));
		case EaseInOutBack:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutBack(time));
		case EaseInElastic:
			return  this->interpolate(startPosition, targetPosition, this->easeInElastic(time));
		case EaseOutElastic:
			return  this->interpolate(startPosition, targetPosition, this->easeOutElastic(time));
		case EaseInOutElastic:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutElastic(time));
		case EaseInBounce:
			return  this->interpolate(startPosition, targetPosition, this->easeInBounce(time));
		case EaseOutBounce:
			return  this->interpolate(startPosition, targetPosition, this->easeOutBounce(time));
		case EaseInOutBounce:
			return  this->interpolate(startPosition, targetPosition, this->easeInOutBounce(time));
		}
		return  this->interpolate(startPosition, targetPosition, time);
	}

	Ogre::Real Interpolator::applyEaseFunction(Ogre::Real v1, Ogre::Real v2, Interpolator::EaseFunctions easeFunctions, Ogre::Real time)
	{
		switch (easeFunctions)
		{
		case Linear:
			return this->interpolate(v1, v2, time);
		case EaseInSine:
			return  this->interpolate(v1, v2, this->easeInSine(time));
		case EaseOutSine:
			return  this->interpolate(v1, v2, this->easeOutSine(time));
		case EaseInOutSine:
			return  this->interpolate(v1, v2, this->easeInOutSine(time));
		case EaseInQuad:
			return  this->interpolate(v1, v2, this->easeInQuad(time));
		case EaseOutQuad:
			return  this->interpolate(v1, v2, this->easeOutQuad(time));
		case EaseInOutQuad:
			return  this->interpolate(v1, v2, this->easeInOutQuad(time));
		case EaseInCubic:
			return  this->interpolate(v1, v2, this->easeInCubic(time));
		case EaseOutCubic:
			return  this->interpolate(v1, v2, this->easeOutCubic(time));
		case EaseInOutCubic:
			return  this->interpolate(v1, v2, this->easeInOutCubic(time));
		case EaseInQuart:
			return  this->interpolate(v1, v2, this->easeInQuart(time));
		case EaseOutQuart:
			return  this->interpolate(v1, v2, this->easeOutQuart(time));
		case EaseInOutQuart:
			return  this->interpolate(v1, v2, this->easeInOutQuart(time));
		case EaseInQuint:
			return  this->interpolate(v1, v2, this->easeInQuint(time));
		case EaseOutQuint:
			return  this->interpolate(v1, v2, this->easeOutQuint(time));
		case EaseInOutQuint:
			return  this->interpolate(v1, v2, this->easeInOutQuint(time));
		case EaseInExpo:
			return  this->interpolate(v1, v2, this->easeInExpo(time));
		case EaseOutExpo:
			return  this->interpolate(v1, v2, this->easeOutExpo(time));
		case EaseInOutExpo:
			return  this->interpolate(v1, v2, this->easeInOutExpo(time));
		case EaseInCirc:
			return  this->interpolate(v1, v2, this->easeInCirc(time));
		case EaseOutCirc:
			return  this->interpolate(v1, v2, this->easeOutCirc(time));
		case EaseInOutCirc:
			return  this->interpolate(v1, v2, this->easeInOutCirc(time));
		case EaseInBack:
			return  this->interpolate(v1, v2, this->easeInBack(time));
		case EaseOutBack:
			return  this->interpolate(v1, v2, this->easeOutBack(time));
		case EaseInOutBack:
			return  this->interpolate(v1, v2, this->easeInOutBack(time));
		case EaseInElastic:
			return  this->interpolate(v1, v2, this->easeInElastic(time));
		case EaseOutElastic:
			return  this->interpolate(v1, v2, this->easeOutElastic(time));
		case EaseInOutElastic:
			return  this->interpolate(v1, v2, this->easeInOutElastic(time));
		case EaseInBounce:
			return  this->interpolate(v1, v2, this->easeInBounce(time));
		case EaseOutBounce:
			return  this->interpolate(v1, v2, this->easeOutBounce(time));
		case EaseInOutBounce:
			return  this->interpolate(v1, v2, this->easeInOutBounce(time));
		}
		return  this->interpolate(v1, v2, time);
	}

	std::vector<Ogre::String> Interpolator::getAllEaseFunctionNames(void) const
	{
		return { "linear", "easeInSine", "easeOutSine", "easeInOutSine", "easeInQuad", "easeOutQuad", "easeInOutQuad", "easeInCubic",
				 "easeOutCubic", "easeInOutCubic", "easeInQuart", "easeOutQuart", "easeInOutQuart", "easeInQuint", "easeOutQuint", "easeInOutQuint", 
			     "easeInExpo", "easeOutExpo", "easeInOutExpo", "easeInCirc", "easeOutCirc", "easeInOutCirc", "easeInBack", "easeOutBack", 
			     "easeInOutBack", "easeInElastic", "easeOutElastic", "easeInOutElastic", "easeOutBounce", "easeInBounce", "easeInOutBounce" };
	}

	Ogre::Real Interpolator::calculateBezierYT(Ogre::Real t, bool invert)
	{
		Ogre::Vector2 p0 = Ogre::Vector2::ZERO;
		Ogre::Vector2 p1 = invert ? Ogre::Vector2(0.5f, 0.0f) : Ogre::Vector2(0.5f, -0.9f);
		Ogre::Vector2 p2 = invert ? Ogre::Vector2(0.5f, 0.1f) : Ogre::Vector2(0.5f, -1.0f);
		Ogre::Vector2 p3 = invert ? Ogre::Vector2(1.0f, 1.0f) : Ogre::Vector2(1.0f, -1.0f);

		Ogre::Real cy = 3.0f * (p1.y - p0.y);
		Ogre::Real by = 3.0f * (p2.y - p1.y) - cy;
		Ogre::Real ay = p3.y - p0.y - cy - by;

		Ogre::Real yt = ay * (t * t * t) + by * (t * t) + cy * t + p0.y;
		return invert ? yt : yt + 1;

	}

	Ogre::Real Interpolator::calcBezierYTPoints(Ogre::Real t, bool invert)
	{
		Ogre::Vector2 p0 = Ogre::Vector2::ZERO;
		Ogre::Vector2 p1 = invert ? Ogre::Vector2(0.0f, 0.9f) : Ogre::Vector2(1.0f, -0.0f);
		Ogre::Vector2 p2 = invert ? Ogre::Vector2(0.5f, 0.1f) : Ogre::Vector2(0.5f, -0.1f);
		Ogre::Vector2 p3 = invert ? Ogre::Vector2(1.0f, 1.0f) : Ogre::Vector2(1.0f, -1.0f);
		Ogre::Real cy = 3.0f * (p1.y - p0.y);
		Ogre::Real by = 3.0f * (p2.y - p1.y) - cy;
		Ogre::Real ay = p3.y - p0.y - cy - by;
		Ogre::Real yt = ay * (t * t * t) + by * (t * t) + cy * t + p0.y;
		return invert ? yt : yt + 1;
	}

	Ogre::Real Interpolator::easeInSine(Ogre::Real t)
	{
		return -1.0f * Ogre::Math::Cos(t * (Ogre::Math::PI / 2.0f)) + 1.0f;
	}

	Ogre::Real Interpolator::easeOutSine(Ogre::Real t)
	{
		return Ogre::Math::Sin(t * (Ogre::Math::PI / 2.0f));
	}

	Ogre::Real Interpolator::easeInOutSine(Ogre::Real t)
	{
		return -0.5f * (Ogre::Math::Cos(Ogre::Math::PI * t) - 1.0f);
	}

	Ogre::Real Interpolator::easeInQuad(Ogre::Real t)
	{
		return t * t;
	}

	Ogre::Real Interpolator::easeOutQuad(Ogre::Real t)
	{
		return t * (2.0f - t);
	}

	Ogre::Real Interpolator::easeInOutQuad(Ogre::Real t)
	{
		return t < 0.5f ? 2.0f * t * t : -1.0f + (4 - 2.0f * t) * t;
	}

	Ogre::Real Interpolator::easeInCubic(Ogre::Real t)
	{
		return t * t * t;
	}

	Ogre::Real Interpolator::easeOutCubic(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return t1 * t1 * t1 + 1.0f;
	}

	Ogre::Real Interpolator::easeInOutCubic(Ogre::Real t)
	{
		return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
	}

	Ogre::Real Interpolator::easeInQuart(Ogre::Real t)
	{
		return t * t * t * t;
	}

	Ogre::Real Interpolator::easeOutQuart(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return 1.0f - t1 * t1 * t1 * t1;
	}

	Ogre::Real Interpolator::easeInOutQuart(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * t1 * t1 * t1 * t1;
	}

	Ogre::Real Interpolator::easeInQuint(Ogre::Real t)
	{
		return t * t * t * t * t;
	}

	Ogre::Real Interpolator::easeOutQuint(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return 1.0f + t1 * t1 * t1 * t1 * t1;
	}

	Ogre::Real Interpolator::easeInOutQuint(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return t < 0.5f ? 16.0f * t * t * t * t * t : 1 + 16 * t1 * t1 * t1 * t1 * t1;
	}

	Ogre::Real Interpolator::easeInExpo(Ogre::Real t)
	{
		if (t == 0.0f)
		{
			return 0.0f;
		}
		return Ogre::Math::Pow(2.0f, 10.0f * (t - 1.0f));
	}

	Ogre::Real Interpolator::easeOutExpo(Ogre::Real t)
	{
		if (t == 1.0f)
		{
			return 1.0f;
		}
		return (-Ogre::Math::Pow(2.0f, -10.0f * t) + 1.0f);
	}

	Ogre::Real Interpolator::easeInOutExpo(Ogre::Real t)
	{
		if (t == 0.0f || t == 1.0f)
		{
			return t;
		}

		const Ogre::Real scaledTime = t * 2.0f;
		const Ogre::Real scaledTime1 = scaledTime - 1.0f;

		if (scaledTime < 1.0f)
		{
			return 0.5f * Ogre::Math::Pow(2.0f, 10.0 * (scaledTime1));
		}
		return 0.5f * (-Ogre::Math::Pow(2.0f, -10.0f * scaledTime1) + 2.0f);

	}

	Ogre::Real Interpolator::easeInCirc(Ogre::Real t)
	{
		const Ogre::Real scaledTime = t / 1.0f;
		return -1.0f * (Ogre::Math::Sqrt(1.0f - scaledTime * t) - 1.0f);
	}

	Ogre::Real Interpolator::easeOutCirc(Ogre::Real t)
	{
		const Ogre::Real t1 = t - 1.0f;
		return Ogre::Math::Sqrt(1.0f - t1 * t1);
	}

	Ogre::Real Interpolator::easeInOutCirc(Ogre::Real t)
	{
		const Ogre::Real scaledTime = t * 2.0f;
		const Ogre::Real scaledTime1 = scaledTime - 2;

		if (scaledTime < 1.0f)
		{
			return -0.5f * (Ogre::Math::Sqrt(1.0f - scaledTime * scaledTime) - 1.0f);
		}

		return 0.5f * (Ogre::Math::Sqrt(1.0f - scaledTime1 * scaledTime1) + 1.0f);
	}

	Ogre::Real Interpolator::easeInBack(Ogre::Real t, Ogre::Real magnitude)
	{
		return t * t * ((magnitude + 1.0f) * t - magnitude);
	}

	Ogre::Real Interpolator::easeOutBack(Ogre::Real t, Ogre::Real magnitude)
	{
		const Ogre::Real scaledTime = (t / 1.0f) - 1.0f;

		return (scaledTime * scaledTime * ((magnitude + 1.0f) * scaledTime + magnitude)) + 1.0f;
	}

	Ogre::Real Interpolator::easeInOutBack(Ogre::Real t, Ogre::Real magnitude)
	{
		const Ogre::Real scaledTime = t * 2.0f;
		const Ogre::Real scaledTime2 = scaledTime - 2.0f;
		const Ogre::Real s = magnitude * 1.525f;

		if (scaledTime < 1.0f)
		{
			return 0.5f * scaledTime * scaledTime * (((s + 1.0f) * scaledTime) - s);
		}

		return 0.5f * (scaledTime2 * scaledTime2 * ((s + 1.0f) * scaledTime2 + s) + 2.0f);
	}

	Ogre::Real Interpolator::easeInElastic(Ogre::Real t, Ogre::Real magnitude)
	{
		if (t == 0.0f || t == 1.0f)
		{
			return t;
		}

		const Ogre::Real scaledTime = t / 1.0f;
		const Ogre::Real scaledTime1 = scaledTime - 1.0f;

		const Ogre::Real p = 1.0f - magnitude;
		const Ogre::Real s = p / ((2.0f * Ogre::Math::PI) * Ogre::Math::ASin(1.0f)).valueRadians();

		return -(Ogre::Math::Pow(2.0f, 10.0f * scaledTime1) * Ogre::Math::Sin((scaledTime1 - s) * (2.0f * Ogre::Math::PI) / p));
	}

	Ogre::Real Interpolator::easeOutElastic(Ogre::Real t, Ogre::Real magnitude)
	{
		const Ogre::Real p = 1.0f - magnitude;
		const Ogre::Real scaledTime = t * 2.0f;

		if (t == 0.0f || t == 1.0f)
		{
			return t;
		}

		const Ogre::Real s = p / ((2.0f * Ogre::Math::PI) * Ogre::Math::ASin(1.0f)).valueRadians();
		return (Ogre::Math::Pow(2.0f, -10.0f * scaledTime) * Ogre::Math::Sin((scaledTime - s) * (2.0f * Ogre::Math::PI) / p)) + 1.0f;
	}

	Ogre::Real Interpolator::easeInOutElastic(Ogre::Real t, Ogre::Real magnitude)
	{
		const Ogre::Real p = 1.0f - magnitude;

		if (t == 0.0f || t == 1.0f)
		{
			return t;
		}

		const Ogre::Real scaledTime = t * 2.0f;
		const Ogre::Real scaledTime1 = scaledTime - 1.0f;
		const Ogre::Real s = p / ((2.0f * Ogre::Math::PI) * Ogre::Math::ASin(1.0f)).valueRadians();

		if (scaledTime < 1.0f)
		{
			return -0.5f * (Ogre::Math::Pow(2.0f, 10.0f * scaledTime1) * Ogre::Math::Sin((scaledTime1 - s) * (2.0f * Ogre::Math::PI) / p));
		}

		return (Ogre::Math::Pow(2.0f, -10.0f * scaledTime1) * Ogre::Math::Sin((scaledTime1 - s) * (2.0f * Ogre::Math::PI) / p) * 0.5f) + 1.0f;
	}

	Ogre::Real Interpolator::easeOutBounce(Ogre::Real t)
	{
		const Ogre::Real scaledTime = t / 1.0f;

		if (scaledTime < (1.0f / 2.75f))
		{
			return 7.5625f * scaledTime * scaledTime;
		}
		else if (scaledTime < (2.0f / 2.75f))
		{
			const Ogre::Real scaledTime2 = scaledTime - (1.5f / 2.75f);
			return (7.5625f * scaledTime2 * scaledTime2) + 0.75f;
		}
		else if (scaledTime < (2.5f / 2.75f))
		{
			const Ogre::Real scaledTime2 = scaledTime - (2.25f / 2.75f);
			return (7.5625f * scaledTime2 * scaledTime2) + 0.9375f;
		}
		else
		{
			const Ogre::Real scaledTime2 = scaledTime - (2.625f / 2.75f);
			return (7.5625f * scaledTime2 * scaledTime2) + 0.984375f;
		}
	}

	Ogre::Real Interpolator::easeInBounce(Ogre::Real t)
	{
		return 1.0f - easeOutBounce(1.0f - t);
	}

	Ogre::Real Interpolator::easeInOutBounce(Ogre::Real t)
	{
		if (t < 0.5f)
		{
			return easeInBounce(t * 2.0f) * 0.5f;
		}
		return (easeOutBounce((t * 2.0f) - 1.0f) * 0.5f) + 0.5f;
	}

}; // namespace end