FROM ubuntu:latest
LABEL description="Simulator for mining operations"

RUN apt-get update -y && \
    apt-get install -y tzdata

RUN apt-get install -y --no-install-recommends\
                    git \
                    gcc \
                    g++ \
                    build-essential \
                    libboost-all-dev \
                    libgtest-dev \
                    cmake \
                    unzip \
                    tar \
                    vim \
                    ca-certificates && \
    apt-get autoclean && \
    apt-get autoremove && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /miningsim

ENV TRUCKS=10000
ENV STATIONS=500
COPY src src
COPY test test
COPY CMakeLists.txt .
WORKDIR /miningsim/build
RUN cmake ..
RUN make -j4
CMD ["sh", "-c", "test/test_stations ; test/test_timerservice ; test/test_trucks ; ./simulator --trucks=${TRUCKS} --stations=${STATIONS}"]
