#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAXTOKEN 100
#define MAXTOKENLEN 20
#define MAXARGS 10

// #define EXECCMDSIZE 20
#define CMDPOOLSIZE 20
#define REDIRCMDSIZE 20
#define PIPECOMSIZE 20
#define STRINGPOLLSIZE 100


typedef struct cmd Cmd;
typedef struct execcmd ExecCmd;
typedef struct redircmd RedirCmd;
typedef struct pipecmd PipeCmd;


struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  int argc;
};

struct redircmd {
  int type;
  Cmd *cmd;
  char *file;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  Cmd *left;
  Cmd *right;
};




#define EXEC 1
#define REDIR 2
#define PIPE 3


ExecCmd exec_arr[CMDPOOLSIZE];
RedirCmd redir_arr[REDIRCMDSIZE];
PipeCmd pipe_arr[PIPECOMSIZE];
char strings[STRINGPOLLSIZE][MAXTOKENLEN];

int exec_used = 0;
int redir_used = 0;
int pipe_used = 0;
char whitespace[] = " \t\r\n\v";
char specialchr[] = "|<>";

int
fork1(void)
{
  int pid;
  pid = fork();
  if(pid == -1){
    fprintf(2, "fork\n");
    exit(-1);
  }
  return pid;
}



Cmd*
parsecmd(char strings[][MAXTOKENLEN])
{
    Cmd* cmd = 0;
    ExecCmd* ecmd;
    PipeCmd* pcmd;
    RedirCmd* rcmd;

    for(int i = 0; strings[i][0] != 0; i++) {
        char* op = strings[i];
        int in;
        if ((in = (strcmp(op, "<") == 0)) || (strcmp(op, ">") == 0)) {
            if (strings[i+1] == 0) {
                fprintf(2, "no redirect file\n");
                exit(-1);
            }
            if (cmd == 0) {
                fprintf(2, "no redirect cmd\n");
                exit(-1);
            }
            rcmd = &(redir_arr[redir_used++]);
            rcmd->type = REDIR;
            rcmd->file = strings[i+1];
            rcmd->cmd = cmd;
            rcmd->mode = in ?  O_RDONLY : O_WRONLY|O_CREATE;
            rcmd->fd = in ? 0 : 1;
            cmd = (Cmd*)rcmd;
            i += 1;
        } else if (strcmp(op, "|") == 0) {
            pcmd = &(pipe_arr[pipe_used++]);
            if (cmd == 0) {
                fprintf(2, "no pipe left cmd\n");
                exit(-1);
            }
            pcmd->type = PIPE;
            pcmd->left = cmd;
            pcmd->right = parsecmd(&strings[i+1]);
            cmd = (Cmd*)pcmd;
            break;
        } else {
            // word
            if (cmd == 0) {
                ecmd = &exec_arr[exec_used++];
                ecmd->type = EXEC;
                if (ecmd->type != EXEC) {
                    fprintf(2, "assign failed\n");
                }
                ecmd->argc = 0;
            } else {
                ecmd = (ExecCmd*)cmd;
                if (cmd->type != EXEC) {
                    fprintf(2, "type not exec\n");
                    exit(-1);
                }
            }
            ecmd->argv[ecmd->argc] = op;
            ecmd->argc += 1;
            cmd = (Cmd*)ecmd;
        }
    }
    return cmd;
}




// in: source : "   abc.t   sd"
// out: buf: "abc.t\0", ret: "   sd"

// in: "  sd"
// out: buf: "sd\0", ret: ""

// in: ""
// out: 0
char* 
gettoken(char* source, char* buf) /* return next token */
{
    char* p;
    char* e;
    int count;
    if (strlen(source) == 0) {
        buf[0] = 0;
        return 0;
    } else {
        for (p=source; p<source+strlen(source);p++) {
            if (!strchr(whitespace, *p)) {
                break;
            }
        }
        e = p;
        for (count = 0; e<source+strlen(source);e++) {
            if (strchr(whitespace, *e)) {
                break;
            } else if (strchr(specialchr, *e)) {
                if (count == 0) {
                    e++;
                    count++;
                }
                break;
            } else {
                count++;
            }
        }
        if (count + 1 > MAXTOKENLEN) {
            fprintf(2, "MAXTOKENLEN exceed");
            exit(-1);
        }
        memmove(buf, p, count);
        buf[count] = 0;
        return e;
    }
}

