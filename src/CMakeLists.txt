cmake_minimum_required(VERSION 3.16)

project(clock-backend-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Multimedia Test)

add_subdirectory(communication ${CMAKE_CURRENT_BINARY_DIR}/communication-build)

add_executable(clock_backend
    main.cpp
    drivers/Container.h
    drivers/Container.cpp
    drivers/platform/Driver.h
    drivers/platform/Driver.cpp
    services/Container.h
    services/Container.cpp
    services/audio/Service.h
    services/audio/Service.cpp
    services/display/Service.h
    services/display/Service.cpp
    services/environment/Service.h
    services/environment/Service.cpp
    services/websocket/Service.h
    services/websocket/Service.cpp
    services/configuration/Service.h
    services/configuration/Service.cpp
    services/ingress/Service.h
    services/ingress/Service.cpp
    services/media/Service.h
    services/media/Service.cpp
    services/rest/Service.h
    services/rest/Service.cpp
    services/status/Service.h
    services/status/Service.cpp
    services/system/Service.h
    services/system/Service.cpp
    applications/Container.h
    applications/Container.cpp
)

target_link_libraries(clock_backend
    PRIVATE
        communication
        Qt6::Core
        Qt6::Multimedia
        Qt6::Network
        Qt6::WebSockets
)

target_include_directories(clock_backend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if (CMAKE_CXX_COMPILER MATCHES "aarch64-poky-linux")
    add_definitions(-DPLATFORM_IS_TARGET)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_IS_HOST)
endif()

add_executable(clock-backend-qt-unit-tests
    tests/BackendServiceTests.cpp
    services/websocket/Service.h
    services/websocket/Service.cpp
    services/configuration/Service.h
    services/configuration/Service.cpp
    services/media/Service.h
    services/media/Service.cpp
    services/status/Service.h
    services/status/Service.cpp
)

target_link_libraries(clock-backend-qt-unit-tests
    PRIVATE
        communication
        Qt6::Core
        Qt6::Multimedia
        Qt6::Network
        Qt6::WebSockets
        Qt6::Test
)

target_include_directories(clock-backend-qt-unit-tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

enable_testing()
add_test(NAME clock-backend-qt-unit-tests COMMAND clock-backend-qt-unit-tests)

set_target_properties(clock_backend PROPERTIES OUTPUT_NAME clock-backend)
