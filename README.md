# miningsim

This is a simulator for a mining operation. More details/specifications are available elsewhere and not available publicly.

## Build instructions

The project can be cloned locally using git:

```
$ git clone git@github.com:mdk2029/miningsim.git
$ cd miningsim
```

### Native Building

This project uses C++20 and needs boost, cmake and google test to build and run. On most linux systems these dependencies will likely already exist or can be easily installed via that distro's package manager. On a Mac, it is recommended to use the provided self contained Dockerfile (see details below).

```
ubuntu e.g:
$ apt update && apt install -y --no-install-recommends gcc g++ cmake libboost-all-dev libgtest-dev
$ mkdir -p build && cd build && cmake .. && make -j4

Run tests:
$ ./test/test_stations && ./test/test_timerservice && ./test/test_trucks

Run the simulation:
$ ./simulator --trucks=100000 --stations=500
```

### Docker Building 

For ease of use, a Dockerfile is also provided that can be used to build and run the project.

```
Macbook building e.g:
$ docker buildx build -t miningsim .

Linux building e.g:
$ docker build -t miningsim .

Run tests and simulation. The container will first run all the unit tests and if there is no error, then run the simulation with 100000 trucks and 500 stations. It will print simulation stats to stdout after the run

$ docker run --rm -e TRUCKS=100000 -e STATIONS=500 miningsim  
```

After building the docker container, for troubleshooting, you can also just exec into it and get a shell

```
$ docker run -it --rm miningsim bash
```

## Simulation details
See inline code comments for details. A good place to start is the overall information provided in `simulation.h` 
