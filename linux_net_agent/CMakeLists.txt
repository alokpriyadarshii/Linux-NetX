cmake_minimum_required(VERSION 3.20)

project(linux_net_agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "linux_net_agent targets Linux hosts and requires Linux/POSIX APIs")
endif()

add_executable(linux-net-agent
    src/main.cpp
)

target_include_directories(linux-net-agent PRIVATE include)
target_compile_options(linux-net-agent PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_libraries(linux-net-agent PRIVATE pthread util)

install(TARGETS linux-net-agent RUNTIME DESTINATION bin)
install(FILES systemd/linux-net-agent.service DESTINATION lib/systemd/system)
