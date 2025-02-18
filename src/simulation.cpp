#include "simulation.h"

///////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG

#define assert_eq(X, Y)                                                        \
  if (X != Y) {                                                                \
    std::cout << X << " / " << Y << std::endl;                                 \
  }                                                                            \
  assert(X == Y);

#define assert_neq(X, Y)                                                       \
  if (X == Y) {                                                                \
    std::cout << X << " / " << Y << std::endl;                                 \
  }                                                                            \
  assert(X != Y);

#else

#define assert_eq(X, Y)
#define assert_neq(X, Y)

#endif

///////////////////////////////////////////////////////////////////////////

SimulationBase::SimulationBase(int numTrucks, int numStations)
    : numTrucks_{numTrucks}, numStations_{numStations}, timerService_{this},
      stations_{numStations, &timerService_} {
  for (int i = 0; i < numTrucks_; i++) {
    trucks_.push_back(Truck{i});
  }
}

Minutes SimulationBase::start() {
  timepoint_t beginning = timerService_.now();
  // Start by putting all trucks into Mining state
  for (Truck &truck : trucks_) {
    assert(truck.state() == Truck::Unloading);
    Minutes miningDuration =
        randomDuration(kMiningDurationMin, kMiningDurationMax);
    truck.startMining(beginning, beginning + miningDuration);
    timerService_.scheduleEvent(
        MiningFinished{{beginning + miningDuration}, &truck});
  }

  // Keep dispatching events. The timeService will call event handlers
  // which will internally enqueue further events. See TimerService for
  // more details
  while (timerService_.dispatchNextEvent()) {
    // Each time an event happens, the timerService's time is updated
    // to the ts of that event.
    if (timerService_.now() - beginning > kSimDuration) {
      break;
    }
  }

  return timerService_.now();
}

////////////////////////////////////////////////////////////////////////

// When a truck finishes unloading, transition it to Mining. In the
// station that it was at, start unloading the next waiting truck if any
void Simulation::onUnloadingFinished(timepoint_t now, Truck *truck,
                                     Station *station) {
  assert_eq(truck->state(), Truck::Unloading);
  assert_eq(station->unloadingTruck_, truck);

  Minutes miningDuration =
      randomDuration(kMiningDurationMin, kMiningDurationMax);
  truck->startMining(now, now + miningDuration);
  timerService_.scheduleEvent(MiningFinished{{now + miningDuration}, truck});

  stations_.onUnloadingFinished(now, station);
  if (station->unloadingTruck_) {
    // There was a waiting truck that is now unloading
    assert_neq(station->unloadingTruck_, truck);
    assert_eq(station->unloadingTruck_->state(), Truck::Unloading);
    timerService_.scheduleEvent(UnloadingFinished{
        {now + kUnloadingDuration}, station->unloadingTruck_, station});
  }
}

// When a truck finishes Mining, find the least loaded station and
// send it there
void Simulation::onMiningFinished(timepoint_t now, Truck *truck) {
  assert_eq((truck->state()), (Truck::Mining));
  Station *unloadingStation = stations_.selectUnloadingStation(truck);
  timerService_.scheduleEvent(
      ArrivedAtStation{{now + kDrivingDuration}, truck, unloadingStation});
}

// When truck arrives at station, either start unloading it or queue it
// behind other waiting trucks
void Simulation::onArrivedAtStation(timepoint_t now, Truck *truck,
                                    Station *station) {
  assert(truck->state() == Truck::Driving);
  assert(truck->unloadingStation() == station);
  assert(station->arrivingTrucks_.front() == truck);

  Truck::State result = stations_.onTruckArrivedForUnloading(station);
  if (result == Truck::Unloading) {
    truck->unloadAtStation(now);
    timerService_.scheduleEvent(
        UnloadingFinished{{now + kUnloadingDuration}, truck, station});
  } else {
    assert(result == Truck::Waiting);
    truck->waitAtStation(now);
  }
}
