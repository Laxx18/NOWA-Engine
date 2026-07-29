/*!
	@file
	@author     Pavel Turin
	@date       08/2009
*/

#include "Precompiled.h"
#include "TreeControl.h"
#include "TreeControlItem.h"

namespace MyGUI
{
	TreeControl::Node::Node(TreeControl* pOwner) :
		GenericNode<Node, TreeControl>(pOwner),
		mbIsPrepared(false),
		mbIsExpanded(true),
		mstrImage("Folder")
	{}

	TreeControl::Node::Node(const UString& strText, Node* pParent) :
		GenericNode<Node, TreeControl>(strText, pParent),
		mbIsPrepared(false),
		mbIsExpanded(false),
		mstrImage("Folder")
	{}

	TreeControl::Node::Node(const UString& strText, const UString& strImage, Node* pParent) :
		GenericNode<Node, TreeControl>(strText, pParent),
		mbIsPrepared(false),
		mbIsExpanded(false),
		mstrImage(strImage)
	{}

	TreeControl::Node::~Node()
	{
		// Make sure this node can never be left dangling inside the owner's
		// selection set (single- or multi-select) once it is destroyed.
		if (nullptr != mpOwner)
		{
			mpOwner->removeFromSelectionSilent(this);
		}
	}

	void TreeControl::Node::prepare()
	{
		if (mbIsPrepared || !mpOwner)
			return;

		mpOwner->eventTreeNodePrepare(mpOwner, this);
		mbIsPrepared = true;
	}

	size_t TreeControl::Node::prepareChildren()
	{
		prepare();

		size_t nResult = 0;
		for (VectorNodePtr::iterator Iterator = getChildren().begin(); Iterator != getChildren().end(); ++Iterator)
		{
			TreeControl::Node* pChild = *Iterator;

			nResult++;

			pChild->prepare();
			if (pChild->isExpanded())
				nResult += pChild->prepareChildren();
		}

		return nResult;
	}

	void TreeControl::Node::setExpanded(bool bIsExpanded)
	{
		if (mbIsExpanded == bIsExpanded)
			return;

		mbIsExpanded = bIsExpanded;

		invalidate();
	}

	void TreeControl::Node::setImage(const UString& strImage)
	{
		mstrImage = strImage;

		invalidate();
	}

	TreeControl::TreeControl() :
		mpWidgetScroll(nullptr),
		mbScrollAlwaysVisible(true),
		mbInvalidated(false),
		mbRootVisible(false),
		mbMultiSelectMode(false),
		mnItemHeight(1),
		mnScrollRange(-1),
		mnTopIndex(0),
		mnTopOffset(0),
		mnFocusIndex(ITEM_NONE),
		mpRoot(nullptr),
		mnExpandedNodes(0),
		mnLevelOffset(0),
		mClient(nullptr)
	{}

	void TreeControl::initialiseOverride()
	{
		Base::initialiseOverride();

		// FIXME: this is not localized, should be revisited at some point
		mpRoot = new Node(this);

		//FIXME
		setNeedKeyFocus(true);

		assignWidget(mpWidgetScroll, "VScroll");
		if (mpWidgetScroll != nullptr)
		{
			mpWidgetScroll->eventScrollChangePosition += newDelegate(this, &TreeControl::notifyScrollChangePosition);
			mpWidgetScroll->eventMouseButtonPressed += newDelegate(this, &TreeControl::notifyMousePressed);
		}

		assignWidget(mClient, "Client");
		if (mClient != nullptr)
		{
			mClient->eventMouseButtonPressed += newDelegate(this, &TreeControl::notifyMousePressed);
			setWidgetClient(mClient);
		}

		MYGUI_ASSERT(nullptr != mpWidgetScroll, "Child VScroll not found in skin (TreeControl must have VScroll)");
		MYGUI_ASSERT(nullptr != mClient, "Child Widget Client not found in skin (TreeControl must have Client)");

		if (isUserString("SkinLine"))
			mstrSkinLine = getUserString("SkinLine");
		if (isUserString("HeightLine"))
			mnItemHeight = utility::parseValue<int>(getUserString("HeightLine"));
		if (isUserString("LevelOffset"))
			mnLevelOffset = utility::parseValue<int>(getUserString("LevelOffset"));

		MYGUI_ASSERT(!mstrSkinLine.empty(), "SkinLine property not found (TreeControl must have SkinLine property)");

		if (mnItemHeight < 1)
			mnItemHeight = 1;

		mpWidgetScroll->setScrollPage((size_t)mnItemHeight);
		mpWidgetScroll->setScrollViewPage((size_t)mnItemHeight);

		invalidate();
	}

