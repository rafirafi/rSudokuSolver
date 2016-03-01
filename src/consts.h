
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

#ifndef CONSTS_H
#define CONSTS_H

#include <stdio.h>

// check only for no valid solution, not no unique solution, costly
//#ifndef CHECK_GRID
//#define CHECK_GRID
//#endif

// set to != 0 to print error/debug/info msgs
#ifndef DO_PRINT_INFO
#define DO_PRINT_INFO 0
#endif

// for 9x9 sudoku : ser D to 3, for 16x16 set D to 4
#ifndef D
#define D 3
#endif

/********************************************/

#define PRINT_INFO(fmt, ...) \
    do { if (DO_PRINT_INFO) fprintf(stderr, fmt,  ##__VA_ARGS__); } while (0)

// use NA as an unique error/uninitialized flag
enum {
    NA = -1
};

// use GUARD for simplicity wherever possible to propagate error
// or absence of result when 0 is not an option
#define GUARD(x)        if (x < 0)  return NA

enum {
    N = D * D,
    NN = (D * D) * (D * D)
};

// unit : cell, col, row, box
enum {
    kUnitCount = 4,
};

#ifdef CHECK_GRID
// enum for sudoku constraints
enum {
    kRowCol = 0,
    kRowCand = 1,
    kColCand = 2,
    kBoxCand = 3,
    kConstraintCount = 4,
};
#endif

// Color in the ranges [-N*NN,0[ and ]0, N * NN]
typedef int Color;

// NodeId in the range [0, N*NN[
typedef int NodeId;

// Color in first, boolean in second
typedef struct { Color first; int second; } Vertex;

#endif // CONSTS_H
