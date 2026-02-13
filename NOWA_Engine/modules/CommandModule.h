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
		friend class CommandModule;
	public:
		virtual void undo(void) = 0;
		virtual void redo(void) = 0;
		/*virtual Ogre::String getUndoMessage(void) const = 0;
		virtual Ogre::String getRedoMessage(void) const = 0;*/

		virtual Ogre::String getDebugName(void) const
		{
			return "ICommand";
		}
	protected:
		bool isAdditional = false;
	};

	// typedef std::stack<std::shared_ptr<ICommand>> CommandStack;
	typedef std::list<std::shared_ptr<ICommand>> CommandStack;

	class EXPORTED CompositeCommand : public ICommand
	{
	public:
		void add(const std::shared_ptr<ICommand>& cmd)
		{
			this->commands.push_back(cmd);
		}

		bool empty(void) const
		{
			return this->commands.empty();
		}

		const std::vector<std::shared_ptr<ICommand>>& getCommands(void) const
		{
			return this->commands;
		}

		virtual Ogre::String getDebugName(void) const override
		{
			return "CompositeCommand";
		}

		virtual void undo(void) override
		{
			for (auto it = this->commands.rbegin(); it != this->commands.rend(); ++it)
			{
				(*it)->undo();
			}
		}

		virtual void redo(void) override
		{
			for (auto& cmd : this->commands)
			{
				cmd->redo();
			}
		}

	private:
		std::vector<std::shared_ptr<ICommand>> commands;
	};

	class EXPORTED CommandModule
	{
	public:
		CommandModule()
			: limitCount(10000)
		{
		}

		~CommandModule()
		{
			if (false == this->transactionStack.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CommandModule] WARNING: Transactions still active at destruction. Depth: " + Ogre::StringConverter::toString(this->transactionStack.size()));
			}
		}

		void setLimit(unsigned int limitCount)
		{
			this->limitCount = limitCount;
		}

		void pushCommand(std::shared_ptr<ICommand> command)
		{
			if (false == this->transactionStack.empty())
			{
				this->transactionStack.back()->add(command);
				return;
			}

			this->redoStack = CommandStack();
			this->undoStack.push_front(command);

			if (this->undoStack.size() > this->limitCount)
			{
				this->undoStack.pop_back();
			}
		}

		void undo(void)
		{
			// Undo's also all additional items (e.g. via terra there are commands on the stack for terra editing during simulation, which would normally not be undo'd because they have nothing todo with the game object manipulations directly).
			while (this->undoStack.size() > 0 && true == this->undoStack.front()->isAdditional)
			{
				this->undoStack.front()->undo();          // undo most recently executed command
				this->undoStack.front()->isAdditional = false;
				this->redoStack.push_front(this->undoStack.front()); // add undone command to undo stack
				this->undoStack.pop_front();
			}

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

		void beginTransaction(const Ogre::String& label)
		{
			auto transaction = std::make_shared<CompositeCommand>();

			this->transactionStack.push_back(transaction);
			this->transactionLabelStack.push_back(label);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CommandModule] Transaction BEGIN: " + label + ", Depth: " + Ogre::StringConverter::toString(this->transactionStack.size()));
		}

		void endTransaction(void)
		{
			if (true == this->transactionStack.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CommandModule] endTransaction called but no transaction active.");
				return;
			}

			auto finishedTransaction = this->transactionStack.back();
			Ogre::String finishedLabel = this->transactionLabelStack.back();

			this->transactionStack.pop_back();
			this->transactionLabelStack.pop_back();

			if (true == this->transactionStack.empty())
			{
				// Outermost transaction → push to undo stack
				if (finishedTransaction && false == finishedTransaction->empty())
				{
					this->redoStack = CommandStack();
					this->undoStack.push_front(finishedTransaction);
				}
			}
			else
			{
				// Nested -> attach to parent transaction
				this->transactionStack.back()->add(finishedTransaction);
			}

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CommandModule] Transaction END: " + finishedLabel + ", Remaining Depth: "
				+ Ogre::StringConverter::toString(this->transactionStack.size()));
		}

		bool isTransactionActive(void) const
		{
			return false == this->transactionStack.empty();
		}

		void debugCheckTransactionIntegrity(void) const
		{
#ifdef _DEBUG
			if (false == this->transactionStack.empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
					"[CommandModule] Transaction mismatch detected. Depth: " + Ogre::StringConverter::toString(this->transactionStack.size()));

				assert(false && "Transaction mismatch detected.");
			}
#endif
		}

		void CommandModule::debugPrintUndoTree(void) const
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "===== UNDO STACK DEBUG START =====");

			int index = 0;

			for (auto& cmd : this->undoStack)
			{
				printCommandRecursive(cmd, 0, index++);
			}

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "===== UNDO STACK DEBUG END =====");
		}

		void CommandModule::printCommandRecursive(const std::shared_ptr<ICommand>& cmd, int depth, int index) const
		{
			std::string indent(depth * 4, ' ');

			Ogre::LogManager::getSingletonPtr()->logMessage( Ogre::LML_TRIVIAL, indent + "[" + Ogre::StringConverter::toString(index) + "] " + typeid(*cmd).name());

			auto composite = std::dynamic_pointer_cast<CompositeCommand>(cmd);
			if (composite)
			{
				const auto& children = composite->getCommands();
				int childIndex = 0;

				for (auto& child : children)
				{
					printCommandRecursive(child, depth + 1, childIndex++);
				}
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

		std::vector<std::shared_ptr<CompositeCommand>> transactionStack;
		std::vector<Ogre::String> transactionLabelStack;
	};

}; //namespace end

#endif
