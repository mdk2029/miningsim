#include <gtest/gtest.h>

#include "simulation.h"
#include "timerservice.h"
#include "truck.h"

// A test simulation class that accumulates all happened events and
// does not invoke any event handlers. The subsequent tests use
// this class to ensure that the TimerService dispatches the expected events
// with the expected details.
struct TestSimulation : public SimulationBase {
  std::vector<SimulationEvent> events_;
  TestSimulation(int numTrucks, int numStations)
      : SimulationBase{numTrucks, numStations} {}

  void onUnloadingFinished(timepoint_t now, Truck *truck,
                           Station *station) override {
    events_.push_back(SimulationEvent{UnloadingFinished{now, truck, station}});
  }
  void onMiningFinished(timepoint_t now, Truck *truck) override {
    events_.push_back(SimulationEvent{MiningFinished{now, truck}});
  }
  void onArrivedAtStation(timepoint_t now, Truck *truck,
                          Station *station) override {
    events_.push_back(SimulationEvent{ArrivedAtStation{now, truck, station}});
  }

  TimerService *timerService() { return &timerService_; }
};

TEST(TimerService, OrderedDispatch) {
  TestSimulation testSimulation(1, 1);
  TimerService *timerService = testSimulation.timerService();

  ASSERT_EQ(timerService->now(), 0);

  Truck t1{1};
  Truck t2{2};
  Station s1{1, timerService};
  timerService->scheduleEvent(MiningFinished{timepoint_t{100}, &t1});
  timerService->scheduleEvent(ArrivedAtStation{timepoint_t{50}, &t2, &s1});

  // Ensure that first event happens, time gets updated to 50
  // and verify details of the event that happened
  ASSERT_TRUE(timerService->dispatchNextEvent());
  ASSERT_EQ(timerService->now(), 50);
  ASSERT_EQ(testSimulation.events_.back(),
            (SimulationEvent{ArrivedAtStation{{50}, &t2, &s1}}));

  // Ensure that second event happens and after that, time has been updated to
  // 100
  ASSERT_TRUE(timerService->dispatchNextEvent());
  ASSERT_EQ(timerService->now(), 100);
  ASSERT_EQ(testSimulation.events_.back(),
            (SimulationEvent{MiningFinished{100, &t1}}));

  // Ensure that no further event happens
  ASSERT_FALSE(timerService->dispatchNextEvent());
}
