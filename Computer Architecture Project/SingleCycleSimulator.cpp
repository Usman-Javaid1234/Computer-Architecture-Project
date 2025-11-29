#include "MatrixProcessor.h"

SingleCycleSimulator::SingleCycleSimulator()
    : pc(0), cycleCount(0), instructionCount(0), halted(false) {

    // Initialize memory and registers to zero
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }
    for (int i = 0; i < NUM_REGISTERS; i++) {
        registers[i] = 0;
    }

    // R0 is hardwired to zero
    registers[0] = 0;

    // Open trace file
    traceFile.open("execution_trace.txt");
    traceFile << "=== Execution Trace ===" << std::endl;
}

void SingleCycleSimulator::loadProgram(const std::string& filename) {
    AssemblyParser parser(memory, instructionMemory, decodedInstructions);
    parser.parseFile(filename);
}

Instruction SingleCycleSimulator::decodeInstruction(uint32_t raw) {
    Instruction inst;
    inst.raw = raw;

    inst.opcode = static_cast<Opcode>((raw >> 26) & 0x3F);

    switch (inst.opcode) {
    case OP_DECLAREM:
        inst.md = (raw >> 22) & 0xF;
        inst.rows = (raw >> 14) & 0xFF;
        inst.cols = (raw >> 6) & 0xFF;
        break;

    case OP_LOADM:
    case OP_STOREM:
        inst.md = (raw >> 22) & 0xF;
        inst.address = raw & 0xFFFF;
        break;

    case OP_ADDM:
    case OP_SUBM:
    case OP_MULM:
    case OP_TRANSPOSE:
        inst.md = (raw >> 22) & 0xF;
        inst.ms1 = (raw >> 18) & 0xF;
        inst.ms2 = (raw >> 14) & 0xF;
        break;

    case OP_SCALE:
        inst.md = (raw >> 22) & 0xF;
        inst.ms1 = (raw >> 18) & 0xF;
        inst.rs1 = (raw >> 13) & 0x1F;
        break;

    case OP_DETERMINANT:
        inst.rd = (raw >> 22) & 0xF;
        inst.ms1 = (raw >> 18) & 0xF;
        break;

        // Scalar instructions
    case OP_ADDI:
    case OP_SUBI:
        inst.rd = (raw >> 21) & 0x1F;
        inst.rs1 = (raw >> 16) & 0x1F;
        inst.immediate = raw & 0xFFFF;
        // Sign extend
        if (inst.immediate & 0x8000) {
            inst.immediate |= 0xFFFF0000;
        }
        break;

    case OP_LW:
    case OP_SW:
        inst.rd = (raw >> 21) & 0x1F;
        inst.rs1 = (raw >> 16) & 0x1F;
        inst.offset = raw & 0xFFFF;
        if (inst.offset & 0x8000) {
            inst.offset |= 0xFFFF0000;
        }
        break;

    case OP_BEQ:
    case OP_BNE:
    case OP_BGT:
    case OP_BLT:
        inst.rs1 = (raw >> 21) & 0x1F;
        inst.rs2 = (raw >> 16) & 0x1F;
        inst.target = raw & 0xFFFF;
        break;

    case OP_J:
        inst.target = raw & 0x3FFFFFF;
        break;

    case OP_MOV:
        inst.rd = (raw >> 21) & 0x1F;
        inst.rs1 = (raw >> 16) & 0x1F;
        break;

    case OP_LI:
        inst.rd = (raw >> 21) & 0x1F;
        inst.immediate = raw & 0xFFFF;
        if (inst.immediate & 0x8000) {
            inst.immediate |= 0xFFFF0000;
        }
        break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        inst.rd = (raw >> 21) & 0x1F;
        inst.rs1 = (raw >> 16) & 0x1F;
        inst.rs2 = (raw >> 11) & 0x1F;
        break;
    }

    return inst;
}

