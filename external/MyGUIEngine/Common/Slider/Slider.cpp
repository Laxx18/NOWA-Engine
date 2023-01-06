#include "MyGUI_Precompiled.h"
#include "Slider.h"
#include "MyGUI_InputManager.h"
#include "MyGUI_Button.h"
#include "MyGUI_ResourceSkin.h"
#include "MyGUI_ImageBox.h"

namespace MyGUI
{

	Slider::Slider() :
		mWidgetStart(nullptr),
		mWidgetEnd(nullptr),
		mWidgetTrack(nullptr),
		mTextBoxCurrentValue(nullptr),
		mfValue(0.f),
		mfMinRange(0.f),
		mfMaxRange(1.f)
	{
	}

	// Init
	//-----------------------------------------------------------------------------------------------
	void Slider::initialiseOverride()
	{
		Base::initialiseOverride();

		eventMouseButtonPressed += newDelegate(this, &Slider::notifyMouse);
		eventMouseDrag += newDelegate(this, &Slider::notifyMouse);
		eventMouseWheel += newDelegate(this, &Slider::notifyMouseWheel);

		// assignWidget(mWidgetTrack, "Track");
		// <Widget type = "Button" skin = "SliderTrackHSkin" position = "35 1 21 14" align = "Left VStretch" name = "Track" / >
		mWidgetTrack = this->createWidget<MyGUI::ImageBox>("ScrollTrackHSkin", MyGUI::IntCoord(35, 1, 21, 14), MyGUI::Align::Left | MyGUI::Align::VStretch);
		if (mWidgetTrack != nullptr)
		{
			mWidgetTrack->eventMouseDrag += newDelegate(this, &Slider::notifyMouse);
			mWidgetTrack->eventMouseButtonPressed += newDelegate(this, &Slider::notifyMouse);
			mWidgetTrack->eventMouseWheel += newDelegate(this, &Slider::notifyMouseWheel);
			mWidgetTrack->setVisible(false);
		}

		// assignWidget(mWidgetStart, "Start");
		// <Widget type = "Button" skin = "ButtonLeftSkin" position = "2 1 12 14" align = "Left VCenter" name = "Start" / >
		mWidgetStart = this->createWidget<MyGUI::Button>("ButtonLeftSkin", MyGUI::IntCoord(2, 1, 12, 14), MyGUI::Align::Left | MyGUI::Align::VStretch);
		if (mWidgetStart != nullptr)
		{
			mWidgetStart->eventMouseButtonPressed += newDelegate(this, &Slider::notifyMouse);
			mWidgetStart->eventMouseWheel += newDelegate(this, &Slider::notifyMouseWheel);
		}
		// assignWidget(mWidgetEnd, "End");
		// <Widget type = "Button" skin = "ButtonRightSkin" position = "112 1 12 14" align = "Right VCenter" name = "End" / >
		mWidgetEnd = this->createWidget<MyGUI::Button>("ButtonRightSkin", MyGUI::IntCoord(112, 1, 12, 14), MyGUI::Align::Right | MyGUI::Align::VStretch);
		if (mWidgetEnd != nullptr)
		{
			mWidgetEnd->eventMouseButtonPressed += newDelegate(this, &Slider::notifyMouse);
			mWidgetEnd->eventMouseWheel += newDelegate(this, &Slider::notifyMouseWheel);
		}
		// assignWidget(mTextBoxCurrentValue, "CurrentValue");
		// <Widget type = "Widget" skin = "TextBoxSkin" position = "20 1 60 14" align = "Center" name = "CurrentValue" / >
		mTextBoxCurrentValue = this->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(20, 1, 60, 14), MyGUI::Align::Center);
	}

	void Slider::shutdownOverride()
	{
		mWidgetStart = nullptr;
		mWidgetEnd = nullptr;
		mWidgetTrack = nullptr;
		mTextBoxCurrentValue = nullptr;

		Base::shutdownOverride();
	}

	void Slider::updateTrack()
	{
		if (mWidgetTrack == nullptr)
			return;

		_forcePick(mWidgetTrack);

		if (!mWidgetTrack->getVisible())
			mWidgetTrack->setVisible(true);

		int iTrack = mWidgetTrack->getSize().width;
		int iSize = mWidgetTrack->getParent()->getSize().width - iTrack;

		int pos = mfValue * iSize;

		mWidgetTrack->setPosition(pos, mWidgetTrack->getTop());
		mTextBoxCurrentValue->setCaption(std::to_string(mfValue));
	}

	/// mouse button
	//-----------------------------------------------------------------------------------------------
	void Slider::notifyMouse(Widget* _sender, int _left, int _top, MouseButton _id)
	{
		//eventMouseButtonPressed(this, _left, _top, _id);
		//if (MouseButton::Left != _id)
		//	return;

		const IntPoint& p = mWidgetTrack->getParent()->getAbsolutePosition();
		const IntSize& s = mWidgetTrack->getParent()->getSize();
		int iTrack = mWidgetTrack->getSize().width;

		float fx = (_left - iTrack / 2 - p.left) / float(s.width - iTrack);
		//float fy = (_top - p.top) / float(s.height);
		fx = std::max(mfMinRange, std::min(mfMaxRange, fx));

		mfValue = fx;
		eventValueChanged(this, mfValue);
		updateTrack();
	}


	float Slider::getValue() const
	{
		return mfValue;
	}

	void Slider::setValue(float val)
	{
		//if (mValue == val)
		//	return;

		mfValue = val; //std::max(0.f, std::min(1.f, val));
		updateTrack();
	}

	void Slider::setMinRange(float minRange)
	{
		mfMinRange = minRange;
	}

	void Slider::setMaxRange(float maxRange)
	{
		mfMaxRange = maxRange;
	}

	// mouse wheel
	//--------------------------------------------------------------------------------
	void Slider::onMouseWheel(int _rel)
	{
		notifyMouseWheel(nullptr, _rel);

		Base::onMouseWheel(_rel);
	}

	void Slider::notifyMouseWheel(Widget* _sender, int _rel)
	{
		mfValue = std::max(mfMinRange, std::min(mfMaxRange, mfValue - _rel / 120.f*0.5f * 0.125f));
		eventValueChanged(this, mfValue);
		updateTrack();
	}

	void Slider::setPropertyOverride(const std::string& _key, const std::string& _value)
	{
		if (_key == "Value")
			setValue(utility::parseValue<float>(_value));
		else
		{
			Base::setPropertyOverride(_key, _value);
			return;
		}
		eventChangeProperty(this, _key, _value);
	}


	// update track on resize
	void Slider::setSize(int _width, int _height)
	{
		setSize(IntSize(_width, _height));
	}

	void Slider::setCoord(int _left, int _top, int _width, int _height)
	{
		setCoord(IntCoord(_left, _top, _width, _height));
	}

	void Slider::setSize(const IntSize& _size)
	{
		Base::setSize(_size);
		updateTrack();
	}

	void Slider::setCoord(const IntCoord& _coord)
	{
		Base::setCoord(_coord);
		updateTrack();
	}

}
