typedef ushort	CHAR16;

typedef uchar	UINT8;
typedef ushort	UINT16;
typedef ulong	UINT32;
typedef uvlong	UINT64;

typedef uintptr	UINTN;

typedef void*	EFI_HANDLE;
typedef UINT32	EFI_STATUS;

enum {
	AllHandles,
	ByRegisterNotify,
	ByProtocol,
};

typedef struct {
	UINT32		Data1;
	UINT16		Data2;
	UINT16		Data3;
	UINT8		Data4[8];
} EFI_GUID;

typedef struct {
	UINT16		ScanCode;
	CHAR16		UnicodeChar;
} EFI_INPUT_KEY;

typedef struct {
	void		*Reset;
	void		*ReadKeyStroke;
	void		*WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
	void		*Reset;
	void		*OutputString;
	void		*TestString;
	void		*QueryMode;
	void		*SetMode;
	void		*SetAttribute;
	void		*ClearScreen;
	void		*SetCursorPosition;
	void		*EnableCursor;
	void		*Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
	UINT32		RedMask;
	UINT32		GreenMask;
	UINT32		BlueMask;
	UINT32		ReservedMask;
} EFI_PIXEL_BITMASK;

enum {
	PixelRedGreenBlueReserved8BitPerColor,
	PixelBlueGreenRedReserved8BitPerColor,
	PixelBitMask,
	PixelBltOnly,
	PixelFormatMax,
};

typedef struct {
	UINT32		Version;
	UINT32		HorizontalResolution;
	UINT32		VerticalResolution;
	UINT32		PixelFormat;
	EFI_PIXEL_BITMASK	PixelInformation;
	UINT32		PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
	UINT32		MaxMode;
	UINT32		Mode;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
	UINTN		SizeOfInfo;
	UINT64		FrameBufferBase;
	UINTN		FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
	void		*QueryMode;
	void		*SetMode;
	void		*Blt;
	EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID = {
	0x9042a9de, 0x23dc, 0x4a38,
	0x96, 0xfb, 0x7a, 0xde,
	0xd0, 0x80, 0x51, 0x6a,
};

typedef struct {
	UINT64		Revision;
	void		*Open;
	void		*Close;
	void		*Delete;
	void		*Read;
	void		*Write;
	void		*GetPosition;
	void		*SetPosition;
	void		*GetInfo;
	void		*SetInfo;
	void		*Flush;
	void		*OpenEx;
	void		*ReadEx;
	void		*WriteEx;
	void		*FlushEx;
} EFI_FILE_PROTOCOL;

typedef struct {
	UINT64		Revision;
	void		*OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
	0x0964e5b22, 0x6459, 0x11d2,
	0x8e, 0x39, 0x00, 0xa0,
	0xc9, 0x69, 0x72, 0x3b,
};

enum {
	EfiReservedMemoryType,
	EfiLoaderCode,
	EfiLoaderData,
	EfiBootServicesCode,
	EfiBootServicesData,
	EfiRuntimeServicesCode,
	EfiRuntimeServicesData,
	EfiConventionalMemory,
	EfiUnusableMemory,
	EfiACPIReclaimMemory,
	EfiACPIMemoryNVS,
	EfiMemoryMappedIO,
	EfiMemoryMappedIOPortSpace,
	EfiPalCode,
	EfiMaxMemoryType,
};

typedef struct {
	UINT32		Type;
	UINT32		Reserved;
	UINT64		PhysicalStart;
	UINT64		VirtualStart;
	UINT64		NumberOfPages;
	UINT64		Attribute;
} EFI_MEMORY_DESCRIPTOR;


typedef struct {
	UINT64	Signature;
	UINT32	Revision;
	UINT32	HeaderSize;
	UINT32	CRC32;
	UINT32	Reserved;
} EFI_TABLE_HEADER;

typedef struct {
	EFI_TABLE_HEADER;

	void		*RaiseTPL;
	void		*RestoreTPL;
	void		*AllocatePages;
	void		*FreePages;
	void		*GetMemoryMap;
	void		*AllocatePool;
	void		*FreePool;

	void		*CreateEvent;
	void		*SetTimer;
	void		*WaitForEvent;
	void		*SignalEvent;
	void		*CloseEvent;
	void		*CheckEvent;

	void		**InstallProtocolInterface;
	void		**ReinstallProtocolInterface;
	void		**UninstallProtocolInterface;

	void		*HandleProtocol;
	void		*Reserved;
	void		*RegisterProtocolNotify;

	void		*LocateHandle;
	void		*LocateDevicePath;
	void		*InstallConfigurationTable;

	void		*LoadImage;
	void		*StartImage;
	void		*Exit;
	void		*UnloadImage;
	void		*ExitBootServices;

	void		*GetNextMonotonicCount;
	void		*Stall;
	void		*SetWatchdogTimer;

	void		*ConnectController;
	void		*DisconnectController;

	void		*OpenProtocol;
	void		*CloseProtocol;

	void		*OpenProtocolInformation;
	void		*ProtocolsPerHandle;
	void		*LocateHandleBuffer;
	void		*LocateProtocol;

	void		*InstallMultipleProtocolInterfaces;
	void		*UninstallMultipleProtocolInterfaces;

	void		*CalculateCrc32;

	void		*CopyMem;
	void		*SetMem;
	void		*CreateEventEx;
} EFI_BOOT_SERVICES;

typedef struct {
	EFI_TABLE_HEADER;

	void		*GetTime;
	void		*SetTime;
	void		*GetWakeupTime;
	void		*SetWakeupTime;

	void		*SetVirtualAddressMap;
	void		*ConvertPointer;

	void		*GetVariable;
	void		*GetNextVariableName;
	void		*SetVariable;

	void		*GetNextHighMonotonicCount;
	void		*ResetSystem;

	void		*UpdateCapsule;
	void		*QueryCapsuleCapabilities;

	void		*QueryVariableInfo;
} EFI_RUNTIME_SERVICES;


EFI_GUID ACPI_20_TABLE_GUID = {
	0x8868e871, 0xe4f1, 0x11d3,
	0xbc, 0x22, 0x00, 0x80,
	0xc7, 0x3c, 0x88, 0x81,
};

EFI_GUID ACPI_10_TABLE_GUID = {
	0xeb9d2d30, 0x2d88, 0x11d3,
	0x9a, 0x16, 0x00, 0x90,
	0x27, 0x3f, 0xc1, 0x4d,
};

typedef struct {
	EFI_GUID	VendorGuid;
	void		*VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct {
	EFI_TABLE_HEADER;

	CHAR16		*FirmwareVendor;
	UINT32		FirmwareRevision;

	EFI_HANDLE	ConsoleInHandle;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL	*ConIn;

	EFI_HANDLE	ConsoleOutHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL	*ConOut;

	EFI_HANDLE	StandardErrorHandle;
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL	*StdErr;

	EFI_RUNTIME_SERVICES	*RuntimeServices;
	EFI_BOOT_SERVICES	*BootServices;

	UINTN			NumberOfTableEntries;
	EFI_CONFIGURATION_TABLE	*ConfigurationTable;
} EFI_SYSTEM_TABLE;
