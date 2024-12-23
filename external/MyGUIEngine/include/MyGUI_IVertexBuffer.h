/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_I_VERTEX_BUFFER_H_
#define MYGUI_I_VERTEX_BUFFER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_VertexData.h"

namespace MyGUI
{

	class MYGUI_EXPORT IVertexBuffer
	{
	public:
		virtual ~IVertexBuffer() { }

		virtual void setVertexCount(size_t _value) = 0;
		virtual size_t getVertexCount() = 0;

		virtual Vertex* lock() = 0;
		virtual void unlock() = 0;
	};

} // namespace MyGUI

#endif // MYGUI_I_VERTEX_BUFFER_H_
