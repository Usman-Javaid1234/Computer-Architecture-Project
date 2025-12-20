#include "MatrixProcessor.h"

// PipelinedSimulator constructor
PipelinedSimulator::PipelinedSimulator() {
    pc = 0;
    halted = false;
    stall = false;
    flush = false;
    cycleCount = 0;
    instructionCount = 0;
    stallCount = 0;
    dataHazardCount = 0;
    controlHazardCount = 0;

    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }

    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }

    // MSPR bank auto-initializes via constructor
    mspr.reset();  // Explicit reset for clarity

    traceFile.open("pipeline_trace.txt");
    traceFile << "========================================" << std::endl;
    traceFile << "Pipeline Execution Trace - Enhanced" << std::endl;
    traceFile << "With Data & Control Hazard Handling" << std::endl;
    traceFile << "With MSPR System" << std::endl;
    traceFile << "========================================" << std::endl;
}

void PipelinedSimulator::loadProgram(const std::string& filename) {
    AssemblyParser parser(memory, instructionMemory, decodedInstructions);
    parser.parseFile(filename);
}

bool PipelinedSimulator::detectDataHazard() {
    // Check for load-use hazard (RAW hazard with load instruction)
    if (!IF_ID.valid || !ID_EX.valid) return false;

    Instruction& idInst = IF_ID.inst;

    // Check if ID/EX stage has a load instruction
    if (ID_EX.memRead) {
        // Check if the instruction in IF/ID uses the destination of the load
        if ((idInst.rs1 != -1 && idInst.rs1 == ID_EX.destReg && idInst.rs1 != 0) ||
            (idInst.rs2 != -1 && idInst.rs2 == ID_EX.destReg && idInst.rs2 != 0)) {
            dataHazardCount++;
            traceFile << "  [DATA HAZARD DETECTED] Load-use hazard on R"
                << ID_EX.destReg << std::endl;
            return true;
        }
    }

    return false;
}

bool PipelinedSimulator::detectControlHazard() {
    // Control hazard occurs when a branch/jump is in EX stage
    if (!ID_EX.valid) return false;

    Opcode op = ID_EX.inst.opcode;
    if (op == OP_BEQ || op == OP_BNE || op == OP_BGT || op == OP_BLT || op == OP_J) {
        controlHazardCount++;
        traceFile << "  [CONTROL HAZARD DETECTED] Branch/Jump in EX stage" << std::endl;
        return true;
    }
    return false;
}

void PipelinedSimulator::handleDataHazard() {
    // Insert stall: bubble in ID/EX, prevent IF/ID update
    stall = true;
    stallCount++;
    traceFile << "  [STALL INSERTED] Stalling pipeline for 1 cycle" << std::endl;

    // Keep IF/ID register unchanged (don't advance)
    // Insert NOP (bubble) into ID/EX by invalidating it
    ID_EX.valid = false;

    // Don't increment PC
}

void PipelinedSimulator::handleControlHazard() {
    // For branch instructions, we flush IF/ID when branch is taken
    // This happens in execute stage when we determine branch outcome
    if (EX_MEM.branchTaken && EX_MEM.valid) {
        flush = true;
        IF_ID.valid = false;  // Flush the instruction after branch
        ID_EX.valid = false;  // ALSO flush the instruction that just entered ID/EX!
        traceFile << "  [FLUSH] Flushing IF/ID and ID/EX due to taken branch" << std::endl;
    }
}

bool PipelinedSimulator::needsForwarding(int rs, int& forwardValue) {
    if (rs == 0 || rs == -1) return false;

    // EX/MEM forwarding (EX hazard)
    if (EX_MEM.valid && EX_MEM.regWrite && rs == EX_MEM.destReg) {
        forwardValue = EX_MEM.aluResult;
        traceFile << "    [FORWARD EX/MEM→EX] R" << rs << " = " << forwardValue << std::endl;
        return true;
    }

    // MEM/WB forwarding (MEM hazard)
    if (MEM_WB.valid && MEM_WB.regWrite && rs == MEM_WB.destReg) {
        forwardValue = MEM_WB.writeBackData;
        traceFile << "    [FORWARD MEM/WB→EX] R" << rs << " = " << forwardValue << std::endl;
        return true;
    }

    return false;
}

