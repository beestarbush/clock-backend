# Add local to x allowed list
xhost + local:

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
BACKEND_SRC_DIR="${SCRIPT_DIR}/src"

docker run --rm -it --network host -e DISPLAY="$DISPLAY" -v "${BACKEND_SRC_DIR}:/workdir" -v /tmp/.X11-unix:/tmp/.X11-unix qtbuilder ./run.sh
