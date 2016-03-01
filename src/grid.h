
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

#ifndef GRID_H
#define GRID_H

#include "consts.h"
#include "customtypes.h"

/*
 * Summary:
 *
 * Only searches for incoherences, can only solve a valid grid with one solution.
 *
 * Merge node into color, a color is a collection of nodes which are true or false together
 * A color Y have a reverse color -Y such that (Y XOR -Y) is true.
 * Here, a color is never empty, always >= 1 node is associated with it.
 *
 * Use exclusion rules only, rules created using the fact that 1 and only 1 node is true in a unit
 *
 * Adjust D in consts.h at compile time, set D to 3 for 9x9 sudoku, to 4 for 16x16
 * For D > 4, grid_populate and grid_get_grid_str are not implemented.
 */

typedef struct
{
#ifdef CHECK_GRID
    int constraint_cnt_check[kConstraintCount][NN]; // to check if constraint are violated
#endif
    NodeId validated_nodes[NN]; // the nodes validated already at their index as in the solution string
    int validated_size; // the number of nodes validated
    ColorVecMap color_to_nodes; // which nodes are of a given color
    ColorVecMap color_to_exclusion_idx; // in which rules a color appears
    IntVec *color_exclusions; // NN * kUnitCount rules, always one and only one color true by rule
    IntVec to_validate; // colors to validate
    IntVec to_merge; // consecutive pair of colors to merge
    ColorVecMap true_to_false_colors; // rules as adjacency list : if color/key true, colors/values false
} Grid;

// init, necessary to initialize storage to sane values
// return NA if alloc fails
int  grid_init(Grid *grid);
// free the allocated memory
void grid_free(Grid *grid);
// copy from src to dst, src and dst must be initialized, alloc if necessary
// return NA if alloc fails
int  grid_copy(const Grid *src, Grid *dst);
// init the data for an empty grid
// return NA if alloc fails
int  grid_init_data(Grid *grid);
// populate a grid from a grid string
// return NA if alloc fails, if string is not of the expected lenght, and for a 9x9 sudoku if
// the number of clues is < 17
int  grid_populate(Grid *grid, const char *grid_str);
// 9 x 9 sudoku grid as a string of 81 characters, any character not [1-9] is considered as an empty cell
// if compiled with D = 4 in consts.h => 16 x 16 sudoku, any character not [0-9], [a-f] or [A-F] is considered as an empty cell
// return NA if an alloc error occurs, and if CHECK_GRID is defined, if the grid is not valid
// else return the number of nodes validated, a solved grid returns NN
int  grid_solve(Grid *grid);
// put the validated node in str, use '.' for the positions not solved
void grid_get_grid_str(Grid *grid, char str[NN + 1]);
// use N character by grid position, if a validated or a candidate node exists use same convention as input grid string
// if the candidate don't exist, use '.'
void grid_get_cands_str(Grid *grid, char str[NN * N + 1]);

#endif // GRID_H
