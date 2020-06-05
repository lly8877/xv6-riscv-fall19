#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    
    int fds1[2];
    int fds2[2];
    char buf[100];

    pipe(fds1);
    pipe(fds2);

    int pid = fork();
    if (pid == 0) {
        // child
        write(fds1[1], "ping\n", 5);
        read(fds2[0], buf, 12);
        fprintf(1, "%d: received pong\n", getpid());
        exit();
    } else {
        // parent
        read(fds1[0], buf, 5);
        fprintf(1, "%d: received ping\n", getpid());
        write(fds2[1], "pong\n", 12);
        wait();
        exit();
    }
}