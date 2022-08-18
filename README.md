# fstt
Fixed Size Terminal Terminal

Spawn, detach, reattach pseudoterminals. Once created, the size of the pseudoterminal cannot be changed.
## Compilation
Simply compile and link all the .c files.
```
clang -O3 -c *.c
clang -o fstt *.o
```
Then move fstt into a directory in your PATH. This program is only tested and maintained for linux.
## Usage
Running the program creates a terminal.
By default, name of created pseudoterminal is the decimal expansion of the smallest non-negative integer that is not already the name of a terminal.

To attach a terminal, specify the name after the -a option.

To set the size, specify the rows and columns after the -s option.

To set the shell, specify the shell after the -e option, if the shell is not in PATH then absolute path is required.

To detach, press Ctrl+B, then either D or d, this is to be consistent with the popular tmux program.
Not all features will be consistent though.

To input a Ctrl+B to the program running on the pseudoterminal, press Ctrl+B again.
