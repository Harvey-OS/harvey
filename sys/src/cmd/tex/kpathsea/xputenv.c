#include <kpathsea/config.h>

void
xputenv P2C(const_string, var_name,  const_string, value)
{
  string new_item = concat3 (var_name, "=", value);
  unsigned name_len = strlen (var_name);
  if (putenv (new_item) < 0)
        FATAL1 ("putenv (%s) failed", new_item);

   free (new_item);
}

void
xputenv_int P2C(const_string, var_name,  int, num)
{
  char str[MAX_INT_LENGTH];
  sprintf (str, "%d", num);
  
  xputenv (var_name, str);
}
