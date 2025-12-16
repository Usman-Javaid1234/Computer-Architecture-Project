# ===============================================
# PROGRAM 2: Simple Hazard Demonstration
# Focus: Load-Use Stalls & Branch Flushes
# ===============================================
#
# Purpose: Clear demonstration of:
#   1. Load-Use Data Hazards (require stalls)
#   2. Branch Control Hazards (require flushes)
#
# Design: Minimal code, maximum clarity
# Total Instructions: 12
# Expected Stalls: 2
# Expected Flushes: 2
# ===============================================

.data
# Simple data values
VALUE1: 100
    10

VALUE2: 104
    20

VALUE3: 108
    5

RESULT: 112
    0

.text
# ===== Part 1: LOAD-USE STALL #1 =====
# Demonstrates: Classic load-use data hazard

# Instruction 1: Load VALUE1 into R1
LW R1, 100(R0)              # R1 = 10

# Instruction 2: Use R1 immediately - STALL!
# CRITICAL: BLT needs R1 but LW hasn't completed
# Pipeline MUST insert 1-cycle STALL here
BLT R1, R0, SKIP1           # Compare R1 with 0 (not taken)
                            # DATA HAZARD: Load-use on R1
                            # STALL: 1 cycle inserted

# Instruction 3: This will execute after branch resolves
LI R2, 1                    # R2 = 1 (mark: we didn't skip)

SKIP1:
# ===== Part 2: LOAD-USE STALL #2 =====
# Demonstrates: Another load-use pattern

# Instruction 4: Load VALUE2 into R3
LW R3, 104(R0)              # R3 = 20

# Instruction 5: Add using loaded value - STALL!
# CRITICAL: ADD needs R3 but LW hasn't completed
# Pipeline MUST insert 1-cycle STALL here
ADD R4, R3, R1              # R4 = R3 + R1 = 30
                            # DATA HAZARD: Load-use on R3
                            # STALL: 1 cycle inserted

# ===== Part 3: BRANCH FLUSH #1 =====
# Demonstrates: Taken branch causing flush

# Instruction 6: Load counter
LW R5, 108(R0)              # R5 = 5

# Instruction 7: Decrement counter
SUBI R5, R5, 1              # R5 = 4

# Instruction 8: Branch if not zero - TAKEN!
# CRITICAL: Branch will be taken, next instruction flushed
BNE R5, R0, CONTINUE        # Branch to CONTINUE
                            # CONTROL HAZARD: Branch taken
                            # FLUSH: Next instruction killed

# Instruction 9: This gets FLUSHED (never executes)
LI R6, 99                   # NEVER EXECUTES (flushed)

CONTINUE:
# ===== Part 4: BRANCH FLUSH #2 =====
# Demonstrates: Another taken branch

# Instruction 10: Compare values
BLT R5, R1, END             # if R5 < R1, go to END
                            # R5=4, R1=10, so TAKEN
                            # CONTROL HAZARD: Branch taken
                            # FLUSH: Next instruction killed

# Instruction 11: This gets FLUSHED (never executes)
LI R7, 88                   # NEVER EXECUTES (flushed)

END:
# Instruction 12: Store result
SW R4, 112(R0)              # Store R4=30 to memory

# Instruction 13: Halt
HALT

