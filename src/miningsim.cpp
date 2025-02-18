#include "simulation.h"
#include <boost/program_options.hpp>
#include <chrono>

namespace po = boost::program_options;

int main(int argc, char **argv) {
  int numTrucks = -1;
  int numStations = -1;
  try {
    po::options_description desc{"Options"};
    desc.add_options()("help,h", "help screen")(
        "trucks,n", po::value<int>(&numTrucks),
        "Number of trucks in simulation. Must be >= 1")(
        "stations,m", po::value<int>(&numStations),
        "Number of unload stations in simulation. Must be >= 1");
    po::variables_map vm;
    po::store(parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") || numTrucks < 1 || numStations < 1) {
      std::cout << desc << std::endl;
      return 0;
    }

    std::cout << "Starting simulation with numTrucks=" << numTrucks
              << " ,numStations=" << numStations << std::endl;

    // Set up the simulation
    Simulation sim{numTrucks, numStations};

    // Run the simulation
    auto beg = std::chrono::system_clock::now();
    Minutes duration = sim.start();
    auto end = std::chrono::system_clock::now();

    std::cout
        << "Finished simulation. Simulated time: [" << duration
        << " min]; Real time: ["
        << std::chrono::duration_cast<std::chrono::seconds>(end - beg).count()
        << " sec]" << std::endl;

    // Gather and print stats for trucks and stations
    TrucksStats trucksStats;
    sim.forEachTruck([&trucksStats](Truck *tr) {
      trucksStats.absorbTruck(tr->retrieveStats());
    });

    trucksStats.printStats();
    sim.stations().printStats();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}