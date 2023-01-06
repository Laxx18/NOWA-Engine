/*!
	@file
	@author		Albert Semenov
	@date		03/2011
*/

#ifndef MYGUI_HYPER_TEXT_BOX_H_
#define MYGUI_HYPER_TEXT_BOX_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_Widget.h"
#include "StackPanel.h"
#include "ScrollViewPanel.h"
#include "WrapPanel.h"

namespace MyGUI
{

	class HyperTextBox :
		public Widget
	{
		MYGUI_RTTI_DERIVED( HyperTextBox )

	public:
		HyperTextBox();

		void setCaption(const UString& _value);

		void addItem(const std::string& _value);

		size_t getItemsCount() const;

		void removeItemAt(size_t _index);

		void removeAllItems();

		void updateContent();

		const IntSize& getSpacer() const;
		void setSpacer(const IntSize& _value);

		const std::string& getHeader1Font() const;
		void setHeader1Font(const std::string& _value);

		const std::string& getHeader2Font() const;
		void setHeader2Font(const std::string& _value);

		const std::string& getHeader3Font() const;
		void setHeader3Font(const std::string& _value);

		const std::string& getDefaultFont() const;
		void setDefaultFont(const std::string& _value);

		const std::string& getBoldFont() const;
		void setBoldFont(const std::string& _value);

		const std::string& getItalicFont() const;
		void setItalicFont(const std::string& _value);

		const std::string& getBoldItalicFont() const;
		void setBoldItalicFont(const std::string& _value);

		void setUrlColour(const MyGUI::Colour& urlColour);

		void setTextSelectable(bool textSelectable);

		std::string getSelectedText(void) const;

	/*events:*/
		/** Event : Click to Url.\n
			signature : void method(MyGUI::HyperTextBox* _sender, const std::string& _url)
			@param _sender widget that called this event
			@param _url
		*/
		delegates::CMultiDelegate2<HyperTextBox*, const std::string&> eventUrlClick;

		/** Event : Text selected.\n
			signature : void method(MyGUI::HyperTextBox* _sender, const std::string& _selectedText)
			@param _sender widget that called this event
			@param _selectedText
		*/
		delegates::CMultiDelegate2<HyperTextBox*, const std::string&> eventTextSelected;

		/** Event : Key pressed inside.\n
			signature : void method(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char)\n
			@param _sender widget that called this event
			@param _key code
			@param _char of pressed symbol (for multilanguage applications)
			@param _selectedText
		*/
		delegates::CMultiDelegate4<MyGUI::Widget*, MyGUI::KeyCode, Char, const std::string&> eventNotifyInsideKeyButtonPressed;

	protected:
		virtual void initialiseOverride();
		virtual void shutdownOverride();

		virtual void setPropertyOverride(const std::string& _key, const std::string& _value);

	private:
		void parseParagraph(Widget* _parent, const std::string& _value);
		void parseText(Widget* _parent, const std::string& _value);
		void parseTag(const std::string& _value);

		void OnTextLostFocus(Widget* _sender, Widget* _new);
		void OnTextSetFocus(Widget* _sender, Widget* _old);
		void OnTextClick(Widget* _sender);
		void OnMouseButtonReleased(Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
		void notifyKeyButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _ch);
		void updateBreakHeight();

	private:
		bool mBold;
		bool mItalic;
		bool mStrike;
		bool mUnder;
		bool mImage;
		bool mHeader1;
		bool mHeader2;
		bool mHeader3;
		bool mColour;
		bool mUrl;
		bool mTextSelectable;
		std::string mSelectedText;
		Colour mColourValue;
		IntSize mImageSize;
		IntSize mSpacer;
		std::string mTextSkin;
		std::string mParagraphSkin;
		std::string mImageSkin;
		std::string mLineSkin;
		std::string mUrlValue;
		std::string mLinkPoiner;

		std::string mHeader1Font;
		std::string mHeader2Font;
		std::string mHeader3Font;
		std::string mDefaultFont;
		std::string mBoldFont;
		std::string mItalicFont;
		std::string mBoldItalicFont;

		StackPanel* mStackPanel;
		ScrollViewPanel* mScrollViewPanel;
		WrapPanel* mCurrentWrapPanel;

		int mBreakHeight;
		MyGUI::Colour mUrlColour;
	};

} // namespace MyGUI

#endif // MYGUI_HYPER_TEXT_BOX_H_
