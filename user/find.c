#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// find
// Write a simple version of the UNIX find program: find all the files in a directory tree whose name matches a string. Your solution should be in the file user/find.c.

// Some hints:

// Look at user/ls.c to see how to read directories.
// Use recursion to allow find to descend into sub-directories.
// Don't recurse into "." and "..".
// Changes to the file system persist across runs of qemu; to get a clean file system run make clean and then make qemu.
// You'll need to use C strings. Have a look at K&R (the C book), for example Section 5.5.
// Optional: support regular expressions in name matching. grep.c has some primitive support for regular expressions.

// Your solution is correct if produces the following output (when the file system contains a file a/b):

//     $ make qemu
//     ...
//     init: starting sh
//     $ mkdir a
//     $ echo > a/b
//     $ find . b
//     ./a/b
//     $ 

// grade
// mkdir Hn7sgfjM
// echo > Hn7sgfjM/TqZrZ6Mb
// mkdir Hn7sgfjM/ZQ7ckVcb
// echo > Hn7sgfjM/ZQ7ckVcb/TqZrZ6Mb
// mkdir MooBeLkP
// echo > MooBeLkP/TqZrZ6Mb
// find . TqZrZ6Mb

void
find(char *folder, char *pattern) {
    // char* targets[100];
    struct stat st;
    struct dirent de;
    char buf[100];
    char* p;
    int fd;
    // char** t = targets;
    // 1. find files and dirs in folder
    // TODO: what does 0 mean?

    // buf = "$folder/"
    printf("find(%s, %s)\n", folder, pattern);
    if((fd = open(folder, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", folder);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", folder);
        close(fd);
        return;
    }

    strcpy(buf, folder);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if(de.inum == 0)
            continue;
        strcpy(p, de.name);
        // might overflow here!
        
        // buf = "$folder/$element\0\0\0\0...."
        if (strcmp(de.name, ".") == 0||  strcmp(de.name, "..") == 0) {
            continue;
        }
        printf("visit %s %s\n", de.name, buf);
        if (stat(buf, &st) < 0) {
            fprintf(2, "error stat file");
            continue;
        }
        if (st.type == 1) {
            // folder
            // 3. call find on all those dirs, and -append results to ret array- print.
            find(buf, pattern);
        } else if (st.type == 2) {
            // file
            // 2. filter files by name & -added to ret array- print it.
            if (strcmp(pattern, de.name) == 0) {
                // int i = malloc(len())
                // *p++ = buf
                printf(buf);
                printf("\n");
            }
        }
    }
}

int
main(int argc, char *argv[])
{
    if(argc == 1) {
        fprintf(2, "usage: find [path] pattern\n");
        exit();
    } else if (argc == 2){
        find(".", argv[1]);
        exit();
    } else if (argc == 3) {
        find(argv[1], argv[2]);
        exit();
    } else if (argc > 3) {
        fprintf(2, "too much args\n");
        exit();
    }
    exit();
}
