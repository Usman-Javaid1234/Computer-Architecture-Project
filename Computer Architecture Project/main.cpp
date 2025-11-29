#include "MatrixProcessor.h"

int main(int argc, char* argv[]) {
    std::string filename = "program1.asm";

    if (argc > 1) {
        filename = argv[1];
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Matrix Core Processor Simulator" << std::endl;
    std::cout << "With Scalar & Control Flow Support" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        SingleCycleSimulator sim;
        sim.loadProgram(filename);
        sim.run();
        sim.printState();
        sim.printMemoryRange(0, 10);      // MATRIX_A area
        sim.printMemoryRange(100, 110);   // MATRIX_B area
        sim.printMemoryRange(200, 210);   // MATRIX_C area
        sim.printMemoryRange(300, 310);   // Results area
        sim.printMemoryRange(400, 420);   // DECLAREM allocations
        sim.printMemoryRange(910, 913); // Program 2 data (N_VALUE, ARRAY, THRESHOLD, sum)
        sim.printMemoryRange(929, 930);
        std::cout << "\n========================================" << std::endl;
        std::cout << "Performance Metrics:" << std::endl;
        std::cout << "  Total Cycles: " << sim.getCycleCount() << std::endl;
        std::cout << "  Instructions: " << sim.getInstructionCount() << std::endl;
        std::cout << "  CPI: " << std::fixed << std::setprecision(2) << sim.getCPI() << std::endl;
        std::cout << "========================================" << std::endl;

        std::cout << "\nTrace file: execution_trace.txt" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}