#include <u.h>
#include <libc.h>
#include <tos.h>

void
main(int argc, char *argv[]){
  if (_tos == nil){
    exits("FAIL");
  }
  if(getpid() == _tos->pid){
    exits("OK");
  }
  exits("FAIL");
}
