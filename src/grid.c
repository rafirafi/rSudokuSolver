
/*
 * This code is part of rSudokuSolver
 * Copyright (C) 2016 rafirafi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "grid.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
 * grid_validate_check_single        => if one color only in a rule it is true. rule{A} => A true
 * grid_merge_check_pair             => if 2 colors only in a rule they are exclusive (XOR). rule{A, B} => (A,B)->(C, -C)
 * grid_validate_check_pair_1        => if 2 times the same color in a unit it is false. rule{A, A, B, ...} => A false
 * grid_validate_check_pair_2        => if a color and its reverse in a rule, the other colors are false. rule{A, -A, B, ...} => (B, ...) false
 * grid_get_true_to_false_colors     => use rules to construct adjacency list : rule(A, B, C) + rule(A, D, E) => A true : B, C, D, E false
 * grid_merge_check_SCC              => find strong conn. component using adjacency list. (A true <=> B false AND A false <=> B true) => (A,B)->(C, -C)
 * grid_validate_check_cycle         => search contradiction : one color A tested as true is false when :
 *                                   1/ a rule empty OR 2/ any color true AND false at the same time
 *                                   Use true=>false pair and count color false in a rule, when one color remains it is injected as true
 * grid_validate_check_cycle_level_2 => construct a color 'tree' as in validate_check_cycle for a color A
 *                                   then begin a search from this 'tree' with another color B and its opposite -B
 *                                   if both leads to contradiction it means B and -B => A false, A is always false.
 */

int  grid_validate_node(Grid *grid, NodeId node_id);
#ifdef CHECK_GRID
int  grid_remove_node(Grid *grid, NodeId node_id);
#endif
int  grid_validate_enqueue(Grid *grid, Color color);
int  grid_validate_purge(Grid *grid);
int  grid_validate_color(Grid *grid, Color color);
int  grid_validate_check_single(Grid *grid);
int  grid_merge_enqueue(Grid *grid, const Color colors[2]);
int  grid_merge_check_pair(Grid *grid);
int  grid_merge_purge(Grid *grid);
int  grid_merge_colors(Grid *grid, const Color colors[2]);
void grid_remove_rule(Grid *grid, int idx);
int  grid_validate_check_pair_1(Grid *grid);
int  grid_validate_check_pair_2(Grid *grid);
int  grid_get_true_to_false_colors(Grid *grid);
int  grid_merge_check_SCC(Grid *grid);
int  grid_validate_check_cycle(Grid *grid);
int  grid_validate_check_cycle_level_2(Grid *grid);

static inline int grid_char_to_int(const char c)
{
    if (D == 3) {
        return c >= '1' && c <= '9' ? c - '1' : NA ;
    } else if (D == 4) {
        return (c >= '0' && c <= '9') ? c - '0' :
               ((c >= 'A' && c <= 'F') ? c - 'A' + 10 :
               ((c >= 'a' && c <= 'f') ? c - 'a' + 10 : NA));
    }
    return NA;
}

static inline char int_to_grid_char(const int n)
{
    if (D == 3) {
        return n + '1';
    } else if (D == 4) {
        return n < 10 ? '0' + n : 'A' - 10 + n;
    }
    return NA;
}

static inline Color rev_color(Color color)
{
    return -color;
}

static inline Color abs_color(Color color)
{
    return abs(color);
}

// NOTE: if fails no need to call grid_free
int grid_init(Grid *grid)
{
#ifdef CHECK_GRID
    for (int i = 0; i < kConstraintCount; i++) {
        memset(grid->constraint_cnt_check[i], 0x00, NN * sizeof(int));
    }
#endif
    memset(grid->validated_nodes, 0xFF, NN * sizeof(NodeId));
    grid->validated_size = 0;
    cvmap_init(&grid->color_to_nodes);
    cvmap_init(&grid->color_to_exclusion_idx);
    cvmap_init(&grid->true_to_false_colors);
    grid->color_exclusions = malloc(NN * 4 * sizeof(ColorVecMap));
    if (!grid->color_exclusions) {
        return NA;
    }
    for (int i = 0; i < NN * 4; i++) {
        ivec_init(&grid->color_exclusions[i]);
    }
    ivec_init(&grid->to_validate);
    ivec_init(&grid->to_merge);
    return 0;
}

