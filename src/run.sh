#!/bin/bash

cd build
#QT_LOGGING_RULES="WebSocketService.debug=false;EnvironmentService.debug=false" ./clock-backend -platform xcb
QT_LOGGING_RULES="" ./clock-backend -platform xcb
