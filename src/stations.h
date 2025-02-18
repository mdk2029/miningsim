#pragma once

#include "truck.h"
#include <boost/intrusive/set.hpp>
#include <deque>
#include <list>
#include <optional>

namespace bi = boost::intrusive;
class TimerService;

// Station represents one UnloadingStation.
struct Station {
  // Station ID.
  int id_;
  TimerService *timerService_ = nullptr;
  // The truck that is currently being unloaded or nullptr.
  Truck *unloadingTruck_ = nullptr;
  // FIFO Queue of trucks that have arrived and are waiting to be unloaded.
  std::deque<Truck *> waitingTrucks_;
  // FIFO Queue of trucks that have been dispatched to this station but have
  // not yet arrived. The front of the queue is the first arriving truck.
  std::deque<Truck *> arrivingTrucks_;

  // Total Idle and Busy time for this station.
  Minutes idleDuration_ = 0;
  Minutes busyDuration_ = 0;
  // When did the idle/busy phase start. A station will keep toggling
  //  between these two phases.
  timepoint_t phaseStartTs_ = 0;

  // intrusive hook to be able to store all Station's in an ordered
  // container, see class Stations below for more details.
  bi::set_member_hook<bi::link_mode<bi::auto_unlink>> sHook_;

  // Constructor
  Station(int id, TimerService *timerSvc) : id_{id}, timerService_{timerSvc} {}

  // When is this station going to be free? The station may already be free
  // or will become free after all currently unloading/waiting/arriving trucks
  // have been processed. This value is used to determine which is the least
  // loaded truck at any time.
  timepoint_t freeTs() const;

  // Stations are ordered based on their freeTs.
  bool operator<(const Station &rhs) const { return freeTs() < rhs.freeTs(); }
};

////////////////////////////////////////////////////////////////////////////

// Stations is a container for all the stations. We need to maintain all
// stations in a suitable data structure where the load in a station can be
// updated (as trucks arrive / leave) and the least loaded such station can be
// determined. Ideally, we could use a PriorityQueue implemented as a binary
// heap here. But as a simpler alternative, we will use a multiset (which is
// internally implemented as a RedBlack tree). Further, we will use the
// boost::intrusive::multiset since it lets us do the following operation
// efficiently - Given a Station* , be able to quickly unlink it from the
// container, update its load, and then re-insert into the container. If we used
// a STL multiset, we would first have to locate the Station using a linear scan
// since the multiset is ordered by the station load and is not directly
// searchable using the station ID.
class Stations {
  // We need to create all Station objects in a container which guarantees
  // that the location of the object will not move during run-time.
  std::list<Station> stationHolder_;
  using SetMemberHookOption =
      bi::member_hook<Station,
                      bi::set_member_hook<bi::link_mode<bi::auto_unlink>>,
                      &Station::sHook_>;

  // This is the ordered view of all Stations. It will be maintained based on
  // each Station's load. The way this works internally in boost is that it is a
  // view on objects owned and held elsewhere (stationHolder_ in our case).
  bi::multiset<Station, SetMemberHookOption, bi::constant_time_size<false>>
      stations_;
  friend class StationsTest_StationEta_Test;
  friend class StationsTest_StationEta2_Test;

public:
  Stations(int numStations, TimerService *timerSvc);

  // When a truck has finished Mining, this method is used to determine
  // which UnloadingStation to send the truck to.
  Station *selectUnloadingStation(Truck *truck);

  // Event dispatched by timer service when a truck arrives for unloading.
  // If station was free, truck immediately begins unloading, else it
  // is queued up in the station's FIFO waitingTrucks_ queue.
  // Returns either Truck::Unloading or Truck::Waiting to indicate what
  // happened.
  Truck::State onTruckArrivedForUnloading(Station *st);
  // Event dispatched by timer service when an unloading truck finishes
  // unloading and is ready to start mining again.
  void onUnloadingFinished(timepoint_t now, Station *st);

  void printStats() const;
};
