#include "MatrixProcessor.h"
#include <cstring>

// ============================================
// Matrix Core Processor - Main Program
// MSPR-Based Simulator Driver
// ============================================

// Color codes for terminal output (optional)
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// Simulator mode enumeration
enum SimulatorMode {
    MODE_PIPELINED,
    MODE_SINGLE_CYCLE,
    MODE_BOTH
};

// Configuration structure
struct SimulatorConfig {
    std::string filename;
    SimulatorMode mode;
    bool verbose;
    bool showMSPRState;
    bool showMemory;
    bool comparePerformance;

    SimulatorConfig() :
        filename("program2.asm"),
        mode(MODE_BOTH),
        verbose(false),
        showMSPRState(true),
        showMemory(true),
        comparePerformance(false) {
    }
};

// Print welcome banner
void printBanner() {
    std::cout << BOLD << CYAN;
    std::cout << "       MATRIX CORE PROCESSOR SIMULATOR v3.0            " << std::endl;
    std::cout << "       With MSPR (Matrix Special Purpose Registers)    " << std::endl;
    std::cout << "                                                       " << std::endl;
    std::cout << std::endl;
}

// Print usage information
void printUsage(const char* progName) {
    std::cout << BOLD << "USAGE:" << RESET << std::endl;
    std::cout << "  " << progName << " [options] [program.asm]" << std::endl;
    std::cout << std::endl;
    std::cout << BOLD << "OPTIONS:" << RESET << std::endl;
    std::cout << "  -p, --pipelined    Run pipelined simulator (default)" << std::endl;
    std::cout << "  -s, --single       Run single-cycle simulator" << std::endl;
    std::cout << "  -b, --both         Run both and compare performance" << std::endl;
    std::cout << "  -v, --verbose      Enable verbose output" << std::endl;
    std::cout << "  -m, --no-memory    Don't display memory contents" << std::endl;
    std::cout << "  -r, --no-mspr      Don't display MSPR state" << std::endl;
    std::cout << "  -h, --help         Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << BOLD << "EXAMPLES:" << RESET << std::endl;
    std::cout << "  " << progName << " program1_mspr.asm" << std::endl;
    std::cout << "  " << progName << " -s program2_mspr.asm" << std::endl;
    std::cout << "  " << progName << " -b -v program3_mspr.asm" << std::endl;
    std::cout << std::endl;
}

// Parse command line arguments
SimulatorConfig parseArguments(int argc, char* argv[]) {
    SimulatorConfig config;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "-p" || arg == "--pipelined") {
            config.mode = MODE_PIPELINED;
        }
        else if (arg == "-s" || arg == "--single") {
            config.mode = MODE_SINGLE_CYCLE;
        }
        else if (arg == "-b" || arg == "--both") {
            config.mode = MODE_BOTH;
            config.comparePerformance = true;
        }
        else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "-m" || arg == "--no-memory") {
            config.showMemory = false;
        }
        else if (arg == "-r" || arg == "--no-mspr") {
            config.showMSPRState = false;
        }
        else if (arg[0] != '-') {
            config.filename = arg;
        }
        else {
            std::cerr << YELLOW << "Warning: Unknown option '" << arg << "'" << RESET << std::endl;
        }
    }

    return config;
}

// Print MSPR bank state (enhanced version)
void printMSPRBankState(const std::string& title) {
    std::cout << "\n" << BOLD << CYAN;
    std::cout << "  " << std::setw(50) << std::left << title << "  " << std::endl;
}

// Display memory in a structured format
void displayMemoryStructured(const std::string& label, int start, int end) {

    std::cout << "\n" << BOLD << label << RESET << " [" << start << "-" << end << "]:" << std::endl;
}

