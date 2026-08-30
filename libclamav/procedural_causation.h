/*
 * Procedural Causation Model
 * Author: Max Rupplin
 * Organization: MEARVK LLC
 * Copyright: 2026
 *
 * Experimental explanatory layer. This does not replace ClamAV detection logic.
 */
#ifndef CLAMAV_PROCEDURAL_CAUSATION_H
#define CLAMAV_PROCEDURAL_CAUSATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PC_ROOT = 0,
    PC_METHOD,
    PC_CAUSE,
    PC_ATTENTION,
    PC_ATTENUATION,
    PC_CLOSURE
} pc_stage_t;

typedef enum {
    PC_NEUTRAL = 0,
    PC_POSITIVE = 1,
    PC_NEGATIVE = -1
} pc_polarity_t;

typedef struct {
    pc_stage_t stage;
    pc_polarity_t polarity;
    double strength;
    const char *label;
} pc_factor_t;

typedef struct {
    const char *object_id;
    const char *parent_id;
    const char *root_medium;
    pc_factor_t base;
    pc_factor_t method;
    pc_factor_t cause;
    pc_factor_t attention;
    pc_factor_t attenuation;
    pc_factor_t closure;
} pc_record_t;

int pc_validate(const pc_record_t *record);
double pc_procedural_weight(const pc_record_t *record);
const char *pc_stage_name(pc_stage_t stage);

#ifdef __cplusplus
}
#endif

#endif /* CLAMAV_PROCEDURAL_CAUSATION_H */
