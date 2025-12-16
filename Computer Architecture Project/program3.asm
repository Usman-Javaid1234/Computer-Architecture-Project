# ===============================================
# PROGRAM 3: Reusable Functions with Hazards
# Assignment 3, Task C
# ===============================================
#
# Purpose: Matrix comparison function with hazards
# - Demonstrates function call pattern
# - Load-use hazards in comparison loop
# - Control hazards from branches
# - Register dependencies
#
# Total Instructions: ~40
# Expected Hazards:
# - Data Hazards: 8-12 (load-use in comparisons)
# - Control Hazards: 12-16 (function calls, loops)
# ===============================================

.data
# Test Matrix A (2x2)
MAT_A: 0
    1, 2
    3, 4

# Test Matrix B (same as A - should be equal)
MAT_B: 100
    1, 2
    3, 4

# Test Matrix C (different from A - should be not equal)
MAT_C: 200
    5, 6
    7, 8

.text
# ===== Main Program =====
# Demonstrates: Function call pattern
#               Multiple comparisons

# === Load and Compare First Pair (A vs B) ===

# Instruction 1-2: Declare and load Matrix A
DECLAREM M1, 2, 2           # Declare M1 as 2x2
LOADM M1, MAT_A             # Load from address 0
                            # M1 allocated at address 400-403

# Instruction 3-4: Declare and load Matrix B
DECLAREM M2, 2, 2           # Declare M2 as 2x2
LOADM M2, MAT_B             # Load from address 100
                            # M2 allocated at address 404-407

# Instruction 5: Set up for comparison
# R10 will hold result (1=equal, 0=not equal)
# R13 = M1 base, R14 = M2 base

# Instruction 6: Call comparison function
J COMPARE_M1_M2             # Jump to comparison
                            # CONTROL HAZARD: flush next

AFTER_COMPARE1:
# Instruction 7: Store first comparison result
SW R10, 300(R0)             # MEM[300] = result (should be 1)

# === Load and Compare Second Pair (A vs C) ===

# Instruction 8-9: Declare and load Matrix C
DECLAREM M3, 2, 2           # Declare M3 as 2x2
LOADM M3, MAT_C             # Load from address 200
                            # M3 allocated at address 408-411

# Instruction 10: Call second comparison
J COMPARE_M1_M3             # Jump to comparison
                            # CONTROL HAZARD: flush next

AFTER_COMPARE2:
# Instruction 11: Store second comparison result
SW R10, 304(R0)             # MEM[304] = result (should be 0)

# Instruction 12: Halt
HALT

# ===============================================
# FUNCTION: COMPARE_M1_M2
# Compares M1 (addr 400) and M2 (addr 404)
# Returns: R10 = 1 if equal, 0 if not equal
# ===============================================
COMPARE_M1_M2:
# Initialize comparison
LI R10, 1                   # Assume equal
LI R12, 4                   # 4 elements to compare
LI R13, 400                 # M1 base address
LI R14, 404                 # M2 base address

COMPARE_LOOP1:
# CRITICAL SECTION: Load-use hazards!

# Load element from M1
LW R15, 0(R13)              # R15 = M1[i]

# Load element from M2
LW R1, 0(R14)               # R1 = M2[i]

# Compare elements
# CRITICAL DATA HAZARD: Load-use hazard!
# Both R15 and R1 just loaded, not available until MEM/WB
# BEQ needs both in EX stage immediately
# SOLUTION: Pipeline must insert 1-cycle STALL
BEQ R15, R1, ELEMENTS_EQUAL1
                            # STALL inserted here (4 times)
                            # CONTROL HAZARD: branch may be taken

# Elements are not equal
LI R10, 0                   # Set result to not equal
J COMPARE_DONE1             # Exit comparison
                            # CONTROL HAZARD: flush next

ELEMENTS_EQUAL1:
# Elements match, continue checking

# Increment M1 pointer
# HAZARD: R13 depends on itself (forwarding resolves)
ADDI R13, R13, 1            # Next M1 element

# Increment M2 pointer  
# HAZARD: R14 depends on itself (forwarding resolves)
ADDI R14, R14, 1            # Next M2 element

# Decrement counter
# HAZARD: R12 depends on itself (forwarding resolves)
SUBI R12, R12, 1            # Decrement count

# Check if more elements remain
# CONTROL HAZARD: Branch decision in EX stage
BNE R12, R0, COMPARE_LOOP1  # Continue if count > 0
                            # Branch taken 3 times
                            # 3 flushes

COMPARE_DONE1:
# Return to caller
J AFTER_COMPARE1            # Jump back
                            # CONTROL HAZARD: flush next

# ===============================================
# FUNCTION: COMPARE_M1_M3
# Compares M1 (addr 400) and M3 (addr 408)
# Returns: R10 = 1 if equal, 0 if not equal
# ===============================================
COMPARE_M1_M3:
# Initialize comparison
LI R10, 1                   # Assume equal
LI R12, 4                   # 4 elements to compare
LI R13, 400                 # M1 base address
LI R14, 408                 # M3 base address

