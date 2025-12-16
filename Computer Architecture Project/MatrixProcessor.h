#ifndef MATRIX_PROCESSOR_H
#define MATRIX_PROCESSOR_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

// Constants
const int MEMORY_SIZE = 1024;              // Data memory size in words
const int INSTRUCTION_MEMORY_SIZE = 256;   // Instruction memory size
const int NUM_REGISTERS = 32;              // General purpose registers
const int NUM_MATRIX_DESCRIPTORS = 16;     // Matrix descriptors M0-M15

// Instruction Opcodes
enum Opcode {
    // Matrix Operations (from Assignment 2)
    OP_DECLAREM = 0,    // 000000
    OP_LOADM = 1,       // 000001
    OP_STOREM = 2,      // 000010
    OP_ADDM = 3,        // 000011
    OP_SUBM = 4,        // 000100
    OP_MULM = 5,        // 000101
    OP_SCALE = 6,       // 000110
    OP_DETERMINANT = 7, // 000111
    OP_TRANSPOSE = 8,   // 001000

    // Scalar/Control Instructions (NEW for Assignment 3)
    OP_ADDI = 9,        // 001001 - Add immediate
    OP_SUBI = 10,       // 001010 - Subtract immediate
    OP_LW = 11,         // 001011 - Load word
    OP_SW = 12,         // 001100 - Store word
    OP_BEQ = 13,        // 001101 - Branch if equal
    OP_BNE = 14,        // 001110 - Branch if not equal
    OP_BGT = 15,        // 001111 - Branch if greater than
    OP_BLT = 16,        // 010000 - Branch if less than
    OP_J = 17,          // 010001 - Jump
    OP_MOV = 18,        // 010010 - Move register
    OP_LI = 19,         // 010011 - Load immediate
    OP_ADD = 20,        // 010100 - Add registers
    OP_SUB = 21,        // 010101 - Subtract registers
    OP_MUL = 22,        // 010110 - Multiply registers
    OP_DIV = 23,        // 010111 - Divide registers
    OP_HALT = 63        // 111111 - Halt execution
};

// Matrix Descriptor Structure
struct MatrixDescriptor {
    int baseAddress;
    int rows;
    int cols;
    bool valid;

    MatrixDescriptor() : baseAddress(0), rows(0), cols(0), valid(false) {}
};

// Instruction Structure
struct Instruction {
    Opcode opcode;

    // Matrix operands
    int md;             // Destination matrix descriptor
    int ms1;            // Source matrix descriptor 1
    int ms2;            // Source matrix descriptor 2
    int rows;           // Rows for DECLAREM
    int cols;           // Cols for DECLAREM
    int address;        // Memory address for LOAD/STORE

    // Scalar operands
    int rd;             // Destination register
    int rs1;            // Source register 1
    int rs2;            // Source register 2
    int immediate;      // Immediate value
    int offset;         // Memory offset
    std::string label;  // Branch/Jump label
    int target;         // Branch/Jump target address

    uint32_t raw;       // Raw 32-bit instruction

    Instruction() : opcode(OP_HALT), md(0), ms1(0), ms2(0),
        rd(0), rs1(0), rs2(0), rows(0), cols(0),
        address(0), immediate(0), offset(0), target(0), raw(0) {
    }
};

// Assembly Parser Class
class AssemblyParser {
private:
    std::map<std::string, int> symbolTable;      // Labels -> addresses
    std::map<std::string, int> dataLabels;       // Data labels -> memory addresses
    int* memory;
    std::vector<uint32_t>& instructionMemory;
    std::vector<Instruction>& decodedInstructions;  // Store decoded for labels
    int currentDataAddress;
    int currentInstructionIndex;

    std::string trim(const std::string& str);
    std::string removeComment(const std::string& line);
    std::vector<int> parseCommaSeparated(const std::string& line);
    void parseDataLine(const std::string& line);
    void parseInstructionLine(const std::string& line);
    uint32_t encodeInstruction(const Instruction& inst);
    int parseMatrixDescriptor(const std::string& md);
    int parseRegister(const std::string& reg);
    void resolveLabels();  // Second pass to resolve branch targets

public:
    AssemblyParser(int* mem, std::vector<uint32_t>& instMem,
        std::vector<Instruction>& decodedInst);
    void parseFile(const std::string& filename);
};

// Single-Cycle Simulator Class
class SingleCycleSimulator {
private:
    int memory[MEMORY_SIZE];
    int registers[NUM_REGISTERS];
    MatrixDescriptor matrixDescriptors[NUM_MATRIX_DESCRIPTORS];
    std::vector<uint32_t> instructionMemory;
    std::vector<Instruction> decodedInstructions;
    int pc;
    int cycleCount;
    int instructionCount;
    bool halted;
    std::ofstream traceFile;

