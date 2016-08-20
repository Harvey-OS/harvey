/* acpi.h */
/* File for Harvey-specific ACPI defines. */

/* ACPI redefines things found in libc, but this one thing we need.
 */

#if 0
typedef
struct Lock {
	int32_t	key;
	int32_t	sem;
} Lock;

typedef struct QLp QLp;
struct QLp
{
	int	inuse;
	QLp	*next;
	char	state;
};

typedef
struct QLock
{
	Lock	lock;
	int	locked;
	QLp	*head;
	QLp 	*tail;
} QLock;

#endif
#define ACPI_USE_SYSTEM_INTTYPES
typedef uint8_t BOOLEAN;
typedef QLock ACPI_MUTEX;
typedef uint64_t COMPILER_DEPENDENT_UINT64;
typedef uint64_t UINT64;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;
typedef int64_t COMPILER_DEPENDENT_INT64;
typedef int64_t INT64;
typedef int32_t INT32;
typedef int16_t INT16;
typedef int8_t INT8;
typedef int ACPI_THREAD_ID;

#define ACPI_MACHINE_WIDTH 64
#pragma clang diagnostic ignored "-Wunused-variable"

