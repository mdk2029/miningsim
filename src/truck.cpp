#include "truck.h"
#include "simulation.h"
#include <iomanip>
#include <iostream>

Truck::Truck(int id) : id_{id} {}

Truck::Truck(int id, State st, Station *unloadingStation)
    : id_{id}, state_{st}, unloadingStation_{unloadingStation} {
  for (auto &duration : stateDurations_) {
    duration = 0;
  }
}

void Truck::startMining(timepoint_t now, timepoint_t end) {
  assert(state_ == Unloading);
  state_ = Mining;
  unloadingStation_ = nullptr;
  stateEntryTs_ = now;
  stateExitTs_ = end;
  stateDurations_[Mining] += (stateExitTs_ - stateEntryTs_);
}

void Truck::proceedToUnloadingStation(timepoint_t now,
                                      Station *assignedUnloadingStation) {
  assert(state_ == Mining);
  assert(!unloadingStation_);
  unloadingStation_ = assignedUnloadingStation;
  state_ = Driving;
  stateEntryTs_ = now;
  stateExitTs_ = now + kDrivingDuration;
  stateDurations_[Driving] += kDrivingDuration;
}

void Truck::unloadAtStation(timepoint_t now) {
  assert(state_ == Driving || state_ == Waiting);
  assert(unloadingStation_);
  state_ = Unloading;
  stateEntryTs_ = now;
  stateExitTs_ = now + kUnloadingDuration;
  stateDurations_[Unloading] += kUnloadingDuration;
}

void Truck::waitAtStation(timepoint_t now) {
  assert(state_ == Driving);
  assert(unloadingStation_);
  state_ = Waiting;
  stateEntryTs_ = now;

  assert(unloadingStation_->unloadingTruck_->state() == Truck::Unloading);
  // Calculate wait time
  assert(unloadingStation_->waitingTrucks_.back() == this);
  stateExitTs_ = unloadingStation_->unloadingTruck_->stateExitTs_;
  stateExitTs_ +=
      ((unloadingStation_->waitingTrucks_.size() - 1) * kUnloadingDuration);

  stateDurations_[Waiting] += (stateExitTs_ - stateEntryTs_);
}

const std::array<Minutes, 4> &Truck::retrieveStats() const {
  return stateDurations_;
}

////////////////////////////////////////////////////////////

TrucksStats::TrucksStats() {
  for (Stats &st : allTrucksStats_) {
    st = Stats{};
  }
  allTrucksStats_[Truck::Mining].name("Mining   ");
  allTrucksStats_[Truck::Driving].name("Driving  ");
  allTrucksStats_[Truck::Waiting].name("Waiting  ");
  allTrucksStats_[Truck::Unloading].name("Unloading");
}

void TrucksStats::absorbTruck(const std::array<Minutes, 4> &truckStats) {
  for (auto st :
       {Truck::Mining, Truck::Driving, Truck::Waiting, Truck::Unloading}) {
    allTrucksStats_[st].addObservation(truckStats[st]);
  }
}

void TrucksStats::printStats() const {
  double mining = allTrucksStats_[Truck::Mining].total();
  double driving = allTrucksStats_[Truck::Driving].total();
  double waiting = allTrucksStats_[Truck::Waiting].total();
  double unloading = allTrucksStats_[Truck::Unloading].total();

  std::cout << std::fixed;
  std::cout << std::setprecision(2);
  std::cout << "Trucks avg utilization: "
            << mining / (mining + driving + waiting + unloading)
            << "; State breakdown:" << std::endl;
  std::cout << "\t\tAvg\t\tstddev\n";
  for (auto &st : allTrucksStats_) {
    std::cout << st.name() << "\t" << st.mean() << "\t\t" << st.stddev()
              << std::endl;
  }
}
