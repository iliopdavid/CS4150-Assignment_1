#include <sys/ptrace.h>  
#include <sys/wait.h> 
#include <sys/user.h> 
#include <iostream>
#include <fstream>
#include <set> 
#include <unistd.h>   
#include <string>     

// Function to save system calls to a file
void saveToFile(const std::set<long>& syscalls, const std::string& filename) {
    std::ofstream file(filename); // Open the file in write mode
    if (file.is_open()) {
        for (const auto& syscall : syscalls) {
            file << syscall << std::endl; // Write each system call number to the file
        }
        file.close(); // Close the file
        std::cout << "System calls saved to " << filename << std::endl;
    } else {
        std::cerr << "Error opening file: " << filename << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Check if the user has provided the program to trace
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program-to-trace>" << std::endl;
        return 1;
    }

    pid_t child = fork(); // Create a new process
    if (child == 0) {
        // Child process: Prepare for tracing
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr); // Allow parent to trace this process
        execlp(argv[1], argv[1], nullptr); // Replace child process with the target program
    } else {
        // Parent process: Trace the child process
        std::set<long> syscallSet; // To store unique system call numbers
        int status; // To store process status information
        while (true) {
            waitpid(child, &status, 0); // Wait for the child process to stop or exit
            if (WIFEXITED(status)) break; // Check if the child process has exited

            // Allow the child to continue and wait for the next system call
            ptrace(PTRACE_SYSCALL, child, nullptr, nullptr);
            waitpid(child, &status, 0); // Wait for the child to reach the next stop

            if (WIFEXITED(status)) break;

            // Retrieve the system call number made by the child process
            struct user_regs_struct regs; // Structure to store register data
            ptrace(PTRACE_GETREGS, child, nullptr, &regs); // Get the register values of the child process
            syscallSet.insert(regs.orig_rax);

            // Allow the child to continue after retrieving the system call
            ptrace(PTRACE_SYSCALL, child, nullptr, nullptr);
        }

        // Save all unique system calls to a file
        saveToFile(syscallSet, "syscalls.txt");
        std::cout << "Tracing complete. System calls saved." << std::endl;
    }

    return 0;
}