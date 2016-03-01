
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

#include "customtypes.h"

// internal : map color to internal idx for *Map structs
extern inline int  color_to_idx(Color color);

// VertexMap : use vertex{color, true value} as key,
// an int (!= 0, 0 is used as empty flag internally) as value

// return 0 if key is in the map, 0 if not
extern inline int  vmap_count(const VertexMap *map, const Vertex *v);
// remove all keys, also used for init, always clear before first use
extern inline void vmap_clear(VertexMap *map);
// return the value for the key
extern inline int  vmap_get(const VertexMap *map, const Vertex *v);
// set a value for the key (0 is forbidden)
extern inline void vmap_assign(VertexMap *map, const Vertex *v, int value);

// IntVec : a simple dynamically allocated array of int

// init, necessary to initialize storage to sane values, don't alloc
extern inline void ivec_init(IntVec *vec);
// clear the array (set size to 0)
extern inline void ivec_clear(IntVec *vec);
// free the allocated memory
extern inline void ivec_free(IntVec *vec);
// insert a value at the end of the array, alloc if necessary, return NA if alloc fails
extern inline int  ivec_push_back(IntVec *vec, int value);
// return the first index of the value from idx_start included or NA if not found
extern inline int  ivec_find_first_from(const IntVec *vec, int idx_start, int value);
// treat the array as a collection of pair and return an even index if a pair with
// absolute values same as 'values' is found, order is not important
// array must be a collection of pair
// return NA if not found
extern inline int  ivec_absolute_pair_exists(const IntVec *vec, const int values[2]);
// erase the first occurence of value of found
extern inline void ivec_erase_one(IntVec *vec, int value);
// erase value at idx, idx must be valid
extern inline void ivec_erase_at_idx(IntVec *vec, int idx);
// return the value stored at idx, idx must be valid
extern inline int  ivec_at_idx(const IntVec *vec, int idx);
// return a pointer to value stored at idx in order to allow modification, idx must be valid
extern inline int  *ivec_ptr_at_idx(IntVec *vec, int idx);
// copy from src to dst, src and dst must be initialized, alloc if necessary
// return NA if alloc fails
extern inline int  ivec_copy(const IntVec *src, IntVec *dst);
// return the size of the array
extern inline int  ivec_size(const IntVec *vec);
// internal : alloc storage, return NA if alloc fails
extern inline int  ivec_alloc_store(IntVec *vec);

// ColorVecMap : map with Color as key and a dynamic array of int as value

// init, necessary to initialize storage to sane values, don't alloc
extern inline void cvmap_init(ColorVecMap *cvm);
// remove all keys
extern inline void cvmap_clear(ColorVecMap *cvm);
// free the allocated memory
extern inline void cvmap_free(ColorVecMap *cvm);
// return a pointer to the key
extern inline IntVec *cvmap_get_IntVec(ColorVecMap *cvm, const Color c);
// return 1 if key is in map, else 0
extern inline int  cvmap_count(const ColorVecMap *cvm, const Color c);
// remove a key from the map
extern inline void cvmap_erase(ColorVecMap *cvm, const Color c);
// return all the keys in the map
extern inline const IntVec *cvmap_keys(ColorVecMap *cvm);
// insert key if necessary, add 'value' to tthe values for the key
// return NA if alloc fails
extern inline int  cvmap_insert_one(ColorVecMap *cvm, const Color c, int value);
// copy from src to dst, src and dst must be initialized, alloc if necessary
// return NA if alloc fails
extern inline int  cvmap_copy(const ColorVecMap *src, ColorVecMap *dst);
