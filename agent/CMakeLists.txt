# Max Rupplin - MEARVK LLC - 2026.
# Opt-in ClamAV system agent. Sits co-concerned with clamd/freshclam/clamonacc.
# It observes service state and proposes restorative actions; it never clears a
# ClamAV detection and is dry-run by default.

cmake_minimum_required(VERSION 3.15)

# The agent is a small C++17 program. It has no dependency on libclamav; it
# talks to the ClamAV *services* through systemctl/clamavctl at runtime.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The operator-facing CLI (clamavctl-style).
add_executable(clamav_agent clamav_agent_main.cpp)
target_include_directories(clamav_agent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# The co-concern bridge demonstration (maps posture -> gate/herald conditioning).
add_executable(clamav_bridge bridge.cpp)
target_compile_definitions(clamav_bridge PRIVATE LEGAL_CLAM_BRIDGE_MAIN)
target_include_directories(clamav_bridge PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Register the agent self-tests with CTest, matching ClamAV's CTest-based flow.
# Testing must be enabled at the top-level project (ClamAV's build calls
# enable_testing()/include(CTest)); we also enable it here so the agent subdir
# can be built and tested standalone.
if(NOT DEFINED CMAKE_TESTING_ENABLED)
    enable_testing()
endif()
add_test(NAME clamav_agent_selftest COMMAND clamav_agent selftest)
add_test(NAME clamav_bridge_selftest COMMAND clamav_bridge)

# Install the CLI and the systemd units so a packager can wire the advisory
# supervisor. All installs are optional and keyed off the opt-in build.
install(TARGETS clamav_agent RUNTIME DESTINATION bin)
install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/systemd/clamav-agent.service
        ${CMAKE_CURRENT_SOURCE_DIR}/systemd/clamav-agent.timer
        DESTINATION lib/systemd/system OPTIONAL)
