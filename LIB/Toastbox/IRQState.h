#pragma once

class IRQState {
public:
    // Functions provided by client
    static bool SetInterruptsEnabled(bool en);
    static void WaitForInterrupt();
    
    static IRQState Enabled(void);
    
    static IRQState Disabled(void);
    
    IRQState()                  = default;  // use the compiler-generated version of that function
    IRQState(const IRQState& x) = delete;   // I don't want the compiler to generate that function automatically
    IRQState(IRQState&& x)      = default;
    
    ~IRQState();
    
    void enable();
    
    void disable();
    
    void restore();
    
private:
    static void _Assert(bool cond);
    
    bool _prevEn = false;
    bool _prevEnValid = false;
};