	void TreeControl::shutdownOverride()
	{
		mpWidgetScroll = nullptr;
		mClient = nullptr;
		// FIXME: this is not localized, should be revisited at some point
		delete mpRoot;

		Base::shutdownOverride();
	}

	void TreeControl::setRootVisible(bool bValue)
	{
		if (mbRootVisible == bValue)
			return;

		mbRootVisible = bValue;
		invalidate();
	}

	void TreeControl::setMultiSelectMode(bool bValue)
	{
		mbMultiSelectMode = bValue;

		// Switching back to single-select mode while more than one node is
		// selected: keep only the most recently selected node.
		if (false == mbMultiSelectMode && mSelectedNodes.size() > 1)
		{
			Node* pKeep = mSelectedNodes.back();
			clearSelection();
			addToSelection(pKeep);
		}
	}

	void TreeControl::setSelection(Node* pSelection)
	{
		clearSelection();

		if (nullptr != pSelection)
		{
			addToSelection(pSelection);
		}
	}

	void TreeControl::addToSelection(Node* pNode)
	{
		if (nullptr == pNode)
			return;

		// Already selected, nothing to do.
		for (VectorNodePtr::const_iterator it = mSelectedNodes.cbegin(); it != mSelectedNodes.cend(); ++it)
		{
			if (*it == pNode)
				return;
		}

		if (false == mbMultiSelectMode)
		{
			mSelectedNodes.clear();
		}

		mSelectedNodes.push_back(pNode);

		// Expand all ancestors so the newly selected node is visible.
		Node* pAncestor = pNode;
		while (nullptr != pAncestor)
		{
			pAncestor->setExpanded(true);
			pAncestor = pAncestor->getParent();
		}

		invalidate();
		eventTreeNodeSelected(this, pNode);
	}

	void TreeControl::removeFromSelection(Node* pNode)
	{
		if (nullptr == pNode)
			return;

		for (VectorNodePtr::iterator it = mSelectedNodes.begin(); it != mSelectedNodes.end(); ++it)
		{
			if (*it == pNode)
			{
				mSelectedNodes.erase(it);
				invalidate();
				eventTreeNodeSelected(this, mSelectedNodes.empty() ? nullptr : mSelectedNodes.back());
				return;
			}
		}
	}

	void TreeControl::toggleSelection(Node* pNode)
	{
		if (nullptr == pNode)
			return;

		for (VectorNodePtr::const_iterator it = mSelectedNodes.cbegin(); it != mSelectedNodes.cend(); ++it)
		{
			if (*it == pNode)
			{
				removeFromSelection(pNode);
				return;
			}
		}

		addToSelection(pNode);
	}

	void TreeControl::clearSelection(void)
	{
		mSelectedNodes.clear();

		// Reset the visual "selected" state of every currently visible item
		// immediately, instead of relying on validate() to get around to it
		// on some later frame. This avoids stale highlighted items whenever
		// the item->node mapping changes (scrolling, expand/collapse, rebuild)
		// between this call and the next validate() pass.
		for (size_t i = 0; i < mItemWidgets.size(); ++i)
		{
			mItemWidgets[i]->setStateSelected(false);
			mItemWidgets[i]->setColour(MyGUI::Colour::White);
		}

		invalidate();
	}

