#ifndef AUTO_COMPLETE_SEARCH_H
#define AUTO_COMPLETE_SEARCH_H

#include "defines.h"

namespace NOWA
{
	/**
	* @class	AutoCompleteSearch
	* @brief	Search mechanism that is able to find parts in a list that do match a search string and results can be listed.
	*/
	class EXPORTED AutoCompleteSearch
	{
	public:
		/**
		* @class	MatchedItem
		* @brief	The matched item with the start character index at which the item text does match the search string
		*/
		class EXPORTED MatchedItem
		{
			friend class AutoCompleteSearch;
		public:
			MatchedItem();

			void reset(void);

			Ogre::String getMatchedItemText(void) const;

			int getMatchedCharStart(void) const;

			Ogre::String getUserData(void) const;
		private:
			Ogre::String matchedItemText;
			int matchedCharStart;
			Ogre::String userData;
		};
	public:
		/**
		* @class    MatchedItemProps
		* @brief    The structure for the list with matched items, index at which the first item has been matched
		*            in the combo box drop list, the selection start at which the part of the string has matched and
		*            the selection end of the best matched item.
		*/
		class EXPORTED MatchedItems
		{
			friend class AutoCompleteSearch;
		public:
			MatchedItems();

			void reset(void);

			std::vector<MatchedItem>& getResults(void);
		private:
			std::vector<MatchedItem> items;
			bool matchedWithin;
		};
	
	public:
		AutoCompleteSearch();

		void setSearchList(const std::set<std::pair<Ogre::String, Ogre::String>>& searchList);

		void addSearchText(const Ogre::String& searchText, const Ogre::String& userData = Ogre::String());

		void reset(void);

		/**
		* @brief        Performs a search within the text and creates a list with matched items.
		* @param[in]    searchText			The text to search for
		* @return       MatchedItemProps    The structure including a list to get all necessary infos of the matched items.
		*/
		MatchedItems findMatchedItemWithInText(const Ogre::String& searchText);

	private:
		/**
		* @brief        Searches for a part of a string. This function is used in findMatchedItemWithInText.
		* @param[in]    searchStringPart    The search string part
		* @param[in]    currentStringInList    The current string in list to check if the part matches.
		* @return        foundIdx            The selected start index at which the part matches
		* @note                                If nothing matches npos (size_t max value) will be returned.
		*/
		size_t lineInString(Ogre::String searchStringPart, Ogre::String currentStringInList);
	private:
		std::set<std::pair<Ogre::String, Ogre::String>> searchList;
		MatchedItems matchedItems;
	};

};  // namespace end

#endif