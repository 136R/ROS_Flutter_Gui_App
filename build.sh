#!/usr/bin/env sh

set -eu

ROOT="$(cd "$(dirname "$0")" && pwd)"
BACKEND="${ROOT}/backend"
APP="${ROOT}/app"
PROTO_ROOT="${ROOT}/protocol"
PB_OUT="${APP}/lib/protobuf"
CLI_ARGS="$*"

cd "${ROOT}"

if ! command -v protoc >/dev/null 2>&1; then
  echo "error: protoc not found (e.g. apt install protobuf-compiler)" >&2
  exit 1
fi

export PATH="${PATH}:${HOME}/.pub-cache/bin"
if ! command -v protoc-gen-dart >/dev/null 2>&1; then
  echo "error: protoc-gen-dart not found. Run: dart pub global activate protoc_plugin" >&2
  echo "  and ensure ~/.pub-cache/bin is on PATH." >&2
  exit 1
fi

if [ ! -d "${PROTO_ROOT}" ]; then
  echo "error: protocol directory missing: ${PROTO_ROOT}" >&2
  exit 1
fi

set -- "${PROTO_ROOT}"/*.proto
if [ ! -e "$1" ]; then
  echo "error: no .proto files under ${PROTO_ROOT}" >&2
  exit 1
fi

mkdir -p "${PB_OUT}"
protoc --experimental_allow_proto3_optional \
  -I"${PROTO_ROOT}" \
  --dart_out="${PB_OUT}" \
  "$@"
echo "[build] Dart protobuf -> ${PB_OUT}"

if ! command -v flutter >/dev/null 2>&1; then
  echo "error: flutter not installed or not on PATH; cannot build app" >&2
  exit 1
fi

(cd "${APP}" && flutter pub get && flutter gen-l10n && flutter build web)
echo "[build] Flutter web done (${APP}/build/web)"

INSTALL_BIN_DIST="${BACKEND}/src/app/install/bin/dist"
rm -rf "${INSTALL_BIN_DIST}"
mkdir -p "${INSTALL_BIN_DIST}"
cp -a "${APP}/build/web/." "${INSTALL_BIN_DIST}/"

if [ -n "${CLI_ARGS}" ]; then
  # shellcheck disable=SC2086
  sh "${ROOT}/backend/build.sh" ${CLI_ARGS}
else
  sh "${ROOT}/backend/build.sh"
fi
echo "[build] backend done (${BACKEND}/build)"