	void TreeControl::removeFromSelectionSilent(Node* pNode)
	{
		for (VectorNodePtr::iterator it = mSelectedNodes.begin(); it != mSelectedNodes.end(); ++it)
		{
			if (*it == pNode)
			{
				mSelectedNodes.erase(it);
				break;
			}
		}
	}

	void TreeControl::onMouseWheel(int nValue)
	{
		notifyMouseWheel(nullptr, nValue);

		Widget::onMouseWheel(nValue);
	}

	void TreeControl::onKeyButtonPressed(KeyCode Key, Char Character)
	{
		// TODO

		if (MyGUI::KeyCode::ArrowUp == Key)
		{
			Node* pCurrent = getSelection();
			for (size_t i = 0; i < mItemWidgets.size(); i++)
			{
				Node* node = mItemWidgets[i]->getNode();
				if (node && node == pCurrent)
				{
					if (i > 0)
					{
						this->setSelection(mItemWidgets[i - 1]->getNode());
					}
				}
			}
		}
		else if (MyGUI::KeyCode::ArrowDown == Key)
		{
			Node* pCurrent = getSelection();
			for (size_t i = 0; i < mItemWidgets.size(); i++)
			{
				Node* node = mItemWidgets[i]->getNode();
				if (node && node == pCurrent)
				{
					if (i < mItemWidgets.size() - 1)
					{
						try
						{
							this->setSelection(mItemWidgets[i + 1]->getNode());
						}
						catch (...)
						{

						}
					}
				}
			}
		}
		Widget::onKeyButtonPressed(Key, Character);
	}

	void TreeControl::setSize(const IntSize& Size)
	{
		Widget::setSize(Size);

		invalidate();
	}

	void TreeControl::setCoord(const IntCoord& Bounds)
	{
		Widget::setCoord(Bounds);

		invalidate();
	}

	void TreeControl::notifyFrameEntered(float nTime)
	{
		if (!mbInvalidated)
			return;

		mnExpandedNodes = mpRoot->prepareChildren();
		if (mbRootVisible)
			mnExpandedNodes++;

		updateScroll();
		updateItems();

		validate();

		mbInvalidated = false;
		Gui::getInstance().eventFrameStart -= newDelegate(this, &TreeControl::notifyFrameEntered);
	}

	void TreeControl::updateScroll()
	{
		mnScrollRange = (mnItemHeight * (int)mnExpandedNodes) - mClient->getHeight();

		if (!mbScrollAlwaysVisible || mnScrollRange <= 0 || mpWidgetScroll->getLeft() <= mClient->getLeft())
		{
			if (mpWidgetScroll->getVisible())
			{
				mpWidgetScroll->setVisible(false);
				mClient->setSize(mClient->getWidth() + mpWidgetScroll->getWidth(), mClient->getHeight());
			}
		}
		else if (!mpWidgetScroll->getVisible())
		{
			mClient->setSize(mClient->getWidth() - mpWidgetScroll->getWidth(), mClient->getHeight());
			mpWidgetScroll->setVisible(true);
		}

		mpWidgetScroll->setScrollRange(mnScrollRange + 1);

		if (mnExpandedNodes)
			mpWidgetScroll->setTrackSize(mpWidgetScroll->getLineSize() * mClient->getHeight() / mnItemHeight / mnExpandedNodes);
	}