COMPARE_LOOP2:
# CRITICAL SECTION: Load-use hazards!

# Load element from M1
LW R15, 0(R13)              # R15 = M1[i]

# Load element from M3
LW R1, 0(R14)               # R1 = M3[i]

# Compare elements
# CRITICAL DATA HAZARD: Load-use hazard!
# Both R15 and R1 just loaded
# SOLUTION: Pipeline must insert 1-cycle STALL
BEQ R15, R1, ELEMENTS_EQUAL2
                            # STALL inserted here (up to 4 times)
                            # Will detect mismatch early

# Elements are not equal
LI R10, 0                   # Set result to not equal
J AFTER_COMPARE2            # Exit and return
                            # CONTROL HAZARD: flush next

ELEMENTS_EQUAL2:
# Elements match, continue

# Increment M1 pointer
ADDI R13, R13, 1            # Next M1 element

# Increment M3 pointer
ADDI R14, R14, 1            # Next M3 element

# Decrement counter
SUBI R12, R12, 1            # Decrement count

# Check if more elements remain
BNE R12, R0, COMPARE_LOOP2  # Continue if count > 0
                            # Will detect mismatch on first iteration

# All elements equal (won't reach here for M1 vs M3)
J AFTER_COMPARE2            # Jump back

# ===============================================
# Expected Results:
# ===============================================
# Register State (final):
#   R10 = 0 (last comparison result: not equal)
#   R12 = 3 (loop counter when mismatch found)
#   R13 = 401 (M1 pointer after first comparison)
#   R14 = 409 (M3 pointer after first comparison)
#   R15 = 1 (last M1 element loaded)
#   R1 = 5 (last M3 element loaded)
#
# Memory State:
#   MEM[0-3] = 1,2,3,4 (Matrix A)
#   MEM[100-103] = 1,2,3,4 (Matrix B)
#   MEM[200-203] = 5,6,7,8 (Matrix C)
#   MEM[300] = 1 (A == B, TRUE)
#   MEM[304] = 0 (A == C, FALSE)
#   MEM[400-403] = 1,2,3,4 (M1 data)
#   MEM[404-407] = 1,2,3,4 (M2 data)
#   MEM[408-411] = 5,6,7,8 (M3 data)
#
# Pipeline Performance Analysis:
# ===============================================
# Single-Cycle:
#   - Total Cycles: ~80-100
#   - CPI: 1.0 (by definition)
#
# Pipelined (with hazards):
#   - Base Instructions: ~45
#   - Data Hazards (load-use stalls): 8-12 stalls
#     * First comparison: 4 stalls (LW R15; LW R1; BEQ)
#     * Second comparison: 1 stall (exits on first mismatch)
#     * Total: ~5 stalls
#   - Control Hazards (branch/jump flushes): 12-16 flushes
#     * Function calls (J): 2 flushes
#     * Loop branches (BNE): 3 flushes (first compare)
#     * Conditional branches (BEQ): ~3 flushes
#     * Early exits (J): 2 flushes
#     * Returns (J): 2 flushes
#     * Total: ~12 flushes
#   - Expected Cycles: ~62-73
#   - Expected CPI: 1.38-1.62
#
# Hazard Summary:
# ===============================================
# 1. LOAD-USE HAZARDS (require stalls):
#    Pattern: LW Rx; LW Ry; BEQ Rx, Ry
#    - LW R15, 0(R13)  \
#    - LW R1, 0(R14)    } Both loads complete
#    - BEQ R15, R1      / Then branch needs both values
#    
#    Occurs in:
#    - COMPARE_LOOP1: 4 times (full comparison)
#    - COMPARE_LOOP2: 1 time (early exit on mismatch)
#    Total: ~5 stalls
#
# 2. ALU-to-ALU HAZARDS (resolved by forwarding):
#    - ADDI R13, R13, 1 (pointer increment)
#    - ADDI R14, R14, 1 (pointer increment)
#    - SUBI R12, R12, 1 (counter decrement)
#    All resolved by EX/MEM forwarding
#
# 3. CONTROL HAZARDS (cause flushes):
#    Function Calls:
#    - J COMPARE_M1_M2 (always taken)
#    - J COMPARE_M1_M3 (always taken)
#    
#    Within Functions:
#    - BEQ R15, R1, ELEMENTS_EQUAL (taken 4 times in loop 1)
#    - BNE R12, R0, COMPARE_LOOP (taken 3 times in loop 1)
#    - BEQ R15, R1, ELEMENTS_EQUAL (not taken in loop 2)
#    - J COMPARE_DONE or J AFTER_COMPARE (taken 2 times)
#    - J AFTER_COMPARE1/2 (returns, 2 times)
#    
#    Total: ~12 control hazards causing flushes
#
# Educational Value:
# ===============================================
# This program demonstrates:
# 1. Load-use hazards with TWO loads before use
# 2. Function call/return patterns with jumps
# 3. Early loop exit (second comparison)
# 4. Register dependencies in pointer arithmetic
# 5. Complex control flow with multiple hazards
# 6. Realistic comparison function implementation
# ===============================================