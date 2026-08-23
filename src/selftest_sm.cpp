#include "crossctl/statemachine.h"

#include <cstdio>

namespace crossctl {

static int sm_failures = 0;

static void chk(const char *what, bool ok)
{
    std::printf("  %-34s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) sm_failures++;
}

int selftest_statemachine()
{
    std::printf("statemachine: nominal cycle\n");
    {
        StateMachine sm;
        chk("starts IDLE", sm.state() == State::Idle);
        chk("idle signal CLEAR", sm.output().signal == Signal::Clear);

        sm.handle(Event::TrainDetected);
        chk("-> APPROACHING", sm.state() == State::Approaching);
        chk("approach signal DANGER", sm.output().signal == Signal::Danger);
        chk("approach barrier LOWER", sm.output().barrier == Barrier::Lower);

        sm.handle(Event::TrainDetected);
        chk("-> CLOSING", sm.state() == State::Closing);

        sm.handle(Event::BarrierDownConfirmed);
        chk("-> CLOSED", sm.state() == State::Closed);
        chk("closed barrier HOLD", sm.output().barrier == Barrier::Hold);

        sm.handle(Event::TrainCleared);
        chk("-> OPENING", sm.state() == State::Opening);
        chk("opening signal still DANGER", sm.output().signal == Signal::Danger);

        sm.handle(Event::BarrierUpConfirmed);
        chk("-> IDLE", sm.state() == State::Idle);
        chk("idle again CLEAR", sm.output().signal == Signal::Clear);
    }

    std::printf("statemachine: timeout to fault\n");
    {
        StateMachine sm(3);
        sm.handle(Event::TrainDetected);
        sm.handle(Event::TrainDetected);
        chk("in CLOSING", sm.state() == State::Closing);
        sm.handle(Event::Tick);
        sm.handle(Event::Tick);
        chk("not yet faulted", sm.state() == State::Closing);
        sm.handle(Event::Tick);
        chk("timeout -> FAULT", sm.state() == State::Fault);
        chk("fault barrier LOWER", sm.output().barrier == Barrier::Lower);
        chk("fault signal DANGER", sm.output().signal == Signal::Danger);
    }

    std::printf("statemachine: fault is sticky\n");
    {
        StateMachine sm(2);
        sm.handle(Event::TrainDetected);
        sm.handle(Event::TrainDetected);
        sm.handle(Event::Tick);
        sm.handle(Event::Tick);
        chk("faulted", sm.state() == State::Fault);
        sm.handle(Event::BarrierDownConfirmed);
        chk("ignores BARRIER_DOWN", sm.state() == State::Fault);
        sm.handle(Event::TrainCleared);
        chk("ignores TRAIN_CLEARED", sm.state() == State::Fault);
        sm.handle(Event::Reset);
        chk("RESET clears fault", sm.state() == State::Idle);
    }

    std::printf("statemachine: no timeout in stable states\n");
    {
        StateMachine sm(2);
        for (int i = 0; i < 20; i++) sm.handle(Event::Tick);
        chk("IDLE survives 20 ticks", sm.state() == State::Idle);

        StateMachine sm2(2);
        sm2.handle(Event::TrainDetected);
        sm2.handle(Event::TrainDetected);
        sm2.handle(Event::BarrierDownConfirmed);
        for (int i = 0; i < 20; i++) sm2.handle(Event::Tick);
        chk("CLOSED survives 20 ticks", sm2.state() == State::Closed);
    }

    std::printf("statemachine: unexpected events ignored\n");
    {
        StateMachine sm;
        sm.handle(Event::BarrierDownConfirmed);
        chk("IDLE ignores BARRIER_DOWN", sm.state() == State::Idle);
        sm.handle(Event::TrainCleared);
        chk("IDLE ignores TRAIN_CLEARED", sm.state() == State::Idle);
    }

    return sm_failures;
}

} // namespace crossctl