# ===============================================
# EXPECTED RESULTS
# ===============================================
#
# Registers:
#   R0 = 0 (always zero)
#   R1 = 10 (from VALUE1)
#   R2 = 1 (executed, not skipped)
#   R3 = 20 (from VALUE2)
#   R4 = 30 (R3 + R1 = 20 + 10)
#   R5 = 4 (decremented from 5)
#   R6 = 0 (never set, instruction flushed)
#   R7 = 0 (never set, instruction flushed)
#
# Memory:
#   [100] = 10 (VALUE1)
#   [104] = 20 (VALUE2)
#   [108] = 5 (VALUE3)
#   [112] = 30 (RESULT = R4)
#
# ===============================================
# PIPELINE ANALYSIS
# ===============================================
#
# Total Instructions: 13
# Completed Instructions: 11 (2 flushed, never complete)
#
# STALLS:
#   Stall #1: Instruction 2 (BLT after LW R1)
#   Stall #2: Instruction 5 (ADD after LW R3)
#   Total Stalls: 2
#
# FLUSHES:
#   Flush #1: Instruction 9 (LI R6 after BNE)
#   Flush #2: Instruction 11 (LI R7 after BLT)
#   Total Flushes: 2
#
# PERFORMANCE:
#   Base Cycles: 13 (if no hazards)
#   Stall Penalty: 2 cycles
#   Flush Penalty: 2 cycles
#   Total Cycles: 17
#   Instructions Completed: 11
#   CPI: 17/11 = 1.55
#
# ===============================================
# HAZARD TIMELINE
# ===============================================
#
# STALL #1 (Instructions 1-2):
# ----------------------------
# Cycle  1: LW R1 in IF
# Cycle  2: LW R1 in ID,  BLT in IF
# Cycle  3: LW R1 in EX,  BLT in ID
# Cycle  4: LW R1 in MEM, **STALL** detected!
#           BLT needs R1 but not available yet
#           Pipeline inserts BUBBLE in ID/EX
#           BLT stays in IF/ID
# Cycle  5: LW R1 in WB,  BLT in ID (held)
#           R1 now available via MEM/WB forwarding
# Cycle  6: BLT in EX (finally proceeds)
#
# STALL #2 (Instructions 4-5):
# ----------------------------
# Similar pattern for LW R3 followed by ADD R4,R3,R1
# 1 cycle stall inserted
#
# FLUSH #1 (Instructions 8-9):
# ----------------------------
# Cycle N:   BNE in IF
# Cycle N+1: BNE in ID, LI R6 in IF (speculative)
# Cycle N+2: BNE in EX, LI R6 in ID
#            Branch resolves as TAKEN
#            PC updated to CONTINUE
#            LI R6 in IF/ID is FLUSHED (killed)
# Cycle N+3: [CONTINUE] in IF
#
# FLUSH #2 (Instructions 10-11):
# ----------------------------
# Similar pattern for BLT followed by LI R7
# Instruction flushed when branch taken
#
# ===============================================
# VERIFICATION CHECKLIST
# ===============================================
#
# ✓ R1 = 10 (loaded correctly)
# ✓ R2 = 1 (branch not taken, executed)
# ✓ R3 = 20 (loaded correctly)
# ✓ R4 = 30 (addition with forwarding after stall)
# ✓ R5 = 4 (decremented)
# ✓ R6 = 0 (flushed, never set)
# ✓ R7 = 0 (flushed, never set)
# ✓ Memory[112] = 30 (result stored)
#
# ✓ Pipeline Stalls = 2
# ✓ Data Hazards = 2
# ✓ Control Hazards = 2
# ✓ Total Cycles ≈ 17
# ✓ CPI ≈ 1.55
#
# ===============================================
# KEY LEARNING POINTS
# ===============================================
#
# 1. LOAD-USE HAZARDS:
#    - LW followed immediately by instruction using result
#    - Cannot be resolved by forwarding alone
#    - Requires 1-cycle pipeline stall
#    - After stall, forwarding provides correct value
#
# 2. BRANCH CONTROL HAZARDS:
#    - Branch direction unknown until EX stage
#    - "Predict not taken" - fetch sequential instruction
#    - If branch taken, flush speculatively fetched instruction
#    - Update PC to branch target
#    - 1-cycle penalty for each taken branch
#
# 3. FORWARDING vs STALLING:
#    - Forwarding works for ALU-to-ALU dependencies
#    - Stalling required when data not yet computed (load-use)
#    - Example: SUBI R5,R5,1 followed by BNE R5,R0
#      → R5 forwarded from EX/MEM, no stall needed
#
# 4. FLUSHED INSTRUCTIONS:
#    - Never complete execution
#    - Don't appear in instruction count
#    - Still consume 1-2 cycles in pipeline before flush
#    - Registers remain unchanged
#
# ===============================================
# TRACE FILE EVIDENCE
# ===============================================
#
# Look for these patterns in pipeline_trace.txt:
#
# [DATA HAZARD DETECTED] Load-use hazard on R1
# [STALL INSERTED] Stalling pipeline for 1 cycle
# [PIPELINE STATE] IF:- ID:- EX:V MEM:V [STALLED]
#
# [CONTROL HAZARD DETECTED] Branch/Jump in EX stage
# [FLUSH] Flushing IF/ID due to taken branch
# [PIPELINE STATE] IF:V ID:- EX:V MEM:V [FLUSHED]
#
# ===============================================