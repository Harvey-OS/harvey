/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct SmbRawHeader {
	uchar protocol[4];
	uchar command;
	//	union {
	//		struct {
	//			uchar errorclass;
	//			uchar reserved;
	//			uchar error[2];
	//		} doserror;
	uchar status[4];
	//	};
	uchar flags;
	uchar flags2[2];
	//	union {
	uchar extra[12];
	//		struct {
	//			uchar pidhigh[2];
	//			uchar securitysignature[8];
	//		};
	//	};
	uchar tid[2];
	uchar pid[2];
	uchar uid[2];
	uchar mid[2];
	uchar wordcount;
	uchar parameterwords[1];
} SmbRawHeader;

enum { SmbHeaderFlagReserved = (3 << 1),
       SmbHeaderFlagCaseless = (1 << 3),
       SmbHeaderFlagServerIgnore = (1 << 4),
       SMB_FLAGS_SERVER_TO_REDIR = (1 << 7),
};

enum { SMB_FLAGS2_KNOWS_LONG_NAMES = (1 << 0),
       SMB_FLAGS2_KNOWS_EAS = (1 << 1),
       SMB_FLAGS2_SECURITY_SIGNATURE = (1 << 2),
       SMB_FLAGS2_RESERVED1 = (1 << 3),
       SMB_FLAGS2_IS_LONG_NAME = (1 << 6),
       SMB_FLAGS2_EXT_SEC = (1 << 1),
       SMB_FLAGS2_DFS = (1 << 12),
       SMB_FLAGS2_PAGING_IO = (1 << 13),
       SMB_FLAGS2_ERR_STATUS = (1 << 14),
       SMB_FLAGS2_UNICODE = (1 << 15),
};

enum { SMB_COM_CREATE_DIRECTORY = 0x00,
       SMB_COM_DELETE_DIRECTORY = 0x01,
       SMB_COM_OPEN = 0x02,
       SMB_COM_CREATE = 0x03,
       SMB_COM_CLOSE = 0x04,
       SMB_COM_FLUSH = 0x05,
       SMB_COM_DELETE = 0x06,
       SMB_COM_RENAME = 0x07,
       SMB_COM_QUERY_INFORMATION = 0x08,
       SMB_COM_SET_INFORMATION = 0x09,
       SMB_COM_READ = 0x0A,
       SMB_COM_WRITE = 0x0B,
       SMB_COM_LOCK_BYTE_RANGE = 0x0C,
       SMB_COM_UNLOCK_BYTE_RANGE = 0x0D,
       SMB_COM_CREATE_TEMPORARY = 0x0E,
       SMB_COM_CREATE_NEW = 0x0F,
       SMB_COM_CHECK_DIRECTORY = 0x10,
       SMB_COM_PROCESS_EXIT = 0x11,
       SMB_COM_SEEK = 0x12,
       SMB_COM_LOCK_AND_READ = 0x13,
       SMB_COM_WRITE_AND_UNLOCK = 0x14,
       SMB_COM_READ_RAW = 0x1A,
       SMB_COM_READ_MPX = 0x1B,
       SMB_COM_READ_MPX_SECONDARY = 0x1C,
       SMB_COM_WRITE_RAW = 0x1D,
       SMB_COM_WRITE_MPX = 0x1E,
       SMB_COM_WRITE_MPX_SECONDARY = 0x1F,
       SMB_COM_WRITE_COMPLETE = 0x20,
       SMB_COM_QUERY_SERVER = 0x21,
       SMB_COM_SET_INFORMATION2 = 0x22,
       SMB_COM_QUERY_INFORMATION2 = 0x23,
       SMB_COM_LOCKING_ANDX = 0x24,
       SMB_COM_TRANSACTION = 0x25,
       SMB_COM_TRANSACTION_SECONDARY = 0x26,
       SMB_COM_IOCTL = 0x27,
       SMB_COM_IOCTL_SECONDARY = 0x28,
       SMB_COM_COPY = 0x29,
       SMB_COM_MOVE = 0x2A,
       SMB_COM_ECHO = 0x2B,
       SMB_COM_WRITE_AND_CLOSE = 0x2C,
       SMB_COM_OPEN_ANDX = 0x2D,
       SMB_COM_READ_ANDX = 0x2E,
       SMB_COM_WRITE_ANDX = 0x2F,
       SMB_COM_NEW_FILE_SIZE = 0x30,
       SMB_COM_CLOSE_AND_TREE_DISC = 0x31,
       SMB_COM_TRANSACTION2 = 0x32,
       SMB_COM_TRANSACTION2_SECONDARY = 0x33,
       SMB_COM_FIND_CLOSE2 = 0x34,
       SMB_COM_FIND_NOTIFY_CLOSE = 0x35,
       /* Used by Xenix/Unix 0x60 - 0x6E */,
       SMB_COM_TREE_CONNECT = 0x70,
       SMB_COM_TREE_DISCONNECT = 0x71,
       SMB_COM_NEGOTIATE = 0x72,
       SMB_COM_SESSION_SETUP_ANDX = 0x73,
       SMB_COM_LOGOFF_ANDX = 0x74,
       SMB_COM_TREE_CONNECT_ANDX = 0x75,
       SMB_COM_QUERY_INFORMATION_DISK = 0x80,
       SMB_COM_SEARCH = 0x81,
       SMB_COM_FIND = 0x82,
       SMB_COM_FIND_UNIQUE = 0x83,
       SMB_COM_FIND_CLOSE = 0x84,
       SMB_COM_NT_TRANSACT = 0xA0,
       SMB_COM_NT_TRANSACT_SECONDARY = 0xA1,
       SMB_COM_NT_CREATE_ANDX = 0xA2,
       SMB_COM_NT_CANCEL = 0xA4,
       SMB_COM_NT_RENAME = 0xA5,
       SMB_COM_OPEN_PRINT_FILE = 0xC0,
       SMB_COM_WRITE_PRINT_FILE = 0xC1,
       SMB_COM_CLOSE_PRINT_FILE = 0xC2,
       SMB_COM_GET_PRINT_QUEUE = 0xC3,
       SMB_COM_READ_BULK = 0xD8,
       SMB_COM_WRITE_BULK = 0xD9,
       SMB_COM_NO_ANDX_COMMAND = 0xff,
};