	void TreeControl::updateItems()
	{
		int nPosition = mnTopIndex * mnItemHeight + mnTopOffset;

		int nHeight = (int)mItemWidgets.size() * mnItemHeight - mnTopOffset;
		while ((nHeight <= (mClient->getHeight() + mnItemHeight)) && mItemWidgets.size() < mnExpandedNodes)
		{
			TreeControlItem* pItem = mClient->createWidget<TreeControlItem>(
				mstrSkinLine,
				0,
				nHeight,
				mClient->getWidth(),
				mnItemHeight,
				Align::Top | Align::HStretch);

			pItem->eventMouseButtonPressed += newDelegate(this, &TreeControl::notifyMousePressed);
			pItem->eventMouseButtonDoubleClick += newDelegate(this, &TreeControl::notifyMouseDoubleClick);
			pItem->eventMouseWheel += newDelegate(this, &TreeControl::notifyMouseWheel);
			pItem->eventMouseSetFocus += newDelegate(this, &TreeControl::notifyMouseSetFocus);
			pItem->eventMouseLostFocus += newDelegate(this, &TreeControl::notifyMouseLostFocus);
			pItem->_setInternalData((size_t)mItemWidgets.size());
			pItem->getButtonExpandCollapse()->eventMouseButtonClick += newDelegate(this, &TreeControl::notifyExpandCollapse);

			mItemWidgets.push_back(pItem);

			nHeight += mnItemHeight;
		};

		if (nPosition >= mnScrollRange)
		{
			if (mnScrollRange <= 0)
			{
				if (nPosition || mnTopOffset || mnTopIndex)
				{
					nPosition = 0;
					mnTopIndex = 0;
					mnTopOffset = 0;
				}
			}
			else
			{
				int nCount = mClient->getHeight() / mnItemHeight;
				mnTopOffset = mnItemHeight - (mClient->getHeight() % mnItemHeight);

				if (mnTopOffset == mnItemHeight)
				{
					mnTopOffset = 0;
					nCount--;
				}

				mnTopIndex = ((int)mnExpandedNodes) - nCount - 1;
				nPosition = mnTopIndex * mnItemHeight + mnTopOffset;
			}
		}

		mpWidgetScroll->setScrollPosition(nPosition);
	}

