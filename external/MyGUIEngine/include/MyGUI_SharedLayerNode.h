/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_SHARED_LAYER_NODE_H_
#define MYGUI_SHARED_LAYER_NODE_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_LayerNode.h"

namespace MyGUI
{

	class MYGUI_EXPORT SharedLayerNode :
		public LayerNode
	{
		MYGUI_RTTI_DERIVED( SharedLayerNode )

	public:
		explicit SharedLayerNode(ILayer* _layer, ILayerNode* _parent = nullptr);
		virtual ~SharedLayerNode();

		void addUsing();
		void removeUsing();
		size_t countUsing() const;

	private:
		size_t mCountUsing;
	};

} // namespace MyGUI

#endif // MYGUI_SHARED_LAYER_NODE_H_