// in "a < b | c x | d > p.t" 
// out ["a", "<", "b", "|", "c", "x", "|", "d", ">", "p.t", NULL]
void
tokenize(char *s, char strings[][MAXTOKENLEN])
{
    char buf[MAXTOKENLEN];
    char *n = s;
    for(int i=0;;i++){
        if (i > MAXTOKENLEN) {
            fprintf(2, "MAXTOKENLEN overflow");
            exit(-1);
        }
        n = gettoken(n,buf);
        if (n) {
            strcpy(strings[i], buf);
        } else {
            strings[i][0] = 0;
            break;
        }
    }
}

// borrow from sh.c
int
getcmd(char *buf, int nbuf)
{
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

void
runcmd(Cmd *cmd)
{
  int p[2];
  ExecCmd *ecmd;
  PipeCmd *pcmd;
  RedirCmd *rcmd;

  if(cmd == 0)
    exit(-1);

  switch(cmd->type){
  default:
    fprintf(2, "runcmd\n");
    exit(-1);

  case EXEC:
    ecmd = (ExecCmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(-1);
    exec(ecmd->argv[0], ecmd->argv);
    fprintf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (RedirCmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      fprintf(2, "open %s failed\n", rcmd->file);
      exit(-1);
    }
    runcmd(rcmd->cmd);
    break;

  case PIPE:
    pcmd = (PipeCmd*)cmd;
    if(pipe(p) < 0) {
        fprintf(2, "create pipe error");
        exit(-1);
    }
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    break;
  }
  exit(0);
}

// test
// get token
void
t0() {
    char buf[20];
    char source1[] = "   abc.t   sd";
    char out1[] = "abc.t";
    char out2[] = "sd";
    char out3[] = "";
    char *n;
    if ((n = gettoken(source1, buf)) > 0) {
        if(strcmp(buf, out1)==0) {
            printf("t0-0 PASS\n");
        }
    }

    if ((n = gettoken(n, buf)) > 0) {
        if(strcmp(buf, out2)==0) {
            printf("t0-1 PASS\n");
        }
    }
    
    if ((n = gettoken(n, buf)) == 0) {
        if(strcmp(buf, out3)==0) {
            printf("t0-2 PASS\n");
        }
    }
}

// test
// tokenize
// in "a < b | c x | d > p.t" 
// out ["a", "<", "b", "|", "c", "x", "|", "d", ">", "p.t", NULL]
void
t1() {
    char* in = " a  <  b| c  x| d> p.t\n";
    char *res[11] = { "a", "<", "b", "|", "c", "x", "|", "d", ">", "p.t", "\0"};
    tokenize(in, strings);
    for (int i = 0; i < 11; i++ ) {
        if (strcmp(strings[i], res[i]) == 0) {
            printf("t1-%d PASS\n", i);
        }
    }
}

void
dumparr() {
    ExecCmd* ecmd;
    PipeCmd* pcmd;
    RedirCmd* rcmd;
    for(int i = 0; i < exec_used; i++) {
        ecmd = &exec_arr[i];
        printf("ecmd addr %d, type %d, argv[0] %s, argc %d\n", &exec_arr[i], ecmd->type, ecmd->argv[0], ecmd->argc);
    }
    for(int i = 0; i < pipe_used; i++) {
        pcmd = &pipe_arr[i];
        printf("pcmd addr %d, type %d, left %d, right %d\n", &pipe_arr[i], pcmd->type, pcmd->left, pcmd->right);
    }
    for(int i = 0; i < redir_used; i++) {
        rcmd = &redir_arr[i];
        printf("rcmd addr %d, type %d, f %s, cmd %d\n", &redir_arr[i], rcmd->type, rcmd->file, rcmd->cmd);
    }
}
//  a < b | c x | d > p.t
//  P(R(E(a), b), arg1)
//  arg1 = P(E(c,x), arg2)
//  arg2 = R(E(d),p.t)
void
t2() {
    int last_success = 0;
    char *res[11] = { "a", "<", "b", "|", "c", "x", "|", "d", ">", "p.t"};
    for (int i = 0; i < 10; i++ ) {
        strcpy(strings[i],res[i]);
    }
    strings[10][0] = 0;
    Cmd* cmd = parsecmd(strings);


    dumparr();
    RedirCmd* rcmd;
    PipeCmd* pcmd;
    ExecCmd* ecmd;
    
    // P(R(E(a), b), arg1)
    if (cmd->type != PIPE) goto error;
    pcmd = (PipeCmd*)cmd;
    cmd = pcmd->left;
    last_success = 1;

    // R(E(a), b)
    if (cmd->type != REDIR) goto error;
    rcmd = (RedirCmd*)cmd;
    if (rcmd->fd != 0) goto error;
    if (strcmp(rcmd->file, "b") != 0) goto error;
    if (rcmd->mode != O_RDONLY) goto error;  // O_WRONLY|O_CREATE
    cmd = rcmd->cmd;
    last_success = 2;

    
    // E(a)
    if (cmd->type != EXEC) goto error;
    ecmd = (ExecCmd*)cmd;
    if (ecmd->argc != 1) goto error;
    if (strcmp(ecmd->argv[0], "a") != 0) goto error;
    if (ecmd->argv[1] != 0) goto error;
    last_success = 3;

    
    // P(E(c,x), arg2)
    cmd = pcmd->right;
    if (cmd->type != PIPE) goto error;
    pcmd = (PipeCmd*)cmd;
    printf("pcmd addr %d, type %d, left %d, right %d\n", &pipe_arr[0], pcmd->type, pcmd->left, pcmd->right);
    cmd = pcmd->left;
    last_success = 4;

    // E(c, x)
    if (cmd->type != EXEC) goto error;
    ecmd = (ExecCmd*)cmd;
    if (ecmd->argc != 2) goto error;
    if (strcmp(ecmd->argv[0], "c") != 0) goto error;
    if (strcmp(ecmd->argv[1], "x") != 0) goto error;
    if (ecmd->argv[2] != 0) goto error;
    last_success = 5;

    // arg2 = R(E(d),p.t)
    cmd = pcmd->right;
    if (cmd->type != REDIR) goto error;
    rcmd = (RedirCmd*)cmd;
    if (rcmd->fd != 1) goto error;
    if (strcmp(rcmd->file, "p.t") != 0) goto error;
    if (rcmd->mode != (O_WRONLY|O_CREATE)) goto error;
    cmd = rcmd->cmd;
    last_success = 6;

    // E(d)
    if (cmd->type != EXEC) goto error;
    ecmd = (ExecCmd*)cmd;
    if (ecmd->argc != 1) goto error;
    if (strcmp(ecmd->argv[0], "d") != 0) goto error;
    if (ecmd->argv[1] != 0) goto error;
    last_success = 7;


    printf("t2 PASS\n");
    return;
error:
    printf("t2 FAILED, last success: %d\n", last_success);
}

// borrow from sh.c
int
main(void)
{
  static char buf[100];
  int fd;
  int test = 0;
  if (test) {
      t2();
      exit(0);
  }

  // Ensure that three file descriptors are open.
  while((fd = open("console", O_RDWR)) >= 0){
    if(fd >= 3){
      close(fd);
      break;
    }
  }

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    if(fork1() == 0){
        tokenize(buf, strings);
        runcmd(parsecmd(strings));
    }
    wait(0);
  }
  exit(0);
}