	void TreeControl::validate()
	{
		typedef std::pair<VectorNodePtr::iterator, VectorNodePtr::iterator> PairNodeEnumeration;
		typedef std::list<PairNodeEnumeration> ListNodeEnumeration;
		ListNodeEnumeration EnumerationStack;
		PairNodeEnumeration Enumeration;
		VectorNodePtr vectorNodePtr;
		if (mbRootVisible)
		{
			vectorNodePtr.push_back(mpRoot);
			Enumeration = PairNodeEnumeration(vectorNodePtr.begin(), vectorNodePtr.end());
		}
		else
			Enumeration = PairNodeEnumeration(mpRoot->getChildren().begin(), mpRoot->getChildren().end());

		size_t nLevel = 0;
		size_t nIndex = 0;
		size_t nItem = 0;
		int nOffset = 0 - mnTopOffset;

		while (true)
		{
			if (Enumeration.first == Enumeration.second)
			{
				if (EnumerationStack.empty())
					break;

				Enumeration = EnumerationStack.back();
				EnumerationStack.pop_back();
				nLevel--;
				continue;
			}

			Node* pNode = *Enumeration.first;
			Enumeration.first++;

			if (nIndex >= (size_t)mnTopIndex)
			{
				// Use nOffset (computed fresh this frame from nLevel/mnItemHeight) instead
				// of mItemWidgets[nItem]->getTop(), which reflects the position the widget
				// had at the END of the PREVIOUS validate() pass. Reading a stale getTop()
				// here could trigger an early break whenever the visible structure changed
				// between frames (scrolling, recycling, rapid selection changes), silently
				// skipping setStateSelected()/setUserData() for every remaining item in the
				// pool - leaving them visually stuck with whatever selection highlight they
				// had the last time they WERE reached. nOffset has no such staleness problem:
				// it is deterministically derived purely from this frame's loop iteration.
				if (nItem >= mItemWidgets.size())
					break;

				if (nIndex >= mnExpandedNodes || nOffset > mClient->getHeight())
					break;

				TreeControlItem* pItem = mItemWidgets[nItem];
				pItem->setVisible(true);
				pItem->setCaption(pNode->getText());
				pItem->setPosition(IntPoint(nLevel * mnLevelOffset, nOffset));

				// Recompute the selected state for this item from scratch every
				// frame, against the full selection set. This is what keeps
				// highlighting correct regardless of scrolling, recycling, or
				// tree structure changes between frames.
				bool bIsSelected = false;
				for (VectorNodePtr::const_iterator it = mSelectedNodes.cbegin(); it != mSelectedNodes.cend(); ++it)
				{
					if (*it == pNode)
					{
						bIsSelected = true;
						break;
					}
				}
				// Bulletproof, unconditional selection indicator. This completely
				// bypasses Button's internal normal/highlighted/pushed/_checked
				// state machine and its hover/press tracking (mIsMouseFocus,
				// mIsMousePressed) - none of which can affect a plain colour tint.
				// Forced unconditionally every frame, no early-return/caching path
				// can leave it stuck on a stale value.
				pItem->setStateSelected(bIsSelected);
				pItem->setColour(bIsSelected ? MyGUI::Colour(0.15f, 0.35f, 0.75f) : MyGUI::Colour::White);

				pItem->setUserData(pNode);

				Button* pButtonExpandCollapse = pItem->getButtonExpandCollapse();
				pButtonExpandCollapse->setVisible(pNode->hasChildren());
				pButtonExpandCollapse->setStateSelected(!pNode->isExpanded());

				ImageBox* pIcon = pItem->getIcon();
				if (pIcon)
				{
					ResourceImageSetPtr pIconResource = pIcon->getItemResource();
					if (pIconResource)
					{
						UString strIconType(pNode->isExpanded() ? "Expanded" : "Collapsed");
						ImageIndexInfo IconInfo = pIconResource->getIndexInfo(pNode->getImage(), strIconType);
						if (IconInfo.size.empty())
							pIcon->setItemResourceInfo(pIconResource->getIndexInfo(pNode->getImage(), "Common"));
						else
							pIcon->setItemResourceInfo(IconInfo);
					}
				}

				nOffset += mnItemHeight;
				nItem++;
			}

			nIndex++;

			if (pNode->hasChildren() && pNode->isExpanded())
			{
				EnumerationStack.push_back(Enumeration);
				Enumeration.first = pNode->getChildren().begin();
				Enumeration.second = pNode->getChildren().end();
				nLevel++;
			}
		}

		if (nItem < mItemWidgets.size())
		{
			for (; nItem < mItemWidgets.size(); ++nItem)
			{
				mItemWidgets[nItem]->setStateSelected(false);
				mItemWidgets[nItem]->setColour(MyGUI::Colour::White);
				mItemWidgets[nItem]->setVisible(false);
			}
		}
	}

	void TreeControl::invalidate()
	{
		if (mbInvalidated)
			return;

		Gui::getInstance().eventFrameStart += newDelegate(this, &TreeControl::notifyFrameEntered);
		mbInvalidated = true;
	}

	void TreeControl::scrollTo(size_t nPosition)
	{
		mnTopOffset = ((int)nPosition % mnItemHeight);
		mnTopIndex = ((int)nPosition / mnItemHeight);

		invalidate();
	}

	void TreeControl::sendScrollingEvents(size_t nPosition)
	{
		eventTreeScrolled(this, nPosition);
		if (mnFocusIndex != ITEM_NONE)
			eventTreeNodeMouseSetFocus(this, mItemWidgets[mnFocusIndex]->getNode());
	}