// Helper methods for matrix operations
void SingleCycleSimulator::getMatrixFromMemory(int baseAddr, int rows, int cols,
    std::vector<std::vector<int>>& matrix) {
    matrix.resize(rows, std::vector<int>(cols));
    int addr = baseAddr;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = memory[addr++];
        }
    }
}

void SingleCycleSimulator::writeMatrixToMemory(int baseAddr,
    const std::vector<std::vector<int>>& matrix) {
    int addr = baseAddr;
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            memory[addr++] = matrix[i][j];
        }
    }
}

int SingleCycleSimulator::calculateDeterminant(int baseAddr, int n) {
    std::vector<std::vector<int>> matrix;
    getMatrixFromMemory(baseAddr, n, n, matrix);

    if (n == 1) return matrix[0][0];
    if (n == 2) return matrix[0][0] * matrix[1][1] - matrix[0][1] * matrix[1][0];
    if (n == 3) {
        return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1])
            - matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0])
            + matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
    }
    return 0;  // Larger matrices not supported
}

// Matrix instruction implementations
void SingleCycleSimulator::executeDECLAREM(const Instruction& inst) {
    matrixDescriptors[inst.md].rows = inst.rows;
    matrixDescriptors[inst.md].cols = inst.cols;
    matrixDescriptors[inst.md].valid = true;

    // Allocate memory starting from address 400 to avoid overwriting data section
    static int nextAddress = 400;
    matrixDescriptors[inst.md].baseAddress = nextAddress;
    nextAddress += inst.rows * inst.cols;

    // Zero-initialize the allocated space
    for (int i = 0; i < inst.rows * inst.cols; i++) {
        memory[matrixDescriptors[inst.md].baseAddress + i] = 0;
    }

    traceFile << "  DECLAREM M" << inst.md << ", " << inst.rows << ", " << inst.cols
        << " -> Base: " << matrixDescriptors[inst.md].baseAddress << std::endl;

    cycleCount += 1;
}

void SingleCycleSimulator::executeLOADM(const Instruction& inst) {
    if (!matrixDescriptors[inst.md].valid) {
        std::cerr << "Error: Matrix M" << inst.md << " not declared!" << std::endl;
        return;
    }

    int rows = matrixDescriptors[inst.md].rows;
    int cols = matrixDescriptors[inst.md].cols;
    int destAddr = matrixDescriptors[inst.md].baseAddress;
    int srcAddr = inst.address;

    for (int i = 0; i < rows * cols; i++) {
        memory[destAddr + i] = memory[srcAddr + i];
    }

    traceFile << "  LOADM M" << inst.md << " from " << inst.address << std::endl;
    cycleCount += rows * cols;
}

void SingleCycleSimulator::executeSTOREM(const Instruction& inst) {
    if (!matrixDescriptors[inst.md].valid) {
        std::cerr << "Error: Matrix M" << inst.md << " not declared!" << std::endl;
        return;
    }

    int rows = matrixDescriptors[inst.md].rows;
    int cols = matrixDescriptors[inst.md].cols;
    int srcAddr = matrixDescriptors[inst.md].baseAddress;
    int destAddr = inst.address;

    for (int i = 0; i < rows * cols; i++) {
        memory[destAddr + i] = memory[srcAddr + i];
    }

    traceFile << "  STOREM M" << inst.md << " to " << inst.address << std::endl;
    cycleCount += rows * cols;
}

