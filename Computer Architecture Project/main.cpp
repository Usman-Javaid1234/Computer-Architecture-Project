#include "MatrixProcessor.h"

// Create example program files
void createExamplePrograms() {
    std::ifstream check1("program1.asm");
    if (!check1.good()) {
        std::ofstream prog1("program1.asm");
        prog1 << "# Program 1: Matrix arithmetic\n";
        prog1 << ".data\n";
        prog1 << "MATRIX_A: 0\n    1, 2\n    3, 4\n";
        prog1 << "MATRIX_B: 100\n    5, 6\n    7, 8\n";
        prog1 << "MATRIX_C: 200\n    2, 1\n    1, 2\n";
        prog1 << ".text\n";
        prog1 << "DECLAREM M0, 2, 2\nLOADM M0, MATRIX_A\n";
        prog1 << "DECLAREM M1, 2, 2\nLOADM M1, MATRIX_B\n";
        prog1 << "DECLAREM M2, 2, 2\nLOADM M2, MATRIX_C\n";
        prog1 << "ADDM M3, M0, M1\nSUBM M4, M3, M2\n";
        prog1 << "STOREM M4, 300\nDETERMINANT R1, M4\nHALT\n";
        prog1.close();
    }
    check1.close();

    std::ifstream check2("program2.asm");
    if (!check2.good()) {
        std::ofstream prog2("program2.asm");
        prog2 << "# Program 2: Loops\n";
        prog2 << ".data\n";
        prog2 << "N_VALUE: 900\n    5\n";
        prog2 << "ARRAY: 910\n    10, 3, 15, 2\n";
        prog2 << "THRESHOLD: 920\n    5\n";
        prog2 << ".text\n";
        prog2 << "LW R1, 900(R0)\nLI R2, 0\n";
        prog2 << "COUNTDOWN_LOOP:\n";
        prog2 << "ADD R2, R2, R1\nSUBI R1, R1, 1\n";
        prog2 << "BNE R1, R0, COUNTDOWN_LOOP\n";
        prog2 << "LW R3, 920(R0)\nLI R4, 910\nLI R5, 4\n";
        prog2 << "FILTER_LOOP:\n";
        prog2 << "LW R6, 0(R4)\nBLT R6, R3, SET_ZERO\n";
        prog2 << "J KEEP_VALUE\n";
        prog2 << "SET_ZERO:\nLI R6, 0\n";
        prog2 << "KEEP_VALUE:\n";
        prog2 << "SW R6, 0(R4)\nADDI R4, R4, 1\n";
        prog2 << "SUBI R5, R5, 1\nBNE R5, R0, FILTER_LOOP\n";
        prog2 << "SW R2, 930(R0)\nHALT\n";
        prog2.close();
    }
    check2.close();
}

int main(int argc, char* argv[]) {
    // Create example programs if they don't exist
    createExamplePrograms();

    std::cout << std::endl;

    std::string filename = "program2.asm";

    if (argc > 1) {
        filename = argv[1];
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Matrix Core Processor - PIPELINED Version" << std::endl;
    std::cout << "With Data & Control Hazard Handling" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Loading program: " << filename << std::endl;
    std::cout << std::endl;

    try {
        // Create pipelined simulator instance
        SingleCycleSimulator sim;

        // Load and parse assembly program
        sim.loadProgram(filename);

        // Execute the program with pipeline
        sim.run();

        // Display final processor state
        sim.printState();

        // Display memory contents
        std::cout << "\n========================================" << std::endl;
        std::cout << "MEMORY CONTENTS" << std::endl;
        std::cout << "========================================" << std::endl;

        sim.printMemoryRange(0, 10);
        sim.printMemoryRange(100, 120);
        sim.printMemoryRange(200, 210);
        sim.printMemoryRange(300, 310);
        sim.printMemoryRange(400, 420);
        sim.printMemoryRange(500, 520);
        sim.printMemoryRange(900, 935);
		sim.printMemoryRange(800, 820);

        // Display performance metrics
        std::cout << "\n========================================" << std::endl;
        std::cout << "Single cycle PERFORMANCE METRICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Total Cycles:       " << sim.getCycleCount() << std::endl;
        std::cout << "  Instructions:       " << sim.getInstructionCount() << std::endl;
        std::cout << "  CPI:                " << std::fixed << std::setprecision(2)
            << sim.getCPI() << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\n Single cycle simulation completed successfully!" << std::endl;
		PipelinedSimulator pipelinedSim;
        pipelinedSim.loadProgram(filename);
		pipelinedSim.run();
        // Display final processor state
        sim.printState();

        // Display memory contents
        std::cout << "\n========================================" << std::endl;
        std::cout << "MEMORY CONTENTS" << std::endl;
        std::cout << "========================================" << std::endl;

        pipelinedSim.printMemoryRange(0, 10);
        pipelinedSim.printMemoryRange(100, 120);
        pipelinedSim.printMemoryRange(200, 210);
        pipelinedSim.printMemoryRange(300, 310);
        pipelinedSim.printMemoryRange(400, 420);
        pipelinedSim.printMemoryRange(900, 935);
        pipelinedSim.printMemoryRange(800, 820);
		pipelinedSim.printMemoryRange(500, 520);

        // Display performance metrics
        std::cout << "\n========================================" << std::endl;
        std::cout << "Pipelined PERFORMANCE METRICS" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "  Total Cycles:       " << pipelinedSim.getCycleCount() << std::endl;
        std::cout << "  Instructions:       " << pipelinedSim.getInstructionCount() << std::endl;
        std::cout << "  CPI:                " << std::fixed << std::setprecision(2)
            << pipelinedSim.getCPI() << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\nPipelined simulation completed successfully!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\n========================================" << std::endl;
        std::cerr << "ERROR" << std::endl;
        std::cerr << "========================================" << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "========================================" << std::endl;
        return 1;
    }

    return 0;
}