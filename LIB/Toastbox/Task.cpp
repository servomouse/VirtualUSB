
#include "Task.h"

// #warning temporary implementation!
// uint32_t Task:: TimeMs(void)
// {
//     return 10;
// }

template <typename T, size_t N>
[[noreturn]] void Task::Run(T (&tasks)[N])
{
    while(1)
    {
        bool didWork = false;
        do {
            _IRQ.disable();
            didWork = false;
            for (Task& task : tasks) {
                didWork |= task.run();
            }
        } while (didWork);
        IRQState::WaitForInterrupt();
        _IRQ.restore();
    }
}
template <typename ...T>
[[noreturn]] void Task::Run(T&... ts)
{
    std::reference_wrapper<Task> tasks[] = { static_cast<Task&>(ts)... };
    Run(tasks);
}

Task::Task(std::function<void(void)> fn) : _fn(fn)
{
    _CurrentTask = nullptr;
    _state = State::Run;
    _didWork = false;
    _jmp = nullptr;
    _sleepStartMs = 0;
    _sleepDurationMs = 0;
}

void Task::start()
{
    _state = State::Run;
    _jmp = nullptr;
}
    
void Task::pause()
{
    _state = State::Stop;
}
    
void Task::resume()
{
    _state = State::Run;
}
    
bool Task::run()
{
    // Run the task
    Task*const prevTask = _CurrentTask;
    _CurrentTask = this;
    _didWork = false;
    switch (_state) {
    case State::Run:
    case State::Wait:
        _fn();
        break;
    default:
        break;
    }
    
    switch (_state) {
    case State::Run:
        // The task terminated if it returns in the 'Run' state, so update its state
        _state = State::Stop;
        _jmp = nullptr;
        break;
    default:
        break;
    }
    
    _CurrentTask = prevTask;
    return _didWork;
}
    
void Task::_setSleeping(uint32_t ms)
{
    _state = Task::State::Wait;
    _sleepStartMs = Task::TimeMs();
    _sleepDurationMs = ms;
}
    
void Task::_setWaiting()
{
    _state = Task::State::Wait;
}

void Task::_setRunning()
{
    _state = Task::State::Run;
    _didWork = true;
    _IRQ.restore();
}

bool Task::_sleepDone() const
{
    return (TimeMs()-_sleepStartMs) >= _sleepDurationMs;
}

