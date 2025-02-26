#pragma once
#include "gina_manager.h"
#include "gina_messageview.h"
#include <string>

#define GINA_STR_LOGON_MESSAGE_TITLE 1501

struct MessageOptionControlWrapper
{
    void* actualInstance;
    int optionflag;

    void Press();

    std::wstring GetText();
};