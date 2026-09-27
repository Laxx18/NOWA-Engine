/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_COORD_CONVERTER_H_
#define MYGUI_COORD_CONVERTER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

#include <cmath>

namespace MyGUI
{

	class MYGUI_EXPORT CoordConverter
	{
	public:
		/** Convert pixel coordinates to texture UV coordinates */
		static FloatRect convertTextureCoord(const IntCoord& _coord, const IntSize& _textureSize)
		{
			if (!_textureSize.width || !_textureSize.height) return FloatRect();
			return FloatRect(
				(float)_coord.left / (float)_textureSize.width,
				(float)_coord.top / (float)_textureSize.height,
				(float)_coord.right() / (float)_textureSize.width,
				(float)_coord.bottom() / (float)_textureSize.height);
		}

		/* Convert one relative value to a pixel value.

			Attention: This used to be a plain "int(...)" cast everywhere below, which truncates
			towards zero and therefore loses up to one pixel on EVERY conversion. That is why a
			widget configured with position 0.9 and size 0.1 never reached the bottom edge:

				position 0.9 * 1032 = 928.8 -> int -> 928
				size     0.1 * 1032 = 103.2 -> int -> 103
				928 + 103 = 1031, while the view is 1032 pixels high

			The error is one pixel per conversion, and it accumulates with nesting: the window
			loses one pixel, the client area inside its skin loses another, and a relatively
			sized child inside that loses a third. It is always the right and the bottom edge,
			never left or top, because truncation only ever moves values towards zero.

			Rounding to the nearest pixel removes the systematic bias. std::floor(v + 0.5f) is
			used instead of std::round() so that negative coordinates (widgets scrolled or
			placed outside their parent) round consistently in the same direction.
		*/
		static int convertFromRelative(float _value, int _view)
		{
			return static_cast<int>(std::floor(_value * static_cast<float>(_view) + 0.5f));
		}

		/* Convert from relative to pixel coordinates.
			@param _coord relative coordinates.
		*/
		static IntCoord convertFromRelative(const FloatCoord& _coord, const IntSize& _view)
		{
			return IntCoord(convertFromRelative(_coord.left, _view.width), convertFromRelative(_coord.top, _view.height), convertFromRelative(_coord.width, _view.width), convertFromRelative(_coord.height, _view.height));
		}

		/* Convert from relative to pixel coordinates.
			@param _coord relative coordinates.
		*/
		static IntSize convertFromRelative(const FloatSize& _size, const IntSize& _view)
		{
			return IntSize(convertFromRelative(_size.width, _view.width), convertFromRelative(_size.height, _view.height));
		}

		/* Convert from relative to pixel coordinates.
			@param _coord relative coordinates.
		*/
		static IntPoint convertFromRelative(const FloatPoint& _point, const IntSize& _view)
		{
			return IntPoint(convertFromRelative(_point.left, _view.width), convertFromRelative(_point.top, _view.height));
		}

		/* Convert from pixel to relative coordinates.
			@param _coord pixel coordinates.
		*/
		static FloatCoord convertToRelative(const IntCoord& _coord, const IntSize& _view)
		{
			return FloatCoord(_coord.left / (float)_view.width, _coord.top / (float)_view.height, _coord.width / (float)_view.width, _coord.height / (float)_view.height);
		}

		static FloatSize convertToRelative(const IntSize& _size, const IntSize& _view)
		{
			return FloatSize(_size.width / (float)_view.width, _size.height / (float)_view.height);
		}

		static FloatPoint convertToRelative(const IntPoint& _point, const IntSize& _view)
		{
			return FloatPoint(_point.left / (float)_view.width, _point.top / (float)_view.height);
		}
	};

} // namespace MyGUI

#endif // MYGUI_COORD_CONVERTER_H_