/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_SimpleText.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_LayerNode.h"
#include "MyGUI_FontManager.h"
#include "MyGUI_CommonStateInfo.h"
#include "MyGUI_RenderManager.h"

namespace MyGUI
{

	SimpleText::SimpleText() :
		EditText()
	{
		mIsAddCursorWidth = false;
	}

	SimpleText::~SimpleText()
	{
	}

	void SimpleText::setViewOffset(const IntPoint& _point)
	{
	}

	void SimpleText::doRender()
	{
		bool _update = mRenderItem->getCurrentUpdate();
		if (_update)
			mTextOutDate = true;

		if (nullptr == mFont)
			return;
		if (!mVisible || mEmptyView)
			return;

		if (mTextOutDate)
			updateRawData();

		const IntSize& size = mTextView.getViewSize();

		if (mTextAlign.isRight())
			mViewOffset.left = (- (mCoord.width - size.width)) + mTextOffset.left;
		else if (mTextAlign.isHCenter())
			mViewOffset.left = - ((mCoord.width - size.width) / 2) + mTextOffset.left;
		else
			mViewOffset.left = -mTextOffset.left;

		if (mTextAlign.isBottom())
			mViewOffset.top = (- (mCoord.height - size.height)) + mTextOffset.top;
		else if (mTextAlign.isVCenter())
			mViewOffset.top = - ((mCoord.height - size.height) / 2) + mTextOffset.top;
		else
			mViewOffset.top = -mTextOffset.top;

		Base::doRender();
	}

} // namespace MyGUI