enum { SUCCESS = 0,
       ERRDOS = 0x01,
       ERRSRV = 0x02,
       ERRHRD = 0x03,
       ERRCMD = 0xff,
};

enum { ERRbadfunc = 1,
       ERRbadfile = 2,
       ERRbadpath = 3,
       ERRnofids = 4,
       ERRnoaccess = 5,
       ERRbadfid = 6,
       ERRbadmcb = 7,
       ERRnomem = 8,
       ERRbadmem = 9,
       ERRbadenv = 10,
       ERRbadformat = 11,
       ERRbadaccess = 12,
       ERRbaddata = 13,
       ERRbaddrive = 15,
       ERRremcd = 16,
       ERRdiffdevice = 17,
       ERRnofiles = 18,
       ERRbadshare = 32,
       ERRlock = 33,
       ERRunsup = 50,
       ERRfilexists = 80,
       ERRunknownlevel = 124,
       ERRquota = 512,
       ERRnotalink = 513,
};

enum { ERRerror = 1,
       ERRbadpw = 2,
       ERRaccess = 4,
       ERRinvtid = 5,
       ERRsmbcmd = 64,
       ERRtoomanyuids = 90,
       ERRbaduid = 91,
       ERRnosupport = 65535,
};

enum { CAP_RAW_MODE = 0x0001,
       CAP_MPX_MODE = 0x0002,
       CAP_UNICODE = 0x0004,
       CAP_LARGE_FILES = 0x0008,
       CAP_NT_SMBS = 0x0010,
       CAP_RPC_REMOTE_APIS = 0x0020,
       CAP_STATUS32 = 0x0040,
       CAP_LEVEL_II_OPLOCKS = 0x0080,
       CAP_LOCK_AND_READ = 0x0100,
       CAP_NT_FIND = 0x0200,
       CAP_DFS = 0x1000,
       CAP_INFOLEVEL_PASSTHRU = 0x2000,
       CAP_W2K_SMBS = 0x2000,
       CAP_LARGE_READX = 0x4000,
       CAP_LARGE_WRITEX = 0x8000,
       CAP_UNIX = 0x00800000,
       CAP_BULK_TRANSFER = 0x20000000,
       CAP_COMPRESSED_DATA = 0x40000000,
       CAP_EXTENDED_SECURITY = 0x80000000 };

enum { RapNetShareEnum = 0,
       RapNetShareGetInfo = 1,
       RapNetServerGetInfo = 13,
       RapNetWkstaGetInfo = 63,
       RapNetServerEnum2 = 104,
};

enum { SMB_RAP_NERR_SUCCESS = 0,
       SMB_RAP_ERROR_MORE_DATA = 234,
};

enum { STYPE_DISKTREE, STYPE_PRINTQ, STYPE_DEVICE, STYPE_IPC };