int grid_copy(const Grid *src, Grid *dst)
{
#ifdef CHECK_GRID
    for (int i = 0; i < kConstraintCount; i++) {
        memcpy(dst->constraint_cnt_check[i], src->constraint_cnt_check[i], NN * sizeof(int));
    }
#endif
    memcpy(dst->validated_nodes, src->validated_nodes, NN * sizeof(NodeId));
    dst->validated_size = src->validated_size;
    GUARD(cvmap_copy(&src->color_to_nodes, &dst->color_to_nodes));
    GUARD(cvmap_copy(&src->color_to_exclusion_idx, &dst->color_to_exclusion_idx));
    GUARD(cvmap_copy(&src->true_to_false_colors, &dst->true_to_false_colors));
    if (!dst->color_exclusions) {
        return NA;
    }
    for (int i = 0; i < NN * 4; i++) {
        GUARD(ivec_copy(&src->color_exclusions[i], &dst->color_exclusions[i]));
    }
    GUARD(ivec_copy(&src->to_validate, &dst->to_validate));
    GUARD(ivec_copy(&src->to_merge, &dst->to_merge));

    return 0;
}

void grid_free(Grid *grid)
{
    cvmap_free(&grid->color_to_nodes);
    cvmap_free(&grid->color_to_exclusion_idx);
    cvmap_free(&grid->true_to_false_colors);
    if (grid->color_exclusions) {
        for (int i = 0; i < NN * 4; i++) {
            ivec_free(&grid->color_exclusions[i]);
        }
        free(grid->color_exclusions);
    }
    ivec_free(&grid->to_validate);
    ivec_free(&grid->to_merge);
}

int grid_init_data(Grid *grid)
{
    for (int i = 0; i < N * NN; i++) {
        GUARD(cvmap_insert_one(&grid->color_to_nodes, i + 1, i));
    }

    int excl_cnt = 0;
    for (int u = 0; u < N * NN; u += N, excl_cnt++) {
        for (int i = 0; i < N; i++) {
            GUARD(ivec_push_back(&grid->color_exclusions[excl_cnt], u + i + 1));
        }
    }
    for (int cand = 0; cand < N; cand++) {
        for (int col = 0; col < N; col++) {
            for (int row = 0; row < N; row++) {
                int u = (row * N + col) * N + cand;
                GUARD(ivec_push_back(&grid->color_exclusions[excl_cnt], u + 1));
            }
            excl_cnt++;
        }
        for (int row = 0; row < N; row++) {
            for (int col = 0; col < N; col++) {
                int u = (row * N + col) * N + cand;
                GUARD(ivec_push_back(&grid->color_exclusions[excl_cnt], u + 1));
            }
            excl_cnt++;
        }
        for (int box = 0; box < N; box++) {
            int col_beg = (box % D) * D;
            int row_beg = (box / D) * D;
            for (int idx = 0; idx < N; idx++) {
                int col = col_beg + idx % D;
                int row = row_beg + idx / D;
                int u = (row * N + col) * N + cand;
                GUARD(ivec_push_back(&grid->color_exclusions[excl_cnt], u + 1));
            }
            excl_cnt++;
        }
    }

    for (int excl_idx = 0; excl_idx < kUnitCount * NN; excl_idx++) {
        for (int i = 0; i < N; i++) {
            const Color color = ivec_at_idx(&grid->color_exclusions[excl_idx], i);
            GUARD(cvmap_insert_one(&grid->color_to_exclusion_idx, color, excl_idx));
        }
    }

    return 0;
}

int grid_validate_node(Grid *grid, NodeId node_id)
{
    grid->validated_size++;
    grid->validated_nodes[node_id / N] = node_id;

#ifdef CHECK_GRID
    int cand = node_id % N, row_col = node_id / N;
    int row = row_col / N, col = row_col % N;
    int box = (row / D) * D + (col / D);
    int constraints[kConstraintCount] = {
        row_col, row * N + cand, col * N + cand, box * N + cand
    };

    for (int i = 0; i < kConstraintCount; i++) {
        // NN used as solved flag
        if (grid->constraint_cnt_check[i][constraints[i]] != NN) {
            grid->constraint_cnt_check[i][constraints[i]] = NN;
        } else {
            PRINT_INFO("%s invalid grid\n", __func__);
            return NA;
        }
    }
#endif
    return 0;
}

