# Mini Shell

A simple shell implementation in C that reads and executes commands from user input. This shell supports basic command execution, signal handling, and built-in commands like `cd` and `exit`.

## Features

- **Execute External Commands:** Run any system command (like `ls`, `pwd`, etc.).
- **Built-in Commands:** 
  - `cd` - Change the current directory.
  - `exit` - Exit the shell.
- **Signal Handling:** 
  - Handle `Ctrl+C` gracefully without exiting the shell.

## Getting Started

### Prerequisites

To compile and run this project, you'll need:

- GCC or any C compiler.
- A Unix-like environment (Linux, macOS, etc.).

### Building the Project

Clone the repository and compile the `minishell.c` file using GCC:

```sh
git clone https://github.com/iNoles/Mini-Shell.git
cd Mini-Shell
gcc -o minishell minishell.c
```

### Running the Shell
After compiling, run the shell with:
```sh
./minishell
```

## Handling Signals
- Press Ctrl+C to interrupt a running command or get back to the shell prompt.
- To exit the shell, use the exit command.

## Project Highlights
This project showcases your understanding of:
- System Programming: Managing low-level operations like input/output, memory allocation, and signal handling using C standard libraries.
- Process Management: Creating and managing child processes using fork(), executing commands with execvp(), and synchronizing with waitpid().
- Inter-process Communication: Handling signals, like SIGINT for Ctrl+C, to communicate between user and shell processes gracefully.

## Memory Management
This shell program dynamically allocates memory for user input and parsed arguments. All allocated memory is properly freed to prevent memory leaks.

## Contributions
Contributions, issues, and feature requests are welcome! Feel free to check out the issues page if you have any questions or suggestions.
1. **Fork** the project.
2. **Create** your feature branch: `git checkout -b feature/YourFeature`.
3. **Commit** your changes: `git commit -m 'Add YourFeature'`.
4. **Push** to the branch: `git push origin feature/YourFeature`.
5. **Open** a pull request.
