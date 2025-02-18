#pragma once

#include "timerservice.h"
#include <array>
#include <cmath>
#include <inttypes.h>
#include <iomanip>
#include <iostream>

struct Station;

// Represents one truck wihin the simulation
class Truck {
public:
  // A truck is always in one of the following states.
  // The state transition graph is:
  // Mining -> Driving -----------> Unloading ---> Mining
  //             |                     ^
  //             V                     |
  //           Waiting -----------------
  enum State { Mining, Driving, Waiting, Unloading };

private:
  // The truck ID.
  int id_;
  // The state of the truck.
  State state_ = Unloading;
  // The timepoints at which current state was entered/exited
  // Due to the specifics of this simulation, we are able
  // to always calculate the exit ts based on the entry ts of the
  // state.
  timepoint_t stateEntryTs_;
  timepoint_t stateExitTs_;
  // Is null when in Mining state. In all other states, it is the
  // assigned unloading station for this round of unloading.
  Station *unloadingStation_ = nullptr;

  // Track total time spent in each of the 4 states
  std::array<Minutes, 4> stateDurations_ = {};

  friend class TrucksTest_TruckLifecycle_Test;
  friend class StationsTest_StationEta_Test;
  friend class StationsTest_StationEta2_Test;

public:
  // Start off as if we have just finished unloading and are about to start
  // mining
  Truck(int id);
  Truck(int id, State st, Station *unloadingStation);
  State state() const { return state_; }
  Station *unloadingStation() { return unloadingStation_; }
  timepoint_t stateEntryTs() const { return stateEntryTs_; }
  timepoint_t stateExitTs() const { return stateExitTs_; }

  // These are event specific state updaters. They are called from the
  // Simulation class that drives the overall event handling logic across
  // Trucks and Stations.
  void startMining(timepoint_t now, timepoint_t end);
  void proceedToUnloadingStation(timepoint_t now,
                                 Station *assignedUnloadingStation);
  void unloadAtStation(timepoint_t now);
  void waitAtStation(timepoint_t now);

  // This function retrieves the cumulative times spent in each state so far by
  // this truck.
  const std::array<Minutes, 4> &retrieveStats() const;
};

/////////////////////////////////////////////////////////////////////////////
// This is a helper class used to calculate stats across all trucks
class TrucksStats {
  // An inner private class that tracks the running avg and variance
  // of a stream of observations
  class Stats {
    std::string name_;
    int num_ = 0;
    double total_ = 0;
    double k_ = 0.0;
    double ex_ = 0.0;
    double ex2_ = 0.0;

  public:
    // See here for more details:
    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    void addObservation(Minutes m) {
      if (num_ == 0) {
        k_ = m;
      }
      num_ += 1;
      ex_ += (m - k_);
      ex2_ += (m - k_) * (m - k_);
      total_ += m;
    }

    void name(std::string_view name) { name_ = name; }

    std::string_view name() const { return name_; }

    double total() const { return total_; }

    double mean() const { return k_ + ex_ / num_; }

    double stddev() const {
      double variance = (ex2_ - ((ex_ * ex_) / num_)) / (num_ - 1);
      return sqrt(variance);
    }
  };

  std::array<Stats, 4> allTrucksStats_ = {};

public:
  TrucksStats();
  // Accumulates stats of a single truck into the cumulative stats of all trucks
  void absorbTruck(const std::array<Minutes, 4> &truckStats);
  // Prints cumulative stats accumulated so far.
  void printStats() const;
};
