#pragma once
#include <memory>

namespace Toastbox {

template <typename Fn>
class DeferFn {
public:
    DeferFn(Fn&& fn) : _fn(std::move(fn)) {}
    ~DeferFn() { _fn(); }
private:
    Fn _fn;
};

template <typename F>
DeferFn<F> Defer(F&& f) {
    return DeferFn<F>(std::move(f));
};

} // namespace Toastbox

#define _DeferConcat2(x, y) x ## y
#define _DeferConcat(x, y) _DeferConcat2(x, y)
#define Defer(action) auto _DeferConcat(defer, __COUNTER__) = Toastbox::Defer([&](){ action; });
