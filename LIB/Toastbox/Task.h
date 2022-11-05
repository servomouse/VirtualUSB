#pragma once
#include <functional>
#include <stdint.h>
#include "IRQState.h"

#define TaskBegin()                         \
    Task& _task = (*Task::_CurrentTask);    \
    if (_task._jmp) goto *_task._jmp;       \
    _task._setRunning();

#define TaskYield() ({                      \
    _task._setWaiting();                    \
    _TaskYield();                           \
    _task._setRunning();                    \
})

#define TaskWait(cond) ({                   \
    _task._setWaiting();                    \
    decltype(cond) c;                       \
    while (!(c = (cond))) _TaskYield();     \
    _task._setRunning();                    \
    c;                                      \
})

#define TaskSleepMs(ms) ({                  \
    _task._setSleeping(ms);                 \
    do _TaskYield();                        \
    while (!_task._sleepDone());            \
    _task._setRunning();                    \
})

#define _TaskYield() ({                     \
    __label__ jmp;                          \
    _task._jmp = &&jmp;                     \
    return;                                 \
    jmp:;                                   \
})

class Task
{
public:
    // Functions provided by client
    static uint32_t TimeMs();
    
    enum class State {
        Run,
        Wait,
        Stop,
    };
    
    // using TaskFn = std::function<void(void)>;
    
    template <typename T, size_t N>
    [[noreturn]] static void Run(T (&tasks)[N]);
    
    template <typename ...T>
    [[noreturn]] static void Run(T&... ts);
    
    Task(std::function<void(void)> fn);
    
    void start();
    
    void pause();
    
    void resume();
    
    bool run();
    
    void _setSleeping(uint32_t ms);
    
    void _setWaiting();
    
    void _setRunning();
    
    bool _sleepDone() const;
    
    static inline IRQState _IRQ;
    std::function<void(void)> _fn;
    static inline Task* _CurrentTask;
    State _state;
    bool _didWork;
    void* _jmp;
    uint32_t _sleepStartMs;
    uint32_t _sleepDurationMs;
};
