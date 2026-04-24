#!/usr/bin/env sh

set -eu

BACKEND_ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "${BACKEND_ROOT}"

sudo apt-get install libsdl2-dev libsdl2-image-dev libc-ares-dev protobuf-compiler -y
cmake -S "${BACKEND_ROOT}" -B "${BACKEND_ROOT}/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${BACKEND_ROOT}/build/install" \
  "$@"
cmake --build "${BACKEND_ROOT}/build"

cmake --build "${BACKEND_ROOT}/build" --target install