void PipelinedSimulator::fetch() {
    if (halted) {
        return;
    }

    if (pc >= decodedInstructions.size()) {
        halted = true;
        return;
    }

    IF_ID.inst = decodedInstructions[pc];
    IF_ID.pc = pc;
    IF_ID.valid = true;

    traceFile << "[IF] PC=" << pc << " Inst: ";
    // Print instruction name inline
    switch (IF_ID.inst.opcode) {
    case OP_ADD: traceFile << "ADD"; break;
    case OP_SUB: traceFile << "SUB"; break;
    case OP_ADDI: traceFile << "ADDI"; break;
    case OP_SUBI: traceFile << "SUBI"; break;
    case OP_LW: traceFile << "LW"; break;
    case OP_SW: traceFile << "SW"; break;
    case OP_BEQ: traceFile << "BEQ"; break;
    case OP_BNE: traceFile << "BNE"; break;
    case OP_BGT: traceFile << "BGT"; break;
    case OP_BLT: traceFile << "BLT"; break;
    case OP_J: traceFile << "J"; break;
    case OP_LI: traceFile << "LI"; break;
    case OP_MOV: traceFile << "MOV"; break;
    case OP_MUL: traceFile << "MUL"; break;
    case OP_DIV: traceFile << "DIV"; break;
    case OP_HALT: traceFile << "HALT"; break;
    case OP_DECLAREM: traceFile << "DECLAREM"; break;
    case OP_LOADM: traceFile << "LOADM"; break;
    case OP_STOREM: traceFile << "STOREM"; break;
    case OP_ADDM: traceFile << "ADDM"; break;
    case OP_SUBM: traceFile << "SUBM"; break;
    case OP_MULM: traceFile << "MULM"; break;
    case OP_DETERMINANT: traceFile << "DETERMINANT"; break;
    default: traceFile << "UNKNOWN"; break;
    }
    traceFile << std::endl;

    pc++;
}

void PipelinedSimulator::decode() {
    if (!IF_ID.valid) {
        traceFile << "[ID] BUBBLE" << std::endl;
        return;
    }

    // Move IF/ID to ID/EX
    ID_EX = IF_ID;
    Instruction& inst = ID_EX.inst;

    traceFile << "[ID] Decode: ";
    // Print instruction name inline
    switch (inst.opcode) {
    case OP_ADD: traceFile << "ADD"; break;
    case OP_SUB: traceFile << "SUB"; break;
    case OP_ADDI: traceFile << "ADDI"; break;
    case OP_SUBI: traceFile << "SUBI"; break;
    case OP_LW: traceFile << "LW"; break;
    case OP_SW: traceFile << "SW"; break;
    case OP_BEQ: traceFile << "BEQ"; break;
    case OP_BNE: traceFile << "BNE"; break;
    case OP_BGT: traceFile << "BGT"; break;
    case OP_BLT: traceFile << "BLT"; break;
    case OP_J: traceFile << "J"; break;
    case OP_LI: traceFile << "LI"; break;
    case OP_MOV: traceFile << "MOV"; break;
    case OP_MUL: traceFile << "MUL"; break;
    case OP_DIV: traceFile << "DIV"; break;
    case OP_HALT: traceFile << "HALT"; break;
    case OP_DECLAREM: traceFile << "DECLAREM"; break;
    case OP_LOADM: traceFile << "LOADM"; break;
    case OP_STOREM: traceFile << "STOREM"; break;
    case OP_ADDM: traceFile << "ADDM"; break;
    case OP_SUBM: traceFile << "SUBM"; break;
    case OP_MULM: traceFile << "MULM"; break;
    case OP_DETERMINANT: traceFile << "DETERMINANT"; break;
    default: traceFile << "UNKNOWN"; break;
    }

    // Read register values
    if (inst.rs1 != -1 && inst.rs1 < NUM_REGISTERS) {
        ID_EX.readData1 = registers[inst.rs1];
        traceFile << " Rs1=R" << inst.rs1 << "(" << ID_EX.readData1 << ")";
    }

    if (inst.rs2 != -1 && inst.rs2 < NUM_REGISTERS) {
        ID_EX.readData2 = registers[inst.rs2];
        traceFile << " Rs2=R" << inst.rs2 << "(" << ID_EX.readData2 << ")";
    }

    // Set control signals
    ID_EX.memRead = (inst.opcode == OP_LW);
    ID_EX.memWrite = (inst.opcode == OP_SW);
    ID_EX.regWrite = (inst.opcode == OP_ADD || inst.opcode == OP_SUB ||
        inst.opcode == OP_ADDI || inst.opcode == OP_SUBI ||
        inst.opcode == OP_LW || inst.opcode == OP_LI ||
        inst.opcode == OP_MUL || inst.opcode == OP_DIV ||
        inst.opcode == OP_MOV || inst.opcode == OP_DETERMINANT);

    if (ID_EX.regWrite && inst.rd != -1) {
        ID_EX.destReg = inst.rd;
    }

    ID_EX.immediate = inst.immediate;
    ID_EX.branchTaken = false;

    traceFile << std::endl;

    // Clear IF/ID for next instruction
    IF_ID.valid = false;
}

