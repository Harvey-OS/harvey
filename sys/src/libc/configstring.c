Copyright (c) 2013-2016, The Regents of the University of California (Regents).
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Regents nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
#include "configstring.h"
#include "encoding.h"
#include "mtrap.h"
//#include "uart.h"
#include <stdio.h>

static void query_mem(const char* config_string)
{
  query_result res = query_config_string(config_string, "ram{0{addr");
  assert(res.start);
  uintptr_t base = get_uint(res);
  assert(base == DRAM_BASE);
  res = query_config_string(config_string, "ram{0{size");
  mem_size = get_uint(res);
}

static void query_rtc(const char* config_string)
{
  query_result res = query_config_string(config_string, "rtc{addr");
  assert(res.start);
  mtime = (void*)(uintptr_t)get_uint(res);
}

static void query_harts(const char* config_string)
{
  for (int core = 0, hart; ; core++) {
    for (hart = 0; ; hart++) {
      char buf[32];
      snprintf(buf, sizeof buf, "core{%d{%d{ipi", core, hart);
      query_result res = query_config_string(config_string, buf);
      if (!res.start)
        break;
      hls_t* hls = hls_init(num_harts);
      hls->ipi = (void*)(uintptr_t)get_uint(res);

      snprintf(buf, sizeof buf, "core{%d{%d{timecmp", core, hart);
      res = query_config_string(config_string, buf);
      assert(res.start);
      hls->timecmp = (void*)(uintptr_t)get_uint(res);

      num_harts++;
    }
    if (!hart)
      break;
  }
  assert(num_harts);
  assert(num_harts <= MAX_HARTS);
}

void parse_config_string()
{
  uint32_t addr = *(uint32_t*)CONFIG_STRING_ADDR;
  const char* s = (const char*)(uintptr_t)addr;
  //uart_send_string(s);
  query_mem(s);
  query_rtc(s);
  query_harts(s);
}
