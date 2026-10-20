#!/bin/sh
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
releases=${*:-12 13}

for release in $releases; do
    echo "== Debian $release"
    docker run --rm -v "$root":/src:ro "debian:$release" sh -euc '
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -qq
        apt-get install -y -qq build-essential cmake pkg-config \
            libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
            libxss-dev libxkbcommon-dev libgl-dev libegl-dev libasound2-dev \
            libpulse-dev libwayland-dev wayland-protocols libdecor-0-dev > /dev/null
        mkdir -p /work
        cd /src
        tar --exclude=./build --exclude="./build-*" --exclude=./game-data --exclude=./reference -cf - . | tar -xf - -C /work
        cd /work
        cmake -S . -B out -DSKIP_QA=ON -DCMAKE_BUILD_TYPE=Release > /dev/null
        cmake --build out -j"$(nproc)" > build.log 2>&1 || { tail -60 build.log; exit 1; }
        ls out/bin
    '
done
