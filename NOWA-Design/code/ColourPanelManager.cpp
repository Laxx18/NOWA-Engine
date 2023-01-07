#include "NOWAPrecompiled.h"
#include "ColourPanelManager.h"
#include "ColourPanel/ColourPanel.cpp"

ColourPanelManager::ColourPanelManager()
	: colourPanel(nullptr)
{

}

ColourPanelManager::~ColourPanelManager()
{

}

void ColourPanelManager::init(void)
{
	if (nullptr == this->colourPanel)
	{
		this->colourPanel = new MyGUI::ColourPanel();
		this->colourPanel->setVisible(false);
	}
}

void ColourPanelManager::destroyContent(void)
{
	if (nullptr != this->colourPanel)
	{
		delete this->colourPanel;
		this->colourPanel = nullptr;
	}
}

MyGUI::ColourPanel* ColourPanelManager::getColourPanel(void) const
{
	return this->colourPanel;
}

ColourPanelManager* ColourPanelManager::getInstance()
{
	static ColourPanelManager instance;

	return &instance;
}

