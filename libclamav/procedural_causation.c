/*
 * Procedural Causation Model
 * Author: Max Rupplin
 * Organization: MEARVK LLC
 * Copyright: 2026
 */
#include "procedural_causation.h"

#include <math.h>

static double clamp01(double v)
{
    if (v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

int pc_validate(const pc_record_t *record)
{
    if (!record || !record->object_id || !record->root_medium)
        return 0;
    if (record->base.stage != PC_ROOT ||
        record->method.stage != PC_METHOD ||
        record->cause.stage != PC_CAUSE ||
        record->attention.stage != PC_ATTENTION ||
        record->attenuation.stage != PC_ATTENUATION ||
        record->closure.stage != PC_CLOSURE)
        return 0;
    return 1;
}

double pc_procedural_weight(const pc_record_t *record)
{
    if (!pc_validate(record))
        return 0.0;

    /*
     * Explanatory weighting only: it is deliberately not a malware score.
     * Positive/negative polarity expresses procedural influence, not moral
     * or legal character. Attention increases observability; attenuation
     * reduces the effective contribution of a cause; closure records the
     * terminal procedural condition.
     */
    const double b = clamp01(record->base.strength);
    const double m = clamp01(record->method.strength);
    const double c = clamp01(record->cause.strength);
    const double a = clamp01(record->attention.strength);
    const double at = clamp01(record->attenuation.strength);
    const double cl = clamp01(record->closure.strength);
    const double raw = (b + m + c + a + cl) / 5.0;
    return raw * (1.0 - at);
}

const char *pc_stage_name(pc_stage_t stage)
{
    switch (stage) {
    case PC_ROOT: return "root";
    case PC_METHOD: return "method";
    case PC_CAUSE: return "cause";
    case PC_ATTENTION: return "attention";
    case PC_ATTENUATION: return "attenuation";
    case PC_CLOSURE: return "closure";
    default: return "unknown";
    }
}
