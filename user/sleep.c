#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
      fprintf(2, "got %d parameter(s), need exactly 1\n", argc - 1);
      exit();
  }

  if (sleep(atoi(argv[1])) < 0) {
      fprintf(2, "sleep error by syscall\n");
  }
  exit();
}