void SingleCycleSimulator::executeADDM(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid || !matrixDescriptors[inst.ms2].valid) return;

    int rows = matrixDescriptors[inst.ms1].rows;
    int cols = matrixDescriptors[inst.ms1].cols;

    matrixDescriptors[inst.md].rows = rows;
    matrixDescriptors[inst.md].cols = cols;
    matrixDescriptors[inst.md].valid = true;

    if (matrixDescriptors[inst.md].baseAddress == 0 || matrixDescriptors[inst.md].baseAddress < 400) {
        static int nextAddr = 600;  // Start result matrices at 600
        matrixDescriptors[inst.md].baseAddress = nextAddr;
        nextAddr += rows * cols;
    }

    int addr1 = matrixDescriptors[inst.ms1].baseAddress;
    int addr2 = matrixDescriptors[inst.ms2].baseAddress;
    int addrDest = matrixDescriptors[inst.md].baseAddress;

    for (int i = 0; i < rows * cols; i++) {
        memory[addrDest + i] = memory[addr1 + i] + memory[addr2 + i];
    }

    traceFile << "  ADDM M" << inst.md << " = M" << inst.ms1 << " + M" << inst.ms2 << std::endl;
    cycleCount += rows * cols;
}

void SingleCycleSimulator::executeSUBM(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid || !matrixDescriptors[inst.ms2].valid) return;

    int rows = matrixDescriptors[inst.ms1].rows;
    int cols = matrixDescriptors[inst.ms1].cols;

    matrixDescriptors[inst.md].rows = rows;
    matrixDescriptors[inst.md].cols = cols;
    matrixDescriptors[inst.md].valid = true;

    if (matrixDescriptors[inst.md].baseAddress == 0 || matrixDescriptors[inst.md].baseAddress < 400) {
        static int nextAddr = 650;  // Continue after ADDM allocations
        matrixDescriptors[inst.md].baseAddress = nextAddr;
        nextAddr += rows * cols;
    }

    int addr1 = matrixDescriptors[inst.ms1].baseAddress;
    int addr2 = matrixDescriptors[inst.ms2].baseAddress;
    int addrDest = matrixDescriptors[inst.md].baseAddress;

    for (int i = 0; i < rows * cols; i++) {
        memory[addrDest + i] = memory[addr1 + i] - memory[addr2 + i];
    }

    traceFile << "  SUBM M" << inst.md << " = M" << inst.ms1 << " - M" << inst.ms2 << std::endl;
    cycleCount += rows * cols;
}

void SingleCycleSimulator::executeMULM(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid || !matrixDescriptors[inst.ms2].valid) return;

    int m = matrixDescriptors[inst.ms1].rows;
    int n = matrixDescriptors[inst.ms1].cols;
    int p = matrixDescriptors[inst.ms2].cols;

    matrixDescriptors[inst.md].rows = m;
    matrixDescriptors[inst.md].cols = p;
    matrixDescriptors[inst.md].valid = true;

    if (matrixDescriptors[inst.md].baseAddress == 0 || matrixDescriptors[inst.md].baseAddress < 400) {
        static int nextAddr = 500;
        matrixDescriptors[inst.md].baseAddress = nextAddr;
        nextAddr += m * p;
    }

    std::vector<std::vector<int>> A, B, C(m, std::vector<int>(p, 0));
    getMatrixFromMemory(matrixDescriptors[inst.ms1].baseAddress, m, n, A);
    getMatrixFromMemory(matrixDescriptors[inst.ms2].baseAddress, n, p, B);

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < p; j++) {
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    writeMatrixToMemory(matrixDescriptors[inst.md].baseAddress, C);

    traceFile << "  MULM M" << inst.md << " = M" << inst.ms1 << " * M" << inst.ms2 << std::endl;
    cycleCount += m * n * p;
}

void SingleCycleSimulator::executeSCALE(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid) return;

    int rows = matrixDescriptors[inst.ms1].rows;
    int cols = matrixDescriptors[inst.ms1].cols;
    int scalar = registers[inst.rs1];

    matrixDescriptors[inst.md].rows = rows;
    matrixDescriptors[inst.md].cols = cols;
    matrixDescriptors[inst.md].valid = true;

    if (matrixDescriptors[inst.md].baseAddress == 0 || matrixDescriptors[inst.md].baseAddress < 400) {
        static int nextAddr = 500;
        matrixDescriptors[inst.md].baseAddress = nextAddr;
        nextAddr += rows * cols;
    }

    int srcAddr = matrixDescriptors[inst.ms1].baseAddress;
    int destAddr = matrixDescriptors[inst.md].baseAddress;

    for (int i = 0; i < rows * cols; i++) {
        memory[destAddr + i] = memory[srcAddr + i] * scalar;
    }

    traceFile << "  SCALE M" << inst.md << " = M" << inst.ms1 << " * R" << inst.rs1
        << " (=" << scalar << ")" << std::endl;
    cycleCount += rows * cols;
}

