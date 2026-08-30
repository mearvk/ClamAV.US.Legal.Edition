# Max Rupplin - MEARVK LLC - 2026.
# Opt-in include for the eight-level procedural gate scaffold and the ClamAV
# system agent. The upstream ClamAV verdict remains authoritative; this layer
# is additive and advisory.
option(ENABLE_PROCEDURAL_GATING "Build the experimental procedural gating/herald layer" OFF)
if(ENABLE_PROCEDURAL_GATING)
    add_subdirectory(${CMAKE_SOURCE_DIR}/gating ${CMAKE_BINARY_DIR}/gating)
    add_subdirectory(${CMAKE_SOURCE_DIR}/agent ${CMAKE_BINARY_DIR}/agent)
endif()
