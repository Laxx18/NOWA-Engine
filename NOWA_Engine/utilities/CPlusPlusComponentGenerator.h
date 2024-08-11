#ifndef C_PLUS_PLUS_COMPONENT_GENERATOR_H
#define C_PLUS_PLUS_COMPONENT_GENERATOR_H

#include "defines.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

#include "BaseLayout/BaseLayout.h"
#include "PanelView/BasePanelView.h"
#include "PanelView/BasePanelViewItem.h"

#include "Variant.h"

namespace NOWA
{
	class EXPORTED CPlusPlusComponentGenerator : public wraps::BaseLayout
	{
	public:
		CPlusPlusComponentGenerator(MyGUI::Widget* parent = nullptr);
	private:
		Ogre::String loadFileAsString(const Ogre::String& filePath);

		void saveStringToFile(const Ogre::String& filePath, const Ogre::String& content);

		Ogre::String capitalize(const Ogre::String& str);

		bool isComplexType(const Ogre::String& type);

		// Function to find the nth occurrence of a substring
		size_t findNthOccurrence(const Ogre::String& str, const Ogre::String& toFind, int n);

		// Function to insert a substring after the nth occurrence of another substring
		Ogre::String insertAfterNthOccurrence(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n);

		// Function to insert a substring before the nth occurrence of another substring
		Ogre::String insertBeforeNthOccurrence(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n);

		// Function to find the nth occurrence of a substring in reverse order
		size_t findNthOccurrenceReverse(const Ogre::String& str, const Ogre::String& toFind, int n);

		// Function to insert a substring before the nth occurrence of another substring in reverse order
		Ogre::String insertBeforeNthOccurrenceReverse(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n);

		size_t findLastBraceInMethod(const Ogre::String& sourceContent, const Ogre::String& methodName);

		Ogre::String generateVariableAttributeDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants);

		Ogre::String generateVariableDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants);

		Ogre::String generateVariableSetterGetterDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants);

		Ogre::String generateConstructorInitialization(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		Ogre::String generateSetterGetterImplementations(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		Ogre::String generateInitMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		Ogre::String generateCloneMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		Ogre::String generateActualizeValueMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		Ogre::String generateWriteXMLMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants);

		void modifyHeaderFile(const Ogre::String& filePath, const std::vector<Variant>& variants);

		void modifySourceFile(const Ogre::String& filePath, const Ogre::String& className, const std::vector<Variant>& variants);

		void validateInputs(void);

		void generateCode(void);
	private:
		void onInputChanged(MyGUI::Widget* _sender);

		void onGenerateButtonClick(MyGUI::Widget* _sender);

		void onOkButtonClick(MyGUI::Widget* _sender);

		void onAddVariableButtonClick(MyGUI::Widget* _sender);
	private:
		MyGUI::EditBox* classNameEdit;
		// MyGUI::EditBox* variantCountEdit; // This can be removed now
		MyGUI::Button* generateButton;
		MyGUI::Button* okButton;
		MyGUI::Button* addVariableButton;
		MyGUI::Widget* variantContainer;
		int currentVariableIndex;
		Ogre::String currentComponentPluginFolder;
	};

}; // namespace end


#endif