    Instruction decodeInstruction(uint32_t raw);
    void executeInstruction(const Instruction& inst);

    // Matrix instruction execution
    void executeDECLAREM(const Instruction& inst);
    void executeLOADM(const Instruction& inst);
    void executeSTOREM(const Instruction& inst);
    void executeADDM(const Instruction& inst);
    void executeSUBM(const Instruction& inst);
    void executeMULM(const Instruction& inst);
    void executeSCALE(const Instruction& inst);
    void executeDETERMINANT(const Instruction& inst);
    void executeTRANSPOSE(const Instruction& inst);

    // Scalar instruction execution (NEW)
    void executeADDI(const Instruction& inst);
    void executeSUBI(const Instruction& inst);
    void executeLW(const Instruction& inst);
    void executeSW(const Instruction& inst);
    void executeBEQ(const Instruction& inst);
    void executeBNE(const Instruction& inst);
    void executeBGT(const Instruction& inst);
    void executeBLT(const Instruction& inst);
    void executeJ(const Instruction& inst);
    void executeMOV(const Instruction& inst);
    void executeLI(const Instruction& inst);
    void executeADD(const Instruction& inst);
    void executeSUB(const Instruction& inst);
    void executeMUL(const Instruction& inst);
    void executeDIV(const Instruction& inst);

    // Helper methods
    int calculateDeterminant(int baseAddr, int n);
    void getMatrixFromMemory(int baseAddr, int rows, int cols, std::vector<std::vector<int>>& matrix);
    void writeMatrixToMemory(int baseAddr, const std::vector<std::vector<int>>& matrix);

public:
    SingleCycleSimulator();
    void loadProgram(const std::string& filename);
    void run();
    void step();  // Execute one instruction
    void printState();
    void printRegisters();
    void printMatrixDescriptors();
    void printMemoryRange(int start, int end);
    int getCycleCount() const { return cycleCount; }
    int getInstructionCount() const { return instructionCount; }
    double getCPI() const { return instructionCount > 0 ? (double)cycleCount / instructionCount : 0; }
};
struct PipelineRegister {
    bool valid;
    Instruction inst;
    uint32_t pc;

    // ID/EX stage
    int readData1, readData2;
    int immediate;
    bool branchTaken;

    // EX/MEM stage
    int aluResult;
    int memWriteData;
    bool memRead, memWrite;

    // MEM/WB stage
    int memReadData;
    int writeBackData;
    bool regWrite;
    int destReg;

    PipelineRegister() : valid(false), pc(0), readData1(0), readData2(0),
        immediate(0), branchTaken(false), aluResult(0),
        memWriteData(0), memRead(false), memWrite(false),
        memReadData(0), writeBackData(0), regWrite(false),
        destReg(0) {
    }
};

class PipelinedSimulator {
private:
    // Memory and registers
    int memory[MEMORY_SIZE];
    int registers[NUM_REGISTERS];
    MatrixDescriptor matrixDescriptors[NUM_MATRIX_DESCRIPTORS];

    // Instruction memory
    std::vector<uint32_t> instructionMemory;
    std::vector<Instruction> decodedInstructions;

    // Pipeline registers
    PipelineRegister IF_ID;   // Fetch -> Decode
    PipelineRegister ID_EX;   // Decode -> Execute
    PipelineRegister EX_MEM;  // Execute -> Memory
    PipelineRegister MEM_WB;  // Memory -> WriteBack

    // Control signals
    bool stall;
    bool flush;
    int pc;
    bool halted;

    // Performance counters
    int cycleCount;
    int instructionCount;
    int stallCount;
    int dataHazardCount;
    int controlHazardCount;

    // Trace file
    std::ofstream traceFile;

public:
    PipelinedSimulator();
    void loadProgram(const std::string& filename);
    void run();

    // Pipeline stages
    void fetch();
    void decode();
    void execute();
    void memoryAccess();
    void writeBack();

    // Hazard detection and handling
    bool detectDataHazard();
    bool detectControlHazard();
    void handleDataHazard();
    void handleControlHazard();
    bool needsForwarding(int rs, int& forwardValue);

    // Helper functions
    void printState();
    void printPipelineState();
    void printMemoryRange(int start, int end);
    int getCycleCount() const { return cycleCount; }
    int getInstructionCount() const { return instructionCount; }
    int getStallCount() const { return stallCount; }
    double getCPI() const { return (double)cycleCount / instructionCount; }
};

#endif // MATRIX_PROCESSOR_H