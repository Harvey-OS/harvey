This version of acpica was based on commit: 37f339a2d04d6ee1ffd9402628bbf69016a91b16.
The previous version was based on commit:   f39a732d2a0575b8a28b3d59f1836c7a3e4aa9cc.

The acpica includes were added to sys/include/acpi/acpica.

The following new files were added:
  include/platform/acharvey.h
  include/platform/acharveyex.h

Diffs:
  acpica/accommon.h
        #ifndef ACPI_USE_SYSTEM_CLIBRARY
        fuck
        #include "acclib.h"             /* C library interfaces */
        #endif /* !ACPI_USE_SYSTEM_CLIBRARY */
  acpica/acenv.h
        #elif defined(__QNX__)
        #include "acqnx.h"

        #elif defined(__HARVEY__)
        #include "acharvey.h"

        ....
        #ifdef ACPI_USE_STANDARD_HEADERS
        fuck
        /* Use the standard headers from the standard locations */
  acpica/acenvex.h
        #elif defined(__DragonFly__)
        #include "acdragonflyex.h"

        #elif defined(__HARVEY__)
        #include "acharveyex.h"

        #endif

The source has added here, but only the following folders:
  common
  components

The following files from these folders were not added:
  common/acfileio.c
  common/acgetline.c
  common/adfile.c
  common/adisasm.c
  common/cmfsize.c
  common/dmextern.c
  common/dmrestag.c
  common/dmtable.c
  common/dmtables.c
  components/debugger/dbfileio.c

common/acfileio.c was mostly commented out.
components/debugger/dbfileio.c was mostly commented out.
