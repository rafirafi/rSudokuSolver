
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

#ifndef CUSTOMTYPES_H
#define CUSTOMTYPES_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "consts.h"

inline int color_to_idx(Color color)
{
    assert(color);
    return color > 0 ? color : N * NN +  abs(color);
}

/*****************************************************************/

typedef struct
{
    int store[2][2 * N * NN + 1];
} VertexMap;


inline int vmap_count(const VertexMap *map, const Vertex *v)
{
    assert(v);
    return map->store[v->second][color_to_idx(v->first)] ? 1 : 0;
}

inline void vmap_clear(VertexMap *map)
{
    memset(map->store[0], 0x00, (2 * N * NN + 1) * sizeof(int));
    memset(map->store[1], 0x00, (2 * N * NN + 1) * sizeof(int));
}

inline int vmap_get(const VertexMap *map, const Vertex *v)
{
    assert(v);
    return map->store[v->second][color_to_idx(v->first)];
}

// value 0 is not an option as it is used as empty flag
inline void vmap_assign(VertexMap *map, const Vertex *v, int value)
{
    assert(v);
    assert(value);
    map->store[v->second][color_to_idx(v->first)] = value;
}

/*****************************************************************/

typedef struct
{
    int *store;
    int size;
    int capacity;
} IntVec;

inline void ivec_init(IntVec *vec)
{
    memset(vec, 0x00, sizeof(IntVec));
}

inline void ivec_clear(IntVec *vec)
{
    vec->size = 0;
}

inline void ivec_free(IntVec *vec)
{
    if (vec->store != NULL) {
        free(vec->store);
        vec->store = NULL;
    }
}

inline int ivec_alloc_store(IntVec *vec)
{
    int ret = 0;
    if (vec->capacity == 0) {
        vec->capacity = N;
        vec->store = malloc(vec->capacity * sizeof(int));
        if (!vec->store) {
            ret = NA;
        }
    } else {
        vec->capacity *= 2;
        int *old_store = vec->store;
        vec->store = realloc(vec->store, vec->capacity * sizeof(int));
        if (!vec->store) {
            vec->store = old_store;
            ret = NA;
        }
    }
    return ret;
}

inline int ivec_push_back(IntVec *vec, int value)
{
    if (!vec->store || vec->capacity == vec->size) {
        GUARD(ivec_alloc_store(vec));
    }
    assert(vec->capacity > vec->size);
    vec->store[vec->size++] = value;
    return 0;
}

inline int ivec_find_first_from(const IntVec *vec, int idx_start, int value)
{
    int ret = NA;
    for (int i = idx_start, iend = vec->size; i < iend; i++) {
        if (vec->store[i] == value) {
            ret = i;
            break;
        }
    }
    return ret;
}

inline int ivec_absolute_pair_exists(const IntVec *vec, const int values[2])
{
    assert(vec->size % 2 == 0);
    int ret = NA;
    for (int i = 0, iend = vec->size; i < iend; i += 2) {
        if ((abs(vec->store[i]) == abs(values[0]) && abs(vec->store[i + 1]) == abs(values[1]))
                || (abs(vec->store[i]) == abs(values[1]) && abs(vec->store[i + 1]) == abs(values[0]))) {
            ret = i;
            break;
        }
    }
    return ret;
}

inline void ivec_erase_at_idx(IntVec *vec, int idx)
{
    assert(idx >= 0);
    assert(idx < vec->size);
    if (idx != vec->size - 1) {
        memmove(vec->store + idx, vec->store + idx + 1, (vec->size - (idx + 1)) * sizeof(int));
    }
    vec->size--;
}

inline void ivec_erase_one(IntVec *vec, int value)
{
    int idx = ivec_find_first_from(vec, 0, value);
    if (idx == NA) {
        return;
    }
    ivec_erase_at_idx(vec, idx);
}

