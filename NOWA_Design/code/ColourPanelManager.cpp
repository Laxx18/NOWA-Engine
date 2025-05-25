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
	if (this->colourPanel != nullptr)
	{
		// Step 1: Copy pointer and nullify member
		auto panelCopy = this->colourPanel;
		this->colourPanel = nullptr;

		// Step 2: Enqueue the destruction on render thread
		ENQUEUE_DESTROY_COMMAND("ColourPanelManager::destroyContent", _1(panelCopy),
		{
			delete panelCopy;
		});
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

