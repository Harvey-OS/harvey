/* acpi.h */
/* File for Harvey-specific ACPI defines. */

/* ACPI redefines things found in libc, but this one thing we need.
 */

#if 0
typedef
struct Lock {
	i32	key;
	i32	sem;
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
typedef u8 BOOLEAN;
typedef QLock ACPI_MUTEX;
typedef u64 COMPILER_DEPENDENT_UINT64;
typedef u64 UINT64;
typedef u32 UINT32;
typedef u16 UINT16;
typedef u8 UINT8;
typedef i64 COMPILER_DEPENDENT_INT64;
typedef i64 INT64;
typedef i32 INT32;
typedef i16 INT16;
typedef i8 INT8;
typedef int ACPI_THREAD_ID;

#define DEBUGGER_THREADING DEBUGGER_SINGLE_THREADED
#define ACPI_MACHINE_WIDTH 64
#pragma clang diagnostic ignored "-Wunused-variable"

#define ACPI_GET_FUNCTION_NAME __func__