void SingleCycleSimulator::executeDETERMINANT(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid) return;

    int n = matrixDescriptors[inst.ms1].rows;
    int det = calculateDeterminant(matrixDescriptors[inst.ms1].baseAddress, n);
    registers[inst.rd] = det;

    traceFile << "  DETERMINANT R" << inst.rd << " = det(M" << inst.ms1 << ") = " << det << std::endl;
    cycleCount += n * n * n;
}

void SingleCycleSimulator::executeTRANSPOSE(const Instruction& inst) {
    if (!matrixDescriptors[inst.ms1].valid) return;

    int rows = matrixDescriptors[inst.ms1].rows;
    int cols = matrixDescriptors[inst.ms1].cols;

    matrixDescriptors[inst.md].rows = cols;
    matrixDescriptors[inst.md].cols = rows;
    matrixDescriptors[inst.md].valid = true;

    if (matrixDescriptors[inst.md].baseAddress == 0 || matrixDescriptors[inst.md].baseAddress < 400) {
        static int nextAddr = 500;
        matrixDescriptors[inst.md].baseAddress = nextAddr;
        nextAddr += rows * cols;
    }

    std::vector<std::vector<int>> A, AT(cols, std::vector<int>(rows));
    getMatrixFromMemory(matrixDescriptors[inst.ms1].baseAddress, rows, cols, A);

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            AT[j][i] = A[i][j];
        }
    }

    writeMatrixToMemory(matrixDescriptors[inst.md].baseAddress, AT);

    traceFile << "  TRANSPOSE M" << inst.md << " = M" << inst.ms1 << "^T" << std::endl;
    cycleCount += rows * cols;
}