// Run single-cycle simulator
void runSingleCycle(const SimulatorConfig& config) {
    std::cout << BOLD << GREEN;
    std::cout << "         SINGLE-CYCLE SIMULATOR MODE                   " << std::endl;
    std::cout << "Program: " << CYAN << config.filename << RESET << std::endl;
    std::cout << std::endl;

    try {
        SingleCycleSimulator sim;

        // Load program
        if (config.verbose) {
            std::cout << YELLOW << " Loading and parsing program..." << RESET << std::endl;
        }
        sim.loadProgram(config.filename);

        // Execute
        if (config.verbose) {
            std::cout << YELLOW << " Executing program..." << RESET << std::endl;
        }
        sim.run();

        // Display results
        std::cout << "\n" << BOLD << GREEN << " Execution Complete!" << RESET << std::endl;

        // Display processor state
        sim.printState();

        // Display MSPR state if requested
        if (config.showMSPRState) {
            sim.printMatrixDescriptors();
        }

        // Display memory contents if requested
        if (config.showMemory) {
            std::cout << "\n" << BOLD << MAGENTA;
            std::cout << "             MEMORY CONTENTS                           " << std::endl;

            displayMemoryStructured("Input Data", 0, 10);
            sim.printMemoryRange(0, 10);

            displayMemoryStructured("Additional Data", 100, 120);
            sim.printMemoryRange(100, 120);

            displayMemoryStructured("Results", 200, 260);
            sim.printMemoryRange(200, 260);

            displayMemoryStructured("Intermediate", 300, 360);
            sim.printMemoryRange(300, 360);

            displayMemoryStructured("MSPR-Allocated Matrices", 400, 450);
            sim.printMemoryRange(400, 450);

            displayMemoryStructured("Working Memory", 500, 530);
            sim.printMemoryRange(500, 530);

            displayMemoryStructured("Statistics", 700, 760);
            sim.printMemoryRange(700, 760);
        }

        // Performance metrics
        std::cout << "\n" << BOLD << BLUE;
        std::cout << "         PERFORMANCE METRICS                           " << std::endl;
        std::cout << "  Total Cycles:          " << CYAN << sim.getCycleCount() << RESET << std::endl;
        std::cout << "  Instructions Executed: " << CYAN << sim.getInstructionCount() << RESET << std::endl;
        std::cout << "  CPI:                   " << CYAN << std::fixed << std::setprecision(3)
            << sim.getCPI() << RESET << std::endl;
        std::cout << std::endl;

        std::cout << YELLOW << " Detailed trace: execution_trace.txt" << RESET << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "\n" << BOLD << RED;
        std::cerr << "                  ERROR                               " << std::endl;
        std::cerr << RED << e.what() << RESET << std::endl;
        throw;
    }
}

// Run pipelined simulator
void runPipelined(const SimulatorConfig& config) {
    std::cout << BOLD << GREEN;
    std::cout << "        PIPELINED SIMULATOR MODE                     " << std::endl;
    std::cout << "      With Data & Control Hazard Handling               " << std::endl;
    std::cout << "Program: " << CYAN << config.filename << RESET << std::endl;
    std::cout << std::endl;

    try {
        PipelinedSimulator sim;

        // Load program
        if (config.verbose) {
            std::cout << YELLOW << " Loading and parsing program..." << RESET << std::endl;
        }
        sim.loadProgram(config.filename);

        // Execute
        if (config.verbose) {
            std::cout << YELLOW << " Executing program with pipeline..." << RESET << std::endl;
        }
        sim.run();

        // Display results
        std::cout << "\n" << BOLD << GREEN << "Execution Complete!" << RESET << std::endl;

        // Display processor state
        sim.printState();

        // Display memory contents if requested
        if (config.showMemory) {
            std::cout << "\n" << BOLD << MAGENTA;
            std::cout << "              MEMORY CONTENTS                           " << std::endl;

            displayMemoryStructured("Input Data", 0, 10);
            sim.printMemoryRange(0, 10);

            displayMemoryStructured("Additional Data", 100, 120);
            sim.printMemoryRange(100, 120);

            displayMemoryStructured("Results", 200, 220);
            sim.printMemoryRange(200, 220);

            displayMemoryStructured("Intermediate", 300, 360);
            sim.printMemoryRange(300, 360);

            displayMemoryStructured("MSPR-Allocated Matrices", 400, 450);
            sim.printMemoryRange(400, 450);

            displayMemoryStructured("Working Memory", 500, 530);
            sim.printMemoryRange(500, 530);

            displayMemoryStructured("Additional Results", 600, 630);
            sim.printMemoryRange(600, 630);

            displayMemoryStructured("Statistics", 700, 760);
            sim.printMemoryRange(700, 760);
        }

        // Performance metrics
        std::cout << "\n" << BOLD << BLUE;
        std::cout << "       PIPELINE PERFORMANCE METRICS                     " << std::endl;
        std::cout << "  Total Cycles:          " << CYAN << sim.getCycleCount() << RESET << std::endl;
        std::cout << "  Instructions Executed: " << CYAN << sim.getInstructionCount() << RESET << std::endl;
        std::cout << "  CPI:                   " << CYAN << std::fixed << std::setprecision(3)
            << sim.getCPI() << RESET << std::endl;
        std::cout << "  Pipeline Stalls:       " << YELLOW << sim.getStallCount() << RESET << std::endl;

        // Calculate efficiency
        double idealCPI = 1.0;
        double actualCPI = sim.getCPI();
        double efficiency = (idealCPI / actualCPI) * 100.0;

        std::cout << "  Pipeline Efficiency:   " << GREEN << std::fixed << std::setprecision(1)
            << efficiency << "%" << RESET << std::endl;
        std::cout << std::endl;

        std::cout << YELLOW << "📄 Detailed trace: pipeline_trace.txt" << RESET << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "\n" << BOLD << RED;
        std::cerr << "                    ERROR                               " << std::endl;
        std::cerr << RED << e.what() << RESET << std::endl;
        throw;
    }
}

