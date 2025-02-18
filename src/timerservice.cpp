#include "timerservice.h"
#include "simulation.h"
#include <random>

// Helper function to generate random mining durations
Minutes randomDuration(Minutes min, Minutes max) {
  // Use fixed seed to have a deterministic simulation
  static std::mt19937 generator(0);

  // Create a uniform integer distribution between min and max (inclusive)
  std::uniform_int_distribution<Minutes> distribution(min, max);

  // Generate and return the random number
  return distribution(generator);
}

void TimerService::scheduleEvent(MiningFinished evt) {
  events_.insert({evt.ts_, SimulationEvent{evt}});
}

void TimerService::scheduleEvent(ArrivedAtStation evt) {
  events_.insert({evt.ts_, SimulationEvent{evt}});
}

void TimerService::scheduleEvent(UnloadingFinished evt) {
  events_.insert({evt.ts_, SimulationEvent{evt}});
}

// Picks the next event to dispatch. Each event has a compile-time-known
// handler that is invoked when the event happens. Before the event
// handler is invoked, time is advanced.
bool TimerService::dispatchNextEvent() {
  auto itr = events_.begin();
  if (itr != events_.end()) {
    SimulationEvent &evt = itr->second;
    if (MiningFinished *e = std::get_if<MiningFinished>(&evt)) {
      assert(e->ts_ == itr->first);
      now_ = e->ts_;
      simulation_->onMiningFinished(e->ts_, e->truck_);
    } else if (ArrivedAtStation *e = std::get_if<ArrivedAtStation>(&evt)) {
      assert(e->ts_ == itr->first);
      now_ = e->ts_;
      simulation_->onArrivedAtStation(e->ts_, e->truck_, e->station_);
    } else {
      UnloadingFinished *u = std::get_if<UnloadingFinished>(&evt);
      assert(u && u->ts_ == itr->first);
      now_ = u->ts_;
      simulation_->onUnloadingFinished(u->ts_, u->truck_, u->station_);
    }
    events_.erase(itr);
    return true;
  }
  return false;
}