#ifdef CHECK_GRID
int grid_remove_node(Grid *grid, NodeId node_id)
{
    int cand = node_id % N, row_col = node_id / N;
    int row = row_col / N, col = row_col % N;
    int box = (row / D) * D + (col / D);
    int constraints[kConstraintCount] = {
        row_col, row * N + cand, col * N + cand, box * N + cand
    };

    for (int i = 0; i < kConstraintCount; i++) {
        if (grid->constraint_cnt_check[i][constraints[i]] != NN) {
            grid->constraint_cnt_check[i][constraints[i]]++;
            if (grid->constraint_cnt_check[i][constraints[i]] == N) {
                PRINT_INFO("%s invalid grid\n", __func__);
                return NA;
            }
        }
    }

    return 0;
}
#endif

int grid_validate_enqueue(Grid *grid, Color color)
{
#ifdef CHECK_GRID
    if (ivec_find_first_from(&grid->to_validate, 0, rev_color(color)) != NA) {
        PRINT_INFO("%s invalid grid, reverse color and color %+4d are true\n", __func__, color);
        return NA;
    }
#endif
    if (ivec_find_first_from(&grid->to_validate, 0, color) == NA) {
        GUARD(ivec_push_back(&grid->to_validate, color));
        return 1;
    }
    return 0;
}

int grid_populate(Grid *grid, const char *grid_str)
{
    PRINT_INFO("%s\n", __func__);

    if (strlen(grid_str) != NN) {
        PRINT_INFO("%s invalid size %ld\n", __func__, strlen(grid_str));
        return NA;
    }
    int clues = 0;
    for (int i = 0; i < NN; i++) {
        int n = grid_char_to_int(grid_str[i]);
        if (n != NA) {
            clues++;
            int u = i * N + n;
            GUARD(grid_validate_enqueue(grid, u + 1));
        }
    }
    if (D == 3 && clues < 17) {
        PRINT_INFO("%s not enough clues\n", __func__);
        return NA;
    }

    PRINT_INFO("%s clues count %d\n", __func__, clues);

    return 0;
}

int grid_validate_purge(Grid *grid)
{
    int result = 0;
#ifdef CHECK_GRID
    while (ivec_size(&grid->to_validate) != 0) {
        Color color = ivec_at_idx(&grid->to_validate, 0);
        ivec_erase_at_idx(&grid->to_validate, 0);
        int ret = grid_validate_color(grid, color);
        GUARD(ret);
        result += ret;
    }
#else
    int size = ivec_size(&grid->to_validate);
    while (size != 0) {
        Color color = ivec_at_idx(&grid->to_validate, size - 1);
        ivec_erase_at_idx(&grid->to_validate, size - 1);
        int ret = grid_validate_color(grid, color);
        GUARD(ret);
        result += ret;
        size = ivec_size(&grid->to_validate);
    }
#endif
    return result;
}

int  grid_validate_color(Grid *grid, Color color)
{
    assert(ivec_size(&grid->to_merge) == 0);

    // validate
    if (cvmap_count(&grid->color_to_nodes, color) != 0) {

        const IntVec *colors = cvmap_get_IntVec(&grid->color_to_nodes, color);
        for (int i = 0, iend = ivec_size(colors); i < iend; i++) {
            NodeId node_id = ivec_at_idx(colors, i);
            GUARD(grid_validate_node(grid, node_id));
        }
        cvmap_erase(&grid->color_to_nodes, color);

        if (cvmap_count(&grid->color_to_exclusion_idx, color) != 0) {
            const IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, color);
            for (int i = 0, iend = ivec_size(idxs); i < iend; i++) {
                int idx = ivec_at_idx(idxs, i);
                for (int j = 0, jend = ivec_size(&grid->color_exclusions[idx]); j < jend; j++) {
                    Color o_color = ivec_at_idx(&grid->color_exclusions[idx], j);
                    if (color != o_color) {
                        if (cvmap_count(&grid->color_to_exclusion_idx, o_color) != 0) {
                            IntVec *o_idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, o_color);
                            int idx_idx = ivec_find_first_from(o_idxs, 0, idx);
                            // check necessary as colors can be duplicated but the idx is not
                            if (idx_idx != NA) {
                                ivec_erase_at_idx(o_idxs, idx_idx);
                            }
                        }
                        GUARD(grid_validate_enqueue(grid, rev_color(o_color)));
                    }
                }
                ivec_clear(&grid->color_exclusions[idx]);
            }
            cvmap_erase(&grid->color_to_exclusion_idx, color);
        }
    }

    // unvalidate
    color = rev_color(color);

    if (cvmap_count(&grid->color_to_nodes, color) != 0) {
#ifdef CHECK_GRID
        const IntVec *colors = cvmap_get_IntVec(&grid->color_to_nodes, color);
        for (int i = 0, iend = ivec_size(colors); i < iend; i++) {
            NodeId node_id = ivec_at_idx(colors, i);
            GUARD(grid_remove_node(grid, node_id));
        }
#endif
        cvmap_erase(&grid->color_to_nodes, color);
        // remove every occurence of color from rules
        if (cvmap_count(&grid->color_to_exclusion_idx, color) != 0) {
            const IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, color);
            for (int i = 0, iend = ivec_size(idxs); i < iend; i++) {
                int idx = ivec_at_idx(idxs, i);
                int idx_idx = 0;
                while ((idx_idx = ivec_find_first_from(&grid->color_exclusions[idx], idx_idx, color)) != NA) {
                    ivec_erase_at_idx(&grid->color_exclusions[idx], idx_idx);
                }
            }
            cvmap_erase(&grid->color_to_exclusion_idx, color);
        }
    }

    return 0;
}

