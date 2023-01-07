#include "NOWAPrecompiled.h"
#include "AutoCompleteSearch.h"

namespace NOWA
{
	AutoCompleteSearch::MatchedItem::MatchedItem()
		: matchedCharStart(-1)
	{
	}

	void AutoCompleteSearch::MatchedItem::reset(void)
	{
		this->matchedCharStart = -1;
		this->matchedItemText.clear();
	}

	Ogre::String AutoCompleteSearch::MatchedItem::getMatchedItemText(void) const
	{
		return this->matchedItemText;
	}

	int AutoCompleteSearch::MatchedItem::getMatchedCharStart(void) const
	{
		return this->matchedCharStart;
	}

	Ogre::String AutoCompleteSearch::MatchedItem::getUserData(void) const
	{
		return this->userData;
	}

	///////////////////////////////////////////////////////////

	AutoCompleteSearch::MatchedItems::MatchedItems()
		: matchedWithin(false)
	{

	}

	void AutoCompleteSearch::MatchedItems::reset(void)
	{
		this->items.clear();
		this->matchedWithin = false;
	}

	std::vector<AutoCompleteSearch::MatchedItem>& AutoCompleteSearch::MatchedItems::getResults(void)
	{
		return this->items;
	}

	///////////////////////////////////////////////////////////

	AutoCompleteSearch::AutoCompleteSearch()
	{

	}

	void AutoCompleteSearch::setSearchList(const std::set<std::pair<Ogre::String, Ogre::String>>& searchList)
	{
		this->searchList = searchList;
	}

	void AutoCompleteSearch::addSearchText(const Ogre::String& searchText, const Ogre::String& userData)
	{
		this->searchList.emplace(std::make_pair(searchText, userData));
	}

	void AutoCompleteSearch::reset(void)
	{
		this->searchList.clear();
	}

	AutoCompleteSearch::MatchedItems AutoCompleteSearch::findMatchedItemWithInText(const Ogre::String& searchText)
	{
		this->matchedItems.reset();
		int matchedCharStart = -1;

		// Text empty, get the whole list
		if (true == searchText.empty()/* || searchText.size() < 3*/)
		{
			for (auto& currentText : this->searchList)
			{
				MatchedItem item;
				item.matchedItemText = currentText.first;
				item.userData = currentText.second;
				item.matchedCharStart = 0;
				this->matchedItems.items.emplace_back(item);
			}
			return this->matchedItems;
		}

		Ogre::String tempSearchText = searchText;
		Ogre::StringUtil::toUpperCase(tempSearchText);

		// Worst case of the whole algorithm O(GetCount() * len(currentStringInListHigh)) = O(n*n)
		for (auto currentText : this->searchList)
		{
			Ogre::String tempCurrentText = currentText.first;
			Ogre::StringUtil::toUpperCase(tempCurrentText);
			
			// if an exact match, we can use this even though it may not be
			// unique. I.e. with 273 and 273A, we should select the 273 as
			// a unique match
			if (tempSearchText == tempCurrentText)
			{
				MatchedItem item;
				item.matchedItemText = currentText.first;
				item.userData = currentText.second;
				item.matchedCharStart = 0;
				// append the item
				this->matchedItems.items.emplace_back(item);
				// if the search string is just one char and has not matched, go directly to the next string in list
			}
			else
			{
				// if the string contains the prefix/postfix in the line
				matchedCharStart = static_cast<int>(this->lineInString(searchText, currentText.first));
				if (matchedCharStart != Ogre::String::npos)
				{
					MatchedItem item;
					item.matchedItemText = currentText.first;
					item.userData = currentText.second;
					item.matchedCharStart = matchedCharStart;
					this->matchedItems.items.emplace_back(item);
					this->matchedItems.matchedWithin = true; // Indicated that word has been detected within a string
				}
			}
		}
		return this->matchedItems;
	}

	size_t AutoCompleteSearch::lineInString(Ogre::String searchStringPart, Ogre::String currentStringInList)
	{
		size_t foundIdx = Ogre::String::npos;
		size_t strLen = currentStringInList.size();
		size_t strPartLen = searchStringPart.size();
		// check if the part has even a chance to match
		if ((strLen > 0) && (strPartLen > 0) && (strPartLen < strLen))
		{
			// CString currentStringInListHigh = currentStringInList;
			// CString searchStringPartHigh = searchStringPart;
			// perform a case independent search within the string to get the index position at which position it starts to match
			// plus the length of the part to get the end of the match for selection
			// worst case O(len(currentStringInListHigh))
			Ogre::StringUtil::toUpperCase(currentStringInList);
			Ogre::StringUtil::toUpperCase(searchStringPart);
			
			foundIdx = currentStringInList.find(searchStringPart);
		}
		return foundIdx;
	}

}; // namespace end