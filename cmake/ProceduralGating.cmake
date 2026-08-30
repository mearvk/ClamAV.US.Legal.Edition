# Max Rupplin - MEARVK LLC - 2026.
# Opt-in include for the eight-level procedural gate scaffold.
option(ENABLE_PROCEDURAL_GATING "Build the experimental procedural gating/herald layer" OFF)
if(ENABLE_PROCEDURAL_GATING)
    add_subdirectory(${CMAKE_SOURCE_DIR}/gating ${CMAKE_BINARY_DIR}/gating)
endif()
