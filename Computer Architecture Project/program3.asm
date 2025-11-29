# ===============================================
# PROGRAM 3: Reusable Function Implementation
# Assignment 3, Task C (6-10 instructions for function)
# ===============================================
#
# Purpose: Compare two matrices element-by-element
# Returns 1 if equal, 0 if different
#
# Total Instructions: 15
# ===============================================

.data
# Test Matrix A
MAT_A: 0
    1, 2
    3, 4

# Test Matrix B (same as A)
MAT_B: 100
    1, 2
    3, 4

# Test Matrix C (different from A)
MAT_C: 200
    5, 6
    7, 8

.text
# ===== Main Program =====

# Load first pair of matrices
DECLAREM M1, 2, 2
LOADM M1, MAT_A      # Loads [1,2,3,4] into M1's allocated space (400-403)

DECLAREM M2, 2, 2
LOADM M2, MAT_B      # Loads [1,2,3,4] into M2's allocated space (404-407)

# Call comparison function
J MATRIX_COMPARE

AFTER_COMPARE1:
SW R10, 300(R0)      # Store result 1

# Load third matrix
DECLAREM M3, 2, 2
LOADM M3, MAT_C      # Loads [5,6,7,8] into M3's allocated space (408-411)

# Call comparison again
J MATRIX_COMPARE_M1M3

AFTER_COMPARE2:
SW R10, 304(R0)      # Store result 2

HALT

# ===== FUNCTION: MATRIX_COMPARE (M1 vs M2) =====
MATRIX_COMPARE:
LI R10, 1            # Assume equal
LI R12, 4            # 4 elements to compare
LI R13, 400          # M1 base address
LI R14, 404          # M2 base address

COMPARE_LOOP:
LW R15, 0(R13)       # Load from M1
LW R1, 0(R14)        # Load from M2
BEQ R15, R1, ELEMENTS_EQUAL
LI R10, 0            # Not equal
J COMPARE_DONE

ELEMENTS_EQUAL:
ADDI R13, R13, 1     # Next M1 element
ADDI R14, R14, 1     # Next M2 element
SUBI R12, R12, 1     # Decrement counter
BNE R12, R0, COMPARE_LOOP

COMPARE_DONE:
J AFTER_COMPARE1

# ===== FUNCTION: MATRIX_COMPARE_M1M3 (M1 vs M3) =====
MATRIX_COMPARE_M1M3:
LI R10, 1            # Assume equal
LI R12, 4            # 4 elements
LI R13, 400          # M1 base
LI R14, 408          # M3 base

COMPARE_LOOP2:
LW R15, 0(R13)       # Load from M1
LW R1, 0(R14)        # Load from M3
BEQ R15, R1, EQUAL2
LI R10, 0            # Not equal
J AFTER_COMPARE2

EQUAL2:
ADDI R13, R13, 1     # Next M1 element
ADDI R14, R14, 1     # Next M3 element
SUBI R12, R12, 1     # Decrement counter
BNE R12, R0, COMPARE_LOOP2
J AFTER_COMPARE2

# ===============================================
# Expected Results:
# Memory[300] = 1 (A == B)
# Memory[304] = 0 (A != C)
# ===============================================