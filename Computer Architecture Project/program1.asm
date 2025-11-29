# ===============================================
# PROGRAM 1: Basic Arithmetic & Memory Operations
# Assignment 3, Task A (8-12 instructions)
# ===============================================
#
# Purpose: Compute D = (A + B) - C
# where A, B, C are 2x2 matrices stored in memory
#
# Total Instructions: 11
# ===============================================

.data
# Matrix A at address 0
MATRIX_A: 0
    1, 2
    3, 4

# Matrix B at address 100
MATRIX_B: 100
    5, 6
    7, 8

# Matrix C at address 200
MATRIX_C: 200
    2, 1
    1, 2

.text
# Instruction 1: Declare matrix M0 for A (2x2)
DECLAREM M0, 2, 2

# Instruction 2: Load matrix A from memory
LOADM M0, MATRIX_A

# Instruction 3: Declare matrix M1 for B (2x2)
DECLAREM M1, 2, 2

# Instruction 4: Load matrix B from memory
LOADM M1, MATRIX_B

# Instruction 5: Declare matrix M2 for C (2x2)
DECLAREM M2, 2, 2

# Instruction 6: Load matrix C from memory
LOADM M2, MATRIX_C

# Instruction 7: Add A + B, store in M3
# M3 = M0 + M1 = A + B
ADDM M3, M0, M1

# Instruction 8: Subtract C from (A+B), store in M4
# M4 = M3 - M2 = (A+B) - C
SUBM M4, M3, M2

# Instruction 9: Store final result D at memory address 300
STOREM M4, 300

# Instruction 10: Calculate determinant of result
DETERMINANT R1, M4

# Instruction 11: Halt
HALT

# ===============================================
# Expected Results:
# D at address 300: [4, 7, 9, 10]
# Determinant in R1: -23
# ===============================================
