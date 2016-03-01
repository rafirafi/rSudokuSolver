 rSudokuSolver
=============
 Solver, overview :
-------

 Only searches for incoherences, can only solve a valid grid with one solution.

 Merge node into color, a color is a collection of nodes which are true or false together
 
 A color Y has a reverse color -Y such that (Y XOR -Y) is true.
 
 Here, a color is never empty, always >= 1 node is associated with it.

 Use exclusion rules only, rules created using the fact that 1 and only 1 node is true in a unit.

 Methods used by the solver, overview :
-------

* grid_validate_check_single        => if one color only in a rule it is true. rule{A} => A true
* grid_merge_check_pair             => if 2 colors only in a rule they are exclusive (XOR). rule{A, B} => (A,B)->(C, -C)
* grid_validate_check_pair_1        => if 2 times the same color in a unit it is false. rule{A, A, B, ...} => A false
* grid_validate_check_pair_2        => if a color and its reverse in a rule, the other colors are false. rule{A, -A, B, ...} => (B, ...) false
* grid_get_true_to_false_colors     => use rules to construct adjacency list : rule(A, B, C) + rule(A, D, E) => A true : B, C, D, E false
* grid_merge_check_SCC              => find strong conn. component using adjacency list. (A true <=> B false AND A false <=> B true) => (A,B)->(C, -C)
* grid_validate_check_cycle         => search contradiction : one color A tested as true is false when :
                                   1/ a rule empty OR 2/ any color true AND false at the same time
                                   Use true=>false pair and count color false in a rule, when one color remains it is injected as true
* grid_validate_check_cycle_level_2 => construct a color 'tree' as in validate_check_cycle for a color A
                                   then begin a search from this 'tree' with another color B and its opposite -B
                                   if both leads to contradiction it means B and -B => A false, A is always false.

 Compilation :
-------
``` 
  cc -std=c99 -DNDEBUG -Wall -Wextra -Werror -O2 -I. main.c grid.c customtypes.c -o ./rSudokuSolver
``` 
 for options adjust in consts.h, or define at compile time :
- verbose : -DDO_PRINT_INFO=1
- check grid validity while solving : -DCHECK_GRID
- for solving 16x16 sudoku : -DD=4

 Usage :
-------
``` 
 cat grids.txt | ./rSudokuSolver
 echo 000540002000001000100009006904000100020800059000100204005400080008020007090008000 | ./rSudokuSolver 
 ```

-------
This code is released under the GPL version 3.

Copyright (C) 2016 rafirafi.
