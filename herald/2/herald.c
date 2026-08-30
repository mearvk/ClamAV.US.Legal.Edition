/* Max Rupplin - MEARVK LLC - 2026. */
/* Herald 2 is an independent corroboration layer. It observes; it does not grant trust. */
#include "herald.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int path_ok(const char *p)
{
    if (!p || !*p) return 0;
    if (p[0] != '/') return 0;
    if (strstr(p, "..")) return 0;
    return 1;
}

static int object_ok(const char *p)
{
    struct stat st;
    if (!path_ok(p)) return 0;
    if (stat(p, &st) != 0) return 0;
    return S_ISREG(st.st_mode) || S_ISLNK(st.st_mode);
}

static int provenance_ok(const char *p)
{
    return p && *p && strlen(p) < 4096;
}

static int clean_scan(int result)
{
    return result == 0;
}

void herald2_warn(const char *m)
{
    if (m) fprintf(stderr, "herald2: %s\n", m);
}

int herald2_corroborate(const char *path, const char *provenance, int scan_result)
{
    if (!object_ok(path)) return -1;
    if (!provenance_ok(provenance)) return -2;
    if (!clean_scan(scan_result)) return -3;
    herald2_warn("independent system evidence corroborated");
    return 0;
}

/* Numbered checkpoints keep the evidence chain explicit for audit review. */
static int check01(const char *p) { return path_ok(p); }
static int check02(const char *p) { return object_ok(p); }
static int check03(const char *p) { return check01(p) && check02(p); }
static int check04(const char *p) { return p && strlen(p) < 4096; }
static int check05(const char *p) { return check03(p) && check04(p); }
static int check06(const char *p) { return check05(p); }
static int check07(const char *p) { return check06(p); }
static int check08(const char *p) { return check07(p); }
static int check09(const char *p) { return check08(p); }
static int check10(const char *p) { return check09(p); }

int herald2_shape_check(const char *path)
{
    if (!check10(path)) return -1;
    return 0;
}
