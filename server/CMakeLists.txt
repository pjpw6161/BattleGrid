cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(BattleGridServer VERSION 0.1.0 LANGUAGES CXX)

find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    message(FATAL_ERROR
        "nlohmann-json was not found. Install it with "
        "`C:/tools/vcpkg/vcpkg.exe install nlohmann-json:x64-windows`, "
        "or use the server/vcpkg.json manifest with "
        "`cmake -S server -B server/build "
        "-DCMAKE_TOOLCHAIN_FILE=C:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake`."
    )
endif()
find_package(Threads REQUIRED)

add_executable(battlegrid-server
    src/main.cpp
    src/core/Logger.cpp
    src/config/ServerConfig.cpp
    src/server/GameServer.cpp
    src/game/GameLoop.cpp
    src/game/PlayerInput.cpp
    src/game/PlayerState.cpp
    src/game/ProjectileState.cpp
    src/game/GameRoom.cpp
    src/game/RoomManager.cpp
    src/net/WebSocketServer.cpp
    src/net/Session.cpp
    src/protocol/JsonProtocol.cpp
    src/protocol/MessageDispatcher.cpp
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

target_link_libraries(battlegrid-server PRIVATE
    Boost::system
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_compile_definitions(battlegrid-server PRIVATE
    BOOST_ASIO_NO_DEPRECATED
)

if(MSVC)
    target_compile_options(battlegrid-server PRIVATE /W4 /permissive-)
else()
    target_compile_options(battlegrid-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_compile_definitions(battlegrid-server PRIVATE
        _WIN32_WINNT=0x0A00
        NOMINMAX
    )

    target_link_libraries(battlegrid-server PRIVATE
        ws2_32
        mswsock
    )
endif()
