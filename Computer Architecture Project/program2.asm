# ===============================================
# PROGRAM 2: Control Flow & Loop
# Assignment 3, Task B (10-15 instructions)
# ===============================================
#
# Purpose: Countdown loop + array threshold filter
# Total Instructions: 16
# ===============================================

.data
# Counter value N
N_VALUE: 900
    5

# Array of 4 values
ARRAY: 910
    10, 3, 15, 2

# Threshold value
THRESHOLD: 920
    5

.text
# ===== Part 1: Countdown Loop =====
# Initialize: counter = 5, sum = 0

# Instruction 1: Load counter value N into R1
LW R1, 900(R0)

# Instruction 2: Initialize sum to 0
LI R2, 0

COUNTDOWN_LOOP:
# Instruction 3: Add current counter to sum
ADD R2, R2, R1

# Instruction 4: Decrement counter
SUBI R1, R1, 1

# Instruction 5: Check if counter > 0
BNE R1, R0, COUNTDOWN_LOOP

# ===== Part 2: Array Threshold Filter =====

# Instruction 6: Load threshold into R3
LW R3, 920(R0)

# Instruction 7: Initialize array index
LI R4, 910

# Instruction 8: Initialize counter for 4 elements
LI R5, 4

FILTER_LOOP:
# Instruction 9: Load array element
LW R6, 0(R4)

# Instruction 10: Compare with threshold
BLT R6, R3, SET_ZERO
J KEEP_VALUE

SET_ZERO:
# Instruction 11: Set to 0
LI R6, 0

KEEP_VALUE:
# Instruction 12: Store back
SW R6, 0(R4)

# Instruction 13: Move to next element
ADDI R4, R4, 1

# Instruction 14: Decrement counter
SUBI R5, R5, 1
BNE R5, R0, FILTER_LOOP

# Instruction 15: Store final sum
SW R2, 930(R0)

# Instruction 16: Halt
HALT

# ===============================================
# Expected Results:
# Sum at address 930: 15
# Filtered array at 910-913: [10, 0, 15, 0]
# ===============================================
