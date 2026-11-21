cmake_minimum_required(VERSION 3.16)
project(HotelManagementSystem VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include(FetchContent)

# 1. Standalone Asio (required by Crow)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)

# 2. Crow v1.2.0
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Crow)

# 3. nlohmann/json v3.11.3
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

set(BACKEND_SOURCES
    backend/src/main.cpp
    backend/src/Database.cpp
    backend/src/APIRouter.cpp
    backend/src/AuthManager.cpp
    backend/src/HotelManager.cpp
    backend/src/ImageManager.cpp
    backend/src/UploadManager.cpp
    backend/src/RoomManager.cpp
    backend/src/BookingManager.cpp
    backend/src/BillingManager.cpp
    backend/src/CancellationManager.cpp
    backend/src/SearchManager.cpp
    backend/src/ReportManager.cpp
    backend/src/ReceiptManager.cpp
    backend/src/BackupManager.cpp
    backend/src/AIManager.cpp
    backend/src/Utils.cpp
    backend/lib/sqlite3.c
)

add_executable(hotel_server ${BACKEND_SOURCES})

target_include_directories(hotel_server PRIVATE
    backend/include
    backend/lib
    ${ASIO_INCLUDE_DIR}
)

find_package(Threads REQUIRED)

target_link_libraries(hotel_server PRIVATE
    Crow::Crow
    nlohmann_json::nlohmann_json
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(hotel_server PRIVATE ws2_32 mswsock)
    target_compile_definitions(hotel_server PRIVATE _WIN32_WINNT=0x0601)
endif()

if(MSVC)
    target_compile_options(hotel_server PRIVATE /W3)
    set_source_files_properties(backend/lib/sqlite3.c PROPERTIES COMPILE_FLAGS /w)
else()
    target_compile_options(hotel_server PRIVATE -Wall -Wextra)
    set_source_files_properties(backend/lib/sqlite3.c PROPERTIES COMPILE_FLAGS "-w")
endif()

add_custom_command(TARGET hotel_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/frontend
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/frontend
    COMMENT "Copying frontend to bin/"
)

message(STATUS "─────────────────────────────────────────────")
message(STATUS " HotelMS | Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " Asio: ${ASIO_INCLUDE_DIR}")
message(STATUS "─────────────────────────────────────────────")
