
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

/*
 * Compilation :
 *
 *  cc -std=c99 -DNDEBUG -Wall -Wextra -Werror -O2 -I. main.c grid.c customtypes.c -o ./rSudokuSolver
 *
 * for options adjust in consts.h, or define at compile time :
 * verbose : -DDO_PRINT_INFO=1
 * check grid validity while solving : -DCHECK_GRID
 * for solving 16x16 sudoku : -DD=4
 *
 * Usage :
 *
 * cat grids.txt | ./rSudokuSolver
 * echo 000540002000001000100009006904000100020800059000100204005400080008020007090008000 | ./rSudokuSolver
 *
 */

/*
 * Solver, overview :
 *
 * Only searches for incoherences, can only solve a valid grid with one solution.
 *
 * Merge node into color, a color is a collection of nodes which are true or false together
 * A color Y has a reverse color -Y such that (Y XOR -Y) is true.
 * Here, a color is never empty, always >= 1 node is associated with it.
 *
 * Use exclusion rules only, rules created using the fact that 1 and only 1 node is true in a unit.
 *
 * Methods used by the solver, overview :
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "grid.h"

int main(int argc, char *argv[])
{
    (void)argc, (void)argv;

    clock_t start = clock();

    char grid_str[NN * N + 1] = "";

    Grid base_grid;
    if (grid_init(&base_grid) == NA) {
        return EXIT_FAILURE;
    }
    // init one time here, then copy before populating the active grid
    if (grid_init_data(&base_grid) == NA) {
        return EXIT_FAILURE;
    }

    Grid grid;
    if (grid_init(&grid) == NA) {
        return EXIT_FAILURE;
    }

    int grid_cnt = 0, solved_grid_cnt = 0;

    while (scanf(" %729s", grid_str) == 1)
    {
        if (grid_copy(&base_grid, &grid) == NA) {
            break;
        }

        if (grid_populate(&grid, grid_str) == NA) {
            continue;
        }

        grid_cnt++;

        fprintf(stderr, "%s\n", grid_str);

        int validated_size = grid_solve(&grid);
        if (validated_size == NA) {
            break;
        }

        grid_get_grid_str(&grid, grid_str);
        fprintf(stderr, "%s\n\n", grid_str);

        solved_grid_cnt += (validated_size == NN);
    }

    clock_t end = clock();
    uint64_t us = ((end - start)/(double)CLOCKS_PER_SEC) * 1000000;

    fprintf(stderr, "solved %d / %d %3.3f%% time grid % 3.3f us time total %ld us\n",
            solved_grid_cnt, grid_cnt, 100.f * solved_grid_cnt / (grid_cnt == 0 ? 1.f : (float)grid_cnt),
            (float)us / (float)(grid_cnt == 0 ? 1 : grid_cnt), us);

    grid_free(&grid);
    grid_free(&base_grid);

    return EXIT_SUCCESS;
}
