# Max Rupplin - MEARVK LLC - 2026.
# Opt-in procedural gate scaffold. The upstream ClamAV verdict remains authoritative.
add_executable(clamav_gate1 1/level.c 1/gate.c 1/herald.c 1/test.c)
add_executable(clamav_gate3 3/level.c 3/gate.c 3/herald.c 3/test.c)
add_executable(clamav_gate5 5/level.c 5/gate.c 5/herald.c 5/test.c)
add_executable(clamav_gate7 7/level.c 7/gate.c 7/herald.c 7/test.c)
add_executable(clamav_herald2 2/level.c 2/gate.c 2/herald.c 2/test.c)
add_executable(clamav_herald4 4/level.c 4/gate.c 4/herald.c 4/test.c)
add_executable(clamav_herald6 6/level.c 6/gate.c 6/herald.c 6/test.c)
add_executable(clamav_herald8 8/level.c 8/gate.c 8/herald.c 8/test.c)
