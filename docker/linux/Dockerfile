FROM debian:13

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build ccache pkg-config \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
        libxss-dev libxkbcommon-dev libgl-dev libegl-dev libasound2-dev \
        libpulse-dev libwayland-dev wayland-protocols libdecor-0-dev \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -m 777 /ccache
ENV CCACHE_DIR=/ccache

WORKDIR /src
