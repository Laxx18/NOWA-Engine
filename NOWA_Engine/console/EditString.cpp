#include "NOWAPrecompiled.h"
#include "EditString.h"

namespace NOWA
{
	EditString::EditString()
		: insert(true),
		position(text.begin()),
		caret(0),
		timeSinceLastPress(0.1f)
	{

	}

	EditString::~EditString()
	{

	}

	EditString::EditString(std::string newText)
	{
		this->setText(newText);
	}

	void EditString::setText(std::string& newText)
	{
		this->text = newText;
		this->position = text.end();
		this->caret = (int)text.length();
	}

	std::string& EditString::getText(void)
	{
		return this->text;
	}

	void EditString::clear(void)
	{
		this->text.clear();
		this->position = this->text.end();
		this->caret = 0;
	}

	bool EditString::inserting(void)
	{
		return this->insert;
	}

	int EditString::getPosition(void)
	{
		return this->caret;
	}

	bool EditString::injectKeyPress(const OIS::KeyEvent arg)
	{
		bool keyUsed = true;

		if (isgraph(arg.text) || isspace(arg.text))
		{
			if (this->insert || this->position == this->text.end())
			{
				this->position = this->text.insert(this->position, arg.text);
			}
			else
			{
				*this->position = arg.text;
			}
			this->position++;
			this->caret++;
		}
		else
		{
			switch (arg.key)
			{
			case OIS::KC_INSERT:
				this->insert = !this->insert;
				break;

			case OIS::KC_HOME:
				this->position = this->text.begin();
				this->caret = 0;
				break;

			case OIS::KC_END:
				this->position = this->text.end();
				this->caret = (int)this->text.length();
				break;

			default:
				keyUsed = false;
				break;
			}
		}

		return keyUsed;
	}

	void EditString::updateKeyPress(OIS::Keyboard* keyboard, float dt)
	{
		if (this->timeSinceLastPress >= 0.0f)
		{
			this->timeSinceLastPress -= dt;
		}
		else
		{
			if (keyboard->isKeyDown(OIS::KC_BACK))
			{
				if (this->position != this->text.begin())
				{
					this->position = this->text.erase(--this->position);
					--this->caret;
				}
			}
			else if (keyboard->isKeyDown(OIS::KC_LEFT))
			{
				if (this->position != this->text.begin())
				{
					this->position--;
					this->caret--;
				}
			}
			else if (keyboard->isKeyDown(OIS::KC_RIGHT))
			{
				if (this->position != this->text.end())
				{
					this->position++;
					this->caret++;
				}
			}
			else if (keyboard->isKeyDown(OIS::KC_DELETE))
			{
				if (this->position != this->text.end())
				{
					this->position = this->text.erase(this->position);
				}
			}
			this->timeSinceLastPress = 0.1f;
		}
	}

}; // Namespace end