// return NA in case of alloc faillure
// and if CHECK_GRID is defined in case of invalid grid
int grid_solve(Grid *grid)
{
    while (1) {
        int ret = 0;
        do {
            GUARD(grid_validate_purge(grid));
            ret = grid_validate_check_single(grid);
            GUARD(ret);
        } while (ret > 0);

        if (grid->validated_size == NN) {
            return NN;
        }

        do {
            ret = grid_merge_check_pair(grid);
            GUARD(ret);
            if (ret > 0) {
                GUARD(grid_merge_purge(grid));
            }
        } while(ret > 0);

        ret = grid_validate_check_pair_1(grid);
        GUARD(ret);
        if (ret > 0) {
            continue;
        }

        ret = grid_validate_check_pair_2(grid);
        GUARD(ret);
        if (ret > 0) {
            continue;
        }

        GUARD(grid_get_true_to_false_colors(grid));

        ret = grid_merge_check_SCC(grid);
        GUARD(ret);
        if (ret > 0) {
            GUARD(grid_merge_purge(grid));
            continue;
        }

        ret = grid_validate_check_cycle(grid);
        GUARD(ret);
        if (ret > 0) {
            continue;
        }

        ret = grid_validate_check_cycle_level_2(grid);
        GUARD(ret);
        if (ret > 0) {
            continue;
        }

        break;
    }

    return grid->validated_size;
}

int grid_validate_check_single(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        if (ivec_size(&grid->color_exclusions[idx]) == 1) {
            Color color = ivec_at_idx(&grid->color_exclusions[idx], 0);
            int ret = grid_validate_enqueue(grid, color);
            GUARD(ret);
            result += ret;
        }
    }
    return result;
}

int grid_merge_enqueue(Grid *grid, const Color colors[2])
{ 
    if (abs_color(colors[0]) == abs_color(colors[1])) {
#ifdef CHECK_GRID
        if ((colors[0] < 0) != (colors[1] < 0)) {
            PRINT_INFO("%s invalid grid merging reverse colors\n", __func__);
            return NA;
        }
#endif
        return 0;
    }
    if (ivec_absolute_pair_exists(&grid->to_merge, colors) == NA) {
        GUARD(ivec_push_back(&grid->to_merge, colors[0]));
        GUARD(ivec_push_back(&grid->to_merge, colors[1]));
        return 1;
    }

    return 0;
}

int grid_merge_check_pair(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        if (ivec_size(&grid->color_exclusions[idx]) == 2) {
            const Color colors[2] = { ivec_at_idx(&grid->color_exclusions[idx], 0),
                                      rev_color(ivec_at_idx(&grid->color_exclusions[idx], 1))};
            int ret = grid_merge_enqueue(grid, colors);
            GUARD(ret);
            result += ret;
            grid_remove_rule(grid, idx);
        }
    }
    return result;
}

int grid_merge_purge(Grid *grid)
{
    int size = ivec_size(&grid->to_merge);
    while (size != 0) {
        Color colors[2] = { ivec_at_idx(&grid->to_merge, size - 2),
                            ivec_at_idx(&grid->to_merge, size - 1)};
        ivec_erase_at_idx(&grid->to_merge, size - 1);
        ivec_erase_at_idx(&grid->to_merge, size - 2);
        GUARD(grid_merge_colors(grid, colors));
        size = ivec_size(&grid->to_merge);
    }
    return 0;
}

