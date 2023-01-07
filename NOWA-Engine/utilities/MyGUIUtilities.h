#ifndef MYGUI_UTILITIES_H
#define MYGUI_UTILITIES_H

#include "defines.h"

#include "MyGUI.h"

namespace NOWA
{

	class EXPORTED MyGUIUtilities
	{
	public:
		/**
		* @brief		Sets the font size for the given widget.
		* @param[in]	textBox			The textBox to set the font size for
		* @param[in]	fontSize		The font size to set. Minimum value is 10 and maximum value is 60. Odd font sizes will be rounded to the next even font size. E.g. 23 will become 24.
		* return		actualFontSize	Returns the actual font size, that has maybe been adapted due to border constraints or odd to even rounding.
		* @note			Internally the correct font name is chosen, that matches the font size in order to prevent aliasing rendering artifacts.
		*/
		template <class T>
		unsigned int setFontSize(T* textBox, unsigned int fontSize)
		{
			if (nullptr == textBox)
				return fontSize;

			// Allow only even numbers like 20, 22, hence 23 e.g. will become 24
			if (fontSize % 2 != 0)
			{
				fontSize += 1;
			}
			if (fontSize > 60)
			{
				fontSize = 60;
			}
			else if (fontSize < 10)
			{
				fontSize = 10;
			}

			Ogre::String fontName = "NOWA_" + Ogre::StringConverter::toString(fontSize) + "_Font";
			// Choses the correct font
			textBox->setFontName(fontName);
			// Does mess up with rendering: because the font is already used as font size, and additionally adding font size to font size is terrible.
			// textBox->setFontHeight(fontSize);

			return fontSize;
		}

	public:
		static MyGUIUtilities* getInstance();
	};

}; // namespace end


#endif