enum { SV_TYPE_WORKSTATION = 0x00000001,
       SV_TYPE_SERVER = 0x00000002,
       SV_TYPE_SQLSERVER = 0x00000004,
       SV_TYPE_DOMAIN_CTRL = 0x00000008,
       SV_TYPE_DOMAIN_BAKCTRL = 0x00000010,
       SV_TYPE_TIME_SOURCE = 0x00000020,
       SV_TYPE_AFP = 0x00000040,
       SV_TYPE_NOVELL = 0x00000080,
       SV_TYPE_DOMAIN_MEMBER = 0x00000100,
       SV_TYPE_PRINTQ_SERVER = 0x00000200,
       SV_TYPE_DIALIN_SERVER = 0x00000400,
       SV_TYPE_SERVER_UNIX = 0x00000800,
       SV_TYPE_NT = 0x00001000,
       SV_TYPE_WFW = 0x00002000,
       SV_TYPE_SERVER_MFPN = 0x00004000,
       SV_TYPE_SERVER_NT = 0x00008000,
       SV_TYPE_POTENTIAL_BROWSER = 0x00010000,
       SV_TYPE_BACKUP_BROWSER = 0x00020000,
       SV_TYPE_MASTER_BROWSER = 0x00040000,
       SV_TYPE_DOMAIN_MASTER = 0x00080000,
       SV_TYPE_SERVER_OSF = 0x00100000,
       SV_TYPE_SERVER_VMS = 0x00200000,
       SV_TYPE_WIN95_PLUS = 0x00400000,
       SV_TYPE_DFS_SERVER = 0x00800000,
       SV_TYPE_ALTERNATE_XPORT = 0x20000000,
       SV_TYPE_LOCAL_LIST_ONLY = 0x40000000,
       SV_TYPE_DOMAIN_ENUM = 0x80000000,
       SV_TYPE_ALL = 0xFFFFFFFF,
};

enum { SMB_TRANS2_OPEN = 0,
       SMB_TRANS2_FIND_FIRST2 = 1,
       SMB_TRANS2_FIND_NEXT2 = 2,
       SMB_TRANS2_QUERY_FS_INFORMATION = 3,
       SMB_TRANS2_SET_FS_INFORMATION = 4,
       SMB_TRANS2_QUERY_PATH_INFORMATION = 5,
       SMB_TRANS2_SET_PATH_INFORMATION = 6,
       SMB_TRANS2_QUERY_FILE_INFORMATION = 7,
       SMB_TRANS2_SET_FILE_INFORMATION = 8,
       SMB_TRANS2_FSCTL = 9,
       SMB_TRANS2_IOCTL2 = 0xA,
       SMB_TRANS2_FIND_NOTIFY_FIRST = 0xB,
       SMB_TRANS2_FIND_NOTIFY_NEXT = 0xC,
       SMB_TRANS2_CREATE_DIRECTORY = 0xD,
       SMB_TRANS2_SESSION_SETUP = 0xE,
       SMB_TRANS2_GET_DFS_REFERRAL = 0x10,
       SMB_TRANS2_REPORT_DFS_INCONSISTENCY = 0x11,
};

enum { SMB_FIND_CLOSE = 1,
       SMB_FIND_CLOSE_EOS = 2,
       SMB_FIND_RETURN_RESUME_KEYS = 4,
       SMB_FIND_CONTINUE = 8,
       SMB_FIND_BACKUP = 16 };

enum { SMB_INFO_STANDARD = 1,
       SMB_FIND_FILE_BOTH_DIRECTORY_INFO = 0x104,
       SMB_QUERY_FILE_BASIC_INFO = 0x101,
       SMB_QUERY_FILE_STANDARD_INFO = 0x102,
       SMB_QUERY_FILE_EA_INFO = 0x103,
       SMB_QUERY_FILE_ALL_INFO = 0x107,
       SMB_QUERY_FILE_STREAM_INFO = 0x109,
};

enum { SMB_SET_FILE_BASIC_INFO = 0x101,
       SMB_SET_FILE_DISPOSITION_INFO = 0x102,
       SMB_SET_FILE_ALLOCATION_INFO = 0x103,
       SMB_SET_FILE_END_OF_FILE_INFO = 0x104,
};

enum { SMB_ATTR_READ_ONLY = (1 << 0),
       SMB_ATTR_HIDDEN = (1 << 1),
       SMB_ATTR_SYSTEM = (1 << 2),
       SMB_ATTR_DIRECTORY = (1 << 4),
       SMB_ATTR_ARCHIVE = (1 << 5),
       SMB_ATTR_NORMAL = (1 << 7),
       SMB_ATTR_COMPRESSED = 0x800,
       SMB_ATTR_TEMPORARY = 0x100,
       SMB_ATTR_WRITETHROUGH = 0x80000000,
       SMB_ATTR_NO_BUFFERING = 0x20000000,
       SMB_ATTR_RANDOM_ACCESS = 0x10000000,
};

