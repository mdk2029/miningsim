#include "stations.h"
#include "simulation.h"
#include "timerservice.h"

// Calculates the ts at which this station will become free.
timepoint_t Station::freeTs() const {
  timepoint_t ts = timerService_->now();
  if (unloadingTruck_) {
    ts = unloadingTruck_->stateExitTs();
  }
  if (!waitingTrucks_.empty()) {
    ts += waitingTrucks_.size() * kUnloadingDuration;
  }
  for (Truck *t : arrivingTrucks_) {
    assert(t->state() == Truck::Driving);
    if (t->stateExitTs() <= ts) {
      // Truck will wait in waitingTrucks_, eta is additive on top of waiting
      // time
      ts += kUnloadingDuration;
    } else {
      // Truck will proceed to unloading immediately on arrival
      ts = t->stateExitTs() + kUnloadingDuration;
    }
  }

  return ts;
}

///////////////////////////////////////////////////////////////////////////

Stations::Stations(int numStations, TimerService *timerSvc) {
  for (int i = 0; i < numStations; i++) {
    stationHolder_.push_back(Station{i, timerSvc});
  }
  for (Station &st : stationHolder_) {
    stations_.insert(st);
  }
}

Station *Stations::selectUnloadingStation(Truck *truck) {
  // select "smallest" element from stations_
  auto itr = stations_.begin();
  Station &st = *itr;
  // Remove it from stations_. Note that this does not destroy the actual
  // Station object
  stations_.erase(itr);
  truck->proceedToUnloadingStation(st.timerService_->now(), &st);
  st.arrivingTrucks_.push_back(truck);
  // Reinsert Station into the container
  stations_.insert(st);
  return &st;
}

Truck::State Stations::onTruckArrivedForUnloading(Station *st) {
  assert(!st->arrivingTrucks_.empty());
  st->sHook_.unlink();
  Truck *truck = st->arrivingTrucks_.front();
  st->arrivingTrucks_.pop_front();
  Truck::State result;
  if (!st->unloadingTruck_) {
    st->unloadingTruck_ = truck;
    // Station was previously idle and is now busy
    st->idleDuration_ += (st->timerService_->now() - st->phaseStartTs_);
    st->phaseStartTs_ = st->timerService_->now();
    result = Truck::Unloading;
  } else {
    st->waitingTrucks_.push_back(truck);
    result = Truck::Waiting;
  }
  stations_.insert(*st);
  return result;
}

void Stations::onUnloadingFinished(timepoint_t now, Station *st) {
  st->unloadingTruck_ = nullptr;
  st->sHook_.unlink();
  // If we have waitingTrucks, start unloading the earliest one
  if (!st->waitingTrucks_.empty()) {
    Truck *truck = st->waitingTrucks_.front();
    st->waitingTrucks_.pop_front();
    truck->unloadAtStation(now);
    st->unloadingTruck_ = truck;
  }

  // Station was previously busy and is now idle
  st->busyDuration_ += (now - st->phaseStartTs_);
  st->phaseStartTs_ = now;

  stations_.insert(*st);
}

// Print some station stats. To keep things simple, am only calculating the
// the average cumulative utilization across all stations. Since the
// information is stored on a per Station basis, it should also
// be possible to present a per-Station statistic (like mean idleTime with
// stddev or mean waitingTrucks queue size)
void Stations::printStats() const {
  double totalIdle = 0.0;
  double totalBusy = 0.0;
  for (const auto& st : stationHolder_) {
    totalIdle += st.idleDuration_;
    totalBusy += st.busyDuration_;
  }

  std::cout << "Avg station utilization: " << totalBusy/(totalIdle + totalBusy) << std::endl;
}
