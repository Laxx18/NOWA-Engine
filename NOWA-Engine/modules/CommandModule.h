#ifndef COMMAND_MODULE_H
#define COMMAND_MODULE_H

#include <stack>
#include <list>
#include <iostream>
#include <memory>
#include "defines.h"

namespace NOWA
{
	class EXPORTED ICommand
	{
	public:
		virtual void undo(void) = 0;
		virtual void redo(void) = 0;
		/*virtual Ogre::String getUndoMessage(void) const = 0;
		virtual Ogre::String getRedoMessage(void) const = 0;*/
	};

	// typedef std::stack<std::shared_ptr<ICommand>> CommandStack;
	typedef std::list<std::shared_ptr<ICommand>> CommandStack;

	class EXPORTED CommandModule
	{
	public:
		CommandModule()
			: limitCount(10000)
		{
		}

		void setLimit(unsigned int limitCount)
		{
			this->limitCount = limitCount;
		}

		void pushCommand(std::shared_ptr<ICommand> command)
		{
			this->redoStack = CommandStack(); // clear the redo stack
			this->undoStack.push_front(command);
			if (this->undoStack.size() > this->limitCount)
			{
				this->redoStack.pop_front();
			}
		}

		void undo(void)
		{
			if (this->undoStack.size() <= 0)
			{
				return;
			}
			this->undoStack.front()->undo();          // undo most recently executed command
			this->redoStack.push_front(this->undoStack.front()); // add undone command to undo stack
			this->undoStack.pop_front();                  // remove top entry from undo stack
		}
		
		void undoAll(void)
		{
			while (this->undoStack.size() > 0)
			{
				this->undoStack.front()->undo();          // undo most recently executed command
				this->redoStack.push_front(this->undoStack.front()); // add undone command to undo stack
				this->undoStack.pop_front();                  // remove top entry from undo stack
			}
		}

		void redo(void)
		{
			if (this->redoStack.size() <= 0)
			{
				return;
			}
			this->redoStack.front()->redo();          // redo most recently executed command
			this->undoStack.push_front(this->redoStack.front()); // add undone command to redo stack
			this->redoStack.pop_front();                  // remove top entry from redo stack
		}
		
		void redoAll(void)
		{
			while (this->redoStack.size() > 0)
			{
				this->redoStack.front()->redo();          // redo most recently executed command
				this->undoStack.push_front(this->redoStack.front()); // add undone command to redo stack
				this->redoStack.pop_front();                  // remove top entry from redo stack
			}
		}

		/*Ogre::String getUndoMessage(void) const
		{
			return this->undoStack.top()->getUndoMessage();
		}

		Ogre::String getRedoMessage(void) const
		{
			return this->undoStack.top()->getRedoMessage();
		}*/

		bool canUndo(void)
		{
			return this->undoStack.size() > 0;
		}

		bool canRedo(void)
		{
			return this->redoStack.size() > 0;
		}

		void clear(void)
		{
			this->undoStack.clear();
			this->redoStack.clear();
		}
	private:
		CommandStack undoStack;
		CommandStack redoStack;
		unsigned int limitCount;
	};

}; //namespace end

#endif
