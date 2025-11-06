#pragma once
#include <Geode/Geode.hpp>
using namespace geode::prelude;

// Utility functions for enabling/disabling buttons and toggles
// Credits to HJfod
namespace ButtonUtils
{

    inline void enableButton(CCMenuItemSpriteExtra *btn, bool enabled, bool visualOnly)
    {
        btn->setEnabled(enabled || visualOnly);
        if (auto spr = typeinfo_cast<CCRGBAProtocol *>(btn->getNormalImage()))
        {
            spr->setCascadeColorEnabled(true);
            spr->setCascadeOpacityEnabled(true);
            spr->setColor(enabled ? ccWHITE : ccGRAY);
            spr->setOpacity(enabled ? 255 : 200);
        }
    }

    inline void enableToggle(CCMenuItemToggler *toggle, bool enabled, bool visualOnly)
    {
        toggle->setEnabled(enabled || visualOnly);
        enableButton(toggle->m_onButton, enabled, visualOnly);
        enableButton(toggle->m_offButton, enabled, visualOnly);
    }

}
