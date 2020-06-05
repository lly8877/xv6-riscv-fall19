#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAXCOUNT 35

void
readFromParent(int* fds) {
    uint32 buf[MAXCOUNT];
    uint32* ptr = buf;
    uint32 mod = 0;
    while(read(fds[0], ptr, sizeof(uint32)) == sizeof(uint32)) {
        if (*ptr == 0) {
            if (mod == 0) {
                exit();
            }
            break;
        } else if (mod == 0) {
            mod = *ptr;
            printf("prime %d\n", mod);
        } else if ((*ptr)%mod != 0) {
            ptr++;
        }
    }
    if (fork()) {
        for (ptr=buf;(*ptr)!=0;ptr++) {
            write(fds[1], ptr, sizeof(uint32));
        }
        write(fds[1], ptr, sizeof(uint32));
        wait();
        exit();
    } else {
        readFromParent(fds);
    }
}


int
main(int argc, char *argv[])
{
    int fds[2];
    uint32 buf;

    if(pipe(fds) != 0) {
      printf("pipe() failed\n");
      exit();
    }
    
    if (fork() != 0) {    
        for (buf = 2; buf <= 35; buf++) {
            write(fds[1], &buf, sizeof(uint32));
        }
        buf = 0;
        write(fds[1], &buf, sizeof(uint32));
        wait();
        exit();
    } else {
        readFromParent(fds);
    }
    return 0;
}
