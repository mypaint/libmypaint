#include "mypaint-symmetry.h"
#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_NUM_MATRICES 16

void
allocation_failure_warning(int num)
{
    fprintf(stderr, "Critical: failed to allocate memory for %d transformation matrices!\n", num);
}

gboolean
allocate_symmetry_matrices(MyPaintSymmetryData* data, int num_matrices)
{
    int bytes = num_matrices * sizeof(MyPaintTransform);
    void* allocated = realloc(data->symmetry_matrices, bytes);
    if (!allocated) {
        allocation_failure_warning(num_matrices);
        data->num_symmetry_matrices = 0;
        return FALSE;
    } else {
        data->symmetry_matrices = allocated;
        data->num_symmetry_matrices = num_matrices;
        return TRUE;
    }
}

gboolean
symmetry_states_equal(const MyPaintSymmetryState* const s1, const MyPaintSymmetryState* const s2)
{
    return s1->type == s2->type && s1->center_x == s2->center_x && s1->center_y == s2->center_y &&
           s1->angle == s2->angle && s1->num_lines == s2->num_lines;
}

int
num_matrices_required(const MyPaintSymmetryState* state)
{
    switch (state->type) {
    case MYPAINT_SYMMETRY_TYPE_VERTICAL:
    case MYPAINT_SYMMETRY_TYPE_HORIZONTAL:
        return 1;
    case MYPAINT_SYMMETRY_TYPE_VERTHORZ:
        return 3;
    case MYPAINT_SYMMETRY_TYPE_ROTATIONAL:
        return state->num_lines - 1;
    case MYPAINT_SYMMETRY_TYPE_SNOWFLAKE:
        return 2 * state->num_lines - 1;
    default:
        return 0;
    }
}

/* Public functions */

MyPaintSymmetryState
symmetry_state_default()
{
    MyPaintSymmetryState base = {
        .type = MYPAINT_SYMMETRY_TYPE_VERTICAL, .center_x = 0.0, .center_y = 0.0, .angle = 0.0, .num_lines = 2};
    return base;
}

/* If the symmetry state has changed since last, recalculate matrices */
void
mypaint_update_symmetry_state(MyPaintSymmetryData* const self)
{
    if (!self->pending_changes || symmetry_states_equal(&self->state_current, &self->state_pending)) return;
    // Need to recalculate matrices
    // Check if we need to allocate more space
    const int required = num_matrices_required(&self->state_pending);
    if (self->num_symmetry_matrices < required) {
        // Try to allocate space for matrices, skip recalculations if it fails
        if (!allocate_symmetry_matrices(self, required)) return;
    }
    const MyPaintSymmetryState symm = self->state_pending;
    self->state_current = symm;
    float cx = symm.center_x;
    float cy = symm.center_y;
    // Convert angle to radians
    float angle = symm.angle * (M_PI / 180.0);
    float rot_angle = (2.0 * M_PI) / symm.num_lines;
    MyPaintTransform* matrices = self->symmetry_matrices;
    const MyPaintTransform m = mypaint_transform_translate(mypaint_transform_unit(), -cx, -cy);
    switch (symm.type) {
    case MYPAINT_SYMMETRY_TYPE_HORIZONTAL:
    case MYPAINT_SYMMETRY_TYPE_VERTICAL: {
        if (symm.type == MYPAINT_SYMMETRY_TYPE_VERTICAL) {
            angle += M_PI / 2.0;
        }
        matrices[0] = mypaint_transform_reflect(m, -angle);
    } break;
    case MYPAINT_SYMMETRY_TYPE_VERTHORZ: {
        float v_angle = angle + M_PI / 2.0;
        matrices[0] = mypaint_transform_reflect(m, -angle);
        matrices[1] = mypaint_transform_reflect(matrices[0], -v_angle);
        matrices[2] = mypaint_transform_reflect(matrices[1], -angle);
    } break;
    case MYPAINT_SYMMETRY_TYPE_SNOWFLAKE: {
        int base_idx = symm.num_lines - 1;
        for (int i = 0; i < symm.num_lines; ++i) {
            matrices[base_idx + i] =
                mypaint_transform_reflect(mypaint_transform_rotate_cw(m, rot_angle * i), -i * rot_angle - angle);
        }
    }
    case MYPAINT_SYMMETRY_TYPE_ROTATIONAL: {
        for (int i = 1; i < symm.num_lines; ++i) {
            matrices[i - 1] = mypaint_transform_rotate_cw(m, rot_angle * i);
        }
    } break;
    default:
        fprintf(stderr, "Warning: Unhandled symmetry type: %d\n", symm.type);
        return;
    }
    for (int i = 0; i < required; ++i) {
        matrices[i] = mypaint_transform_translate(matrices[i], cx, cy);
    }
    self->pending_changes = FALSE;
}

MyPaintSymmetryData
mypaint_default_symmetry_data()
{
    MyPaintSymmetryData symm_data = {
        .state_current = {.type = -1},
        .state_pending = symmetry_state_default(),
        .pending_changes = TRUE,
        .active = FALSE,
        .num_symmetry_matrices = DEFAULT_NUM_MATRICES,
        .symmetry_matrices = NULL,
    };
    if (allocate_symmetry_matrices(&symm_data, DEFAULT_NUM_MATRICES)) {
        mypaint_update_symmetry_state(&symm_data);
    }
    return symm_data;
}

void
mypaint_symmetry_data_destroy(MyPaintSymmetryData* data)
{
    if (data->symmetry_matrices != NULL) {
        free(data->symmetry_matrices);
    }
}

void
mypaint_symmetry_set_pending(
    MyPaintSymmetryData* data, gboolean active, float center_x, float center_y, float symmetry_angle,
    MyPaintSymmetryType symmetry_type, int rot_symmetry_lines)
{
    data->active = active;
    data->state_pending.center_x = center_x;
    data->state_pending.center_y = center_y;
    data->state_pending.type = symmetry_type;
    data->state_pending.num_lines = MAX(2, rot_symmetry_lines);
    data->state_pending.angle = symmetry_angle;

    data->pending_changes = TRUE;
}
