# ===============================================
# PROGRAM 1: Matrix Multiplication Chain (FIXED)
# Pipeline-Safe Version
# ===============================================
#
# Purpose: Compute E = (A × B) × scalar
# Fixed for proper pipelined execution
#
# ===============================================

.data
# Matrix A (2x3) at address 0
MATRIX_A: 0
    1, 2, 3
    4, 5, 6

# Matrix B (3x2) at address 10
MATRIX_B: 10
    2, 1
    3, 2
    1, 4

# Scalar multiplier at address 20
SCALAR: 20
    2

.text
# ===== Part 1: Initialize Matrices =====

# Matrix A (2x3)
DECLAREM M0, 2, 3
LOADM M0, MATRIX_A

# Matrix B (3x2)
DECLAREM M1, 3, 2
LOADM M1, MATRIX_B

# ===== Part 2: Matrix Multiplication =====
# M2 = M0 × M1 (2x3 × 3x2 = 2x2)

MULM M2, M0, M1

# Store result at 300
STOREM M2, 300

# ===== Part 3: Load Scalar =====

LW R5, 20(R0)

# Add NOPs for pipeline safety
LI R6, 0
ADD R6, R6, R0

# ===== Part 4: Scale Result =====

SCALE M3, M2, R5

# Store scaled result at 350
STOREM M3, 350

# ===== Part 5: Calculate Determinant =====

DETERMINANT R10, M3

# Store determinant
SW R10, 400(R0)

# ===== Part 6: Calculate Trace =====

# Load diagonal elements
LW R11, 350(R0)    # M3[0,0]
LI R1, 0           # NOP
LW R12, 353(R0)    # M3[1,1] (offset 3 in row-major)
LI R2, 0           # NOP

# Sum diagonal
ADD R13, R11, R12

# Store trace
SW R13, 404(R0)

# Success flag
LI R21, 1
SW R21, 408(R0)

HALT

# ===============================================
# Expected Results:
# ===============================================
#
# Memory [0-5]: MATRIX_A = [1,2,3,4,5,6]
# Memory [10-15]: MATRIX_B = [2,1,3,2,1,4]
# Memory [20]: SCALAR = 2
# Memory [300-303]: A×B = [13,17,31,38]
# Memory [350-353]: (A×B)×2 = [26,34,62,76]
# Memory [400]: det = -132
# Memory [404]: trace = 102
# Memory [408]: success = 1
#
# ===============================================