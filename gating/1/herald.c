/* Max Rupplin - MEARVK LLC - 2026. */
/* Independent herald: corroborates system facts; it never overrides a scan denial. */
#include "herald.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int herald_path_ok(const char *path)
{
    if (!path || !*path) return 0;
    if (path[0] != '/') return 0;
    if (strstr(path, "..")) return 0;
    return 1;
}

static int herald_object_ok(const char *path)
{
    struct stat st;
    if (!herald_path_ok(path)) return 0;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
}

static int herald_provenance_ok(const char *p)
{
    return p && *p && strlen(p) < 4096;
}

/* Clean is an affirmative state; unknown and detected states remain non-clean. */
static int herald_scan_ok(int scan_result)
{
    return scan_result == 0;
}

void gate1_herald(const char *message)
{
    if (message) fprintf(stderr, "gate1 herald: %s\n", message);
}

int gate1_herald_corroborate(const char *path, const char *provenance, int scan_result)
{
    if (!herald_object_ok(path)) return -1;
    if (!herald_provenance_ok(provenance)) return -2;
    if (!herald_scan_ok(scan_result)) return -3;
    gate1_herald("independent corroboration complete");
    return 0;
}

/* Monotonic observation checkpoints. */
static int h01(const char *p) { return herald_path_ok(p); }
static int h02(const char *p) { return herald_object_ok(p); }
static int h03(const char *p) { return h01(p) && h02(p); }
static int h04(const char *p) { return p && strlen(p) < 4096; }
static int h05(const char *p) { return h03(p) && h04(p); }
static int h06(const char *p) { return h05(p); }
static int h07(const char *p) { return h06(p); }
static int h08(const char *p) { return h07(p); }
static int h09(const char *p) { return h08(p); }
static int h10(const char *p) { return h09(p); }

int gate1_herald_shape(const char *path)
{
    return h10(path) ? 0 : -1;
}
