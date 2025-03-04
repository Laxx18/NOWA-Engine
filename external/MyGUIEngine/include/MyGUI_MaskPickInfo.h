/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_MASK_PICK_INFO_H_
#define MYGUI_MASK_PICK_INFO_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

	class MYGUI_EXPORT MaskPickInfo
	{
	public:
		MaskPickInfo();

		bool load(const std::string& _file);

		bool pick(const IntPoint& _point, const IntCoord& _coord) const;

		bool empty() const;

	private:
		std::vector<uint8> mData;
		int mWidth;
		int mHeight;
	};

} // namespace MyGUI

#endif // MYGUI_MASK_PICK_INFO_H_
