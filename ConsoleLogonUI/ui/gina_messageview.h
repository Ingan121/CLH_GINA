#pragma once
#include "gina_manager.h"
#include "gina_messageview.h"
#include <string>

struct MessageOptionControlWrapper
{
    void* actualInstance;
    int optionflag;

    void Press();

    std::wstring GetText();
};