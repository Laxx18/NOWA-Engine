#include "NOWAPrecompiled.h"
#include "CPlusPlusComponentGenerator.h"
#include "modules/DeployResourceModule.h"

namespace
{
	Ogre::String QM()
	{
		return "\"";
	}
}

namespace NOWA
{
	CPlusPlusComponentGenerator::CPlusPlusComponentGenerator(MyGUI::Widget* parent)
		: wraps::BaseLayout("", parent),
		currentVariableIndex(0) // Initialize current index
	{
		assignWidget(this->classNameEdit, "ClassNameEdit");
		assignWidget(this->generateButton, "GenerateButton");
		assignWidget(this->okButton, "OkButton");
		assignWidget(this->addVariableButton, "AddVariableButton");
		assignWidget(this->variantContainer, "VariantContainer");

		this->classNameEdit->setOnlyText("MyTestComponent");

		int height = this->generateButton->getTop() + this->generateButton->getHeight() + 60;
		this->getMainWidget()->setSize(this->getMainWidget()->getWidth(), height);

		this->generateButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onGenerateButtonClick);
		this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onOkButtonClick);
		this->addVariableButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onAddVariableButtonClick);
		// Add event listener to class name edit to check validation
		this->classNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onInputChanged);
	}

	Ogre::String CPlusPlusComponentGenerator::loadFileAsString(const Ogre::String& filePath)
	{
		Ogre::String completeFilePath = this->currentComponentPluginFolder + "/" + filePath;

		std::ifstream file(completeFilePath);
		if (false == file.good())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CPlusPlusComponentGenerator]: Cannot load file, because the path: " + completeFilePath + " does not exist.");
			return "";
		}

		std::ostringstream content;
		content << file.rdbuf();
		file.close();
		return content.str();
	}

	void CPlusPlusComponentGenerator::saveStringToFile(const Ogre::String& filePath, const Ogre::String& content)
	{
		Ogre::String completeFilePath = this->currentComponentPluginFolder + "/" + filePath;

		std::ofstream file(completeFilePath);

		if (false == file.good())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CPlusPlusComponentGenerator]: Cannot save file, because the path: " + completeFilePath + " does not exist.");
			return;
		}

		file << content;
		file.close();
	}

	Ogre::String CPlusPlusComponentGenerator::capitalize(const Ogre::String& str)
	{
		if (true == str.empty())
		{
			return str;
		}
		Ogre::String result = str;
		result[0] = std::toupper(result[0]);
		return result;
	}

	bool CPlusPlusComponentGenerator::isComplexType(const Ogre::String& type)
	{
		static const std::unordered_set<Ogre::String> complexTypes = {
			"Ogre::Vector2",
			"Ogre::Vector3",
			"Ogre::Vector4",
			"Ogre::String",
			"std::vector<Ogre::String>"
		};
		return complexTypes.find(type) != complexTypes.end();
	}

	size_t CPlusPlusComponentGenerator::findNthOccurrence(const Ogre::String& str, const Ogre::String& toFind, int n)
	{
		size_t pos = str.find(toFind);
		int count = 1;

		while (count < n && pos != Ogre::String::npos)
		{
			pos = str.find(toFind, pos + 1);
			count++;
		}

		return pos;
	}

	Ogre::String CPlusPlusComponentGenerator::insertAfterNthOccurrence(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n)
	{
		size_t pos = this->findNthOccurrence(str, toFind, n);
		if (pos != Ogre::String::npos)
		{
			// Find the end of the line after the found position
			size_t endOfLine = str.find('\n', pos);
			if (endOfLine != Ogre::String::npos)
			{
				str.insert(endOfLine + 1, toInsert);
			}
			else
			{
				str.append("\n" + toInsert);
			}
		}
		return str;
	}

	Ogre::String CPlusPlusComponentGenerator::insertBeforeNthOccurrence(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n)
	{
		size_t pos = this->findNthOccurrence(str, toFind, n);
		if (pos != Ogre::String::npos)
		{
			str.insert(pos - 1, toInsert);
		}
		return str;
	}

	size_t CPlusPlusComponentGenerator::findNthOccurrenceReverse(const Ogre::String& str, const Ogre::String& toFind, int n)
	{
		size_t pos = str.rfind(toFind);
		int count = 1;

		while (count < n && pos != Ogre::String::npos)
		{
			pos = str.rfind(toFind, pos - 1);
			count++;
		}

		return pos;
	}

	Ogre::String CPlusPlusComponentGenerator::insertBeforeNthOccurrenceReverse(Ogre::String str, const Ogre::String& toFind, const Ogre::String& toInsert, int n)
	{
		size_t pos = this->findNthOccurrenceReverse(str, toFind, n);
		if (pos != Ogre::String::npos)
		{
			str.insert(pos - 1, toInsert);
		}
		return str;
	}

	size_t CPlusPlusComponentGenerator::findLastBraceInMethod(const Ogre::String& sourceContent, const Ogre::String& methodName)
	{
		size_t methodStartPos = sourceContent.find(methodName);
		if (methodStartPos == Ogre::String::npos)
		{
			throw std::runtime_error("Method not found in the source content.");
		}

		// Find the opening brace of the method
		size_t methodBodyStartPos = sourceContent.find('{', methodStartPos);
		if (methodBodyStartPos == Ogre::String::npos)
		{
			throw std::runtime_error("Opening brace of the method not found.");
		}

		// Find the last closing brace within the method
		size_t braceCount = 0;
		size_t lastBracePos = methodBodyStartPos;
		for (size_t i = methodBodyStartPos; i < sourceContent.size(); ++i)
		{
			if (sourceContent[i] == '{')
			{
				++braceCount;
			}
			else if (sourceContent[i] == '}')
			{
				--braceCount;
				if (braceCount == 0)
				{
					lastBracePos = i;
					break;
				}
			}
		}

		return lastBracePos;
	}

	Ogre::String CPlusPlusComponentGenerator::generateVariableSetterGetterDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants)
	{
		std::ostringstream privateDeclarations;
		std::ostringstream publicDeclarations;

		for (const auto& variant : variants)
		{
			if (true == this->isComplexType(variant.getStrType()))
			{
				Ogre::String setter;

				setter += "\t\tvoid set" + capitalize(variant.getName()) + "(const " + variant.getStrType() + "& " + variant.getName() + "); \n\n";
				Ogre::String getter = "\t\t" + variant.getStrType() + " get" + capitalize(variant.getName()) + "(void) const;\n\n";
				if (headerContent.find(setter) == Ogre::String::npos)
				{
					publicDeclarations << setter;
				}
				if (headerContent.find(getter) == Ogre::String::npos)
				{
					publicDeclarations << getter;
				}
			}
			else
			{
				Ogre::String setter;
				setter += "\t\tvoid set" + capitalize(variant.getName()) + "(" + variant.getStrType() + " " + variant.getName() + "); \n\n";
				Ogre::String getter = "\t\t" + variant.getStrType() + " get" + capitalize(variant.getName()) + "(void) const;\n\n";
				if (headerContent.find(setter) == Ogre::String::npos)
				{
					publicDeclarations << setter;
				}
				if (headerContent.find(getter) == Ogre::String::npos)
				{
					publicDeclarations << getter;
				}
			}
		}

		return publicDeclarations.str();
	}


	Ogre::String CPlusPlusComponentGenerator::generateVariableAttributeDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants)
	{
		std::ostringstream attributeDeclarations;

		for (const auto& variant : variants)
		{
			Ogre::String attributeDeclaration;

			attributeDeclaration += "\t\tstatic const Ogre::String Attr" + capitalize(variant.getName()) + "(void) { return " + QM() + capitalize(variant.getName()) + QM() + "; }\n";
			
			if (headerContent.find(attributeDeclaration) == Ogre::String::npos)
			{
				attributeDeclarations << attributeDeclaration;
			}
		}

		Ogre::String declarations = attributeDeclarations.str();
		return declarations;
	}

	Ogre::String CPlusPlusComponentGenerator::generateVariableDeclarations(const Ogre::String& headerContent, const std::vector<Variant>& variants)
	{
		std::ostringstream privateDeclarations;

		for (const auto& variant : variants)
		{
			Ogre::String declaration;

			declaration += "\t\tVariant* " + variant.getName() + ";\n";
			if (headerContent.find(declaration) == Ogre::String::npos)
			{
				privateDeclarations << declaration;
			}
		}

		Ogre::String declarations = privateDeclarations.str();
		return declarations;
	}

	Ogre::String CPlusPlusComponentGenerator::generateConstructorInitialization(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream constructorBody;

		unsigned int i = 0;
		for (const auto& variant : variants)
		{
			Ogre::String constructorVariable;

			constructorVariable += "\t\t" + variant.getName() + "(new Variant(";
			constructorVariable += className + "::Attr" + capitalize(variant.getName()) += "(), ";

			if (variant.getStrType() == "bool")
			{
				constructorVariable += "true";
			}
			else if (variant.getStrType() == "Ogre::Real")
			{
				constructorVariable += "0.0f";
			}
			else if (variant.getStrType() == "int")
			{
				constructorVariable += "static_cast<int>(0)";
			}
			else if (variant.getStrType() == "unsigned int")
			{
				constructorVariable += "static_cast<unsigned int>(0)";
			}
			else if (variant.getStrType() == "unsigned long")
			{
				constructorVariable += "static_cast<unsigned long>(0)";
			}
			else if (variant.getStrType() == "Ogre::Vector2")
			{
				constructorVariable += "Ogre::Vector2::ZERO";
			}
			else if (variant.getStrType() == "Ogre::Vector3")
			{
				constructorVariable += "Ogre::Vector3::ZERO";
			}
			else if (variant.getStrType() == "Ogre::Vector4")
			{
				constructorVariable += "Ogre::Vector4::ZERO";
			}
			else if (variant.getStrType() == "Ogre::String")
			{
				constructorVariable += "Ogre::String()";
			}
			else if (variant.getStrGetter() == "getListSelectedValue()")
			{
				constructorVariable += "std::vector<Ogre::String>()";
			}

			if (variant.getStrType() == "unsigned long")
			{
				constructorVariable += ", this->attributes, true))";
			}
			else
			{
				constructorVariable += ", this->attributes))";
			}

			if (i < variants.size() - 1)
			{
				constructorVariable += ",\n";
			}
			else
			{
				constructorVariable += "\n";
			}

			if (sourceContent.find(constructorVariable) == Ogre::String::npos)
			{
				constructorBody << constructorVariable;
			}

			i++;
		}

		return constructorBody.str();
	}

	Ogre::String CPlusPlusComponentGenerator::generateInitMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream initMethod;

		for (const auto& variant : variants)
		{
			Ogre::String loadData;

			loadData += "\t\tif (propertyElement && XMLConverter::getAttrib(propertyElement, " + QM() + "name" + QM() + ") == " + className + "::Attr" + capitalize(variant.getName()) + "())\n";
			loadData += "\t\t{\n";

			if (variant.getStrGetter() != "getListSelectedValue()")
			{
				loadData += "\t\t\tthis->" + variant.getName() + "->setValue(";
			}
			else
			{
				loadData += "\t\t\tthis->" + variant.getName() + "->setListSelectedValue(";
			}

			if (variant.getStrType() == "bool")
			{
				loadData += "XMLConverter::getAttribBool(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "Ogre::Real")
			{
				loadData += "XMLConverter::getAttribReal(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "int")
			{
				loadData += "XMLConverter::getAttribInt(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "unsigned int")
			{
				loadData += "XMLConverter::getAttribUnsignedInt(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "unsigned long")
			{
				loadData += "XMLConverter::getAttribUnsignedLong(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "Ogre::Vector2")
			{
				loadData += "XMLConverter::getAttribVector2(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "Ogre::Vector3")
			{
				loadData += "XMLConverter::getAttribVector3(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "Ogre::Vector4")
			{
				loadData += "XMLConverter::getAttribVector4(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "Ogre::String")
			{
				loadData += "XMLConverter::getAttrib(propertyElement, " + QM() + "data" + QM() + ")";
			}
			else if (variant.getStrType() == "std::vector<Ogre::String>")
			{
				loadData += "XMLConverter::getAttrib(propertyElement, " + QM() + "data" + QM() + ")";
			}

			loadData += ");\n";

			loadData += "\t\t\tpropertyElement = propertyElement->next_sibling(" + QM() + "property" + QM() + ");\n";
			loadData += "\t\t}\n";

			if (sourceContent.find(loadData) == Ogre::String::npos)
			{
				initMethod << loadData;
			}
		}
		return initMethod.str().c_str();
	}

	Ogre::String CPlusPlusComponentGenerator::generateCloneMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream cloneMethod;

		unsigned int i = 0;

		for (const auto& variant : variants)
		{
			Ogre::String cloneVariable;

			cloneVariable += "\t\tclonedCompPtr->set" + capitalize(variant.getName()) + "(";
			cloneVariable += "this->" + variant.getName() + "->" + variant.getStrGetter();

			cloneVariable += ");\n";

			if (i < variants.size() - 1)
			{
			}
			else
			{
				cloneVariable += "\n";
			}

			if (sourceContent.find(cloneVariable) == Ogre::String::npos)
			{
				cloneMethod << cloneVariable;
			}

			i++;
		}

		return cloneMethod.str().c_str();
	}

	Ogre::String CPlusPlusComponentGenerator::generateActualizeValueMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream actualizeValueMethod;

		for (const auto& variant : variants)
		{
			Ogre::String actualizeVariable;
			actualizeVariable += "\t\telse if (" + className + "::Attr" + capitalize(variant.getName()) + "() == attribute->getName())\n";
			actualizeVariable += "\t\t{\n";
			actualizeVariable += "\t\t\tthis->set" + capitalize(variant.getName()) + "(attribute->" + variant.getStrGetter() + ");\n";
			actualizeVariable += "\t\t}\n";

			if (sourceContent.find(actualizeVariable) == Ogre::String::npos)
			{
				actualizeValueMethod << actualizeVariable;
			}
		}

		return actualizeValueMethod.str().c_str();
	}

	Ogre::String CPlusPlusComponentGenerator::generateWriteXMLMethod(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream writeXMLMethod;

		for (const auto& variant : variants)
		{
			Ogre::String writeVariable;

			writeVariable += "\n\t\tpropertyXML = doc.allocate_node(node_element, \"property\");\n";
			writeVariable += "\t\tpropertyXML->append_attribute(doc.allocate_attribute(\"type\", \"";

			if (variant.getStrType() == "bool")
			{
				writeVariable += "12\"));\n";  // Assuming "12" corresponds to bool
			}
			else if (variant.getStrType() == "Ogre::Real")
			{
				writeVariable += "6\"));\n";   // Assuming "6" corresponds to real
			}
			else if (variant.getStrType() == "int")
			{
				writeVariable += "2\"));\n";   // Assuming "2" corresponds to int
			}
			else if (variant.getStrType() == "unsigned int")
			{
				writeVariable += "2\"));\n";   // Assuming same type as int
			}
			else if (variant.getStrType() == "unsigned long")
			{
				writeVariable += "2\"));\n";   // Assuming same type as int
			}
			else if (variant.getStrType() == "Ogre::Vector2")
			{
				writeVariable += "8\"));\n";   // Assuming "8" corresponds to vector2
			}
			else if (variant.getStrType() == "Ogre::Vector3")
			{
				writeVariable += "9\"));\n";   // Assuming "9" corresponds to vector3
			}
			else if (variant.getStrType() == "Ogre::Vector4")
			{
				writeVariable += "10\"));\n";  // Assuming "10" corresponds to vector4
			}
			else if (variant.getStrType() == "Ogre::String")
			{
				writeVariable += "7\"));\n";   // Assuming "7" corresponds to string
			}
			else if (variant.getStrType() == "std::vector<Ogre::String>")
			{
				writeVariable += "7\"));\n";   // Assuming string representation for vector
			}
			else
			{
				writeVariable += "0\"));\n";   // Default case
			}

			writeVariable += "\t\tpropertyXML->append_attribute(doc.allocate_attribute(\"name\", \"" + capitalize(variant.getName()) + "\"));\n";

			// Assuming a method to convert to string representation
			writeVariable += "\t\tpropertyXML->append_attribute(doc.allocate_attribute(\"data\", XMLConverter::ConvertString(doc, this->" + variant.getName() + "->" + variant.getStrGetter() + ")));\n";
			writeVariable += "\t\tpropertiesXML->append_node(propertyXML);\n";

			if (sourceContent.find(writeVariable) == Ogre::String::npos)
			{
				writeXMLMethod << writeVariable;
			}
		}

		return writeXMLMethod.str().c_str();
	}

	Ogre::String CPlusPlusComponentGenerator::generateSetterGetterImplementations(const Ogre::String& sourceContent, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		std::ostringstream implementations;

		for (const auto& variant : variants)
		{
			// Setter method
			if (isComplexType(variant.getStrType()))
			{
				Ogre::String setter = "\n\tvoid " + className + "::set" + capitalize(variant.getName()) + "(const " + variant.getStrType() + "& " + variant.getName() + ")\n";
				setter += "\t{\n";
				if (variant.getStrGetter() != "getListSelectedValue()")
				{
					setter += "\t\tthis->" + variant.getName() + "->setValue(" + variant.getName() + ");\n";
				}
				else
				{
					setter += "\t\tthis->" + variant.getName() + "->setListSelectedValue(" + variant.getName() + ");\n";
				}
				setter += "\t}\n";

				if (sourceContent.find(setter) == Ogre::String::npos)
				{
					implementations << setter;
				}

				Ogre::String getter = "\n\t" + variant.getStrType() + " " + className + "::get" + capitalize(variant.getName()) + "(void) const\n";

				getter += "\t{\n";
				getter += "\t\treturn this->" + variant.getName() + "->" + variant.getStrGetter() + ";\n";
				getter += "\t}\n";

				if (sourceContent.find(getter) == Ogre::String::npos)
				{
					implementations << getter;
				}
			}
			else
			{
				Ogre::String setter = +"\n\tvoid " + className + "::set" + capitalize(variant.getName()) + "(" + variant.getStrType() + " " + variant.getName() + ")\n";
				setter += "\t{\n";
				setter += "\t\tthis->" + variant.getName() + "->setValue(" + variant.getName() + ");\n";
				setter += "\t}\n";

				if (sourceContent.find(setter) == Ogre::String::npos)
				{
					implementations << setter;
				}

				Ogre::String getter = "\n\t" + variant.getStrType() + " " + className + "::get" + capitalize(variant.getName()) + "(void) const\n";

				getter += "\t{\n";
				getter += "\t\treturn this->" + variant.getName() + "->" + variant.getStrGetter() + ";\n";
				getter += "\t}\n";

				if (sourceContent.find(getter) == Ogre::String::npos)
				{
					implementations << getter;
				}
			}
		}

		return implementations.str();
	}

	void CPlusPlusComponentGenerator::modifyHeaderFile(const Ogre::String& filePath, const std::vector<Variant>& variants)
	{
		Ogre::String headerContent = this->loadFileAsString(filePath);
		if (true == headerContent.empty())
		{
			return;
		}

		Ogre::String publicSetterGetterDeclarations = this->generateVariableSetterGetterDeclarations(headerContent, variants);
		Ogre::String variableAttributeDeclarations = this->generateVariableAttributeDeclarations(headerContent, variants);
		Ogre::String variableDeclarations = this->generateVariableDeclarations(headerContent, variants);
		
		headerContent = this->insertBeforeNthOccurrence(headerContent, "public:", publicSetterGetterDeclarations, 3);
		headerContent = this->insertBeforeNthOccurrenceReverse(headerContent, "private:", variableAttributeDeclarations, 1);
		headerContent = this->insertBeforeNthOccurrenceReverse(headerContent, "};", variableDeclarations, 2);
		
		// Saves modified source content
		saveStringToFile(filePath, headerContent);
	}

	void CPlusPlusComponentGenerator::modifySourceFile(const Ogre::String& filePath, const Ogre::String& className, const std::vector<Variant>& variants)
	{
		Ogre::String sourceContent = loadFileAsString(filePath);
		if (true == sourceContent.empty())
		{
			return;
		}

		// Insert constructor initializations
		Ogre::String constructorInitialization = generateConstructorInitialization(sourceContent, className, variants);
		size_t pos = sourceContent.find(className + "::" + className + "()");
		if (pos != Ogre::String::npos)
		{
			size_t initPos = sourceContent.find("{", pos);
			if (initPos != Ogre::String::npos)
			{
				sourceContent.insert(initPos - 2, ",\n" + constructorInitialization);
			}
		}

		// Insert init method code
		Ogre::String initMethod = generateInitMethod(sourceContent, className, variants);
		pos = sourceContent.find("bool " + className + "::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)");
		if (pos != Ogre::String::npos)
		{
			size_t initPos = sourceContent.find("return true;", pos);
			if (initPos != Ogre::String::npos)
			{
				sourceContent.insert(initPos - 2, initMethod);
			}
		}

		// Insert clone method code
		Ogre::String cloneMethod = generateCloneMethod(sourceContent, className, variants);
		pos = sourceContent.find("GameObjectCompPtr " + className + "::clone(GameObjectPtr clonedGameObjectPtr)");
		if (pos != Ogre::String::npos)
		{
			size_t clonePos = sourceContent.find("GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));", pos);
			if (clonePos != Ogre::String::npos)
			{
				sourceContent.insert(clonePos - 2, cloneMethod);
			}
		}

		// Insert actualizeValue method code
		Ogre::String actualizeValueMethod = generateActualizeValueMethod(sourceContent, className, variants);
		pos = this->findLastBraceInMethod(sourceContent, className + "::actualizeValue(Variant* attribute)");

		if (pos != Ogre::String::npos)
		{
			sourceContent.insert(pos - 1, actualizeValueMethod);
		}

		// Insert writeXML method code
		Ogre::String writeXMLMethod = generateWriteXMLMethod(sourceContent, className, variants);
		pos = this->findLastBraceInMethod(sourceContent, className + "::writeXML(xml_node<>* propertiesXML");
		if (pos != Ogre::String::npos)
		{
			sourceContent.insert(pos - 1, writeXMLMethod);
		}

		// Insert method implementations
		Ogre::String methodImplementations = generateSetterGetterImplementations(sourceContent, className, variants);
		pos = sourceContent.find("// Lua registration part");
		if (pos != Ogre::String::npos)
		{
			sourceContent.insert(pos - 2, methodImplementations);
		}

		// Save modified source content
		saveStringToFile(filePath, sourceContent);
	}

	void CPlusPlusComponentGenerator::onInputChanged(MyGUI::Widget* _sender)
	{
		this->validateInputs();
	}

	void CPlusPlusComponentGenerator::validateInputs(void)
	{
		bool allValid = !this->classNameEdit->getCaption().empty();

		for (int i = 0; i < this->currentVariableIndex; ++i)
		{
			MyGUI::ComboBox* typeCombo = this->variantContainer->findWidget("TypeCombo" + std::to_string(i))->castType<MyGUI::ComboBox>(false);
			MyGUI::EditBox* nameEdit = this->variantContainer->findWidget("NameEdit" + std::to_string(i))->castType<MyGUI::EditBox>(false);

			if (nullptr == typeCombo || nullptr == nameEdit || true == typeCombo->getCaption().empty() || true == nameEdit->getCaption().empty())
			{
				allValid = false;
				break;
			}
		}

		this->generateButton->setEnabled(allValid);
	}

	void CPlusPlusComponentGenerator::onGenerateButtonClick(MyGUI::Widget* _sender)
	{
		Ogre::String componentName = this->classNameEdit->getOnlyText();
		if (false == componentName.empty())
		{
			NOWA::DeployResourceModule::getInstance()->createCPlusPlusComponentPluginProject(componentName);
			this->currentComponentPluginFolder = NOWA::DeployResourceModule::getInstance()->getCurrentComponentPluginFolder();
		}


		this->generateCode();
	}

	void CPlusPlusComponentGenerator::onOkButtonClick(MyGUI::Widget* _sender)
	{
		this->mMainWidget->setVisible(false);
	}

	void CPlusPlusComponentGenerator::onAddVariableButtonClick(MyGUI::Widget* _sender)
	{
		// Create a new variable input field
		MyGUI::ComboBox* typeCombo = this->variantContainer->createWidget<MyGUI::ComboBox>("ComboBox", MyGUI::IntCoord(0, this->currentVariableIndex * 40, 180, 30), MyGUI::Align::Default, "TypeCombo" + std::to_string(this->currentVariableIndex));
		typeCombo->setEditStatic(true);
		
		const auto& allStrTypes = Variant::getAllStrTypes();
		for (size_t i = 0; i < allStrTypes.size(); i++)
		{
			typeCombo->addItem(allStrTypes[i]);
		}

		MyGUI::EditBox* nameEdit = this->variantContainer->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(290, this->currentVariableIndex * 40, 180, 30), MyGUI::Align::Default, "NameEdit" + std::to_string(this->currentVariableIndex));

		// Attach event listeners to validate input
		typeCombo->eventComboAccept += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onInputChanged);
		nameEdit->eventEditTextChange += MyGUI::newDelegate(this, &CPlusPlusComponentGenerator::onInputChanged);


		++this->currentVariableIndex; // Increment index for next variable

		{
			const auto& position = this->generateButton->getPosition();
			this->generateButton->setPosition(position.left, position.top + this->currentVariableIndex * 40);
		}

		{
			const auto& position = this->okButton->getPosition();
			this->okButton->setPosition(position.left, position.top + this->currentVariableIndex * 40);
		}

		int height = this->generateButton->getTop() + this->generateButton->getHeight() + 60;
		this->getMainWidget()->setSize(this->getMainWidget()->getWidth(), height);

		// Checks if we can enable the generate button
		this->validateInputs();
	}

	void CPlusPlusComponentGenerator::generateCode(void)
	{
		Ogre::String className = this->classNameEdit->getCaption();

		std::vector<Variant> variants;
		for (int i = 0; i < this->currentVariableIndex; ++i)
		{
			// this->manipulationWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow");
			MyGUI::ComboBox* typeCombo = this->variantContainer->findWidget("TypeCombo" + std::to_string(i))->castType<MyGUI::ComboBox>();
			MyGUI::EditBox* nameEdit = this->variantContainer->findWidget("NameEdit" + std::to_string(i))->castType<MyGUI::EditBox>();

			if (typeCombo && nameEdit)
			{
				Variant variant(nameEdit->getCaption(), typeCombo->getCaption());
				variants.push_back(variant);
			}
		}

		Ogre::String headerFilePath = className + ".h";
		Ogre::String sourceFilePath = className + ".cpp";

		modifyHeaderFile(headerFilePath, variants);
		modifySourceFile(sourceFilePath, className, variants);
	}

	

}; // namespace end