	void TreeControl::notifyMousePressed(Widget* pSender, int nLeft, int nTop, MouseButton nID)
	{
		if ((nID == MouseButton::Left || nID == MouseButton::Right) && pSender != mpWidgetScroll)
		{
			Node* pNode = nullptr;
			if (pSender != mClient && pSender->getVisible())
				pNode = *pSender->getUserData<Node*>();

			if (nID == MouseButton::Left)
			{
				// Multi-select (accumulating/toggling) only kicks in while Shift is
				// held AND multi-select mode is enabled. A plain click - even in
				// multi-select mode - must always replace the whole selection with
				// just the clicked node (or clear it, if empty space was clicked),
				// exactly like classic single-selection. Without this check every
				// click went through toggleSelection() regardless of Shift, so
				// selections only ever accumulated and nothing was ever replaced.
				bool bShiftMultiSelect = true == mbMultiSelectMode && InputManager::getInstance().isShiftPressed();

				if (true == bShiftMultiSelect)
				{
					// Shift-click on empty space: leave the current selection as is.
					// Shift-click on an item: toggle just that item in/out.
					if (nullptr != pNode)
						toggleSelection(pNode);
				}
				else
				{
					setSelection(pNode);
				}
			}
			else // MouseButton::Right
			{
				// Right-click on an item that is already part of the current
				// selection should open the context menu without changing the
				// selection; otherwise it replaces the selection with just
				// this node (classic single-click-to-select behaviour).
				bool bAlreadySelected = false;
				for (VectorNodePtr::const_iterator it = mSelectedNodes.cbegin(); it != mSelectedNodes.cend(); ++it)
				{
					if (*it == pNode)
					{
						bAlreadySelected = true;
						break;
					}
				}

				if (false == bAlreadySelected)
					setSelection(pNode);

				eventTreeNodeContextMenu(this, pNode);
			}
		}
	}

	void TreeControl::notifyMouseWheel(Widget* pSender, int nValue)
	{
		if (mnScrollRange <= 0)
			return;

		int nPosition = (int)mpWidgetScroll->getScrollPosition();
		if (nValue < 0)
			nPosition += mnItemHeight;
		else
			nPosition -= mnItemHeight;

		if (nPosition >= mnScrollRange)
			nPosition = mnScrollRange;
		else if (nPosition < 0)
			nPosition = 0;

		if ((int)mpWidgetScroll->getScrollPosition() == nPosition)
			return;

		mpWidgetScroll->setScrollPosition(nPosition);

		scrollTo(nPosition);
		sendScrollingEvents(nPosition);
	}

	void TreeControl::notifyMouseDoubleClick(Widget* pSender)
	{
		Node* pSelection = getSelection();
		if (nullptr != pSelection)
			eventTreeNodeActivated(this, pSelection);
	}

	void TreeControl::notifyMouseSetFocus(Widget* pSender, Widget* pPreviousWidget)
	{
		mnFocusIndex = *pSender->_getInternalData<size_t>();
		eventTreeNodeMouseSetFocus(this, mItemWidgets[mnFocusIndex]->getNode());
	}

	void TreeControl::notifyMouseLostFocus(Widget* pSender, Widget* pNextWidget)
	{
		if (!pNextWidget || (pNextWidget->getParent() != mClient))
		{
			mnFocusIndex = ITEM_NONE;
			eventTreeNodeMouseLostFocus(this, nullptr);
		}
	}

	void TreeControl::notifyScrollChangePosition(ScrollBar* pSender, size_t nPosition)
	{
		scrollTo(nPosition);
		sendScrollingEvents(nPosition);
	}

	void TreeControl::notifyExpandCollapse(Widget* pSender)
	{
		TreeControlItem* pItem = pSender->getParent()->castType<TreeControlItem>(false);
		if (!pItem)
			return;

		Node* pNode = pItem->getNode();
		pNode->setExpanded(!pNode->isExpanded());

		if (false == pNode->isExpanded())
		{
			// Collapsing this node hides all of its descendants, so any
			// selected descendant must be dropped from the selection set,
			// otherwise it would keep contributing a (now invisible) selected
			// state and getSelection()/eventTreeNodeSelected would still point
			// to a node the user can no longer see.
			bool bSelectionChanged = false;
			for (VectorNodePtr::iterator it = mSelectedNodes.begin(); it != mSelectedNodes.end();)
			{
				if ((*it)->hasAncestor(pNode))
				{
					it = mSelectedNodes.erase(it);
					bSelectionChanged = true;
				}
				else
					++it;
			}

			if (bSelectionChanged)
				eventTreeNodeSelected(this, mSelectedNodes.empty() ? nullptr : mSelectedNodes.back());
		}

		invalidate();
	}
}