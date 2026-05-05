FROM ubuntu:24.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        libboost-all-dev \
        nlohmann-json3-dev \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src/server

COPY server/CMakeLists.txt ./CMakeLists.txt
COPY server/src ./src

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j

FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        libboost-system1.83.0 \
        libgcc-s1 \
        libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/server/build/battlegrid-server ./battlegrid-server

EXPOSE 7777

CMD ["./battlegrid-server", "--host", "0.0.0.0", "--port", "7777", "--tick-rate", "30"]