void PipelinedSimulator::execute() {
    if (!ID_EX.valid) {
        traceFile << "[EX] BUBBLE" << std::endl;
        return;
    }

    // Move ID/EX to EX/MEM
    EX_MEM = ID_EX;
    Instruction& inst = EX_MEM.inst;

    traceFile << "[EX] Execute: ";
    // Print instruction name inline
    switch (inst.opcode) {
    case OP_ADD: traceFile << "ADD"; break;
    case OP_SUB: traceFile << "SUB"; break;
    case OP_ADDI: traceFile << "ADDI"; break;
    case OP_SUBI: traceFile << "SUBI"; break;
    case OP_LW: traceFile << "LW"; break;
    case OP_SW: traceFile << "SW"; break;
    case OP_BEQ: traceFile << "BEQ"; break;
    case OP_BNE: traceFile << "BNE"; break;
    case OP_BGT: traceFile << "BGT"; break;
    case OP_BLT: traceFile << "BLT"; break;
    case OP_J: traceFile << "J"; break;
    case OP_LI: traceFile << "LI"; break;
    case OP_MOV: traceFile << "MOV"; break;
    case OP_MUL: traceFile << "MUL"; break;
    case OP_DIV: traceFile << "DIV"; break;
    case OP_HALT: traceFile << "HALT"; break;
    case OP_DECLAREM: traceFile << "DECLAREM"; break;
    case OP_LOADM: traceFile << "LOADM"; break;
    case OP_STOREM: traceFile << "STOREM"; break;
    case OP_ADDM: traceFile << "ADDM"; break;
    case OP_SUBM: traceFile << "SUBM"; break;
    case OP_MULM: traceFile << "MULM"; break;
    case OP_DETERMINANT: traceFile << "DETERMINANT"; break;
    default: traceFile << "UNKNOWN"; break;
    }

    // Get operands with forwarding
    int operand1 = EX_MEM.readData1;
    int operand2 = EX_MEM.readData2;

    if (inst.rs1 != -1 && inst.rs1 != 0) {
        if (!needsForwarding(inst.rs1, operand1)) {
            operand1 = registers[inst.rs1];
        }
    }

    if (inst.rs2 != -1 && inst.rs2 != 0) {
        if (!needsForwarding(inst.rs2, operand2)) {
            operand2 = registers[inst.rs2];
        }
    }

    // Handle matrix operations (inline to avoid separate method)
    if (inst.opcode >= OP_DECLAREM && inst.opcode <= OP_TRANSPOSE) {
        traceFile << " MatrixOp=";

        switch (inst.opcode) {
        case OP_DECLAREM: {
            // Allocate MSPR in EX stage, memory initialization happens in MEM stage
            static int nextAddress = 400;
            uint32_t allocBase = nextAddress;
            nextAddress += inst.rows * inst.cols;

            // Write to MSPR immediately in EX
            mspr.writeMatrix(inst.md, allocBase, inst.rows, inst.cols);

            // Mark that we need to zero-initialize in MEM stage
            EX_MEM.aluResult = allocBase;  // Pass base address to MEM stage
            EX_MEM.memWrite = true;  // Signal to MEM stage

            traceFile << "DECLAREM M" << inst.md << "[" << inst.rows << "x" << inst.cols
                << "]@" << allocBase << " [MSPR allocated]" << std::endl;
            break;
        }

        case OP_LOADM: {
            traceFile << "LOADM_DEBUG: md=" << inst.md << " valid=" << mspr.isValid(inst.md);
            if (mspr.isValid(inst.md)) {
                int srcAddr = inst.address;
                int destAddr = mspr.readBase(inst.md);
                int size = mspr.readRows(inst.md) * mspr.readCols(inst.md);

                traceFile << " src=" << srcAddr << " dest=" << destAddr << " size=" << size;

                for (int i = 0; i < size; i++) {
                    memory[destAddr + i] = memory[srcAddr + i];
                }
                traceFile << " LOADM M" << inst.md << " from[" << srcAddr << "] COMPLETE!" << std::endl;
            }
            else {
                traceFile << " INVALID MSPR!" << std::endl;
            }
            break;
        }

        case OP_STOREM: {
            traceFile << "STOREM_DEBUG: md=" << inst.md << " valid=" << mspr.isValid(inst.md);
            if (mspr.isValid(inst.md)) {
                int srcAddr = mspr.readBase(inst.md);
                int destAddr = inst.address;
                int size = mspr.readRows(inst.md) * mspr.readCols(inst.md);

                traceFile << " src=" << srcAddr << " dest=" << destAddr << " size=" << size << std::endl;
                traceFile << "  Writing: ";

                // Perform memory copy in EX stage (simplified model)
                for (int i = 0; i < size; i++) {
                    int value = memory[srcAddr + i];
                    memory[destAddr + i] = value;
                    traceFile << "[" << (destAddr + i) << "]=" << value << " ";
                }

                traceFile << std::endl;
                traceFile << "  STOREM M" << inst.md << " COMPLETE!" << std::endl;
            }
            else {
                traceFile << " INVALID MSPR!" << std::endl;
            }
            break;
        }

        case OP_ADDM: {
            if (mspr.isValid(inst.ms1) && mspr.isValid(inst.ms2)) {
                int rows = mspr.readRows(inst.ms1);
                int cols = mspr.readCols(inst.ms1);

                if (mspr.readBase(inst.md) == 0 || mspr.readBase(inst.md) < 400) {
                    static int nextAddr = 600;
                    mspr.writeMatrix(inst.md, nextAddr, rows, cols);  // Atomic write
                    nextAddr += rows * cols;
                }
                else {
                    mspr.writeRows(inst.md, rows);
                    mspr.writeCols(inst.md, cols);
                    mspr.setValid(inst.md, true);
                }

                int addr1 = mspr.readBase(inst.ms1);
                int addr2 = mspr.readBase(inst.ms2);
                int addrDest = mspr.readBase(inst.md);

                for (int i = 0; i < rows * cols; i++) {
                    memory[addrDest + i] = memory[addr1 + i] + memory[addr2 + i];
                }
                traceFile << "ADDM M" << inst.md << "=M" << inst.ms1 << "+M" << inst.ms2 << std::endl;
            }
            break;
        }

        case OP_SUBM: {
            if (mspr.isValid(inst.ms1) && mspr.isValid(inst.ms2)) {
                int rows = mspr.readRows(inst.ms1);
                int cols = mspr.readCols(inst.ms1);

                if (mspr.readBase(inst.md) == 0 || mspr.readBase(inst.md) < 400) {
                    static int nextAddr = 650;
                    mspr.writeMatrix(inst.md, nextAddr, rows, cols);  // Atomic write
                    nextAddr += rows * cols;
                }
                else {
                    mspr.writeRows(inst.md, rows);
                    mspr.writeCols(inst.md, cols);
                    mspr.setValid(inst.md, true);
                }

                int addr1 = mspr.readBase(inst.ms1);
                int addr2 = mspr.readBase(inst.ms2);
                int addrDest = mspr.readBase(inst.md);

                for (int i = 0; i < rows * cols; i++) {
                    memory[addrDest + i] = memory[addr1 + i] - memory[addr2 + i];
                }
                traceFile << "SUBM M" << inst.md << "=M" << inst.ms1 << "-M" << inst.ms2 << std::endl;
            }
            break;
        }

        case OP_MULM: {
            if (mspr.isValid(inst.ms1) && mspr.isValid(inst.ms2)) {
                int m = mspr.readRows(inst.ms1);
                int n = mspr.readCols(inst.ms1);
                int p = mspr.readCols(inst.ms2);

                // Allocate MSPR for result (m x p)
                if (mspr.readBase(inst.md) == 0 || mspr.readBase(inst.md) < 400) {
                    static int nextAddr = 500;
                    mspr.writeMatrix(inst.md, nextAddr, m, p);  // Result is m x p
                    nextAddr += m * p;
                }
                else {
                    mspr.writeRows(inst.md, m);
                    mspr.writeCols(inst.md, p);
                    mspr.setValid(inst.md, true);
                }

                int baseA = mspr.readBase(inst.ms1);
                int baseB = mspr.readBase(inst.ms2);
                int baseC = mspr.readBase(inst.md);

                // Matrix multiplication: C = A × B
                for (int i = 0; i < m; i++) {
                    for (int j = 0; j < p; j++) {
                        int sum = 0;
                        for (int k = 0; k < n; k++) {
                            sum += memory[baseA + i * n + k] * memory[baseB + k * p + j];
                        }
                        memory[baseC + i * p + j] = sum;
                    }
                }

                traceFile << "MULM M" << inst.md << "=M" << inst.ms1 << "×M" << inst.ms2
                    << " (" << m << "×" << n << ")×(" << n << "×" << p << ")=("
                    << m << "×" << p << ")" << std::endl;
            }
            break;
        }

        case OP_SCALE: {
            if (mspr.isValid(inst.ms1)) {
                int rows = mspr.readRows(inst.ms1);
                int cols = mspr.readCols(inst.ms1);
                int scalar = registers[inst.rs1];

                // Allocate MSPR for result
                if (mspr.readBase(inst.md) == 0 || mspr.readBase(inst.md) < 400) {
                    static int nextAddr = 500;
                    mspr.writeMatrix(inst.md, nextAddr, rows, cols);
                    nextAddr += rows * cols;
                }
                else {
                    mspr.writeRows(inst.md, rows);
                    mspr.writeCols(inst.md, cols);
                    mspr.setValid(inst.md, true);
                }

                int srcAddr = mspr.readBase(inst.ms1);
                int destAddr = mspr.readBase(inst.md);

                for (int i = 0; i < rows * cols; i++) {
                    memory[destAddr + i] = memory[srcAddr + i] * scalar;
                }

                traceFile << "SCALE M" << inst.md << "=M" << inst.ms1 << "×R" << inst.rs1
                    << "(=" << scalar << ")" << std::endl;
            }
            break;
        }

        case OP_DETERMINANT: {
            if (mspr.isValid(inst.ms1)) {
                int baseAddr = mspr.readBase(inst.ms1);
                int n = mspr.readRows(inst.ms1);
                int det = 0;

                if (n == 1) {
                    det = memory[baseAddr];
                }
                else if (n == 2) {
                    det = memory[baseAddr] * memory[baseAddr + 3] -
                        memory[baseAddr + 1] * memory[baseAddr + 2];
                }
                else if (n == 3) {
                    int a = memory[baseAddr], b = memory[baseAddr + 1], c = memory[baseAddr + 2];
                    int d = memory[baseAddr + 3], e = memory[baseAddr + 4], f = memory[baseAddr + 5];
                    int g = memory[baseAddr + 6], h = memory[baseAddr + 7], i = memory[baseAddr + 8];
                    det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
                }

                EX_MEM.aluResult = det;
                EX_MEM.regWrite = true;
                EX_MEM.destReg = inst.rd;
                traceFile << "DET(M" << inst.ms1 << ")=" << det << "→R" << inst.rd << std::endl;
            }
            break;
        }

        case OP_TRANSPOSE: {
            if (mspr.isValid(inst.ms1)) {
                int rows = mspr.readRows(inst.ms1);
                int cols = mspr.readCols(inst.ms1);

                // Allocate MSPR for transpose (dimensions swapped!)
                if (mspr.readBase(inst.md) == 0 || mspr.readBase(inst.md) < 400) {
                    static int nextAddr = 500;
                    mspr.writeMatrix(inst.md, nextAddr, cols, rows);  // SWAPPED!
                    nextAddr += rows * cols;
                }
                else {
                    mspr.writeRows(inst.md, cols);  // SWAPPED!
                    mspr.writeCols(inst.md, rows);  // SWAPPED!
                    mspr.setValid(inst.md, true);
                }

                int srcAddr = mspr.readBase(inst.ms1);
                int destAddr = mspr.readBase(inst.md);

                // Transpose operation
                for (int i = 0; i < rows; i++) {
                    for (int j = 0; j < cols; j++) {
                        memory[destAddr + j * rows + i] = memory[srcAddr + i * cols + j];
                    }
                }

                traceFile << "TRANSPOSE M" << inst.md << "=M" << inst.ms1 << "^T "
                    << "(" << rows << "×" << cols << ")→(" << cols << "×" << rows << ")"
                    << std::endl;
            }
            break;
        }
        }

        // Matrix operations complete - let them propagate to WB stage for instruction counting
        // DO NOT set ID_EX.valid = false here!
        // DO NOT return early!
        // Let the instruction flow through MEM and WB stages

        traceFile << std::endl;

        // Clear ID/EX (moved to end of function)
        // ID_EX.valid = false;  // MOVED TO BOTTOM
        // Fall through to continue pipeline flow
    }
    else {
        // Execute scalar operations (only if NOT a matrix operation)
        switch (inst.opcode) {
        case OP_ADD:
            EX_MEM.aluResult = operand1 + operand2;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_SUB:
            EX_MEM.aluResult = operand1 - operand2;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_ADDI:
            EX_MEM.aluResult = operand1 + inst.immediate;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_SUBI:
            EX_MEM.aluResult = operand1 - inst.immediate;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_MUL:
            EX_MEM.aluResult = operand1 * operand2;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_DIV:
            EX_MEM.aluResult = (operand2 != 0) ? operand1 / operand2 : 0;
            traceFile << " Result=" << EX_MEM.aluResult;
            break;
        case OP_LI:
            EX_MEM.aluResult = inst.immediate;
            traceFile << " Result=" << inst.immediate;
            break;
        case OP_MOV:
            EX_MEM.aluResult = operand1;
            traceFile << " Result=" << operand1;
            break;
        case OP_LW:
            EX_MEM.aluResult = operand1 + inst.offset;
            traceFile << " MemAddr=" << EX_MEM.aluResult;
            break;
        case OP_SW:
            EX_MEM.aluResult = operand1 + inst.offset;
            // Forward write data if needed
            EX_MEM.memWriteData = registers[inst.rd];
            int forwardedWriteData;
            if (needsForwarding(inst.rd, forwardedWriteData)) {
                EX_MEM.memWriteData = forwardedWriteData;
            }
            traceFile << " MemAddr=" << EX_MEM.aluResult << " Data=" << EX_MEM.memWriteData;
            break;
        case OP_BEQ:
            EX_MEM.branchTaken = (operand1 == operand2);
            if (EX_MEM.branchTaken) {
                pc = inst.target;
                traceFile << " TAKEN→PC=" << pc;
            }
            else {
                traceFile << " NOT_TAKEN";
            }
            break;
        case OP_BNE:
            EX_MEM.branchTaken = (operand1 != operand2);
            if (EX_MEM.branchTaken) {
                pc = inst.target;
                traceFile << " TAKEN→PC=" << pc;
            }
            else {
                traceFile << " NOT_TAKEN";
            }
            break;
        case OP_BGT:
            EX_MEM.branchTaken = (operand1 > operand2);
            if (EX_MEM.branchTaken) {
                pc = inst.target;
                traceFile << " TAKEN→PC=" << pc;
            }
            break;
        case OP_BLT:
            EX_MEM.branchTaken = (operand1 < operand2);
            if (EX_MEM.branchTaken) {
                pc = inst.target;
                traceFile << " TAKEN→PC=" << pc;
            }
            break;
        case OP_J:
            pc = inst.target;
            EX_MEM.branchTaken = true;
            traceFile << " JUMP→PC=" << pc;
            break;
        case OP_HALT:
            halted = true;
            traceFile << " HALT";
            break;
        }

        traceFile << std::endl;
    }  // End of else block for scalar operations

    // Clear ID/EX - this now happens for ALL instructions (matrix and scalar)
    ID_EX.valid = false;
}

