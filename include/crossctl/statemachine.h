#ifndef CROSSCTL_STATEMACHINE_H
#define CROSSCTL_STATEMACHINE_H

#include <cstdint>

namespace crossctl {

enum class State : uint8_t {
    Idle = 0,
    Approaching,
    Closing,
    Closed,
    Opening,
    Fault
};

enum class Event : uint8_t {
    TrainDetected = 0,
    TrainCleared,
    BarrierDownConfirmed,
    BarrierUpConfirmed,
    Tick,
    Reset
};

enum class Barrier : uint8_t { Raise = 0, Lower, Hold };
enum class Signal  : uint8_t { Danger = 0, Clear };

struct Output {
    Barrier barrier;
    Signal  signal;
};

const char *to_string(State s);
const char *to_string(Event e);
const char *to_string(Barrier b);
const char *to_string(Signal s);

class StateMachine {
public:
    explicit StateMachine(uint32_t move_timeout_ticks = 10);

    void  handle(Event e);
    State state() const { return state_; }
    Output output() const;
    uint32_t ticks_in_state() const { return ticks_; }

private:
    void enter(State s);

    State    state_;
    uint32_t ticks_;
    uint32_t timeout_;
};

} // namespace crossctl

#endif
