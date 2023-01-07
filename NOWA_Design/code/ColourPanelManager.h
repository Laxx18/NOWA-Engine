#ifndef COLOUR_PANEL_MANAGER_H
#define COLOUR_PANEL_MANAGER_H

#include "ColourPanel/ColourPanel.h"

class ColourPanelManager
{
public:
	ColourPanelManager();
	~ColourPanelManager();

	void init(void);

	void destroyContent(void);

	MyGUI::ColourPanel* getColourPanel(void) const;
public:
	static ColourPanelManager* getInstance();
private:
	MyGUI::ColourPanel* colourPanel;
};

#endif