inline int ivec_at_idx(const IntVec *vec, int idx)
{
    assert(idx >= 0);
    assert(idx < vec->size);
    return vec->store[idx];
}

inline int *ivec_ptr_at_idx(IntVec *vec, int idx)
{
    assert(idx >= 0);
    assert(idx < vec->size);
    return &vec->store[idx];
}

inline int ivec_copy(const IntVec *src, IntVec *dst)
{
    if (dst->capacity < src->size) {
        dst->capacity = src->size;
        int *old_store = dst->store;
        dst->store = realloc(dst->store, dst->capacity * sizeof(int));
        if (!dst->store) {
            dst->store = old_store;
            return NA;
        }
    }
    if (src->size && src->store) {
        dst->store = memcpy(dst->store, src->store, src->size * sizeof(int));
    }
    dst->size = src->size;
    return 0;
}

inline int ivec_size(const IntVec *vec)
{
    return vec->size;
}

/*****************************************************************/

typedef struct
{
    IntVec list;
    int marked[2 * N * NN + 1];
    IntVec store[2 * N * NN + 1];
} ColorVecMap;

inline void cvmap_init(ColorVecMap *cvm)
{
    ivec_init(&cvm->list);
    memset(cvm->marked, 0x00, (2 * N * NN + 1) * sizeof(int));
    for (int i = 0; i < 2 * N * NN + 1; i++) {
        ivec_init(&cvm->store[i]);
    }
}

inline void cvmap_clear(ColorVecMap *cvm)
{
    ivec_clear(&cvm->list);
    memset(cvm->marked, 0x00, (2 * N * NN + 1) * sizeof(int));
    for (int i = 0; i < 2 * N * NN + 1; i++) {
        ivec_clear(&cvm->store[i]);
    }
}

inline void cvmap_free(ColorVecMap *cvm)
{
    ivec_free(&cvm->list);
    for (int i = 0; i < 2 * N * NN + 1; i++) {
        ivec_free(&cvm->store[i]);
    }
}

inline IntVec *cvmap_get_IntVec(ColorVecMap *cvm, const Color c)
{
    assert(cvm->marked[color_to_idx(c)]);
    return &cvm->store[color_to_idx(c)];
}

inline int cvmap_count(const ColorVecMap *cvm, const Color c)
{
    return cvm->marked[color_to_idx(c)];
}

inline void cvmap_erase(ColorVecMap *cvm, const Color c)
{
    assert(cvm->marked[color_to_idx(c)] != 0);
    cvm->marked[color_to_idx(c)] = 0;
}

inline const IntVec *cvmap_keys(ColorVecMap *cvm)
{
    for (int i = 0; i < cvm->list.size;) {
        int idx = color_to_idx(ivec_at_idx(&cvm->list, i));
        if (cvm->marked[idx] == 0) {
            ivec_erase_at_idx(&cvm->list, i);
        } else {
            i++;
        }
    }
    return &cvm->list;
}

inline int cvmap_insert_one(ColorVecMap *cvm, const Color c, int value)
{
    int idx = color_to_idx(c);
    if (cvm->marked[idx] == 0) {
        cvm->marked[idx] = 1;
        ivec_clear(&cvm->store[idx]);
        if (ivec_find_first_from(&cvm->list, 0, c) == NA) {
            GUARD(ivec_push_back(&cvm->list, c));
        }
    }
    GUARD(ivec_push_back(&cvm->store[idx], value));
    return 0;
}

inline int cvmap_copy(const ColorVecMap *src, ColorVecMap *dst)
{
    memcpy(dst->marked, src->marked, (2 * N * NN + 1) * sizeof(int));
    GUARD(ivec_copy(&src->list, &dst->list));
    for (int i = 0; i < 2 * N * NN + 1; i++) {
        GUARD(ivec_copy(&src->store[i], &dst->store[i]));
    }
    return 0;
}

#endif // CUSTOMTYPES_H
