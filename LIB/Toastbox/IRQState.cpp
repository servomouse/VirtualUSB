#include "IRQState.h"
#include <stdio.h>
#include <cstdlib>  // for abort() function

IRQState IRQState::get_object(void)
{
    IRQState a;
    return a;
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
    

