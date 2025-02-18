#include <gtest/gtest.h>

#include "simulation.h"
#include "timerservice.h"
#include "truck.h"

TEST(TrucksTest, TruckLifecycle) {
  Truck truck{1};
  Station st{1, nullptr};
  ASSERT_EQ(truck.state(), Truck::Unloading);

  timepoint_t ts = 10;
  truck.startMining(ts, ts + 114);
  ASSERT_EQ(truck.state(), Truck::Mining);
  ASSERT_FALSE(truck.unloadingStation());
  ASSERT_EQ(truck.stateEntryTs(), ts);
  ASSERT_EQ(truck.stateExitTs(), ts + 114);

  ts += 114;
  truck.proceedToUnloadingStation(ts, &st);
  ASSERT_EQ(truck.state(), Truck::Driving);
  ASSERT_EQ(truck.unloadingStation(), &st);
  ASSERT_EQ(truck.stateEntryTs(), ts);
  ASSERT_EQ(truck.stateExitTs(), ts + 30);

  ts += 30;
  truck.unloadAtStation(ts);
  ASSERT_EQ(truck.state(), Truck::Unloading);
  ASSERT_EQ(truck.unloadingStation(), &st);
  ASSERT_EQ(truck.stateEntryTs(), ts);
  ASSERT_EQ(truck.stateExitTs(), ts + 5);

  ts += 5;
  truck.startMining(ts, ts + 219);
  ts += 219;
  truck.proceedToUnloadingStation(ts, &st);
  ts += 30;

  // This time, assume that st has an unloading truck and a waiting truck
  Truck tr2{2};
  tr2.stateEntryTs_ = ts - 3;
  tr2.stateExitTs_ = ts + 2;
  st.unloadingTruck_ = &tr2;

  Truck tr3{3};
  tr3.state_ = Truck::Waiting;
  tr3.unloadingStation_ = &st;
  st.waitingTrucks_.push_back(&tr3);

  // Now make truck wait at station. It will be queued up after all the above
  // trucks
  st.waitingTrucks_.push_back(&truck);
  truck.waitAtStation(ts);
  ASSERT_EQ(truck.stateExitTs(), ts + 2 + 5);

  auto &stats = truck.retrieveStats();
  ASSERT_EQ(stats[Truck::Mining], 114 + 219);
  ASSERT_EQ(stats[Truck::Driving], 30 + 30);
  ASSERT_EQ(stats[Truck::Waiting], 0 + 7);
  ASSERT_EQ(stats[Truck::Unloading], 5);
}