void grid_remove_rule(Grid *grid, int idx)
{
    for (int i = 0, iend = ivec_size(&grid->color_exclusions[idx]); i < iend; i++) {
        Color color = ivec_at_idx(&grid->color_exclusions[idx], i);
        if (cvmap_count(&grid->color_to_exclusion_idx, color) != 0) {
            IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, color);
            int idx_idx = 0;
            while ((idx_idx = ivec_find_first_from(idxs, idx_idx, idx)) != NA) {
                ivec_erase_at_idx(idxs, idx_idx);
            }
            // TODO : ? remove idxs if empty
        }
    }
    ivec_clear(&grid->color_exclusions[idx]);
}

int grid_merge_colors(Grid *grid, const Color colors[2])
{
    assert(ivec_size(&grid->to_validate) == 0);

    if (abs_color(colors[0]) == abs_color(colors[1])) {
#ifdef CHECK_GRID
        if ((colors[0] < 0) != (colors[1] < 0)) {
            PRINT_INFO("%s invalid grid\n", __func__);
            return NA;
        }
#endif
        return 0;
    }

    PRINT_INFO("%s %+4d %+4d\n", __func__, colors[0], colors[1]);

    Color src = colors[0], dst = colors[1];
    for (int i = 0; i < 2; i++) {
        if (i != 0) {
            src = rev_color(src), dst = rev_color(dst);
        }
        for (int j = 0, jend = ivec_size(&grid->to_merge); j < jend; j++) {
            // change color
            Color *color = ivec_ptr_at_idx(&grid->to_merge, j);
            if (*color == src) {
                *color = dst;
            }
        }
        if (cvmap_count(&grid->color_to_nodes, src) != 0) {
            const IntVec *src_nodes = cvmap_get_IntVec(&grid->color_to_nodes, src);
            for (int j = 0, jend = ivec_size(src_nodes); j < jend; j++) {
                GUARD(cvmap_insert_one(&grid->color_to_nodes, dst, ivec_at_idx(src_nodes, j)));
            }
            cvmap_erase(&grid->color_to_nodes, src);
        }

        if (cvmap_count(&grid->color_to_exclusion_idx, src) != 0) {
            const IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, src);
            for (int j = 0, jend = ivec_size(idxs); j < jend; j++) {
                int idx = ivec_at_idx(idxs, j);
                // as result of consecutive merges, duplicate can exist
                int idx_idx = 0;
                while ((idx_idx = ivec_find_first_from(&grid->color_exclusions[idx], idx_idx, src)) != NA) {
                    Color *color = ivec_ptr_at_idx(&grid->color_exclusions[idx], idx_idx);
                    *color = dst;
                    idx_idx++;
                }
                if (cvmap_count(&grid->color_to_exclusion_idx, dst) == 0
                        || ivec_find_first_from(cvmap_get_IntVec(&grid->color_to_exclusion_idx, dst), 0, idx) == NA) {
                    GUARD(cvmap_insert_one(&grid->color_to_exclusion_idx, dst, idx));
                }
            }
            cvmap_erase(&grid->color_to_exclusion_idx, src);
        }
    }

    return 1;
}

int grid_validate_check_pair_1(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        const int size = ivec_size(&grid->color_exclusions[idx]);
        if (size > 2) {
            for (int i = 0; i < size; i++) {
                Color color = ivec_at_idx(&grid->color_exclusions[idx], i);
                for (int j = i + 1; j < size; j++) {
                    if (color == ivec_at_idx(&grid->color_exclusions[idx], j)) {
                        int ret = grid_validate_enqueue(grid, rev_color(color));
                        GUARD(ret);
                        result += ret;
                    }
                }
            }
        }
    }
    return result;
}

int grid_validate_check_pair_2(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        const int size = ivec_size(&grid->color_exclusions[idx]);
        if (size <= 2) {
            continue;
        }
        for (int i = 0, done = 0; i < size && !done; i++) {
            Color r_color = rev_color(ivec_at_idx(&grid->color_exclusions[idx], i));
            for (int j = i + 1; j < size && !done; j++) {
                if (r_color == ivec_at_idx(&grid->color_exclusions[idx], j)) {
                    for (int k = 0; k < size; k++) {
                        Color color = ivec_at_idx(&grid->color_exclusions[idx], k);
                        if (abs_color(r_color) == abs_color(color)) {
                            continue;
                        }
                        int ret = grid_validate_enqueue(grid, rev_color(color));
                        GUARD(ret);
                        result += ret;
                    }
                    done = 1;
                    break;
                }
            }
        }
    }
    return result;
}

