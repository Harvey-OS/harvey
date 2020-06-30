#include <u.h>
#include <libc.h>
#include <tos.h>

void
main(int argc, char *argv[]){
  if (_tos == nil){
    exits("FAIL");
  }
  if(getpid() == _tos->prof.pid){
    exits(nil);
  }
  exits("FAIL");
}
