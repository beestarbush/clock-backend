xhost + local:

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
APP_SRC_DIR="${SCRIPT_DIR}/src"

sudo docker run --rm -it -e DISPLAY="$DISPLAY" -v "${APP_SRC_DIR}:/workdir" -v /tmp/.X11-unix:/tmp/.X11-unix qtbuilder