void PipelinedSimulator::memoryAccess() {
    if (!EX_MEM.valid) {
        traceFile << "[MEM] BUBBLE" << std::endl;
        return;
    }

    // Move EX/MEM to MEM/WB
    MEM_WB = EX_MEM;

    traceFile << "[MEM] ";

    if (MEM_WB.memRead) {
        int addr = MEM_WB.aluResult;
        if (addr >= 0 && addr < MEMORY_SIZE) {
            MEM_WB.memReadData = memory[addr];
            MEM_WB.writeBackData = MEM_WB.memReadData;
            traceFile << "Load[" << addr << "]=" << MEM_WB.memReadData;
        }
    }
    else if (MEM_WB.memWrite) {
        int addr = MEM_WB.aluResult;
        if (addr >= 0 && addr < MEMORY_SIZE) {
            memory[addr] = MEM_WB.memWriteData;
            traceFile << "Store[" << addr << "]=" << MEM_WB.memWriteData;
        }
    }
    else {
        MEM_WB.writeBackData = MEM_WB.aluResult;
        traceFile << "Pass ALU=" << MEM_WB.aluResult;
    }

    traceFile << std::endl;

    // Clear EX/MEM
    EX_MEM.valid = false;
}

void PipelinedSimulator::writeBack() {
    if (!MEM_WB.valid) {
        traceFile << "[WB] BUBBLE" << std::endl;
        return;
    }

    traceFile << "[WB] ";

    // Handle DETERMINANT result writeback
    if (MEM_WB.inst.opcode == OP_DETERMINANT) {
        if (MEM_WB.destReg != 0 && MEM_WB.destReg != -1) {
            registers[MEM_WB.destReg] = MEM_WB.aluResult;
            traceFile << "R" << MEM_WB.destReg << "←" << MEM_WB.aluResult;
            instructionCount++;
        }
    }
    // Handle regular register writeback
    else if (MEM_WB.regWrite && MEM_WB.destReg != 0 && MEM_WB.destReg != -1) {
        registers[MEM_WB.destReg] = MEM_WB.writeBackData;
        traceFile << "R" << MEM_WB.destReg << "←" << MEM_WB.writeBackData;
        instructionCount++;
    }
    else {
        traceFile << "No WB";
        // Count instructions that don't write to registers
        if (MEM_WB.inst.opcode == OP_SW || MEM_WB.inst.opcode == OP_HALT ||
            (MEM_WB.inst.opcode >= OP_BEQ && MEM_WB.inst.opcode <= OP_J) ||
            (MEM_WB.inst.opcode >= OP_DECLAREM && MEM_WB.inst.opcode <= OP_TRANSPOSE)) {
            // This now covers ALL matrix operations: DECLAREM(0) through TRANSPOSE(8)
            instructionCount++;
        }
    }

    traceFile << std::endl;

    // Clear MEM/WB
    MEM_WB.valid = false;
}