int grid_get_true_to_false_colors(Grid *grid)
{
    cvmap_clear(&grid->true_to_false_colors);

    const IntVec *keys = cvmap_keys(&grid->color_to_exclusion_idx);
    for (int i = 0, iend = ivec_size(keys); i < iend; i++) {
        Color color = ivec_at_idx(keys, i);
        const IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, color);
        for (int j = 0, jend = ivec_size(idxs); j < jend; j++) {
            int idx = ivec_at_idx(idxs, j);
            const IntVec *color_exclusion = &grid->color_exclusions[idx];
            int size = ivec_size(color_exclusion);
            if (size <= 2) {
                continue;
            }
            for (int k = 0; k < size; k++) {
                Color o_color = ivec_at_idx(color_exclusion, k);
                if (color == o_color) {
                    continue;
                }
                assert(color != rev_color(o_color));
                if (cvmap_count(&grid->true_to_false_colors, color) == 0
                        || ivec_find_first_from(cvmap_get_IntVec(&grid->true_to_false_colors, color), 0, o_color) == NA) {
                    GUARD(cvmap_insert_one(&grid->true_to_false_colors, color, o_color));
                }
                if (cvmap_count(&grid->true_to_false_colors, o_color) == 0
                        || ivec_find_first_from(cvmap_get_IntVec(&grid->true_to_false_colors, o_color), 0, color) == NA) {
                    GUARD(cvmap_insert_one(&grid->true_to_false_colors, o_color, color));
                }
            }
        }
    }

    return 0;
}

typedef struct SCCSearch {
    VertexMap indices, low_links;
    IntVec stack_color, stack_polarity;
} SCCSearch;

static int ss_on_stack(const SCCSearch *ss, const Vertex *v)
{
    int idx = 0;
    while ((idx = ivec_find_first_from(&ss->stack_color, idx, v->first)) != NA) {
        if (ivec_at_idx(&ss->stack_polarity, idx) == v->second) {
            return idx;
        }
        idx++;
    }
    return NA;
}

// Tarjan algo.
static int ss_strong_connect(Grid *grid, SCCSearch *ss, Vertex v, int cur_index)
{
    int result = 0;

    vmap_assign(&ss->indices, &v, cur_index);
    vmap_assign(&ss->low_links, &v, cur_index);

    cur_index = cur_index + 1;

    GUARD(ivec_push_back(&ss->stack_color, v.first));
    GUARD(ivec_push_back(&ss->stack_polarity, v.second));

    Vertex w;
    w.second = !v.second;

    const IntVec *false_colors = (cvmap_count(&grid->true_to_false_colors, v.first)
                              ? cvmap_get_IntVec(&grid->true_to_false_colors, v.first) : NULL);
    for (int i = -1, iend = ((v.second && false_colors) ? ivec_size(false_colors) : 0); i < iend; i++) {
        w.first = (i == -1 ? rev_color(v.first) : ivec_at_idx(false_colors, i));
        if (vmap_count(&ss->indices, &w) == 0) {
            int ret = ss_strong_connect(grid, ss, w, cur_index);
            GUARD(ret);
            result += ret;
            if (vmap_get(&ss->low_links, &w) < vmap_get(&ss->low_links, &v)) {
                vmap_assign(&ss->low_links, &v, vmap_get(&ss->low_links, &w));
            }
        } else if (ss_on_stack(ss, &w) != NA) {
            if (vmap_get(&ss->indices, &w) < vmap_get(&ss->low_links, &v)) {
                vmap_assign(&ss->low_links, &v, vmap_get(&ss->indices, &w));
            }
        }
    }
    if (vmap_get(&ss->low_links, &v) == vmap_get(&ss->indices, &v)) {
        Vertex y;
        int cnt = 0;
        Color colors[2];
        do {
            int size = ivec_size(&ss->stack_color);
            y.first = ivec_at_idx(&ss->stack_color, size - 1);
            ivec_erase_at_idx(&ss->stack_color, size - 1);
            y.second = ivec_at_idx(&ss->stack_polarity, size - 1);
            ivec_erase_at_idx(&ss->stack_polarity, size - 1);
            if (cnt == 0) {
                colors[0] = (y.second ? y.first : rev_color(y.first));
            } else {
                colors[1] = (y.second ? y.first : rev_color(y.first));
                int ret = grid_merge_enqueue(grid, colors);
                GUARD(ret);
                result += ret;
            }
            cnt++;
        }  while ((y.first != v.first) || (y.second != v.second));
    }
    return result;
}


