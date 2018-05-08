#include <u.h>
#include <libc.h>
#include <tos.h>

void
main(int argc, char *argv[]){
  if (_tos == nil){
    exits("nil tos");
  }
  print("getpid() = <%d>\n", getpid());
  print("Tos<%p>{\n"
         "   cyclefreq <%u>\n"
         "   kcycles   <%lld>\n"
         "   pcycles   <%lld>\n"
         "   pid       <%d>\n"
         "   nixtype   <%d>\n"
         "   core      <%d>\n"
         "}\n",_tos, _tos->cyclefreq, _tos->kcycles, _tos->pcycles, _tos->pid, _tos->nixtype, _tos->core);
  exits(nil);
}
