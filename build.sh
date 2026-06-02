#!/bin/sh
OUTFILE="$(pwd)/src/git_version.h"

GIT_TAG=$(git describe --tags --always --dirty 2>/dev/null || echo "unknown")
GIT_COMMIT_HASH=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_COMMIT_HASH_SHORT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_DIRTY=$(git diff --quiet || echo "-dirty")

cat > "$OUTFILE" <<EOF
#pragma once
#define GIT_TAG "${GIT_TAG}"
#define GIT_COMMIT_HASH "${GIT_COMMIT_HASH}"
#define GIT_COMMIT_HASH_SHORT "${GIT_COMMIT_HASH_SHORT}"
#define GIT_DIRTY "${GIT_DIRTY}"
EOF

docker run --rm -i -v $(pwd)/src:/workdir appbuilder ./build.sh #"--clean"