int grid_merge_check_SCC(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    SCCSearch ss;
    // init
    vmap_clear(&ss.indices);
    vmap_clear(&ss.low_links);
    ivec_init(&ss.stack_color);
    ivec_init(&ss.stack_polarity);

    const int cur_index = 1;
    const IntVec *keys = cvmap_keys(&grid->color_to_nodes);
    for (int i = 0, iend = ivec_size(keys); i < iend; i++) {
        Color color = ivec_at_idx(keys, i);
        Vertex x = { color, 1 };
        if (vmap_count(&ss.indices, &x) == 0) {
            int ret = ss_strong_connect(grid, &ss, x, cur_index);
            if (ret == NA) {
                result = NA;
                break;
            }
            result += ret;
        }
    }

    // clean up
    ivec_free(&ss.stack_color);
    ivec_free(&ss.stack_polarity);

    return result;
}

static int grid_validate_check_cycle_dfs(Grid *grid, VertexMap *visited, IntVec *excl_color_cnt, Vertex v)
{
    vmap_assign(visited, &v, 1);

    // use exclusion rule constraint during the search
    if (v.second == 0 && cvmap_count(&grid->color_to_exclusion_idx, v.first) != 0) {
        const IntVec *idxs = cvmap_get_IntVec(&grid->color_to_exclusion_idx, v.first);
        for (int i = 0, iend = ivec_size(idxs); i < iend; i++) {
            int idx = ivec_at_idx(idxs, i);
            (*(ivec_ptr_at_idx(excl_color_cnt, idx)))--;
            assert(ivec_at_idx(excl_color_cnt, idx) >= 0);
        }
        for (int i = 0, iend = ivec_size(idxs); i < iend; i++) {
            int idx = ivec_at_idx(idxs, i);
            int excl_color_cnt_at_idx = ivec_at_idx(excl_color_cnt, idx);
            if (excl_color_cnt_at_idx == 0) {
                return 0;
            } else if (excl_color_cnt_at_idx == 1) {
                for (int j = 0, jend = ivec_size(&grid->color_exclusions[idx]); j < jend; j++) {
                    Color color = ivec_at_idx(&grid->color_exclusions[idx], j);
                    Vertex x = {color, 0 };
                    if (vmap_count(visited, &x) == 0) {
                        x.second = 1;
                        if (grid_validate_check_cycle_dfs(grid, visited, excl_color_cnt, x) == 1) {
                            return 1;
                        }
                        break;
                    }
                }
            }
        }
    }

    Vertex w, rw;
    w.second = !v.second;
    rw.second = v.second;
    const IntVec *false_colors = (cvmap_count(&grid->true_to_false_colors, v.first)
                              ? cvmap_get_IntVec(&grid->true_to_false_colors, v.first) : NULL);
    for (int i = -1, iend = ((v.second && false_colors) ? ivec_size(false_colors) : 0); i < iend; i++) {
        w.first = rw.first = (i == -1 ? rev_color(v.first) : ivec_at_idx(false_colors, i));
        if (vmap_count(visited, &rw) != 0
                || (vmap_count(visited, &w) == 0
                    && grid_validate_check_cycle_dfs(grid, visited, excl_color_cnt, w) == 1)) {
            return 1;
        }
    }

    return 0;
}

int grid_validate_check_cycle(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    IntVec excl_color_cnt_base;
    ivec_init(&excl_color_cnt_base);
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        if (ivec_push_back(&excl_color_cnt_base, ivec_size(&grid->color_exclusions[idx])) == NA) {
            ivec_free(&excl_color_cnt_base);
            return NA;
        }
    }
    IntVec excl_color_cnt;
    ivec_init(&excl_color_cnt);
    VertexMap visited;

    const IntVec *keys = cvmap_keys(&grid->color_to_nodes);
    for (int i = 0, iend = ivec_size(keys); i < iend; i++) {
        Color color = ivec_at_idx(keys, i);
        Vertex v = { color, 1 };
        if (ivec_copy(&excl_color_cnt_base, &excl_color_cnt) == NA) {
            result = NA;
            break;
        }
        vmap_clear(&visited);
        if (grid_validate_check_cycle_dfs(grid, &visited, &excl_color_cnt, v) == 1) {
            int ret = grid_validate_enqueue(grid, rev_color(color));
            if (ret == NA) {
                result = NA;
                break;
            }
            result += ret;
        }
    }

    ivec_free(&excl_color_cnt);
    ivec_free(&excl_color_cnt_base);

    return result;
}

