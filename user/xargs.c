// xargs
// Write a simple version of the UNIX xargs program: read lines from standard input and run a command for each line, supplying the line as arguments to the command. Your solution should be in the file user/xargs.c.

// The following example illustrates xarg's behavior:
//     $ xargs echo bye
//     hello too
//     bye hello too
//     ctrl-d
//     $
  
// Note that the command here is "echo bye" and the additional arguments are "hello too", making the command "echo bye hello too", which outputs "bye hello too".
// Some hints:

// Use fork and exec system call to invoke the command on each line of input. Use wait in the parent to wait for the child to complete running the command.
// Read from stdin a character at the time until the newline character ('\n').
// kernel/param.h declares MAXARG, which may be useful if you need to declare an argv.
// Changes to the file system persist across runs of qemu; to get a clean file system run make clean and then make qemu.
// xargs, find, and grep combine well:

//   $ find . b | xargs grep hello
  
// will run "grep hello" on each file named b in the directories below ".".
// To test your solution for xargs, run the shell script xargstest.sh. Your solution is correct if it produces the following output:

//   $ make qemu
//   ...
//   init: starting sh
//   $ sh < xargstest.sh
//   $ $ $ $ $ $ hello
//   hello
//   hello
//   $ $   
  
// You may have to fix bugs in your find program. The output has many $ because the xv6 shell is primitive and doesn't realize it is processing commands from a file instead of from the console, and prints a $ for each command in the file.


#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"


int
main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("usage: find [path] pattern\n");
        exit();
    }

    // at least we got a command
    // now we read from stdin
    

    char new_argv[MAXARG][20];
    int argi = argc-1;
    int char_count = 0;
    char* p = new_argv[argi];
    char x;



    for (int i = 1; i < argc; i++) {
        strcpy(new_argv[i-1], argv[i]);
    }

    while (read(0, &x, 1) > 0) {
        // printf("read char: %c", x);
        if (x == '\n') {
            if (char_count == 0 && argi == argc-1) {
                // nop
                printf("empty line");
            } else {
                // printf("newline: %s %s", argv[1], new_argv[0]);
                if (char_count != 0) {
                    *p++ = 0;
                    ++argi;
                    char_count = 0;
                }
                // printf("new line: %s\n", new_argv[0]);
                // TODO: exec in child and wait
                // now we have new_argv with len(argi)
                // need to concat to argv[1:]
                if (fork() == 0) {
                    // child
                    // concat (argv[1:argc], new_argv[:argi])
                    char** p = malloc(argi*(sizeof(char*)+1));
                    char** k = p;
                    for (int i = 0; i < argi; i++) {
                        // printf("for: %s\n", new_argv[i]);
                        *p++ = new_argv[i];
                        // printf("for: %s\n", *(p-1));
                    }
                    *p = 0;

                    // printf("here\n");
                    // printf("exec: %s %s %s %s ", argv[1], k[0], k[1], k[2]);
                    
                    // char *argvv[MAXARG];
                    // argvv[0] = 0;
                    
                    // exec("ls", argvv); 
                    exec(argv[1], k);
                    printf("exec failed!\n");
                    exit();
                } else {
                    // printf("wait");
                    wait();
                    argi = argc-1;
                    p = new_argv[argi];
                    char_count = 0;
                }
            }

        } else if (x == ' ') {
            printf("new word: %s\n", new_argv[argi]);
            // save parameters in argv
            if (char_count != 0) {
                *p++ = 0;
                p = new_argv[++argi];
                char_count = 0;
            }
        } else {
            // normal char
            *p++ = x;
            char_count++;
        }
    }
    exit();
}



