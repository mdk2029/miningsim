#pragma once
#include <inttypes.h>
#include <map>
#include <variant>

//////////////////////////////////////////////////////////////////////////////
// Some helper types for simulated time
using timepoint_t = int64_t;
using Minutes = int;

static constexpr Minutes kUnloadingDuration = Minutes{5};
static constexpr Minutes kDrivingDuration = Minutes{30};
static constexpr Minutes kMiningDurationMin = Minutes{60};
static constexpr Minutes kMiningDurationMax = Minutes{60 * 5};
static constexpr Minutes kSimDuration = Minutes{60 * 24 * 3};

class Truck;
struct Station;

////////////////////////////////////

Minutes randomDuration(Minutes min, Minutes max);

////////////////////////////////////
// These are the events of interest within the simulation. The timer service
// is "event aware" in the sense that the handler for an event is
// not stored within the event (via say a virtual member function dispatch)
// but is instead known to the TimerService. See TimerService::dispatchNextEvent
// for more details.

// Base class for all events. It stores the ts at which the event happened/
// will happen.
struct Event {
  timepoint_t ts_;
  bool operator==(const Event &) const = default;
};

struct MiningFinished : Event {
  Truck *truck_;
  bool operator==(const MiningFinished &) const = default;
};

struct ArrivedAtStation : Event {
  Truck *truck_;
  Station *station_;
  bool operator==(const ArrivedAtStation &) const = default;
};

struct UnloadingFinished : Event {
  Truck *truck_;
  Station *station_;
  bool operator==(const UnloadingFinished &) const = default;
};

// A variant type to store all the three events.
using SimulationEvent =
    std::variant<MiningFinished, ArrivedAtStation, UnloadingFinished>;

///////////////////////////////////////////////////////////////////////////

class SimulationBase;

// TimerService is used to schedule events to happen at specified timepoints
// events are stored in a multimap. After an event's handler is invoked,
// the next event is immediately dispatched. When an event "happens", the
// TimerService's time is advanced to that event's timestamp.
class TimerService {
  timepoint_t now_ = 0;
  SimulationBase *simulation_ = nullptr;
  std::multimap<timepoint_t, SimulationEvent> events_;

  void setNow(timepoint_t now) { now_ = now; }
  friend class StationsTest_StationEta_Test;

public:
  TimerService(SimulationBase *sim) : simulation_{sim} {}
  timepoint_t now() const { return now_; }
  void scheduleEvent(MiningFinished);
  void scheduleEvent(ArrivedAtStation);
  void scheduleEvent(UnloadingFinished);
  bool dispatchNextEvent();
};