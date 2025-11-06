#include "../include/ButtonUtils.hpp"
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/EditButtonBar.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/binding/GameToolbox.hpp>
#include <Geode/binding/Slider.hpp>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI)
{
	struct Fields
	{
		bool isSelected = false;
		CCMenuItemSpriteExtra *button = nullptr;
		EditButtonBar *editButtonBar = nullptr;
		int m_lastSelectedIndex = -1;
	};

	bool init(LevelEditorLayer *editor)
	{
		if (!EditorUI::init(editor))
			return false;

		auto editBar = this->getChildByIDRecursive("edit-tab-bar");
		if (!editBar)
			return true;

		m_fields->editButtonBar = static_cast<EditButtonBar *>(editBar);
		if (!m_fields->editButtonBar)
			return true;

		auto icon = CCSprite::create("adv-timewarp.png"_spr);
		if (!icon)
			return true;

		auto btnSprite = ButtonSprite::create(
			icon,
			32,
			0,
			32,
			0.6f,
			true,
			"GJ_button_01.png",
			false);

		m_fields->button = CCMenuItemSpriteExtra::create(
			btnSprite,
			this,
			menu_selector(MyEditorUI::onAdvTimeWarpPressed));

		m_fields->button->setID("adv-timewarp-button");
		m_fields->editButtonBar->m_buttonArray->insertObject(m_fields->button, 32);
		m_fields->editButtonBar->updateLayout();

		return true;
	}

	void onAdvTimeWarpPressed(CCObject *sender)
	{
		class TimeWarpPopup : public geode::Popup<>
		{
		protected:
			TextInput *m_fromInput = nullptr;
			TextInput *m_toInput = nullptr;
			TextInput *m_durationInput = nullptr;
			Slider *m_fromSlider = nullptr;
			Slider *m_toSlider = nullptr;
			Slider *m_durationSlider = nullptr;
			CCMenuItemToggler *m_easingMenu = nullptr;
			float m_fromTimeMod = 1.0f;
			float m_toTimeMod = 1.0f;
			float m_duration = 0.5f;
			LevelEditorLayer *m_editor = nullptr;
			EditorUI *m_owner = nullptr;
			CCMenuItemSpriteExtra *rightEasingButton = nullptr;
			CCMenuItemSpriteExtra *leftEasingButton = nullptr;
			int m_currentEasingIndex = 0;
			int m_speed = 1;
			CCLabelBMFont *m_easingLabel = nullptr;
			CCLabelBMFont *m_easingChoice = nullptr;
			CCMenu *speedMenu = CCMenu::create();
			std::vector<std::string> easings = {
				"None",
				"Ease In Out",
				"Ease In",
				"Ease Out",
				"Exponential In Out",
				"Exponential In",
				"Exponential Out",
				"Sine In Out",
				"Sine In",
				"Sine Out",
			};
			const float TIME_MOD_MIN = 0.1f;
			const float TIME_MOD_MAX = 2.0f;
			const float DURATION_MAX = 10.0f;
			int stepsDenominator = 2;

			CCNode *createInputRow(
				const std::string &labelText,
				float initialValue,
				TextInput **outInput,
				std::function<void(std::string const &)> inputCallback)
			{
				auto row = CCNode::create();

				// Label
				auto label = CCLabelBMFont::create(labelText.c_str(), "bigFont.fnt");
				label->setScale(0.4f);
				label->setAnchorPoint({0.5f, 0.5f});
				label->setPosition({0, 0});
				row->addChild(label);

				// TextInput
				*outInput = TextInput::create(40.f, fmt::format("{:.2f}", initialValue).c_str(), "bigFont.fnt");
				(*outInput)->setFilter("0123456789.");
				(*outInput)->setScale(0.6f);
				(*outInput)->setAnchorPoint({0, 0.5f});
				(*outInput)->setPosition({label->getScaledContentSize().width / 2 + 5.f, 0.f});
				(*outInput)->setCallback(inputCallback);
				(*outInput)->setString(fmt::format("{:.2f}", initialValue));
				row->addChild(*outInput);

				return row;
			}

			bool setup() override
			{

				this->setTitle("Advanced TimeWarp");
				this->m_closeBtn->removeMeAndCleanup();

				auto root = CCNode::create();
				m_mainLayer->addChildAtPosition(root, Anchor::Center);

				// Help Button
				auto helpSprite =
					CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
				helpSprite->setScale(0.7f);
				auto helpButton = CCMenuItemSpriteExtra::create(
					helpSprite, this, menu_selector(TimeWarpPopup::onInfoClicked));

				// auto helpMenu = CCMenu::create(helpButton, nullptr);
				auto helpMenu = CCMenu::createWithItem(helpButton);
				m_mainLayer->addChildAtPosition(helpMenu, Anchor::TopRight, {-15.f, -15.f});

				// FROM section (left)
				// Row Init
				CCNode *fromRow = createInputRow(
					"From TimeMod:", // label
					1.0f,			 // Initial value
					&m_fromInput,	 // pointer to TextInput* member
					[this](const std::string &text)
					{ this->onFromInputChanged(text); });
				fromRow->setPosition({-80.f, 70.f});
				root->addChild(fromRow);

				// Slider
				m_fromSlider = Slider::create(this, menu_selector(TimeWarpPopup::onFromSliderChanged), 0.5f);
				// slider below the row
				m_fromSlider->setPosition({0.f, -20.f});
				fromRow->addChild(m_fromSlider);
				onFromInputChanged("1.0");

				// TO section (right)
				// Row Init
				CCNode *toRow = createInputRow(
					"To TimeMod:",
					1.0f,
					&m_toInput,
					[this](const std::string &text)
					{ this->onToInputChanged(text); });
				toRow->setPosition({80.f, 70.f});
				root->addChild(toRow);

				// Slider
				m_toSlider = Slider::create(this, menu_selector(TimeWarpPopup::onToSliderChanged), 0.5f);
				m_toSlider->setPosition({0.f, -20.f});
				toRow->addChild(m_toSlider);
				onToInputChanged("1.0");

				// Duration section
				// Row Init
				CCNode *durationRow = createInputRow(
					"Duration:",
					0.5f,
					&m_durationInput,
					[this](const std::string &text)
					{ this->onDurationInputChanged(text); });
				durationRow->setPosition({-80.f, 20.f});
				root->addChild(durationRow);

				// Slider
				m_durationSlider = Slider::create(this, menu_selector(TimeWarpPopup::onDurationSliderChanged), 0.5f);
				m_durationSlider->setPosition({0.f, -20.f});
				durationRow->addChild(m_durationSlider);
				onDurationInputChanged("0.5");

				// Easing section
				// Row Init
				auto easingRow = CCNode::create();
				easingRow->setPosition({80.f, 20.f});
				root->addChild(easingRow);

				// Label
				m_easingLabel = CCLabelBMFont::create("Easing:", "bigFont.fnt");
				m_easingLabel->setScale(0.4f);
				m_easingLabel->setAnchorPoint({0.5f, 0.5f});
				m_easingLabel->setPosition({0.f, 0.f});
				easingRow->addChild(m_easingLabel);

				// Left arrow
				auto leftArrowSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
				leftArrowSprite->setScale(0.6f);
				auto leftEasingButton = CCMenuItemSpriteExtra::create(
					leftArrowSprite, // normal
					nullptr,
					this,
					menu_selector(TimeWarpPopup::onLeftEasingPressed));

				// Right arrow
				auto rightArrowSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
				rightArrowSprite->setScale(0.6f);
				rightArrowSprite->setFlipX(true);
				auto rightEasingButton = CCMenuItemSpriteExtra::create(
					rightArrowSprite,
					nullptr,
					this,
					menu_selector(TimeWarpPopup::onRightEasingPressed));

				// Wrap in a CCMenu
				// auto easingMenu = CCMenu::create(leftEasingButton, rightEasingButton, nullptr);
				auto easingArr = CCArray::create();
				easingArr->addObject(leftEasingButton);
				easingArr->addObject(rightEasingButton);
				auto easingMenu = CCMenu::createWithArray(easingArr);

				easingMenu->setPosition({0.f, -20.f});
				easingRow->addChild(easingMenu);

				// Position buttons relative to menu center
				leftEasingButton->setPosition({-60.f, 0.f});
				rightEasingButton->setPosition({60.f, 0.f});

				// Easing value label
				m_easingChoice = CCLabelBMFont::create(easings[m_currentEasingIndex].c_str(), "bigFont.fnt");
				m_easingChoice->setScale(0.3f);
				m_easingChoice->setAnchorPoint({0.5f, 0.5f});
				m_easingChoice->setPosition({0.f, -20.f});
				easingRow->addChild(m_easingChoice);

				// Speed Change
				speedMenu->setID("select-speed-menu");
				speedMenu->setContentWidth(300.f);
				speedMenu->setLayout(RowLayout::create());

				// Speed Change Label
				auto m_speedLabel = CCLabelBMFont::create("Speed:", "bigFont.fnt");
				m_speedLabel->setScale(0.4f);
				m_speedLabel->setAnchorPoint({0.5f, 0.5f});
				m_speedLabel->setPosition({0.f, -45.f});
				speedMenu->addChild(m_speedLabel);

				// Add speed buttons
				auto addBtn = [&](int tag, const char *sprite, bool disabled)
				{
					auto spr = CCSprite::createWithSpriteFrameName(sprite);
					CCMenuItemSpriteExtra *btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(TimeWarpPopup::onSpeedChanged));

					disabled ? ButtonUtils::enableButton(btn, false, true) : ButtonUtils::enableButton(btn, true, true);
					btn->setTag(tag);
					speedMenu->addChild(btn);
					btn->setScale(0.2f);
					return btn;
				};

				// Add buttons
				addBtn(0, "boost_01_001.png", true);
				addBtn(1, "boost_02_001.png", false);
				addBtn(2, "boost_03_001.png", true);
				addBtn(3, "boost_04_001.png", true);
				addBtn(4, "boost_05_001.png", true);

				speedMenu->updateLayout(false);
				speedMenu->setScale(0.8f);
				m_mainLayer->addChildAtPosition(speedMenu, Anchor::Center, {0, -55}, false);

				// Confirmation Buttons (center-bottom)
				auto buttonMenu = CCMenu::create();
				buttonMenu->setPosition({0.f, -105.f});
				root->addChild(buttonMenu);

				auto closeBtn = CCMenuItemSpriteExtra::create(
					ButtonSprite::create("Close", "goldFont.fnt", "GJ_button_01.png", 0.8f),
					this, menu_selector(TimeWarpPopup::onClose));
				closeBtn->setPositionX(-37.f);

				auto okBtn = CCMenuItemSpriteExtra::create(
					ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", 0.8f),
					this, menu_selector(TimeWarpPopup::onConfirm));
				okBtn->setPositionX(37.f);

				auto reduceStepsCheckbox = CCMenuItemToggler::createWithStandardSprites(
					this,
					menu_selector(TimeWarpPopup::onReduceStepsToggled),
					0.6f);
				reduceStepsCheckbox->setPositionX(80.f);
				buttonMenu->addChild(reduceStepsCheckbox);

				auto reduceStepsCheckboxLabel = CCLabelBMFont::create("Reduce\nTrigger Count", "bigFont.fnt");
				reduceStepsCheckboxLabel->setScale(0.2f);
				reduceStepsCheckboxLabel->setAnchorPoint({0.f, 0.5f});
				reduceStepsCheckboxLabel->setPosition({30.f, 17.5f});
				reduceStepsCheckbox->addChild(reduceStepsCheckboxLabel);

				buttonMenu->addChild(closeBtn);
				buttonMenu->addChild(okBtn);

				return true;
			}

			void onFromSliderChanged(CCObject *sender)
			{
				m_fromTimeMod = m_fromSlider->getValue(); // gets 0..1 normalized
				float fromMapped = TIME_MOD_MIN + m_fromTimeMod * (TIME_MOD_MAX - TIME_MOD_MIN);
				m_fromInput->setString(fmt::format("{:.2f}", fromMapped).c_str());
			}

			void onToSliderChanged(CCObject *sender)
			{
				m_toTimeMod = m_toSlider->getValue();
				float toMapped = TIME_MOD_MIN + m_toTimeMod * (TIME_MOD_MAX - TIME_MOD_MIN);
				m_toInput->setString(fmt::format("{:.2f}", toMapped).c_str());
			}

			void onDurationSliderChanged(CCObject *sender)
			{
				m_duration = m_durationSlider->getValue();
				float durationMapped = TIME_MOD_MIN + m_duration * (TIME_MOD_MAX - TIME_MOD_MIN);
				m_durationInput->setString(fmt::format("{:.2f}", durationMapped).c_str());
			}

			void onFromInputChanged(std::string const &text)
			{
				float value = std::clamp(safe_stof(text, 1.0f), TIME_MOD_MIN, TIME_MOD_MAX);
				m_fromTimeMod = value;
				m_fromSlider->setValue((value - TIME_MOD_MIN) / (TIME_MOD_MAX - TIME_MOD_MIN));
			}

			void onToInputChanged(std::string const &text)
			{
				float value = std::clamp(safe_stof(text, 1.0f), TIME_MOD_MIN, TIME_MOD_MAX);
				m_toTimeMod = value;
				m_toSlider->setValue((value - TIME_MOD_MIN) / (TIME_MOD_MAX - TIME_MOD_MIN));
			}

			void onDurationInputChanged(std::string const &text)
			{
				float value = std::clamp(safe_stof(text, 0.5f), 0.f, DURATION_MAX);
				m_duration = value;

				float sliderValue = std::clamp(value, TIME_MOD_MIN, TIME_MOD_MAX) / TIME_MOD_MAX;
				m_durationSlider->setValue(sliderValue);
			}

			void onLeftEasingPressed(CCObject *sender)
			{
				if (easings.empty())
					return;
				if (m_currentEasingIndex == 0)
					m_currentEasingIndex = easings.size() - 1;
				else
					m_currentEasingIndex--;

				m_easingChoice->setString(easings[m_currentEasingIndex].c_str(), true);
			}

			void onRightEasingPressed(CCObject *sender)
			{
				if (easings.empty())
					return;
				m_currentEasingIndex++;
				if (m_currentEasingIndex >= easings.size())
					m_currentEasingIndex = 0;

				m_easingChoice->setString(easings[m_currentEasingIndex].c_str(), true);
			}

			void toggleButtonVisually(bool isEnabled, CCMenuItemSpriteExtra *btn)
			{
				isEnabled ? ButtonUtils::enableButton(btn, true, true) : ButtonUtils::enableButton(btn, false, true);
			}

			void onReduceStepsToggled(CCObject *sender)
			{
				stepsDenominator = static_cast<CCMenuItemToggler *>(sender)->isToggled() ? 2 : 3;
			}

			void onSpeedChanged(CCObject *sender)
			{
				auto btn = static_cast<CCMenuItemSpriteExtra *>(sender);
				int speed = btn->getTag();
				m_speed = speed;

				// Disable all buttons
				for (int i = 0; i <= 4; i++)
					if (auto btn = dynamic_cast<CCMenuItemSpriteExtra *>(speedMenu->getChildByTag(i)))
						toggleButtonVisually(false, btn);

				// Re-enable the selected button
				if (auto btn = dynamic_cast<CCMenuItemSpriteExtra *>(speedMenu->getChildByTag(m_speed)))
					toggleButtonVisually(true, btn);
			}

			void onInfoClicked(CCObject *)
			{
				// Show explanation popup when "i" clicked
				auto alert = FLAlertLayer::create(
					nullptr,
					"Advanced TimeWarp Help",
					"- <cy>Advanced TimeWarp</c> places <cg>multiple TimeWarp triggers</c> to "
					"simulate smooth easing between time modifications.\n"
					"- <co>From TimeMod</c> and <co>To TimeMod</c> set the start and end speed of the timewarp.\n"
					"- <cr>Duration</c> controls how long the easing transition lasts.\n"
					"- <cp>Easing</c> applies <cg>Geometry Dash easing equations</c> to the speed change.\n"
					"- <cj>Speed</c> adjusts the duration based on player speed.\n"
					"- <cb>Reduce Trigger Count</c> decreases the number of generated TimeWarp triggers.",
					"OK",
					nullptr,
					450.f);
				alert->show();
			}

			float safe_stof(const std::string &text, float defaultValue = 1.0f)
			{
				if (text.empty())
					return defaultValue;

				try
				{
					std::string trimmed = text;
					trimmed.erase(std::remove_if(trimmed.begin(), trimmed.end(), ::isspace), trimmed.end());
					return std::stof(trimmed);
				}
				catch (const std::exception &)
				{
					return defaultValue;
				}
			}

			void onClosePopup()
			{
				this->removeFromParentAndCleanup(true);
			}

			float clampFloat(float val, float minVal, float maxVal)
			{
				return std::max(minVal, std::min(maxVal, val));
			}

			void onConfirm(CCObject *sender)
			{
				if (!m_editor)
				{
					this->onClosePopup();
					return;
				}

				// Get user values
				float from = safe_stof(m_fromInput->getString(), 1.0f);
				float to = safe_stof(m_toInput->getString(), 1.0f);
				float duration = safe_stof(m_durationInput->getString(), 0.5f);

				from = clampFloat(from, TIME_MOD_MIN, TIME_MOD_MAX);
				to = clampFloat(to, TIME_MOD_MIN, TIME_MOD_MAX);
				duration = std::max(TIME_MOD_MIN, duration);

				float unitsPerSecond = 312.f; // default (normal speed)

				switch (m_speed)
				{
				case 0:
					unitsPerSecond = 251.7f;
					break;
				case 1:
					unitsPerSecond = 312.f;
					break;
				case 2:
					unitsPerSecond = 388.f;
					break;
				case 3:
					unitsPerSecond = 468.5f;
					break;
				case 4:
					unitsPerSecond = 576.5f;
					break;
				}

				float minSpacing = 3.f; // minimum distance between triggers
				float totalDistance = unitsPerSecond * duration;
				int estimatedSteps = static_cast<int>(totalDistance / minSpacing);

				// Clamp steps to a reasonable range
				int minSteps = 10;
				int maxSteps = 50;
				int steps = std::clamp(estimatedSteps, minSteps, maxSteps);

				// Reduce slightly for fewer objects. stepsDenominator reduces that even more by enabling the checkbox.
				steps = std::max(steps / stepsDenominator, minSteps);

				// recalc spacing based on clamped step count
				float delta = (to - from) / steps;

				auto objLayer = m_editor->m_objectLayer;
				CCPoint cameraPos = (CCDirector::get()->getWinSize() / 2.f - objLayer->getPosition()) / objLayer->getScale();

				auto snapToGrid = [](float value, float grid = 30.f)
				{
					int pos = static_cast<int>(std::round(value / grid));
					return pos * grid + 15;
				};

				float baseX = snapToGrid(cameraPos.x);
				const float baseY = snapToGrid(cameraPos.y);

				// Select easing
				const std::string &easing = easings[m_currentEasingIndex];

				// Create triggers
				float lastTimeMod = -1.f;
				CCArray *createdTriggers = CCArray::create();
				createdTriggers->retain();

				for (int i = 0; i <= steps; i++)
				{
					float t = float(i) / steps; // linear progress 0..1
					float easedT = applyEasing(t, easings[m_currentEasingIndex]);

					float x = baseX + totalDistance * easedT;
					float currentTimeMod = from + (to - from) * t;
					float roundedTimeMod = std::round(currentTimeMod * 100.f) / 100.f;

					if (std::fabs(roundedTimeMod - lastTimeMod) < 0.001f)
						continue;
					lastTimeMod = roundedTimeMod;

					auto trigger = static_cast<EffectGameObject *>(m_editor->createObject(1935, {x, baseY}, false));
					if (trigger)
					{
						trigger->m_timeWarpTimeMod = roundedTimeMod;
						createdTriggers->addObject(trigger);
					}
				}

				m_owner->deselectAll();
				m_owner->selectObjects(createdTriggers, true);
				m_owner->updateObjectInfoLabel();

				if (easings[m_currentEasingIndex] == "None")
					m_owner->alignObjects(createdTriggers, false);

				this->onClosePopup();
			}

			// Credits to HDanke for the equations on GitHub
			// Apply easing functions on the timewarp placements
			float applyEasing(float t, const std::string &easing)
			{
				using namespace std;
				constexpr float PI = 3.14159265f;

				if (easing == "None")
					return t;

				if (easing == "Ease In Out")
					return t < 0.5f
							   ? 0.5f * (1.f - (1.f - 2.f * t) * (1.f - 2.f * t))
							   : 0.5f * (2.f * t - 1.f) * (2.f * t - 1.f) + 0.5f;
				if (easing == "Ease Out")
					return t * t;
				if (easing == "Ease In")
					return t * (2.f - t);

				if (easing == "Exponential In Out")
					return t == 0.f || t == 1.f
							   ? t
							   : (t < 0.5f
									  ? 0.5f * (1.f - pow(2.f, -20.f * t))
									  : 0.5f * pow(2.f, 20.f * t - 20.f) + 0.5f);

				if (easing == "Exponential Out")
					return t == 0 ? 0 : pow(2, 10 * (t - 1));
				if (easing == "Exponential In")
					return t == 1 ? 1 : 1 - pow(2, -10 * t);

				if (easing == "Sine In Out")
					return t < 0.5f
							   ? 0.5f * sin(PI * t)
							   : 1.f - 0.5f * sin(PI * t);
				if (easing == "Sine Out")
					return -cos(PI / 2 * t) + 1;
				if (easing == "Sine In")
					return sin(PI / 2 * t);

				return t; // fallback
			}

		public:
			static TimeWarpPopup *create(LevelEditorLayer *editor, EditorUI *owner)
			{
				auto ret = new TimeWarpPopup();
				if (ret->initAnchored(340.f, 260.f))
				{
					ret->m_editor = editor;
					ret->m_owner = owner;
					ret->autorelease();
					return ret;
				}
				delete ret;
				return nullptr;
			}
		};

		TimeWarpPopup::create(this->m_editorLayer, this)->show();
	}
};