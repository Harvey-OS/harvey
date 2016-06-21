/* acpi.h */
/* File for Harvey-specific ACPI defines. */

/* ACPI redefines things found in libc, but this one thing we need.
 */

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

typedef QLock ACPI_MUTEX;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;
typedef int ACPI_THREAD_ID;

#define ACPI_MACHINE_WIDTH 64