// Scalar instruction implementations
void SingleCycleSimulator::executeADDI(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1] + inst.immediate;
    registers[0] = 0;  // Keep R0 zero

    traceFile << "  ADDI R" << inst.rd << " = R" << inst.rs1 << " + " << inst.immediate
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeSUBI(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1] - inst.immediate;
    registers[0] = 0;

    traceFile << "  SUBI R" << inst.rd << " = R" << inst.rs1 << " - " << inst.immediate
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeLW(const Instruction& inst) {
    int address = registers[inst.rs1] + inst.offset;
    if (address >= 0 && address < MEMORY_SIZE) {
        registers[inst.rd] = memory[address];
        registers[0] = 0;

        traceFile << "  LW R" << inst.rd << " = MEM[" << address << "] (="
            << registers[inst.rd] << ")" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeSW(const Instruction& inst) {
    int address = registers[inst.rs1] + inst.offset;
    if (address >= 0 && address < MEMORY_SIZE) {
        memory[address] = registers[inst.rd];

        traceFile << "  SW MEM[" << address << "] = R" << inst.rd
            << " (=" << registers[inst.rd] << ")" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeBEQ(const Instruction& inst) {
    traceFile << "  BEQ R" << inst.rs1 << " (=" << registers[inst.rs1]
        << "), R" << inst.rs2 << " (=" << registers[inst.rs2] << ")";

    if (registers[inst.rs1] == registers[inst.rs2]) {
        pc = inst.target;
        traceFile << " -> BRANCH TAKEN to " << inst.target << std::endl;
    }
    else {
        traceFile << " -> NOT TAKEN" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeBNE(const Instruction& inst) {
    traceFile << "  BNE R" << inst.rs1 << " (=" << registers[inst.rs1]
        << "), R" << inst.rs2 << " (=" << registers[inst.rs2] << ")";

    if (registers[inst.rs1] != registers[inst.rs2]) {
        pc = inst.target;
        traceFile << " -> BRANCH TAKEN to " << inst.target << std::endl;
    }
    else {
        traceFile << " -> NOT TAKEN" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeBGT(const Instruction& inst) {
    traceFile << "  BGT R" << inst.rs1 << " (=" << registers[inst.rs1]
        << "), R" << inst.rs2 << " (=" << registers[inst.rs2] << ")";

    if (registers[inst.rs1] > registers[inst.rs2]) {
        pc = inst.target;
        traceFile << " -> BRANCH TAKEN to " << inst.target << std::endl;
    }
    else {
        traceFile << " -> NOT TAKEN" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeBLT(const Instruction& inst) {
    traceFile << "  BLT R" << inst.rs1 << " (=" << registers[inst.rs1]
        << "), R" << inst.rs2 << " (=" << registers[inst.rs2] << ")";

    if (registers[inst.rs1] < registers[inst.rs2]) {
        pc = inst.target;
        traceFile << " -> BRANCH TAKEN to " << inst.target << std::endl;
    }
    else {
        traceFile << " -> NOT TAKEN" << std::endl;
    }
    cycleCount += 1;
}

void SingleCycleSimulator::executeJ(const Instruction& inst) {
    pc = inst.target;
    traceFile << "  J -> Jump to " << inst.target << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeMOV(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1];
    registers[0] = 0;

    traceFile << "  MOV R" << inst.rd << " = R" << inst.rs1
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeLI(const Instruction& inst) {
    registers[inst.rd] = inst.immediate;
    registers[0] = 0;

    traceFile << "  LI R" << inst.rd << " = " << inst.immediate << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeADD(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1] + registers[inst.rs2];
    registers[0] = 0;

    traceFile << "  ADD R" << inst.rd << " = R" << inst.rs1 << " + R" << inst.rs2
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeSUB(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1] - registers[inst.rs2];
    registers[0] = 0;

    traceFile << "  SUB R" << inst.rd << " = R" << inst.rs1 << " - R" << inst.rs2
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeMUL(const Instruction& inst) {
    registers[inst.rd] = registers[inst.rs1] * registers[inst.rs2];
    registers[0] = 0;

    traceFile << "  MUL R" << inst.rd << " = R" << inst.rs1 << " * R" << inst.rs2
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeDIV(const Instruction& inst) {
    if (registers[inst.rs2] != 0) {
        registers[inst.rd] = registers[inst.rs1] / registers[inst.rs2];
    }
    else {
        registers[inst.rd] = 0;
        std::cerr << "Warning: Division by zero!" << std::endl;
    }
    registers[0] = 0;

    traceFile << "  DIV R" << inst.rd << " = R" << inst.rs1 << " / R" << inst.rs2
        << " (=" << registers[inst.rd] << ")" << std::endl;
    cycleCount += 1;
}

void SingleCycleSimulator::executeInstruction(const Instruction& inst) {
    switch (inst.opcode) {
        // Matrix operations
    case OP_DECLAREM:   executeDECLAREM(inst); break;
    case OP_LOADM:      executeLOADM(inst); break;
    case OP_STOREM:     executeSTOREM(inst); break;
    case OP_ADDM:       executeADDM(inst); break;
    case OP_SUBM:       executeSUBM(inst); break;
    case OP_MULM:       executeMULM(inst); break;
    case OP_SCALE:      executeSCALE(inst); break;
    case OP_DETERMINANT: executeDETERMINANT(inst); break;
    case OP_TRANSPOSE:  executeTRANSPOSE(inst); break;

        // Scalar operations
    case OP_ADDI:       executeADDI(inst); break;
    case OP_SUBI:       executeSUBI(inst); break;
    case OP_LW:         executeLW(inst); break;
    case OP_SW:         executeSW(inst); break;
    case OP_BEQ:        executeBEQ(inst); break;
    case OP_BNE:        executeBNE(inst); break;
    case OP_BGT:        executeBGT(inst); break;
    case OP_BLT:        executeBLT(inst); break;
    case OP_J:          executeJ(inst); break;
    case OP_MOV:        executeMOV(inst); break;
    case OP_LI:         executeLI(inst); break;
    case OP_ADD:        executeADD(inst); break;
    case OP_SUB:        executeSUB(inst); break;
    case OP_MUL:        executeMUL(inst); break;
    case OP_DIV:        executeDIV(inst); break;

    case OP_HALT:
        halted = true;
        traceFile << "  HALT - Program terminated" << std::endl;
        break;
    }
}

void SingleCycleSimulator::run() {
    std::cout << "\n=== Running Single-Cycle Simulator ===" << std::endl;

    pc = 0;
    int iterationCount = 0;
    const int MAX_ITERATIONS = 10000;

    while (pc < instructionMemory.size() && !halted && iterationCount < MAX_ITERATIONS) {
        traceFile << "\nCycle " << cycleCount << ", PC=" << pc << std::endl;

        int oldPC = pc;

        // Use decoded instructions if available
        Instruction inst;
        if (pc < decodedInstructions.size()) {
            inst = decodedInstructions[pc];
        }
        else {
            inst = decodeInstruction(instructionMemory[pc]);
        }

        // Execute
        executeInstruction(inst);

        instructionCount++;

        // Update PC (unless branch/jump changed it)
        if (pc == oldPC) {
            pc++;
        }

        iterationCount++;
    }

    if (iterationCount >= MAX_ITERATIONS) {
        std::cout << "Warning: Maximum iteration count reached!" << std::endl;
    }

    traceFile << "\n=== Execution Complete ===" << std::endl;
    traceFile << "Total Cycles: " << cycleCount << std::endl;
    traceFile << "Instructions Executed: " << instructionCount << std::endl;
    traceFile << "CPI: " << getCPI() << std::endl;

    std::cout << "Execution complete." << std::endl;
    std::cout << "Cycles: " << cycleCount << ", Instructions: " << instructionCount
        << ", CPI: " << std::fixed << std::setprecision(2) << getCPI() << std::endl;
}

void SingleCycleSimulator::printRegisters() {
    std::cout << "\n=== Registers ===" << std::endl;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        if (registers[i] != 0 || i == 0) {
            std::cout << "R" << std::setw(2) << i << " = " << std::setw(10) << registers[i];
            if ((i + 1) % 4 == 0) std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void SingleCycleSimulator::printMatrixDescriptors() {
    std::cout << "\n=== Matrix Descriptors ===" << std::endl;
    for (int i = 0; i < NUM_MATRIX_DESCRIPTORS; i++) {
        if (matrixDescriptors[i].valid) {
            std::cout << "M" << i << ": " << matrixDescriptors[i].rows << "x"
                << matrixDescriptors[i].cols << " at address "
                << matrixDescriptors[i].baseAddress << std::endl;
        }
    }
}

void SingleCycleSimulator::printMemoryRange(int start, int end) {
    std::cout << "\nMemory [" << start << "-" << end << "]:" << std::endl;
    bool hasData = false;
    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        if (memory[i] != 0) {
            hasData = true;
            break;
        }
    }

    if (!hasData) {
        // Skip empty ranges for cleaner output
        return;
    }

    for (int i = start; i <= end && i < MEMORY_SIZE; i++) {
        // Show all addresses in this range, including zeros
        std::cout << "  [" << std::setw(4) << i << "] = " << std::setw(6) << memory[i] << std::endl;
    }
}

void SingleCycleSimulator::printState() {
    printRegisters();
    printMatrixDescriptors();
}