#include "crossctl/statemachine.h"

namespace crossctl {

const char *to_string(State s)
{
    switch (s) {
    case State::Idle:        return "IDLE";
    case State::Approaching: return "APPROACHING";
    case State::Closing:     return "CLOSING";
    case State::Closed:      return "CLOSED";
    case State::Opening:     return "OPENING";
    case State::Fault:       return "FAULT";
    }
    return "?";
}

const char *to_string(Event e)
{
    switch (e) {
    case Event::TrainDetected:        return "TRAIN_DETECTED";
    case Event::TrainCleared:         return "TRAIN_CLEARED";
    case Event::BarrierDownConfirmed: return "BARRIER_DOWN";
    case Event::BarrierUpConfirmed:   return "BARRIER_UP";
    case Event::Tick:                 return "TICK";
    case Event::Reset:                return "RESET";
    }
    return "?";
}

const char *to_string(Barrier b)
{
    switch (b) {
    case Barrier::Raise: return "RAISE";
    case Barrier::Lower: return "LOWER";
    case Barrier::Hold:  return "HOLD";
    }
    return "?";
}

const char *to_string(Signal s)
{
    switch (s) {
    case Signal::Danger: return "DANGER";
    case Signal::Clear:  return "CLEAR";
    }
    return "?";
}

StateMachine::StateMachine(uint32_t move_timeout_ticks)
    : state_(State::Idle), ticks_(0), timeout_(move_timeout_ticks)
{
}

void StateMachine::enter(State s)
{
    state_ = s;
    ticks_ = 0;
}

void StateMachine::handle(Event e)
{
    // Reset is the only way out of FAULT, and it is accepted from anywhere.
    if (e == Event::Reset) {
        enter(State::Idle);
        return;
    }
    if (state_ == State::Fault) {
        return;
    }

    if (e == Event::Tick) {
        ticks_++;
        // Only moving states have a deadline; overrunning it is a fault.
        if ((state_ == State::Closing || state_ == State::Opening) &&
            ticks_ >= timeout_) {
            enter(State::Fault);
        }
        return;
    }

    switch (state_) {
    case State::Idle:
        if (e == Event::TrainDetected) enter(State::Approaching);
        break;

    case State::Approaching:
        // Barrier is commanded down here; wait for confirmation.
        if (e == Event::TrainDetected) enter(State::Closing);
        break;

    case State::Closing:
        if (e == Event::BarrierDownConfirmed) enter(State::Closed);
        break;

    case State::Closed:
        if (e == Event::TrainCleared) enter(State::Opening);
        break;

    case State::Opening:
        if (e == Event::BarrierUpConfirmed) enter(State::Idle);
        break;

    case State::Fault:
        break;
    }
}

Output StateMachine::output() const
{
    switch (state_) {
    case State::Idle:        return { Barrier::Raise, Signal::Clear  };
    case State::Approaching: return { Barrier::Lower, Signal::Danger };
    case State::Closing:     return { Barrier::Lower, Signal::Danger };
    case State::Closed:      return { Barrier::Hold,  Signal::Danger };
    case State::Opening:     return { Barrier::Raise, Signal::Danger };
    case State::Fault:       return { Barrier::Lower, Signal::Danger };
    }
    return { Barrier::Lower, Signal::Danger };
}

} // namespace crossctl
