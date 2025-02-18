#pragma once

#include <iostream>

#include "stations.h"
#include "timerservice.h"
#include "truck.h"

/////////////////////////////////////////////////////////////////////////////////
// The overall logic of the simulation is encapsulated in class Simulation.
// It internally uses a timer service to schedule and handle events happening in
// simulated time. The other entities (Truck/Station) do not deal directly with
// the timer service but their event handling is mediated via the Simulation
// class. When an event happens, the Simulation class calls into the appropriate
// entity (Truck/Station) to update that entity's state and then schedules any
// further events that result from the current event. Each entity
// (Truck/Station) updates only its state as appropriate for the event and the
// event logic to coordinate across both entities exists in the Simulation
// class.
//
// The Simulation class uses only one thread. As a future concern - Scalability
// can be achieved via horizontal scaling by just running multiple simulations
// separately, each of which will model one area and the trucks/stations in that
// area. If it is desired to let trucks visit any unloading station i.e. get
// assigned to a station in an area that is being run in another thread, then
// this can be modelled by message passing between threads/processes and still
// keep each Simulation single threaded.
//
// SimulationBase is the base class. It allows the event handlers to be
// customized in tests and thus be able to write unit tests for TimerService
// etc.
class SimulationBase {
  friend class StationsTest_StationEta_Test;

protected:
  int numTrucks_;
  int numStations_;
  TimerService timerService_;
  Stations stations_;
  std::list<Truck> trucks_;

public:
  SimulationBase(int numTrucks, int numStations);

  // Start the simulation. This will run for 72 hours (in simulated time)
  // and return the timepoint at which the simulation stopped. This timepoint
  // is the first event that happened at a time > 72 hours.
  Minutes start();
  const Stations& stations() const { return stations_; }
  const std::list<Truck>& trucks() const { return trucks_; }

  // Event handlers for various events. This handlers are called
  // from the TimerService.
  virtual void onUnloadingFinished(timepoint_t now, Truck *truck,
                                   Station *station) = 0;
  virtual void onMiningFinished(timepoint_t now, Truck *truck) = 0;
  virtual void onArrivedAtStation(timepoint_t now, Truck *truck,
                                  Station *station) = 0;
};

///////////////////////////////////////////////////////////////////////////
// The concrete Simulation class.
class Simulation : public SimulationBase {
  friend class StationsTest_StationEta_Test;

public:
  Simulation(int numTrucks, int numStations)
      : SimulationBase{numTrucks, numStations} {}
  void onUnloadingFinished(timepoint_t now, Truck *truck,
                           Station *station) override;
  void onMiningFinished(timepoint_t now, Truck *truck) override;
  void onArrivedAtStation(timepoint_t now, Truck *truck,
                          Station *station) override;

  // Helper method to do something for each truck
  template <class Func> void forEachTruck(Func &&func) {
    for (Truck &truck : trucks_) {
      func(&truck);
    }
  }
};