void PipelinedSimulator::run() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Pipelined Simulator - Enhanced Version" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Instructions loaded: " << decodedInstructions.size() << std::endl;
    std::cout << std::endl;

    while (!halted || IF_ID.valid || ID_EX.valid || EX_MEM.valid || MEM_WB.valid) {
        cycleCount++;

        traceFile << "\n╔════════════════════════════════════════╗" << std::endl;
        traceFile << "║  CYCLE " << std::setw(4) << cycleCount << std::setw(27) << "║" << std::endl;
        traceFile << "╚════════════════════════════════════════╝" << std::endl;

        // Reset control signals
        stall = false;
        flush = false;

        // Pipeline stages (reverse order: WB → MEM → EX → ID → IF)
        writeBack();
        memoryAccess();

        // Check for data hazards BEFORE executing
        bool dataHazard = detectDataHazard();

        execute();

        // Handle data hazard or decode
        if (dataHazard) {
            handleDataHazard();
        }
        else {
            decode();
        }

        // Handle control hazards after execute
        handleControlHazard();

        // Fetch only if not stalled
        if (!stall) {
            fetch();
        }
        else {
            traceFile << "[IF] STALLED - PC not advanced" << std::endl;
        }

        // Print pipeline state
        printPipelineState();

        // Safety limit
        if (cycleCount > 10000) {
            std::cout << "\nWARNING: Safety limit reached (10000 cycles)" << std::endl;
            break;
        }
    }

    // Print final statistics
    traceFile << "\n╔════════════════════════════════════════╗" << std::endl;
    traceFile << "║     EXECUTION COMPLETE                 ║" << std::endl;
    traceFile << "╚════════════════════════════════════════╝" << std::endl;
    traceFile << "\nPerformance Metrics:" << std::endl;
    traceFile << "-------------------" << std::endl;
    traceFile << "Total Cycles:        " << cycleCount << std::endl;
    traceFile << "Instructions:        " << instructionCount << std::endl;
    traceFile << "CPI:                 " << std::fixed << std::setprecision(3)
        << getCPI() << std::endl;
    traceFile << "Pipeline Stalls:     " << stallCount << std::endl;
    traceFile << "Data Hazards:        " << dataHazardCount << std::endl;
    traceFile << "Control Hazards:     " << controlHazardCount << std::endl;
    traceFile << "Total Hazards:       " << (dataHazardCount + controlHazardCount) << std::endl;

    double idealCPI = 1.0;
    double actualCPI = getCPI();
    double overhead = ((actualCPI - idealCPI) / idealCPI) * 100.0;
    traceFile << "\nPipeline Efficiency:" << std::endl;
    traceFile << "Ideal CPI:           1.000" << std::endl;
    traceFile << "Actual CPI:          " << std::fixed << std::setprecision(3)
        << actualCPI << std::endl;
    traceFile << "Pipeline Overhead:   " << std::fixed << std::setprecision(1)
        << overhead << "%" << std::endl;

    traceFile.close();

    std::cout << "\n========================================" << std::endl;
    std::cout << "Execution Complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Cycles:              " << cycleCount << std::endl;
    std::cout << "Instructions:        " << instructionCount << std::endl;
    std::cout << "CPI:                 " << std::fixed << std::setprecision(3)
        << getCPI() << std::endl;
    std::cout << "Pipeline Stalls:     " << stallCount << std::endl;
    std::cout << "Data Hazards:        " << dataHazardCount << std::endl;
    std::cout << "Control Hazards:     " << controlHazardCount << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nDetailed trace: pipeline_trace.txt" << std::endl;
}

