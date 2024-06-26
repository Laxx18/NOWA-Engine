/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef MYGUI_SUB_WIDGET_MANAGER_H_
#define MYGUI_SUB_WIDGET_MANAGER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Singleton.h"

namespace MyGUI
{

	class MYGUI_EXPORT SubWidgetManager :
		public Singleton<SubWidgetManager>
	{
	public:
		SubWidgetManager();

		void initialise();
		void shutdown();

		const std::string& getCategoryName() const;
		const std::string& getStateCategoryName() const;

	private:
		bool mIsInitialise;
		std::string mCategoryName;
		std::string mStateCategoryName;
	};

} // namespace MyGUI

#endif // MYGUI_SUB_WIDGET_MANAGER_H_
