# Add local to x allowed list
xhost + local:

sudo docker run --rm -it --network host -e DISPLAY=$DISPLAY -v $(pwd)/src:/workdir -v /tmp/.X11-unix:/tmp/.X11-unix appbuilder ./run.sh