int grid_validate_check_cycle_level_2(Grid *grid)
{
    PRINT_INFO("%s\n", __func__);
    int result = 0;
    IntVec excl_color_cnt_base;
    ivec_init(&excl_color_cnt_base);
    for (int idx = 0; idx < kUnitCount * NN; idx++) {
        if (ivec_push_back(&excl_color_cnt_base, ivec_size(&grid->color_exclusions[idx])) == NA) {
            ivec_free(&excl_color_cnt_base);
            return NA;
        }
    }
    IntVec excl_color_cnt;
    ivec_init(&excl_color_cnt);
    IntVec excl_color_cnt_bak;
    ivec_init(&excl_color_cnt_bak);
    VertexMap visited, visited_bak;

    const IntVec *keys = cvmap_keys(&grid->color_to_nodes);
    for (int i = 0, iend = ivec_size(keys); i < iend; i++) {
        Color color = ivec_at_idx(keys, i);
        Vertex v = { color, 1 };
        if (ivec_copy(&excl_color_cnt_base, &excl_color_cnt) == NA) {
            result = NA;
            break;
        }
        vmap_clear(&visited);
        // 1st level : A true => colors reachable in true or false state
        if (grid_validate_check_cycle_dfs(grid, &visited, &excl_color_cnt, v) != 0) {
            PRINT_INFO("%s Check cycle level 1 before, stopping search now\n", __func__);
            result = NA;
            break;
        }
        // 2d level : B and -B true are not reachable by A.
        // B true => A false, -B true => A false, and B true XOR -B true :  => A false
        visited_bak = visited;
        if (ivec_copy(&excl_color_cnt, &excl_color_cnt_bak) == NA) {
            result = NA;
            break;
        }
        for (int j = 0; j < iend; j++) {
            Color o_color = ivec_at_idx(keys, j);
            if (color == o_color || color == rev_color(o_color)) {
                continue;
            }
            Vertex ws[2] = { {o_color, 1}, {rev_color(o_color), 1} };
            visited = visited_bak;
            if (ivec_copy(&excl_color_cnt_bak, &excl_color_cnt) == NA) {
                result = NA;
                break;
            }
            if (grid_validate_check_cycle_dfs(grid, &visited, &excl_color_cnt, ws[0]) == 1) {
                visited = visited_bak;
                if (ivec_copy(&excl_color_cnt_bak, &excl_color_cnt) == NA) {
                    result = NA;
                    break;
                }
                if (grid_validate_check_cycle_dfs(grid, &visited, &excl_color_cnt, ws[1]) == 1) {
                    int ret = grid_validate_enqueue(grid, rev_color(color));
                    if (ret == NA) {
                        result = NA;
                        break;
                    }
                    result += ret;
                    if (result) {
                        break;
                    }
                }
            }
        }
        // search is costly, break early
        if (result != 0) {
            break;
        }
    }

    ivec_free(&excl_color_cnt);
    ivec_free(&excl_color_cnt_base);
    ivec_free(&excl_color_cnt_bak);

    return result;
}

void grid_get_grid_str(Grid *grid, char str[NN + 1])
{
    for (int i = 0; i < NN; i++) {
        int u = grid->validated_nodes[i];
        str[i] = (u == NA ? '.' : int_to_grid_char(u % N));
    }
    str[NN] = '\0';
}

void grid_get_cands_str(Grid *grid, char str[NN * N + 1])
{
    memset(str, '.', NN * N);
    str[NN * N] = '\0';

    const IntVec *keys = cvmap_keys(&grid->color_to_nodes);
    for (int i = 0, iend = ivec_size(keys); i < iend; i++) {
        Color color = ivec_at_idx(keys, i);
        const IntVec *node_ids = cvmap_get_IntVec(&grid->color_to_nodes, color);
        for (int j = 0, jend = ivec_size(node_ids); j < jend; j++) {
            int u = ivec_at_idx(node_ids, j);
            str[u] = int_to_grid_char(u % N);
        }
    }
    for (int i = 0; i < NN; i++) {
        int u = grid->validated_nodes[i];
        if (u != NA) {
            str[u] = int_to_grid_char(u % N);
        }
    }
}