enum { SMB_OFUN_EXIST_SHIFT = 0,
       SMB_OFUN_EXIST_MASK = 3,
       SMB_OFUN_EXIST_FAIL = 0,
       SMB_OFUN_EXIST_OPEN = 1,
       SMB_OFUN_EXIST_TRUNCATE = 2,
       SMB_OFUN_NOEXIST_CREATE = (1 << 4),
};

enum { SMB_OPEN_FLAGS_ADDITIONAL = 1,
       SMB_OPEN_FLAGS_OPLOCK = 2,
       SMB_OPEN_FLAGS_OPBATCH = 4,
       SMB_OPEN_MODE_ACCESS_SHIFT = 0,
       SMB_OPEN_MODE_ACCESS_MASK = 7,
       SMB_OPEN_MODE_SHARE_SHIFT = 4,
       SMB_OPEN_MODE_SHARE_MASK = 7,
       SMB_OPEN_MODE_SHARE_COMPATIBILITY = 0,
       SMB_OPEN_MODE_SHARE_EXCLUSIVE = 1,
       SMB_OPEN_MODE_SHARE_DENY_WRITE = 2,
       SMB_OPEN_MODE_SHARE_DENY_READOREXEC = 3,
       SMB_OPEN_MODE_SHARE_DENY_NONE = 4,
       SMB_OPEN_MODE_WRITE_THROUGH = (1 << 14),
};

enum { SMB_INFO_ALLOCATION = 1,
       SMB_INFO_VOLUME = 2,
       SMB_QUERY_FS_VOLUME_INFO = 0x102,
       SMB_QUERY_FS_SIZE_INFO = 0x103,
       SMB_QUERY_FS_ATTRIBUTE_INFO = 0x105,
};

enum { SMB_CD_SUPERCEDE = 0,
       SMB_CD_OPEN = 1,
       SMB_CD_CREATE = 2,
       SMB_CD_OPEN_IF = 3,
       SMB_CD_OVERWRITE = 4,
       SMB_CD_OVERWRITE_IF = 5,
       SMB_CD_MAX = 5,
};

enum { SMB_DA_SPECIFIC_MASK = 0x0000ffff,
       SMB_DA_SPECIFIC_READ_DATA = 0x00000001,
       SMB_DA_SPECIFIC_WRITE_DATA = 0x00000002,
       SMB_DA_SPECIFIC_APPEND_DATA = 0x00000004,
       SMB_DA_SPECIFIC_READ_EA = 0x00000008,
       SMB_DA_SPECIFIC_WRITE_EA = 0x00000010,
       SMB_DA_SPECIFIC_EXECUTE = 0x00000020,
       SMB_DA_SPECIFIC_DELETE_CHILD = 0x00000040,
       SMB_DA_SPECIFIC_READ_ATTRIBUTES = 0x00000080,
       SMB_DA_SPECIFIC_WRITE_ATTRIBUTES = 0x00000100,
       SMB_DA_STANDARD_MASK = 0x00ff0000,
       SMB_DA_STANDARD_DELETE_ACCESS = 0x00010000,
       SMB_DA_STANDARD_READ_CONTROL_ACCESS = 0x00020000,
       SMB_DA_STANDARD_WRITE_DAC_ACCESS = 0x00040000,
       SMB_DA_STANDARD_WRITE_OWNER_ACCESS = 0x00080000,
       SMB_DA_STANDARD_SYNCHRONIZE_ACCESS = 0x00100000,
       SMB_DA_GENERIC_MASK = 0xf0000000,
       SMB_DA_GENERIC_ALL_ACCESS = 0x10000000,
       SMB_DA_GENERIC_EXECUTE_ACCESS = 0x20000000,
       SMB_DA_GENERIC_WRITE_ACCESS = 0x40000000,
       SMB_DA_GENERIC_READ_ACCESS = 0x80000000 };

enum { SMB_SA_NO_SHARE = 0x00000000,
       SMB_SA_SHARE_READ = 0x00000001,
       SMB_SA_SHARE_WRITE = 0x00000002,
       SMB_SA_SHARE_DELETE = 0x00000004,
};

enum { SMB_CO_DIRECTORY = 0x00000001,
       SMB_CO_WRITETHROUGH = 0x00000002,
       SMB_CO_SEQUENTIAL_ONLY = 0x00000004,
       SMB_CO_FILE = 0x00000040,
       SMB_CO_NO_EA_KNOWLEDGE = 0x00000200,
       SMB_CO_EIGHT_DOT_THREE_ONLY = 0x00000400,
       SMB_CO_RANDOM_ACCESS = 0x00000800,
       SMB_CO_DELETE_ON_CLOSE = 0x00001000,
};
