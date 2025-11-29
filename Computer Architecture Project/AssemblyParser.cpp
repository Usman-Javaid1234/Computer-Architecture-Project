#include "MatrixProcessor.h"

AssemblyParser::AssemblyParser(int* mem, std::vector<uint32_t>& instMem,
    std::vector<Instruction>& decodedInst)
    : memory(mem), instructionMemory(instMem), decodedInstructions(decodedInst),
    currentDataAddress(0), currentInstructionIndex(0) {
}

std::string AssemblyParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");

    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

std::string AssemblyParser::removeComment(const std::string& line) {
    size_t pos = line.find('#');
    if (pos == std::string::npos) {
        pos = line.find(';');  // Also support ; for comments
    }
    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }
    return line;
}

std::vector<int> AssemblyParser::parseCommaSeparated(const std::string& line) {
    std::vector<int> values;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) {
            values.push_back(std::stoi(token));
        }
    }

    return values;
}

void AssemblyParser::parseDataLine(const std::string& line) {
    size_t colonPos = line.find(':');

    if (colonPos != std::string::npos) {
        // Label definition: "MATRIX_A: 0"
        std::string label = trim(line.substr(0, colonPos));
        std::string addrStr = trim(line.substr(colonPos + 1));

        int address = std::stoi(addrStr);
        dataLabels[label] = address;
        currentDataAddress = address;

        std::cout << "  Data Label: " << label << " -> Address: " << address << std::endl;
    }
    else {
        // Data values: "1, 2, 3"
        std::vector<int> values = parseCommaSeparated(line);

        std::cout << "  Data at address " << currentDataAddress << ": ";
        for (int value : values) {
            memory[currentDataAddress++] = value;
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

int AssemblyParser::parseMatrixDescriptor(const std::string& md) {
    if (md[0] == 'M' || md[0] == 'm') {
        return std::stoi(md.substr(1));
    }
    throw std::runtime_error("Invalid matrix descriptor: " + md);
}

int AssemblyParser::parseRegister(const std::string& reg) {
    if (reg[0] == 'R' || reg[0] == 'r') {
        return std::stoi(reg.substr(1));
    }
    throw std::runtime_error("Invalid register: " + reg);
}

uint32_t AssemblyParser::encodeInstruction(const Instruction& inst) {
    uint32_t encoded = 0;

    // Encode based on opcode
    encoded = (inst.opcode << 26);

    switch (inst.opcode) {
    case OP_DECLAREM:
        encoded |= (inst.md << 22) | (inst.rows << 14) | (inst.cols << 6);
        break;

    case OP_LOADM:
    case OP_STOREM:
        encoded |= (inst.md << 22) | (inst.address & 0xFFFF);
        break;

    case OP_ADDM:
    case OP_SUBM:
    case OP_MULM:
    case OP_TRANSPOSE:
        encoded |= (inst.md << 22) | (inst.ms1 << 18) | (inst.ms2 << 14);
        break;

    case OP_SCALE:
        encoded |= (inst.md << 22) | (inst.ms1 << 18) | (inst.rs1 << 13);
        break;

    case OP_DETERMINANT:
        encoded |= (inst.rd << 22) | (inst.ms1 << 18);
        break;

        // Scalar instructions
    case OP_ADDI:
    case OP_SUBI:
        encoded |= (inst.rd << 21) | (inst.rs1 << 16) | (inst.immediate & 0xFFFF);
        break;

    case OP_LW:
    case OP_SW:
        encoded |= (inst.rd << 21) | (inst.rs1 << 16) | (inst.offset & 0xFFFF);
        break;

    case OP_BEQ:
    case OP_BNE:
    case OP_BGT:
    case OP_BLT:
        encoded |= (inst.rs1 << 21) | (inst.rs2 << 16) | (inst.target & 0xFFFF);
        break;

    case OP_J:
        encoded |= (inst.target & 0x3FFFFFF);
        break;

    case OP_MOV:
        encoded |= (inst.rd << 21) | (inst.rs1 << 16);
        break;

    case OP_LI:
        encoded |= (inst.rd << 21) | (inst.immediate & 0xFFFF);
        break;

    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
        encoded |= (inst.rd << 21) | (inst.rs1 << 16) | (inst.rs2 << 11);
        break;

    case OP_HALT:
        // No operands
        break;
    }

    return encoded;
}

void AssemblyParser::parseInstructionLine(const std::string& line) {
    // Check if line has a label (ends with :)
    size_t colonPos = line.find(':');
    std::string actualLine = line;

    if (colonPos != std::string::npos && colonPos < 20) {  // Label at beginning
        std::string label = trim(line.substr(0, colonPos));
        symbolTable[label] = currentInstructionIndex;
        std::cout << "  Code Label: " << label << " at instruction " << currentInstructionIndex << std::endl;

        actualLine = trim(line.substr(colonPos + 1));
        if (actualLine.empty()) return;  // Label only, no instruction
    }

    std::stringstream ss(actualLine);
    std::string opcode;
    ss >> opcode;

    // Convert to uppercase for comparison
    std::transform(opcode.begin(), opcode.end(), opcode.begin(), ::toupper);

    Instruction inst;

    // Parse operands
    std::vector<std::string> operands;
    std::string operand;
    while (ss >> operand) {
        operand.erase(std::remove(operand.begin(), operand.end(), ','), operand.end());
        if (!operand.empty()) {
            operands.push_back(operand);
        }
    }

    // Determine opcode and parse accordingly
    if (opcode == "DECLAREM") {
        inst.opcode = OP_DECLAREM;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.rows = std::stoi(operands[1]);
        inst.cols = std::stoi(operands[2]);
    }
    else if (opcode == "LOADM") {
        inst.opcode = OP_LOADM;
        inst.md = parseMatrixDescriptor(operands[0]);
        if (dataLabels.find(operands[1]) != dataLabels.end()) {
            inst.address = dataLabels[operands[1]];
        }
        else {
            inst.address = std::stoi(operands[1]);
        }
    }
    else if (opcode == "STOREM") {
        inst.opcode = OP_STOREM;
        inst.md = parseMatrixDescriptor(operands[0]);
        if (dataLabels.find(operands[1]) != dataLabels.end()) {
            inst.address = dataLabels[operands[1]];
        }
        else {
            inst.address = std::stoi(operands[1]);
        }
    }
    else if (opcode == "ADDM") {
        inst.opcode = OP_ADDM;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
        inst.ms2 = parseMatrixDescriptor(operands[2]);
    }
    else if (opcode == "SUBM") {
        inst.opcode = OP_SUBM;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
        inst.ms2 = parseMatrixDescriptor(operands[2]);
    }
    else if (opcode == "MULM") {
        inst.opcode = OP_MULM;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
        inst.ms2 = parseMatrixDescriptor(operands[2]);
    }
    else if (opcode == "SCALE") {
        inst.opcode = OP_SCALE;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
        inst.rs1 = parseRegister(operands[2]);
    }
    else if (opcode == "DETERMINANT") {
        inst.opcode = OP_DETERMINANT;
        inst.rd = parseRegister(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
    }
    else if (opcode == "TRANSPOSE") {
        inst.opcode = OP_TRANSPOSE;
        inst.md = parseMatrixDescriptor(operands[0]);
        inst.ms1 = parseMatrixDescriptor(operands[1]);
    }
    // Scalar instructions
    else if (opcode == "ADDI") {
        inst.opcode = OP_ADDI;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.immediate = std::stoi(operands[2]);
    }
    else if (opcode == "SUBI") {
        inst.opcode = OP_SUBI;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.immediate = std::stoi(operands[2]);
    }
    else if (opcode == "LW") {
        inst.opcode = OP_LW;
        inst.rd = parseRegister(operands[0]);
        // Parse offset(rs) format
        size_t openParen = operands[1].find('(');
        if (openParen != std::string::npos) {
            inst.offset = std::stoi(operands[1].substr(0, openParen));
            std::string regPart = operands[1].substr(openParen + 1);
            regPart.erase(std::remove(regPart.begin(), regPart.end(), ')'), regPart.end());
            inst.rs1 = parseRegister(regPart);
        }
        else {
            inst.offset = 0;
            inst.rs1 = parseRegister(operands[1]);
        }
    }
    else if (opcode == "SW") {
        inst.opcode = OP_SW;
        inst.rd = parseRegister(operands[0]);
        size_t openParen = operands[1].find('(');
        if (openParen != std::string::npos) {
            inst.offset = std::stoi(operands[1].substr(0, openParen));
            std::string regPart = operands[1].substr(openParen + 1);
            regPart.erase(std::remove(regPart.begin(), regPart.end(), ')'), regPart.end());
            inst.rs1 = parseRegister(regPart);
        }
        else {
            inst.offset = 0;
            inst.rs1 = parseRegister(operands[1]);
        }
    }
    else if (opcode == "BEQ") {
        inst.opcode = OP_BEQ;
        inst.rs1 = parseRegister(operands[0]);
        inst.rs2 = parseRegister(operands[1]);
        inst.label = operands[2];  // Store label, resolve later
    }
    else if (opcode == "BNE") {
        inst.opcode = OP_BNE;
        inst.rs1 = parseRegister(operands[0]);
        inst.rs2 = parseRegister(operands[1]);
        inst.label = operands[2];
    }
    else if (opcode == "BGT") {
        inst.opcode = OP_BGT;
        inst.rs1 = parseRegister(operands[0]);
        inst.rs2 = parseRegister(operands[1]);
        inst.label = operands[2];
    }
    else if (opcode == "BLT") {
        inst.opcode = OP_BLT;
        inst.rs1 = parseRegister(operands[0]);
        inst.rs2 = parseRegister(operands[1]);
        inst.label = operands[2];
    }
    else if (opcode == "J") {
        inst.opcode = OP_J;
        inst.label = operands[0];
    }
    else if (opcode == "MOV") {
        inst.opcode = OP_MOV;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
    }
    else if (opcode == "LI") {
        inst.opcode = OP_LI;
        inst.rd = parseRegister(operands[0]);
        inst.immediate = std::stoi(operands[1]);
    }
    else if (opcode == "ADD") {
        inst.opcode = OP_ADD;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.rs2 = parseRegister(operands[2]);
    }
    else if (opcode == "SUB") {
        inst.opcode = OP_SUB;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.rs2 = parseRegister(operands[2]);
    }
    else if (opcode == "MUL") {
        inst.opcode = OP_MUL;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.rs2 = parseRegister(operands[2]);
    }
    else if (opcode == "DIV") {
        inst.opcode = OP_DIV;
        inst.rd = parseRegister(operands[0]);
        inst.rs1 = parseRegister(operands[1]);
        inst.rs2 = parseRegister(operands[2]);
    }
    else if (opcode == "HALT") {
        inst.opcode = OP_HALT;
    }
    else {
        std::cerr << "Unknown instruction: " << opcode << std::endl;
        return;
    }

    decodedInstructions.push_back(inst);
    currentInstructionIndex++;
}

void AssemblyParser::resolveLabels() {
    std::cout << "\nResolving branch/jump labels..." << std::endl;

    for (size_t i = 0; i < decodedInstructions.size(); i++) {
        Instruction& inst = decodedInstructions[i];

        if (!inst.label.empty()) {
            if (symbolTable.find(inst.label) != symbolTable.end()) {
                inst.target = symbolTable[inst.label];
                std::cout << "  Instruction " << i << ": " << inst.label
                    << " -> " << inst.target << std::endl;
            }
            else {
                std::cerr << "Error: Undefined label '" << inst.label
                    << "' at instruction " << i << std::endl;
            }
        }

        // Encode instruction
        uint32_t encoded = encodeInstruction(inst);
        instructionMemory.push_back(encoded);
    }
}

void AssemblyParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    bool inDataSection = false;
    bool inTextSection = false;

    std::cout << "\n=== Parsing Assembly File ===" << std::endl;

    // First pass: parse everything
    while (std::getline(file, line)) {
        line = trim(removeComment(line));
        if (line.empty()) continue;

        if (line == ".data") {
            std::cout << "\n[DATA SECTION]" << std::endl;
            inDataSection = true;
            inTextSection = false;
            continue;
        }
        else if (line == ".text") {
            std::cout << "\n[TEXT SECTION]" << std::endl;
            inDataSection = false;
            inTextSection = true;
            continue;
        }

        if (inDataSection) {
            parseDataLine(line);
        }
        else if (inTextSection) {
            parseInstructionLine(line);
        }
    }

    // Second pass: resolve labels and encode
    resolveLabels();

    std::cout << "\n=== Parsing Complete ===" << std::endl;
    std::cout << "Symbol Table (Code Labels):" << std::endl;
    for (const auto& symbol : symbolTable) {
        std::cout << "  " << symbol.first << " -> Instruction " << symbol.second << std::endl;
    }
    std::cout << "Data Labels:" << std::endl;
    for (const auto& label : dataLabels) {
        std::cout << "  " << label.first << " -> Address " << label.second << std::endl;
    }
    std::cout << "Instructions loaded: " << instructionMemory.size() << std::endl;
}