// Run both simulators and compare
void runBoth(const SimulatorConfig& config) {
    std::cout << BOLD << MAGENTA;
    std::cout << "         COMPARATIVE ANALYSIS MODE                      " << std::endl;
    std::cout << "    Running Both Single-Cycle and Pipelined             " << std::endl;
    std::cout << std::endl;

    // Performance metrics
    int singleCycles = 0, pipelinedCycles = 0;
    int singleInstructions = 0, pipelinedInstructions = 0;
    double singleCPI = 0, pipelinedCPI = 0;
    int stalls = 0;

    // Run single-cycle
    try {
        std::cout << BOLD << "═══ SINGLE-CYCLE EXECUTION ═══" << RESET << std::endl;
        SingleCycleSimulator sim;
        sim.loadProgram(config.filename);
        sim.run();

        singleCycles = sim.getCycleCount();
        singleInstructions = sim.getInstructionCount();
        singleCPI = sim.getCPI();

        std::cout << GREEN << "Single-cycle completed" << RESET << std::endl;
        std::cout << "  Cycles: " << singleCycles << ", CPI: " << std::fixed
            << std::setprecision(3) << singleCPI << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << RED << " Single-cycle failed: " << e.what() << RESET << std::endl;
        return;
    }

    std::cout << std::endl;

    // Run pipelined
    try {
        std::cout << MAGENTA << "═══ PIPELINED EXECUTION ═══" << RESET << std::endl;
        PipelinedSimulator sim;
        sim.loadProgram(config.filename);
        sim.run();

        pipelinedCycles = sim.getCycleCount();
        pipelinedInstructions = sim.getInstructionCount();
        pipelinedCPI = sim.getCPI();
        stalls = sim.getStallCount();

        std::cout << GREEN << " Pipelined completed" << RESET << std::endl;
        std::cout << "  Cycles: " << pipelinedCycles << ", CPI: " << std::fixed
            << std::setprecision(3) << pipelinedCPI << std::endl;
        std::cout << "  Stalls: " << stalls << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << RED << " Pipelined failed: " << e.what() << RESET << std::endl;
        return;
    }

    // Comparison
    std::cout << "\n" << BOLD << BLUE;
    std::cout << "           PERFORMANCE COMPARISON                       " << std::endl;

    std::cout << std::left;
    std::cout << "  " << std::setw(30) << "Metric" << std::setw(15) << "Single-Cycle"
        << std::setw(15) << "Pipelined" << "Difference" << std::endl;
    std::cout << "  " << std::setw(30) << "Total Cycles:"
        << std::setw(15) << singleCycles
        << std::setw(15) << pipelinedCycles;

    int cycleDiff = pipelinedCycles - singleCycles;
    if (cycleDiff > 0) {
        std::cout << RED << "+" << cycleDiff << " cycles" << RESET;
    }
    else {
        std::cout << GREEN << cycleDiff << " cycles" << RESET;
    }
    std::cout << std::endl;

    std::cout << "  " << std::setw(30) << "Instructions:"
        << std::setw(15) << singleInstructions
        << std::setw(15) << pipelinedInstructions
        << (singleInstructions == pipelinedInstructions ? GREEN "Same" RESET : RED "Different" RESET)
        << std::endl;

    std::cout << "  " << std::setw(30) << "CPI:"
        << std::setw(15) << std::fixed << std::setprecision(3) << singleCPI
        << std::setw(15) << pipelinedCPI;

    double cpiIncrease = ((pipelinedCPI - singleCPI) / singleCPI) * 100.0;
    if (cpiIncrease > 0) {
        std::cout << RED << "+" << std::setprecision(1) << cpiIncrease << "%" << RESET;
    }
    else {
        std::cout << GREEN << std::setprecision(1) << cpiIncrease << "%" << RESET;
    }
    std::cout << std::endl;

    std::cout << "  " << std::setw(30) << "Pipeline Stalls:"
        << std::setw(15) << "N/A"
        << std::setw(15) << stalls
        << YELLOW << stalls << " stalls" << RESET << std::endl;

    // Calculate speedup (theoretical vs actual)
    double theoreticalSpeedup = (double)singleCycles / pipelinedInstructions; // If perfect pipeline
    double actualSpeedup = (double)singleCycles / pipelinedCycles;
    double efficiency = (actualSpeedup / theoreticalSpeedup) * 100.0;

    std::cout << std::endl;
    std::cout << "  " << BOLD << "Pipeline Analysis:" << RESET << std::endl;
    std::cout << "    Theoretical Best:  " << CYAN << std::fixed << std::setprecision(2)
        << theoreticalSpeedup << "× speedup" << RESET << std::endl;
    std::cout << "    Actual Speedup:    " << CYAN << actualSpeedup << "×" << RESET << std::endl;
    std::cout << "    Pipeline Efficiency: ";

    if (efficiency >= 80.0) {
        std::cout << GREEN;
    }
    else if (efficiency >= 60.0) {
        std::cout << YELLOW;
    }
    else {
        std::cout << RED;
    }
    std::cout << std::setprecision(1) << efficiency << "%" << RESET << std::endl;

    std::cout << std::endl;

    // Conclusion
    if (pipelinedCycles < singleCycles) {
        std::cout << "  " << BOLD << GREEN << "Pipeline provides speedup!" << RESET << std::endl;
    }
    else {
        std::cout << "  " << BOLD << YELLOW << "Pipeline has overhead due to hazards" << RESET << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // Print banner
    printBanner();

    // Parse command line arguments
    SimulatorConfig config = parseArguments(argc, argv);

    // Display configuration if verbose
    if (config.verbose) {
        std::cout << BOLD << "Configuration:" << RESET << std::endl;
        std::cout << "  Program:      " << config.filename << std::endl;
        std::cout << "  Mode:         ";
        switch (config.mode) {
        case MODE_SINGLE_CYCLE: std::cout << "Single-Cycle"; break;
        case MODE_PIPELINED: std::cout << "Pipelined"; break;
        case MODE_BOTH: std::cout << "Comparative (Both)"; break;
        }
        std::cout << std::endl;
        std::cout << "  Show Memory:  " << (config.showMemory ? "Yes" : "No") << std::endl;
        std::cout << "  Show MSPR:    " << (config.showMSPRState ? "Yes" : "No") << std::endl;
        std::cout << std::endl;
    }

    // Run appropriate simulator
    try {
        switch (config.mode) {
        case MODE_SINGLE_CYCLE:
            runSingleCycle(config);
            break;

        case MODE_PIPELINED:
            runPipelined(config);
            break;

        case MODE_BOTH:
            runBoth(config);
            break;
        }

        // Success message
        std::cout << "\n" << BOLD << GREEN;
        std::cout << "         SIMULATION COMPLETED SUCCESSFULLY              " << std::endl;
        std::cout << std::endl;

        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "\n" << BOLD << RED;
        std::cerr << "              SIMULATION FAILED                         " << std::endl;
        std::cerr << RED << "Error: " << e.what() << RESET << std::endl;
        std::cerr << std::endl;
        return 1;
    }
}