void PipelinedSimulator::printPipelineState() {
    traceFile << "\n[PIPELINE STATE] ";
    traceFile << "IF:" << (IF_ID.valid ? "V" : "-") << " ";
    traceFile << "ID:" << (ID_EX.valid ? "V" : "-") << " ";
    traceFile << "EX:" << (EX_MEM.valid ? "V" : "-") << " ";
    traceFile << "MEM:" << (MEM_WB.valid ? "V" : "-");

    if (stall) traceFile << " [STALLED]";
    if (flush) traceFile << " [FLUSHED]";

    traceFile << std::endl;
}

void PipelinedSimulator::printState() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "REGISTER STATE" << std::endl;
    std::cout << "========================================" << std::endl;

    bool hasNonZero = false;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (registers[i] != 0) {
            hasNonZero = true;
            std::cout << "R" << std::setw(2) << i << " = "
                << std::setw(10) << registers[i];
            if ((i + 1) % 4 == 0) std::cout << std::endl;
        }
    }
    if (!hasNonZero) {
        std::cout << "(All registers are zero)" << std::endl;
    }
    std::cout << std::endl;
}

void PipelinedSimulator::printMemoryRange(int start, int end) {
    bool hasData = false;
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            hasData = true;
            break;
        }
    }

    if (!hasData) return;

    std::cout << "\nMemory [" << start << "-" << end << "]:" << std::endl;
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            std::cout << "  [" << std::setw(4) << i << "] = "
                << std::setw(6) << memory[i] << std::endl;
        }
    }
}