#include <gtest/gtest.h>

#include "simulation.h"
#include "timerservice.h"
#include "truck.h"

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

TEST(StationsTest, StationEta) {
  int numTrucks = 10;
  int numStations = 2;
  Simulation simulation(numTrucks, numStations);
  ASSERT_EQ(simulation.stations_.stationHolder_.size(), numStations);

  TimerService *timerService = &simulation.timerService_;
  auto &stations = simulation.stations_.stations_;
  ASSERT_EQ(stations.size(), numStations);

  // All stations should be free initially
  std::set<int> stationIds;
  for (auto itr = stations.begin(); itr != stations.end(); itr++) {
    ASSERT_FALSE(itr->unloadingTruck_);
    ASSERT_EQ(itr->freeTs(), timerService->now());
    stationIds.insert(itr->id_);
  }

  ASSERT_EQ(stationIds, (std::set<int>{0, 1}));

  // Test ETA of stations is adjusted properly when trucks arrive for unloading.
  Station *st1 = &(*stations.begin());
  Station *st2 = &(*(++stations.begin()));
  ASSERT_NE(st1, st2);

  // Truck0 is driving to st1 which is currently free.
  std::unique_ptr<Truck> tr{new Truck{0, Truck::Driving, st1}};
  ASSERT_FALSE(st1->unloadingTruck_);
  st1->arrivingTrucks_.push_back(tr.get());

  // Truck0 arrives at St1.
  simulation.onArrivedAtStation(timerService->now(), tr.get(), st1);
  ASSERT_EQ(tr->state(), Truck::Unloading);
  ASSERT_EQ(st1->freeTs(), Minutes(5));
  ASSERT_EQ(st2->freeTs(), Minutes(0));

  // Test that the other station still has a freeTs of 0 and is the first one
  // in the container now
  ASSERT_EQ(&(*stations.begin()), st2);
  ASSERT_EQ(stations.begin()->freeTs(), 0);

  // Advance time by 3 minutes
  timerService->setNow(timerService->now() + Minutes{3});
  // Send a truck to st2
  std::unique_ptr<Truck> tr1{new Truck{1, Truck::Driving, st2}};
  st2->arrivingTrucks_.push_back(tr1.get());

  // Tr1 arrives
  simulation.onArrivedAtStation(timerService->now(), tr1.get(), st2);
  // Tr1 state should be Unloading
  ASSERT_EQ(tr1->state(), Truck::Unloading);
  // Assert that St2 ETA is now +5
  ASSERT_EQ(st2->freeTs(), Minutes(3 + 5));

  // Tr2 is now sent to St2
  std::unique_ptr<Truck> tr2{new Truck{2, Truck::Driving, st2}};
  st2->arrivingTrucks_.push_back(tr2.get());
  // St2 is still unloading Tr1 and Tr2 will be the first arrivingTruck
  ASSERT_EQ(st2->unloadingTruck_, tr1.get());
  ASSERT_EQ(st2->arrivingTrucks_.size(), 1);

  simulation.onArrivedAtStation(timerService->now(), tr2.get(), st2);
  // Tr2 state should be Waiting
  ASSERT_EQ(tr2->state(), Truck::Waiting);
  ASSERT_TRUE(st2->arrivingTrucks_.empty());
  ASSERT_EQ(st2->waitingTrucks_.size(), 1);
  ASSERT_EQ(st2->waitingTrucks_.front(), tr2.get());
  // Assert that St2 free ETA is now additional +5
  ASSERT_EQ(st2->freeTs(), Minutes(3 + 5 + 5));

  // Test that st1 is now the less loaded station
  ASSERT_EQ(&(*stations.begin()), st1);
}

TEST(StationsTest, StationEta2) {
  TestSimulation testSimulation{1, 1};
  TimerService timerService{&testSimulation};
  Station st{1, &timerService};

  timepoint_t ts = timerService.now();
  // Have st have an unloading truck, a waiting truck and 2 trucks driving to
  // it.
  Truck tr2{2};
  tr2.stateEntryTs_ = ts - 3;
  tr2.stateExitTs_ = ts + 2;
  st.unloadingTruck_ = &tr2;

  Truck tr3{3};
  tr3.state_ = Truck::Waiting;
  tr3.unloadingStation_ = &st;
  st.waitingTrucks_.push_back(&tr3);

  // TR4 will arrive before TR3 finishes
  Truck tr4{4};
  tr4.state_ = Truck::Driving;
  tr4.unloadingStation_ = &st;
  tr4.stateExitTs_ = ts + 3;
  st.arrivingTrucks_.push_back(&tr4);

  // TR5 will arrive after TR4 finishes
  Truck tr5{5};
  tr5.state_ = Truck::Driving;
  tr5.unloadingStation_ = &st;
  tr5.stateExitTs_ = ts + 15;
  st.arrivingTrucks_.push_back(&tr5);

  // Station will only become free at end of tr5
  ASSERT_EQ(st.freeTs(), ts + 15 + 5);
}
