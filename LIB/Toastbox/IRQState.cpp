#include "IRQState.h"
#include <stdio.h>
#include <cstdlib>  // for abort() function

static bool int_enabled = false;

#warning temporary implementation
bool IRQState::SetInterruptsEnabled(bool en)
{
    int_enabled = en;
    return int_enabled;
}

#warning temporary implementation
void IRQState::WaitForInterrupt(void)
{
    return;
}
    
IRQState IRQState::Enabled(void)
{
    IRQState irq;
    irq.enable();
    return irq;
}

IRQState IRQState::Disabled(void)
{
    IRQState irq;
    irq.disable();
    return irq;
}

IRQState::~IRQState()
{
    restore();
}

void IRQState::enable()
{
    _Assert(!_prevEnValid);
    _prevEn = SetInterruptsEnabled(true);
    _prevEnValid = true;
}

void IRQState::disable()
{
    _Assert(!_prevEnValid);
    _prevEn = SetInterruptsEnabled(false);
    _prevEnValid = true;
}

void IRQState::restore()
{
    if (_prevEnValid) {
        SetInterruptsEnabled(_prevEn);
        _prevEnValid = false;
    }
}

void IRQState::_Assert(bool cond)
{
    if (!cond)
        std::abort();
}
    

