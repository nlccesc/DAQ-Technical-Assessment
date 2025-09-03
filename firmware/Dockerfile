FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_TESTS=ON

WORKDIR /app

# Base deps
RUN apt-get update && \
    apt-get install -y python3-pip \
    wget autoconf libtool git make g++ valgrind \
    cmake can-utils libboost-all-dev libxml2-dev curl && \
    rm -rf /var/lib/apt/lists/*

# Copy source
COPY . .

# Fetch Catch2 header for unit testing
RUN mkdir -p /app/tests && \
    curl -L -o /app/tests/catch.hpp \
    https://github.com/catchorg/Catch2/releases/download/v2.13.10/catch.hpp

# Ensure fresh build
RUN rm -rf build && mkdir build

# Configure and build (with optional tests)
RUN cmake -S . -B build -DBUILD_TESTS=${BUILD_TESTS} -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build --parallel

# Run tests automatically during build if enabled
RUN if [ "$BUILD_TESTS" = "ON" ]; then cd build && ctest --output-on-failure; fi

# Default entrypoint (debug shell)
ENTRYPOINT ["bash"]