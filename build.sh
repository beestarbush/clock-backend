#!/bin/sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BACKEND_SRC_DIR="${SCRIPT_DIR}/src"
OUTFILE="${BACKEND_SRC_DIR}/git_version.h"

GIT_TAG=$(cd "${SCRIPT_DIR}" && git describe --tags --always --dirty 2>/dev/null || echo "unknown")
GIT_COMMIT_HASH=$(cd "${SCRIPT_DIR}" && git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_COMMIT_HASH_SHORT=$(cd "${SCRIPT_DIR}" && git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_DIRTY=$(cd "${SCRIPT_DIR}" && git diff --quiet || echo "-dirty")

cat > "$OUTFILE" <<EOF
#pragma once
#define GIT_TAG "${GIT_TAG}"
#define GIT_COMMIT_HASH "${GIT_COMMIT_HASH}"
#define GIT_COMMIT_HASH_SHORT "${GIT_COMMIT_HASH_SHORT}"
#define GIT_DIRTY "${GIT_DIRTY}"
EOF

docker run --rm -i \
	--user "$(id -u):$(id -g)" \
	-v "${BACKEND_SRC_DIR}:/workdir" \
	qtbuilder ./build.sh #"--clean"
