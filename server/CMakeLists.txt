cmake_minimum_required(VERSION 3.20)

project(BattleGridServer VERSION 0.1.0 LANGUAGES CXX)

add_executable(battlegrid-server
    src/main.cpp
    src/core/Logger.cpp
    src/config/ServerConfig.cpp
    src/server/GameServer.cpp
    src/game/GameLoop.cpp
)

target_compile_features(battlegrid-server PRIVATE cxx_std_20)

set_target_properties(battlegrid-server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "battlegrid-server"
)

target_include_directories(battlegrid-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(MSVC)
    target_compile_options(battlegrid-server PRIVATE /W4 /permissive-)
else()
    target_compile_options(battlegrid-server PRIVATE -Wall -Wextra -Wpedantic)
endif()
