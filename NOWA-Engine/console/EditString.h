#ifndef EDIT_STRING_H
#define EDIT_STRING_H

#include "defines.h"
#include <string>

namespace NOWA
{
	class EXPORTED EditString
	{
	public:
		EditString();

		~EditString();

		EditString(std::string newText);

		void setText(std::string& newText);

		std::string& getText(void);

		void clear(void);

		bool inserting(void);

		// Process a key press.  Return true if it was used.
		bool injectKeyPress(const OIS::KeyEvent);

		void updateKeyPress(OIS::Keyboard* keyboard, float dt);

		// gets the current position in the text for cursor placement
		int getPosition(void);
	protected:
		// The text for editing
		std::string text;
		// Overwrite or insert
		bool insert;
		// Position for insert / overwrite
		std::string::iterator position;
		// Caret Position - for positioning the cursor.
		int caret;
		float timeSinceLastPress;
	};

}; // Namespace end

#endif