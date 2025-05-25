#include "NOWAPrecompiled.h"
#include "LuaConsole.h"
#include "main/InputDeviceCore.h"
#include "main/AppStateManager.h"
#include "modules/GraphicsModule.h"

#include <OgreOverlay.h>

//Singleton creation
// template<> NOWA::LuaConsole* Ogre::Singleton<NOWA::LuaConsole>::msSingleton = 0;

namespace NOWA
{

#define CONSOLE_LINE_LENGTH 85
#define CONSOLE_LINE_COUNT 15
#define CONSOLE_MAX_LINES 32000
#define CONSOLE_MAX_HISTORY 64
#define CONSOLE_TAB_STOP 8

	LuaConsole::LuaConsole()
		: initialised(false),
		controlPressed(false)
	{

	}

	LuaConsole::~LuaConsole()
	{
		if (this->initialised)
		{
			this->shutdown();
		}
	}

	LuaConsole* LuaConsole::getSingletonPtr(void)
	{
		return msSingleton;
	}

	LuaConsole& LuaConsole::getSingleton(void)
	{
		assert(msSingleton);
		return (*msSingleton);
	}

	void LuaConsole::init(lua_State* lua)
	{
		if (this->initialised)
		{
			shutdown();
		}

		Ogre::v1::OverlayManager& overlayManager = Ogre::v1::OverlayManager::getSingleton();

		this->height = 1;
		this->startLine = 0;
		this->cursorBlinkTime = 0;
		this->cursorBlink = false;
		this->visible = false;

		this->pInterpreter = new LuaConsoleInterpreter(lua);
		this->print(this->pInterpreter->getOutput());
		this->pInterpreter->clearOutput();

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("LuaConsole::init", _1(&overlayManager),
		{
			this->pTextbox = overlayManager.createOverlayElement("TextArea", "ConsoleText");
			this->pTextbox->setMetricsMode(Ogre::v1::GMM_RELATIVE);
			this->pTextbox->setPosition(0, 0);
			this->pTextbox->setParameter("font_name", "LuaConsole");
			this->pTextbox->setParameter("colour_top", "1 1 1");
			this->pTextbox->setParameter("colour_bottom", "1 1 1");
			this->pTextbox->setParameter("char_height", "0.03");

			this->pPanel = static_cast<Ogre::v1::OverlayContainer*>(overlayManager.createOverlayElement("Panel", "ConsolePanel"));
			this->pPanel->setMetricsMode(Ogre::v1::GMM_RELATIVE);
			this->pPanel->setPosition(0, 0);
			this->pPanel->setDimensions(1, 0);
			// this->pPanel->setMaterialName("Console/background");
			this->pPanel->setMaterialName("Materials/OverlayMaterial");

			this->pPanel->addChild(this->pTextbox);

			// this->pPanel->setPosition(0.0f, 0.0f);

			this->pOverlay = overlayManager.create("Console");
			this->pOverlay->add2D(this->pPanel);
			this->pOverlay->show();

			Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);

			this->initialised = true;
		});
	}

	void LuaConsole::shutdown(void)
	{
		if (this->initialised)
		{
			delete this->pInterpreter;
			// Ogre::OverlayManager::getSingleton().destroyOverlayElement(this->pTextbox);
			// Ogre::OverlayManager::getSingleton().destroyOverlayElement(this->pPanel);
			// Ogre::OverlayManager::getSingleton().destroy(this->pOverlay);
			Ogre::LogManager::getSingleton().getDefaultLog()->removeListener(this);
		}
		this->initialised = false;
	}

	void LuaConsole::setVisible(bool visible)
	{
		this->visible = visible;
	}

	bool LuaConsole::isVisible(void)
	{
		return this->visible;
	}

	void LuaConsole::messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage)
	{
		lml = Ogre::LML_CRITICAL;
		this->print(message);
	}

	void LuaConsole::update(Ogre::Real dt)
	{
		if (this->visible)
		{
			this->cursorBlinkTime += dt;

			if (this->cursorBlinkTime > 0.5f)
			{
				this->cursorBlinkTime -= 0.5f;
				this->cursorBlink = !this->cursorBlink;
				this->textChanged = true;
			}

			if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LCONTROL))
			{
				this->controlPressed = true;
			}
			if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_V) && this->controlPressed)
			{
				HANDLE clip;
				if (OpenClipboard(NULL))
				{
					clip = GetClipboardData(CF_TEXT);
					CloseClipboard();
				}

				this->clipText = (char*)clip;
				this->editLine.setText(this->clipText);
				this->textChanged = true;
			}
			this->controlPressed = false;

			this->editLine.updateKeyPress(InputDeviceCore::getSingletonPtr()->getKeyboard(), dt);
		}

		if (this->visible && this->height < 1.0f)
		{
			this->height += dt * 10.0f;

			ENQUEUE_RENDER_COMMAND("LuaConsole::update1",
			{
				this->pPanel->show();
				this->pTextbox->show();
			});

			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);

			if (this->height >= 1.0f)
			{
				this->height = 1.0f;
			}
		}
		else if (!this->visible && this->height > 0.0f)
		{
			this->height -= dt * 10.0f;
			if (this->height <= 0.0f)
			{
				this->height = 0.0f;
				ENQUEUE_RENDER_COMMAND("LuaConsole::update2",
					{
						this->pPanel->hide();
						this->pTextbox->hide();
					});
				NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
				NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
			}
		}

		ENQUEUE_RENDER_COMMAND("LuaConsole::update3",
		{
			this->pTextbox->setPosition(0.0f, (this->height - 1.0f) * 0.5f);
			this->pPanel->setDimensions(1.0f, this->height * 0.5f);
		});

		if (this->textChanged)
		{
			Ogre::String text("");
			std::list<std::string>::iterator i;
			std::list<std::string>::iterator start;
			std::list<std::string>::iterator end;

			//make sure is in range
			//NOTE: the code elsewhere relies on startLine's signedness.
			//I.e. the ability to go below zero and not wrap around to a high number.
			if (this->startLine < 0)
			{
				this->startLine = 0;
			}
			if ((unsigned)this->startLine > this->lines.size())
			{
				this->startLine = static_cast<unsigned int>(this->lines.size());
			}
			start = this->lines.begin();

			for (int c = 0; c < this->startLine; c++)
			{
				++start;
			}
			end = start;

			for (int c = 0; c < CONSOLE_LINE_COUNT; c++)
			{
				if (end == lines.end())
				{
					break;
				}
				++end;
			}

			for (i = start; i != end; ++i)
			{
				text += (*i) + "\n";
			}

			//add the edit line with cursor
			std::string editLineText(this->editLine.getText() + " ");
			if (cursorBlink)
			{
				editLineText[this->editLine.getPosition()] = '_';
			}
			text += this->pInterpreter->getPrompt() + editLineText;

			ENQUEUE_RENDER_COMMAND_MULTI("LuaConsole::update4", _1(text),
			{
				try
				{
					// bad UTF-8 continuation byte may happen at any time, when a specifig log is printed :( so catch it
					this->pTextbox->setCaption(text);
				}
				catch (std::exception& exception)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Error in set text for Log console: " + Ogre::String(exception.what()));
				}
			});
			this->textChanged = false;
		}
	}

	void LuaConsole::print(std::string text)
	{
		std::string line;
		std::string::iterator pos;
		int column;

		pos = text.begin();
		column = 1;

		while (pos != text.end())
		{
			if (*pos == '\n' || column > CONSOLE_LINE_LENGTH)
			{
				this->lines.push_back(line);
				line.clear();
				if (*pos != '\n')
				{
					--pos;  // We want to keep this character for the next line.
				}
				column = 0;
			}
			else if (*pos == '\t')
			{
				// Push at least 1 space
				line.push_back(' ');
				column++;

				// fill until next multiple of CONSOLE_TAB_STOP
				while ((column % CONSOLE_TAB_STOP) != 0)
				{
					line.push_back(' ');
					column++;
				}
			}
			else
			{
				line.push_back(*pos);
				column++;
			}
			++pos;
		}
		if (line.length())
		{
			if (this->lines.size() > CONSOLE_MAX_LINES - 1)
			{
				this->lines.pop_front();
			}
			this->lines.push_back(line);
		}

		// Make sure last text printed is in view.
		if (this->lines.size() > CONSOLE_LINE_COUNT)
		{
			startLine = static_cast<unsigned int>(lines.size()) - CONSOLE_LINE_COUNT;
		}
		textChanged = true;

		return;
	}

	void LuaConsole::addToHistory(const std::string& cmd)
	{
		this->history.remove(cmd);
		this->history.push_back(cmd);

		if (this->history.size() > CONSOLE_MAX_HISTORY)
		{
			this->history.pop_front();
		}
		this->historyLine = this->history.end();
	}

	bool LuaConsole::injectKeyPress(const OIS::KeyEvent &evt)
	{
		switch (evt.key)
		{
		case OIS::KC_RETURN:
			print(this->pInterpreter->getPrompt() + this->editLine.getText());
			this->pInterpreter->insertLine(this->editLine.getText());
			this->addToHistory(this->editLine.getText());
			this->print(this->pInterpreter->getOutput());
			this->pInterpreter->clearOutput();
			this->editLine.clear();
			break;

		case OIS::KC_PGUP:
			this->startLine -= CONSOLE_LINE_COUNT;
			this->textChanged = true;
			break;

		case OIS::KC_PGDOWN:
			this->startLine += CONSOLE_LINE_COUNT;
			this->textChanged = true;
			break;

		case OIS::KC_UP:
			if (!this->history.empty())
			{
				if (this->historyLine == this->history.begin())
				{
					this->historyLine = this->history.end();
				}
				this->historyLine--;
				this->editLine.setText(*this->historyLine);
				this->textChanged = true;
			}
			break;

		case OIS::KC_DOWN:
			if (!this->history.empty())
			{
				if (this->historyLine != history.end())
				{
					this->historyLine++;
				}
				if (this->historyLine == this->history.end())
				{
					this->historyLine = this->history.begin();
				}
				this->editLine.setText(*this->historyLine);
				this->textChanged = true;
			}
			break;
		default:
			this->textChanged = this->editLine.injectKeyPress(evt);
			break;
		}

		return true;
	}

	void LuaConsole::clearHistory()
	{
		this->history.clear();
	}

	void LuaConsole::clearConsole()
	{
		this->lines.clear();
		this->textChanged = true;
	}

}; // Namespace end