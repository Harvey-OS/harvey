/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Host communication command constants for Chrome EC */

#ifndef __CROS_EC_COMMANDS_H
#define __CROS_EC_COMMANDS_H

/*
 * Current version of this protocol
 *
 * TODO(crosbug.com/p/11223): This is effectively useless; protocol is
 * determined in other ways.  Remove this once the kernel code no longer
 * depends on it.
 */
#define EC_PROTO_VERSION          0x00000002

/* Command version mask */
#define EC_VER_MASK(version) (1UL << (version))

/* I/O addresses for ACPI commands */
#define EC_LPC_ADDR_ACPI_DATA  0x62
#define EC_LPC_ADDR_ACPI_CMD   0x66

/* I/O addresses for host command */
#define EC_LPC_ADDR_HOST_DATA  0x200
#define EC_LPC_ADDR_HOST_CMD   0x204

/* I/O addresses for host command args and params */
/* Protocol version 2 */
#define EC_LPC_ADDR_HOST_ARGS    0x800  /* And 0x801, 0x802, 0x803 */
#define EC_LPC_ADDR_HOST_PARAM   0x804  /* For version 2 params; size is
					 * EC_PROTO2_MAX_PARAM_SIZE */
/* Protocol version 3 */
#define EC_LPC_ADDR_HOST_PACKET  0x800  /* Offset of version 3 packet */
#define EC_LPC_HOST_PACKET_SIZE  0x100  /* Max size of version 3 packet */

/* The actual block is 0x800-0x8ff, but some BIOSes think it's 0x880-0x8ff
 * and they tell the kernel that so we have to think of it as two parts. */
#define EC_HOST_CMD_REGION0    0x800
#define EC_HOST_CMD_REGION1    0x880
#define EC_HOST_CMD_REGION_SIZE 0x80

/* EC command register bit functions */
#define EC_LPC_CMDR_DATA	(1 << 0)  /* Data ready for host to read */
#define EC_LPC_CMDR_PENDING	(1 << 1)  /* Write pending to EC */
#define EC_LPC_CMDR_BUSY	(1 << 2)  /* EC is busy processing a command */
#define EC_LPC_CMDR_CMD		(1 << 3)  /* Last host write was a command */
#define EC_LPC_CMDR_ACPI_BRST	(1 << 4)  /* Burst mode (not used) */
#define EC_LPC_CMDR_SCI		(1 << 5)  /* SCI event is pending */
#define EC_LPC_CMDR_SMI		(1 << 6)  /* SMI event is pending */

#define EC_LPC_ADDR_MEMMAP       0x900
#define EC_MEMMAP_SIZE         255 /* ACPI IO buffer max is 255 bytes */
#define EC_MEMMAP_TEXT_MAX     8   /* Size of a string in the memory map */

/* The offset address of each type of data in mapped memory. */
#define EC_MEMMAP_TEMP_SENSOR      0x00 /* Temp sensors 0x00 - 0x0f */
#define EC_MEMMAP_FAN              0x10 /* Fan speeds 0x10 - 0x17 */
#define EC_MEMMAP_TEMP_SENSOR_B    0x18 /* More temp sensors 0x18 - 0x1f */
#define EC_MEMMAP_ID               0x20 /* 0x20 == 'E', 0x21 == 'C' */
#define EC_MEMMAP_ID_VERSION       0x22 /* Version of data in 0x20 - 0x2f */
#define EC_MEMMAP_THERMAL_VERSION  0x23 /* Version of data in 0x00 - 0x1f */
#define EC_MEMMAP_BATTERY_VERSION  0x24 /* Version of data in 0x40 - 0x7f */
#define EC_MEMMAP_SWITCHES_VERSION 0x25 /* Version of data in 0x30 - 0x33 */
#define EC_MEMMAP_EVENTS_VERSION   0x26 /* Version of data in 0x34 - 0x3f */
#define EC_MEMMAP_HOST_CMD_FLAGS   0x27 /* Host cmd interface flags (8 bits) */
/* Unused 0x28 - 0x2f */
#define EC_MEMMAP_SWITCHES         0x30	/* 8 bits */
/* Unused 0x31 - 0x33 */
#define EC_MEMMAP_HOST_EVENTS      0x34 /* 32 bits */
/* Reserve 0x38 - 0x3f for additional host event-related stuff */
/* Battery values are all 32 bits */
#define EC_MEMMAP_BATT_VOLT        0x40 /* Battery Present Voltage */
#define EC_MEMMAP_BATT_RATE        0x44 /* Battery Present Rate */
#define EC_MEMMAP_BATT_CAP         0x48 /* Battery Remaining Capacity */
#define EC_MEMMAP_BATT_FLAG        0x4c /* Battery State, defined below */
#define EC_MEMMAP_BATT_DCAP        0x50 /* Battery Design Capacity */
#define EC_MEMMAP_BATT_DVLT        0x54 /* Battery Design Voltage */
#define EC_MEMMAP_BATT_LFCC        0x58 /* Battery Last Full Charge Capacity */
#define EC_MEMMAP_BATT_CCNT        0x5c /* Battery Cycle Count */
/* Strings are all 8 bytes (EC_MEMMAP_TEXT_MAX) */
#define EC_MEMMAP_BATT_MFGR        0x60 /* Battery Manufacturer String */
#define EC_MEMMAP_BATT_MODEL       0x68 /* Battery Model Number String */
#define EC_MEMMAP_BATT_SERIAL      0x70 /* Battery Serial Number String */
#define EC_MEMMAP_BATT_TYPE        0x78 /* Battery Type String */
#define EC_MEMMAP_ALS              0x80 /* ALS readings in lux (2 X 16 bits) */
/* Unused 0x84 - 0x8f */
#define EC_MEMMAP_ACC_STATUS       0x90 /* Accelerometer status (8 bits )*/
/* Unused 0x91 */
#define EC_MEMMAP_ACC_DATA         0x92 /* Accelerometer data 0x92 - 0x9f */
#define EC_MEMMAP_GYRO_DATA        0xa0 /* Gyroscope data 0xa0 - 0xa5 */
/* Unused 0xa6 - 0xdf */

/*
 * ACPI is unable to access memory mapped data at or above this offset due to
 * limitations of the ACPI protocol. Do not place data in the range 0xe0 - 0xfe
 * which might be needed by ACPI.
 */
#define EC_MEMMAP_NO_ACPI 0xe0

/* Define the format of the accelerometer mapped memory status byte. */
#define EC_MEMMAP_ACC_STATUS_SAMPLE_ID_MASK  0x0f
#define EC_MEMMAP_ACC_STATUS_BUSY_BIT        (1 << 4)
#define EC_MEMMAP_ACC_STATUS_PRESENCE_BIT    (1 << 7)

/* Number of temp sensors at EC_MEMMAP_TEMP_SENSOR */
#define EC_TEMP_SENSOR_ENTRIES     16
/*
 * Number of temp sensors at EC_MEMMAP_TEMP_SENSOR_B.
 *
 * Valid only if EC_MEMMAP_THERMAL_VERSION returns >= 2.
 */
#define EC_TEMP_SENSOR_B_ENTRIES      8

/* Special values for mapped temperature sensors */
#define EC_TEMP_SENSOR_NOT_PRESENT    0xff
#define EC_TEMP_SENSOR_ERROR          0xfe
#define EC_TEMP_SENSOR_NOT_POWERED    0xfd
#define EC_TEMP_SENSOR_NOT_CALIBRATED 0xfc
/*
 * The offset of temperature value stored in mapped memory.  This allows
 * reporting a temperature range of 200K to 454K = -73C to 181C.
 */
#define EC_TEMP_SENSOR_OFFSET      200

/*
 * Number of ALS readings at EC_MEMMAP_ALS
 */
#define EC_ALS_ENTRIES             2

/*
 * The default value a temperature sensor will return when it is present but
 * has not been read this boot.  This is a reasonable number to avoid
 * triggering alarms on the host.
 */
#define EC_TEMP_SENSOR_DEFAULT     (296 - EC_TEMP_SENSOR_OFFSET)

#define EC_FAN_SPEED_ENTRIES       4       /* Number of fans at EC_MEMMAP_FAN */
#define EC_FAN_SPEED_NOT_PRESENT   0xffff  /* Entry not present */
#define EC_FAN_SPEED_STALLED       0xfffe  /* Fan stalled */

/* Battery bit flags at EC_MEMMAP_BATT_FLAG. */
#define EC_BATT_FLAG_AC_PRESENT   0x01
#define EC_BATT_FLAG_BATT_PRESENT 0x02
#define EC_BATT_FLAG_DISCHARGING  0x04
#define EC_BATT_FLAG_CHARGING     0x08
#define EC_BATT_FLAG_LEVEL_CRITICAL 0x10

/* Switch flags at EC_MEMMAP_SWITCHES */
#define EC_SWITCH_LID_OPEN               0x01
#define EC_SWITCH_POWER_BUTTON_PRESSED   0x02
#define EC_SWITCH_WRITE_PROTECT_DISABLED 0x04
/* Was recovery requested via keyboard; now unused. */
#define EC_SWITCH_IGNORE1		 0x08
/* Recovery requested via dedicated signal (from servo board) */
#define EC_SWITCH_DEDICATED_RECOVERY     0x10
/* Was fake developer mode switch; now unused.  Remove in next refactor. */
#define EC_SWITCH_IGNORE0                0x20

/* Host command interface flags */
/* Host command interface supports LPC args (LPC interface only) */
#define EC_HOST_CMD_FLAG_LPC_ARGS_SUPPORTED  0x01
/* Host command interface supports version 3 protocol */
#define EC_HOST_CMD_FLAG_VERSION_3   0x02

/* Wireless switch flags */
#define EC_WIRELESS_SWITCH_ALL       ~0x00  /* All flags */
#define EC_WIRELESS_SWITCH_WLAN       0x01  /* WLAN radio */
#define EC_WIRELESS_SWITCH_BLUETOOTH  0x02  /* Bluetooth radio */
#define EC_WIRELESS_SWITCH_WWAN       0x04  /* WWAN power */
#define EC_WIRELESS_SWITCH_WLAN_POWER 0x08  /* WLAN power */

/*****************************************************************************/
/*
 * ACPI commands
 *
 * These are valid ONLY on the ACPI command/data port.
 */

/*
 * ACPI Read Embedded Controller
 *
 * This reads from ACPI memory space on the EC (EC_ACPI_MEM_*).
 *
 * Use the following sequence:
 *
 *    - Write EC_CMD_ACPI_READ to EC_LPC_ADDR_ACPI_CMD
 *    - Wait for EC_LPC_CMDR_PENDING bit to clear
 *    - Write address to EC_LPC_ADDR_ACPI_DATA
 *    - Wait for EC_LPC_CMDR_DATA bit to set
 *    - Read value from EC_LPC_ADDR_ACPI_DATA
 */
#define EC_CMD_ACPI_READ 0x80

/*
 * ACPI Write Embedded Controller
 *
 * This reads from ACPI memory space on the EC (EC_ACPI_MEM_*).
 *
 * Use the following sequence:
 *
 *    - Write EC_CMD_ACPI_WRITE to EC_LPC_ADDR_ACPI_CMD
 *    - Wait for EC_LPC_CMDR_PENDING bit to clear
 *    - Write address to EC_LPC_ADDR_ACPI_DATA
 *    - Wait for EC_LPC_CMDR_PENDING bit to clear
 *    - Write value to EC_LPC_ADDR_ACPI_DATA
 */
#define EC_CMD_ACPI_WRITE 0x81

/*
 * ACPI Burst Enable Embedded Controller
 *
 * This enables burst mode on the EC to allow the host to issue several
 * commands back-to-back. While in this mode, writes to mapped multi-byte
 * data are locked out to ensure data consistency.
 */
#define EC_CMD_ACPI_BURST_ENABLE 0x82

/*
 * ACPI Burst Disable Embedded Controller
 *
 * This disables burst mode on the EC and stops preventing EC writes to mapped
 * multi-byte data.
 */
#define EC_CMD_ACPI_BURST_DISABLE 0x83

/*
 * ACPI Query Embedded Controller
 *
 * This clears the lowest-order bit in the currently pending host events, and
 * sets the result code to the 1-based index of the bit (event 0x00000001 = 1,
 * event 0x80000000 = 32), or 0 if no event was pending.
 */
#define EC_CMD_ACPI_QUERY_EVENT 0x84

/* Valid addresses in ACPI memory space, for read/write commands */

/* Memory space version; set to EC_ACPI_MEM_VERSION_CURRENT */
#define EC_ACPI_MEM_VERSION            0x00
/*
 * Test location; writing value here updates test compliment byte to (0xff -
 * value).
 */
#define EC_ACPI_MEM_TEST               0x01
/* Test compliment; writes here are ignored. */
#define EC_ACPI_MEM_TEST_COMPLIMENT    0x02

/* Keyboard backlight brightness percent (0 - 100) */
#define EC_ACPI_MEM_KEYBOARD_BACKLIGHT 0x03
/* DPTF Target Fan Duty (0-100, 0xff for auto/none) */
#define EC_ACPI_MEM_FAN_DUTY           0x04

/*
 * DPTF temp thresholds. Any of the EC's temp sensors can have up to two
 * independent thresholds attached to them. The current value of the ID
 * register determines which sensor is affected by the THRESHOLD and COMMIT
 * registers. The THRESHOLD register uses the same EC_TEMP_SENSOR_OFFSET scheme
 * as the memory-mapped sensors. The COMMIT register applies those settings.
 *
 * The spec does not mandate any way to read back the threshold settings
 * themselves, but when a threshold is crossed the AP needs a way to determine
 * which sensor(s) are responsible. Each reading of the ID register clears and
 * returns one sensor ID that has crossed one of its threshold (in either
 * direction) since the last read. A value of 0xFF means "no new thresholds
 * have tripped". Setting or enabling the thresholds for a sensor will clear
 * the unread event count for that sensor.
 */
#define EC_ACPI_MEM_TEMP_ID            0x05
#define EC_ACPI_MEM_TEMP_THRESHOLD     0x06
#define EC_ACPI_MEM_TEMP_COMMIT        0x07
/*
 * Here are the bits for the COMMIT register:
 *   bit 0 selects the threshold index for the chosen sensor (0/1)
 *   bit 1 enables/disables the selected threshold (0 = off, 1 = on)
 * Each write to the commit register affects one threshold.
 */
#define EC_ACPI_MEM_TEMP_COMMIT_SELECT_MASK (1 << 0)
#define EC_ACPI_MEM_TEMP_COMMIT_ENABLE_MASK (1 << 1)
/*
 * Example:
 *
 * Set the thresholds for sensor 2 to 50 C and 60 C:
 *   write 2 to [0x05]      --  select temp sensor 2
 *   write 0x7b to [0x06]   --  C_TO_K(50) - EC_TEMP_SENSOR_OFFSET
 *   write 0x2 to [0x07]    --  enable threshold 0 with this value
 *   write 0x85 to [0x06]   --  C_TO_K(60) - EC_TEMP_SENSOR_OFFSET
 *   write 0x3 to [0x07]    --  enable threshold 1 with this value
 *
 * Disable the 60 C threshold, leaving the 50 C threshold unchanged:
 *   write 2 to [0x05]      --  select temp sensor 2
 *   write 0x1 to [0x07]    --  disable threshold 1
 */

/* DPTF battery charging current limit */
#define EC_ACPI_MEM_CHARGING_LIMIT     0x08

/* Charging limit is specified in 64 mA steps */
#define EC_ACPI_MEM_CHARGING_LIMIT_STEP_MA   64
/* Value to disable DPTF battery charging limit */
#define EC_ACPI_MEM_CHARGING_LIMIT_DISABLED  0xff

/*
 * ACPI addresses 0x20 - 0xff map to EC_MEMMAP offset 0x00 - 0xdf.  This data
 * is read-only from the AP.  Added in EC_ACPI_MEM_VERSION 2.
 */
#define EC_ACPI_MEM_MAPPED_BEGIN   0x20
#define EC_ACPI_MEM_MAPPED_SIZE    0xe0

/* Current version of ACPI memory address space */
#define EC_ACPI_MEM_VERSION_CURRENT 2


/*
 * This header file is used in coreboot both in C and ACPI code.  The ACPI code
 * is pre-processed to handle constants but the ASL compiler is unable to
 * handle actual C code so keep it separate.
 */
#ifndef __ACPI__

/*
 * Define __packed if someone hasn't beat us to it.  Linux kernel style
 * checking prefers __packed over __attribute__((packed)).
 */
#ifndef __packed
#define __packed __attribute__((packed))
#endif

/* LPC command status byte masks */
/* EC has written a byte in the data register and host hasn't read it yet */
#define EC_LPC_STATUS_TO_HOST     0x01
/* Host has written a command/data byte and the EC hasn't read it yet */
#define EC_LPC_STATUS_FROM_HOST   0x02
/* EC is processing a command */
#define EC_LPC_STATUS_PROCESSING  0x04
/* Last write to EC was a command, not data */
#define EC_LPC_STATUS_LAST_CMD    0x08
/* EC is in burst mode */
#define EC_LPC_STATUS_BURST_MODE  0x10
/* SCI event is pending (requesting SCI query) */
#define EC_LPC_STATUS_SCI_PENDING 0x20
/* SMI event is pending (requesting SMI query) */
#define EC_LPC_STATUS_SMI_PENDING 0x40
/* (reserved) */
#define EC_LPC_STATUS_RESERVED    0x80

/*
 * EC is busy.  This covers both the EC processing a command, and the host has
 * written a new command but the EC hasn't picked it up yet.
 */
#define EC_LPC_STATUS_BUSY_MASK \
	(EC_LPC_STATUS_FROM_HOST | EC_LPC_STATUS_PROCESSING)

/* Host command response codes */
enum ec_status {
	EC_RES_SUCCESS = 0,
	EC_RES_INVALID_COMMAND = 1,
	EC_RES_ERROR = 2,
	EC_RES_INVALID_PARAM = 3,
	EC_RES_ACCESS_DENIED = 4,
	EC_RES_INVALID_RESPONSE = 5,
	EC_RES_INVALID_VERSION = 6,
	EC_RES_INVALID_CHECKSUM = 7,
	EC_RES_IN_PROGRESS = 8,		/* Accepted, command in progress */
	EC_RES_UNAVAILABLE = 9,		/* No response available */
	EC_RES_TIMEOUT = 10,		/* We got a timeout */
	EC_RES_OVERFLOW = 11,		/* Table / data overflow */
	EC_RES_INVALID_HEADER = 12,     /* Header contains invalid data */
	EC_RES_REQUEST_TRUNCATED = 13,  /* Didn't get the entire request */
	EC_RES_RESPONSE_TOO_BIG = 14,   /* Response was too big to handle */
	EC_RES_BUS_ERROR = 15,          /* Communications bus error */
	EC_RES_BUSY = 16                /* Up but too busy.  Should retry */
};

/*
 * Host event codes.  Note these are 1-based, not 0-based, because ACPI query
 * EC command uses code 0 to mean "no event pending".  We explicitly specify
 * each value in the enum listing so they won't change if we delete/insert an
 * item or rearrange the list (it needs to be stable across platforms, not
 * just within a single compiled instance).
 */
enum host_event_code {
	EC_HOST_EVENT_LID_CLOSED = 1,
	EC_HOST_EVENT_LID_OPEN = 2,
	EC_HOST_EVENT_POWER_BUTTON = 3,
	EC_HOST_EVENT_AC_CONNECTED = 4,
	EC_HOST_EVENT_AC_DISCONNECTED = 5,
	EC_HOST_EVENT_BATTERY_LOW = 6,
	EC_HOST_EVENT_BATTERY_CRITICAL = 7,
	EC_HOST_EVENT_BATTERY = 8,
	EC_HOST_EVENT_THERMAL_THRESHOLD = 9,
	EC_HOST_EVENT_THERMAL_OVERLOAD = 10,
	EC_HOST_EVENT_THERMAL = 11,
	EC_HOST_EVENT_USB_CHARGER = 12,
	EC_HOST_EVENT_KEY_PRESSED = 13,
	/*
	 * EC has finished initializing the host interface.  The host can check
	 * for this event following sending a EC_CMD_REBOOT_EC command to
	 * determine when the EC is ready to accept subsequent commands.
	 */
	EC_HOST_EVENT_INTERFACE_READY = 14,
	/* Keyboard recovery combo has been pressed */
	EC_HOST_EVENT_KEYBOARD_RECOVERY = 15,

	/* Shutdown due to thermal overload */
	EC_HOST_EVENT_THERMAL_SHUTDOWN = 16,
	/* Shutdown due to battery level too low */
	EC_HOST_EVENT_BATTERY_SHUTDOWN = 17,

	/* Suggest that the AP throttle itself */
	EC_HOST_EVENT_THROTTLE_START = 18,
	/* Suggest that the AP resume normal speed */
	EC_HOST_EVENT_THROTTLE_STOP = 19,

	/* Hang detect logic detected a hang and host event timeout expired */
	EC_HOST_EVENT_HANG_DETECT = 20,
	/* Hang detect logic detected a hang and warm rebooted the AP */
	EC_HOST_EVENT_HANG_REBOOT = 21,

	/* PD MCU triggering host event */
	EC_HOST_EVENT_PD_MCU = 22,

	/* Battery Status flags have changed */
	EC_HOST_EVENT_BATTERY_STATUS = 23,

	/* EC encountered a panic, triggering a reset */
	EC_HOST_EVENT_PANIC = 24,

	/* Keyboard fastboot combo has been pressed */
	EC_HOST_EVENT_KEYBOARD_FASTBOOT = 25,

	/*
	 * The high bit of the event mask is not used as a host event code.  If
	 * it reads back as set, then the entire event mask should be
	 * considered invalid by the host.  This can happen when reading the
	 * raw event status via EC_MEMMAP_HOST_EVENTS but the LPC interface is
	 * not initialized on the EC, or improperly configured on the host.
	 */
	EC_HOST_EVENT_INVALID = 32
};
/* Host event mask */
#define EC_HOST_EVENT_MASK(event_code) (1UL << ((event_code) - 1))

/* Arguments at EC_LPC_ADDR_HOST_ARGS */
struct ec_lpc_host_args {
	u8 flags;
	u8 command_version;
	u8 data_size;
	/*
	 * Checksum; sum of command + flags + command_version + data_size +
	 * all params/response data bytes.
	 */
	u8 checksum;
} __packed;

/* Flags for ec_lpc_host_args.flags */
/*
 * Args are from host.  Data area at EC_LPC_ADDR_HOST_PARAM contains command
 * params.
 *
 * If EC gets a command and this flag is not set, this is an old-style command.
 * Command version is 0 and params from host are at EC_LPC_ADDR_OLD_PARAM with
 * unknown length.  EC must respond with an old-style response (that is,
 * without setting EC_HOST_ARGS_FLAG_TO_HOST).
 */
#define EC_HOST_ARGS_FLAG_FROM_HOST 0x01
/*
 * Args are from EC.  Data area at EC_LPC_ADDR_HOST_PARAM contains response.
 *
 * If EC responds to a command and this flag is not set, this is an old-style
 * response.  Command version is 0 and response data from EC is at
 * EC_LPC_ADDR_OLD_PARAM with unknown length.
 */
#define EC_HOST_ARGS_FLAG_TO_HOST   0x02

/*****************************************************************************/
/*
 * Byte codes returned by EC over SPI interface.
 *
 * These can be used by the AP to debug the EC interface, and to determine
 * when the EC is not in a state where it will ever get around to responding
 * to the AP.
 *
 * Example of sequence of bytes read from EC for a current good transfer:
 *   1. -                  - AP asserts chip select (CS#)
 *   2. EC_SPI_OLD_READY   - AP sends first byte(s) of request
 *   3. -                  - EC starts handling CS# interrupt
 *   4. EC_SPI_RECEIVING   - AP sends remaining byte(s) of request
 *   5. EC_SPI_PROCESSING  - EC starts processing request; AP is clocking in
 *                           bytes looking for EC_SPI_FRAME_START
 *   6. -                  - EC finishes processing and sets up response
 *   7. EC_SPI_FRAME_START - AP reads frame byte
 *   8. (response packet)  - AP reads response packet
 *   9. EC_SPI_PAST_END    - Any additional bytes read by AP
 *   10 -                  - AP deasserts chip select
 *   11 -                  - EC processes CS# interrupt and sets up DMA for
 *                           next request
 *
 * If the AP is waiting for EC_SPI_FRAME_START and sees any value other than
 * the following byte values:
 *   EC_SPI_OLD_READY
 *   EC_SPI_RX_READY
 *   EC_SPI_RECEIVING
 *   EC_SPI_PROCESSING
 *
 * Then the EC found an error in the request, or was not ready for the request
 * and lost data.  The AP should give up waiting for EC_SPI_FRAME_START,
 * because the EC is unable to tell when the AP is done sending its request.
 */

/*
 * Framing byte which precedes a response packet from the EC.  After sending a
 * request, the AP will clock in bytes until it sees the framing byte, then
 * clock in the response packet.
 */
#define EC_SPI_FRAME_START    0xec

/*
 * Padding bytes which are clocked out after the end of a response packet.
 */
#define EC_SPI_PAST_END       0xed

/*
 * EC is ready to receive, and has ignored the byte sent by the AP.  EC expects
 * that the AP will send a valid packet header (starting with
 * EC_COMMAND_PROTOCOL_3) in the next 32 bytes.
 */
#define EC_SPI_RX_READY       0xf8

/*
 * EC has started receiving the request from the AP, but hasn't started
 * processing it yet.
 */
#define EC_SPI_RECEIVING      0xf9

/* EC has received the entire request from the AP and is processing it. */
#define EC_SPI_PROCESSING     0xfa

/*
 * EC received bad data from the AP, such as a packet header with an invalid
 * length.  EC will ignore all data until chip select deasserts.
 */
#define EC_SPI_RX_BAD_DATA    0xfb

/*
 * EC received data from the AP before it was ready.  That is, the AP asserted
 * chip select and started clocking data before the EC was ready to receive it.
 * EC will ignore all data until chip select deasserts.
 */
#define EC_SPI_NOT_READY      0xfc

/*
 * EC was ready to receive a request from the AP.  EC has treated the byte sent
 * by the AP as part of a request packet, or (for old-style ECs) is processing
 * a fully received packet but is not ready to respond yet.
 */
#define EC_SPI_OLD_READY      0xfd

/*****************************************************************************/

/*
 * Protocol version 2 for I2C and SPI send a request this way:
 *
 *	0	EC_CMD_VERSION0 + (command version)
 *	1	Command number
 *	2	Length of params = N
 *	3..N+2	Params, if any
 *	N+3	8-bit checksum of bytes 0..N+2
 *
 * The corresponding response is:
 *
 *	0	Result code (EC_RES_*)
 *	1	Length of params = M
 *	2..M+1	Params, if any
 *	M+2	8-bit checksum of bytes 0..M+1
 */
#define EC_PROTO2_REQUEST_HEADER_BYTES 3
#define EC_PROTO2_REQUEST_TRAILER_BYTES 1
#define EC_PROTO2_REQUEST_OVERHEAD (EC_PROTO2_REQUEST_HEADER_BYTES +	\
				    EC_PROTO2_REQUEST_TRAILER_BYTES)

#define EC_PROTO2_RESPONSE_HEADER_BYTES 2
#define EC_PROTO2_RESPONSE_TRAILER_BYTES 1
#define EC_PROTO2_RESPONSE_OVERHEAD (EC_PROTO2_RESPONSE_HEADER_BYTES +	\
				     EC_PROTO2_RESPONSE_TRAILER_BYTES)

/* Parameter length was limited by the LPC interface */
#define EC_PROTO2_MAX_PARAM_SIZE 0xfc

/* Maximum request and response packet sizes for protocol version 2 */
#define EC_PROTO2_MAX_REQUEST_SIZE (EC_PROTO2_REQUEST_OVERHEAD +	\
				    EC_PROTO2_MAX_PARAM_SIZE)
#define EC_PROTO2_MAX_RESPONSE_SIZE (EC_PROTO2_RESPONSE_OVERHEAD +	\
				     EC_PROTO2_MAX_PARAM_SIZE)

/*****************************************************************************/

/*
 * Value written to legacy command port / prefix byte to indicate protocol
 * 3+ structs are being used.  Usage is bus-dependent.
 */
#define EC_COMMAND_PROTOCOL_3 0xda

#define EC_HOST_REQUEST_VERSION 3

/* Version 3 request from host */
struct ec_host_request {
	/* Struct version (=3)
	 *
	 * EC will return EC_RES_INVALID_HEADER if it receives a header with a
	 * version it doesn't know how to parse.
	 */
	u8 struct_version;

	/*
	 * Checksum of request and data; sum of all bytes including checksum
	 * should total to 0.
	 */
	u8 checksum;

	/* Command code */
	u16 command;

	/* Command version */
	u8 command_version;

	/* Unused byte in current protocol version; set to 0 */
	u8 reserved;

	/* Length of data which follows this header */
	u16 data_len;
} __packed;

#define EC_HOST_RESPONSE_VERSION 3

/* Version 3 response from EC */
struct ec_host_response {
	/* Struct version (=3) */
	u8 struct_version;

	/*
	 * Checksum of response and data; sum of all bytes including checksum
	 * should total to 0.
	 */
	u8 checksum;

	/* Result code (EC_RES_*) */
	u16 result;

	/* Length of data which follows this header */
	u16 data_len;

	/* Unused bytes in current protocol version; set to 0 */
	u16 reserved;
} __packed;

/*****************************************************************************/
/*
 * Notes on commands:
 *
 * Each command is an 16-bit command value.  Commands which take params or
 * return response data specify structs for that data.  If no struct is
 * specified, the command does not input or output data, respectively.
 * Parameter/response length is implicit in the structs.  Some underlying
 * communication protocols (I2C, SPI) may add length or checksum headers, but
 * those are implementation-dependent and not defined here.
 */

/*****************************************************************************/
/* General / test commands */

/*
 * Get protocol version, used to deal with non-backward compatible protocol
 * changes.
 */
#define EC_CMD_PROTO_VERSION 0x00

struct ec_response_proto_version {
	u32 version;
} __packed;

/*
 * Hello.  This is a simple command to test the EC is responsive to
 * commands.
 */
#define EC_CMD_HELLO 0x01

struct ec_params_hello {
	u32 in_data;  /* Pass anything here */
} __packed;

struct ec_response_hello {
	u32 out_data;  /* Output will be in_data + 0x01020304 */
} __packed;

/* Get version number */
#define EC_CMD_GET_VERSION 0x02

enum ec_current_image {
	EC_IMAGE_UNKNOWN = 0,
	EC_IMAGE_RO,
	EC_IMAGE_RW
};

struct ec_response_get_version {
	/* Null-terminated version strings for RO, RW */
	char version_string_ro[32];
	char version_string_rw[32];
	char reserved[32];       /* Was previously RW-B string */
	u32 current_image;  /* One of ec_current_image */
} __packed;

/* Read test */
#define EC_CMD_READ_TEST 0x03

struct ec_params_read_test {
	u32 offset;   /* Starting value for read buffer */
	u32 size;     /* Size to read in bytes */
} __packed;

struct ec_response_read_test {
	u32 data[32];
} __packed;

/*
 * Get build information
 *
 * Response is null-terminated string.
 */
#define EC_CMD_GET_BUILD_INFO 0x04

/* Get chip info */
#define EC_CMD_GET_CHIP_INFO 0x05

struct ec_response_get_chip_info {
	/* Null-terminated strings */
	char vendor[32];
	char name[32];
	char revision[32];  /* Mask version */
} __packed;

/* Get board HW version */
#define EC_CMD_GET_BOARD_VERSION 0x06

struct ec_response_board_version {
	u16 board_version;  /* A monotonously incrementing number. */
} __packed;

/*
 * Read memory-mapped data.
 *
 * This is an alternate interface to memory-mapped data for bus protocols
 * which don't support direct-mapped memory - I2C, SPI, etc.
 *
 * Response is params.size bytes of data.
 */
#define EC_CMD_READ_MEMMAP 0x07

struct ec_params_read_memmap {
	u8 offset;   /* Offset in memmap (EC_MEMMAP_*) */
	u8 size;     /* Size to read in bytes */
} __packed;

/* Read versions supported for a command */
#define EC_CMD_GET_CMD_VERSIONS 0x08

struct ec_params_get_cmd_versions {
	u8 cmd;      /* Command to check */
} __packed;

struct ec_params_get_cmd_versions_v1 {
	u16 cmd;     /* Command to check */
} __packed;

struct ec_response_get_cmd_versions {
	/*
	 * Mask of supported versions; use EC_VER_MASK() to compare with a
	 * desired version.
	 */
	u32 version_mask;
} __packed;

/*
 * Check EC communcations status (busy). This is needed on i2c/spi but not
 * on lpc since it has its own out-of-band busy indicator.
 *
 * lpc must read the status from the command register. Attempting this on
 * lpc will overwrite the args/parameter space and corrupt its data.
 */
#define EC_CMD_GET_COMMS_STATUS		0x09

/* Avoid using ec_status which is for return values */
enum ec_comms_status {
	EC_COMMS_STATUS_PROCESSING	= 1 << 0,	/* Processing cmd */
};

struct ec_response_get_comms_status {
	u32 flags;		/* Mask of enum ec_comms_status */
} __packed;

/* Fake a variety of responses, purely for testing purposes. */
#define EC_CMD_TEST_PROTOCOL		0x0a

/* Tell the EC what to send back to us. */
struct ec_params_test_protocol {
	u32 ec_result;
	u32 ret_len;
	u8 buf[32];
} __packed;

/* Here it comes... */
struct ec_response_test_protocol {
	u8 buf[32];
} __packed;

/* Get prococol information */
#define EC_CMD_GET_PROTOCOL_INFO	0x0b

/* Flags for ec_response_get_protocol_info.flags */
/* EC_RES_IN_PROGRESS may be returned if a command is slow */
#define EC_PROTOCOL_INFO_IN_PROGRESS_SUPPORTED (1 << 0)

struct ec_response_get_protocol_info {
	/* Fields which exist if at least protocol version 3 supported */

	/* Bitmask of protocol versions supported (1 << n means version n)*/
	u32 protocol_versions;

	/* Maximum request packet size, in bytes */
	u16 max_request_packet_size;

	/* Maximum response packet size, in bytes */
	u16 max_response_packet_size;

	/* Flags; see EC_PROTOCOL_INFO_* */
	u32 flags;
} __packed;


/*****************************************************************************/
/* Get/Set miscellaneous values */

/* The upper byte of .flags tells what to do (nothing means "get") */
#define EC_GSV_SET        0x80000000

/* The lower three bytes of .flags identifies the parameter, if that has
   meaning for an individual command. */
#define EC_GSV_PARAM_MASK 0x00ffffff

struct ec_params_get_set_value {
	u32 flags;
	u32 value;
} __packed;

struct ec_response_get_set_value {
	u32 flags;
	u32 value;
} __packed;

/* More than one command can use these structs to get/set paramters. */
#define EC_CMD_GSV_PAUSE_IN_S5	0x0c


/*****************************************************************************/
/* Flash commands */

/* Get flash info */
#define EC_CMD_FLASH_INFO 0x10

/* Version 0 returns these fields */
struct ec_response_flash_info {
	/* Usable flash size, in bytes */
	u32 flash_size;
	/*
	 * Write block size.  Write offset and size must be a multiple
	 * of this.
	 */
	u32 write_block_size;
	/*
	 * Erase block size.  Erase offset and size must be a multiple
	 * of this.
	 */
	u32 erase_block_size;
	/*
	 * Protection block size.  Protection offset and size must be a
	 * multiple of this.
	 */
	u32 protect_block_size;
} __packed;

/* Flags for version 1+ flash info command */
/* EC flash erases bits to 0 instead of 1 */
#define EC_FLASH_INFO_ERASE_TO_0 (1 << 0)

/*
 * Version 1 returns the same initial fields as version 0, with additional
 * fields following.
 *
 * gcc anonymous structs don't seem to get along with the __packed directive;
 * if they did we'd define the version 0 struct as a sub-struct of this one.
 */
struct ec_response_flash_info_1 {
	/* Version 0 fields; see above for description */
	u32 flash_size;
	u32 write_block_size;
	u32 erase_block_size;
	u32 protect_block_size;

	/* Version 1 adds these fields: */
	/*
	 * Ideal write size in bytes.  Writes will be fastest if size is
	 * exactly this and offset is a multiple of this.  For example, an EC
	 * may have a write buffer which can do half-page operations if data is
	 * aligned, and a slower word-at-a-time write mode.
	 */
	u32 write_ideal_size;

	/* Flags; see EC_FLASH_INFO_* */
	u32 flags;
} __packed;

/*
 * Read flash
 *
 * Response is params.size bytes of data.
 */
#define EC_CMD_FLASH_READ 0x11

struct ec_params_flash_read {
	u32 offset;   /* Byte offset to read */
	u32 size;     /* Size to read in bytes */
} __packed;

/* Write flash */
#define EC_CMD_FLASH_WRITE 0x12
#define EC_VER_FLASH_WRITE 1

/* Version 0 of the flash command supported only 64 bytes of data */
#define EC_FLASH_WRITE_VER0_SIZE 64

struct ec_params_flash_write {
	u32 offset;   /* Byte offset to write */
	u32 size;     /* Size to write in bytes */
	/* Followed by data to write */
} __packed;

/* Erase flash */
#define EC_CMD_FLASH_ERASE 0x13

struct ec_params_flash_erase {
	u32 offset;   /* Byte offset to erase */
	u32 size;     /* Size to erase in bytes */
} __packed;

/*
 * Get/set flash protection.
 *
 * If mask!=0, sets/clear the requested bits of flags.  Depending on the
 * firmware write protect GPIO, not all flags will take effect immediately;
 * some flags require a subsequent hard reset to take effect.  Check the
 * returned flags bits to see what actually happened.
 *
 * If mask=0, simply returns the current flags state.
 */
#define EC_CMD_FLASH_PROTECT 0x15
#define EC_VER_FLASH_PROTECT 1  /* Command version 1 */

/* Flags for flash protection */
/* RO flash code protected when the EC boots */
#define EC_FLASH_PROTECT_RO_AT_BOOT         (1 << 0)
/*
 * RO flash code protected now.  If this bit is set, at-boot status cannot
 * be changed.
 */
#define EC_FLASH_PROTECT_RO_NOW             (1 << 1)
/* Entire flash code protected now, until reboot. */
#define EC_FLASH_PROTECT_ALL_NOW            (1 << 2)
/* Flash write protect GPIO is asserted now */
#define EC_FLASH_PROTECT_GPIO_ASSERTED      (1 << 3)
/* Error - at least one bank of flash is stuck locked, and cannot be unlocked */
#define EC_FLASH_PROTECT_ERROR_STUCK        (1 << 4)
/*
 * Error - flash protection is in inconsistent state.  At least one bank of
 * flash which should be protected is not protected.  Usually fixed by
 * re-requesting the desired flags, or by a hard reset if that fails.
 */
#define EC_FLASH_PROTECT_ERROR_INCONSISTENT (1 << 5)
/* Entire flash code protected when the EC boots */
#define EC_FLASH_PROTECT_ALL_AT_BOOT        (1 << 6)

struct ec_params_flash_protect {
	u32 mask;   /* Bits in flags to apply */
	u32 flags;  /* New flags to apply */
} __packed;

struct ec_response_flash_protect {
	/* Current value of flash protect flags */
	u32 flags;
	/*
	 * Flags which are valid on this platform.  This allows the caller
	 * to distinguish between flags which aren't set vs. flags which can't
	 * be set on this platform.
	 */
	u32 valid_flags;
	/* Flags which can be changed given the current protection state */
	u32 writable_flags;
} __packed;

/*
 * Note: commands 0x14 - 0x19 version 0 were old commands to get/set flash
 * write protect.  These commands may be reused with version > 0.
 */

/* Get the region offset/size */
#define EC_CMD_FLASH_REGION_INFO 0x16
#define EC_VER_FLASH_REGION_INFO 1

enum ec_flash_region {
	/* Region which holds read-only EC image */
	EC_FLASH_REGION_RO = 0,
	/* Region which holds rewritable EC image */
	EC_FLASH_REGION_RW,
	/*
	 * Region which should be write-protected in the factory (a superset of
	 * EC_FLASH_REGION_RO)
	 */
	EC_FLASH_REGION_WP_RO,
	/* Number of regions */
	EC_FLASH_REGION_COUNT,
};

struct ec_params_flash_region_info {
	u32 region;  /* enum ec_flash_region */
} __packed;

struct ec_response_flash_region_info {
	u32 offset;
	u32 size;
} __packed;

/* Read/write VbNvContext */
#define EC_CMD_VBNV_CONTEXT 0x17
#define EC_VER_VBNV_CONTEXT 1
#define EC_VBNV_BLOCK_SIZE 16

enum ec_vbnvcontext_op {
	EC_VBNV_CONTEXT_OP_READ,
	EC_VBNV_CONTEXT_OP_WRITE,
};

struct ec_params_vbnvcontext {
	u32 op;
	u8 block[EC_VBNV_BLOCK_SIZE];
} __packed;

struct ec_response_vbnvcontext {
	u8 block[EC_VBNV_BLOCK_SIZE];
} __packed;

/*****************************************************************************/
/* PWM commands */

/* Get fan target RPM */
#define EC_CMD_PWM_GET_FAN_TARGET_RPM 0x20

struct ec_response_pwm_get_fan_rpm {
	u32 rpm;
} __packed;

/* Set target fan RPM */
#define EC_CMD_PWM_SET_FAN_TARGET_RPM 0x21

/* Version 0 of input params */
struct ec_params_pwm_set_fan_target_rpm_v0 {
	u32 rpm;
} __packed;

/* Version 1 of input params */
struct ec_params_pwm_set_fan_target_rpm_v1 {
	u32 rpm;
	u8 fan_idx;
} __packed;

/* Get keyboard backlight */
#define EC_CMD_PWM_GET_KEYBOARD_BACKLIGHT 0x22

struct ec_response_pwm_get_keyboard_backlight {
	u8 percent;
	u8 enabled;
} __packed;

/* Set keyboard backlight */
#define EC_CMD_PWM_SET_KEYBOARD_BACKLIGHT 0x23

struct ec_params_pwm_set_keyboard_backlight {
	u8 percent;
} __packed;

/* Set target fan PWM duty cycle */
#define EC_CMD_PWM_SET_FAN_DUTY 0x24

/* Version 0 of input params */
struct ec_params_pwm_set_fan_duty_v0 {
	u32 percent;
} __packed;

/* Version 1 of input params */
struct ec_params_pwm_set_fan_duty_v1 {
	u32 percent;
	u8 fan_idx;
} __packed;

/*****************************************************************************/
/*
 * Lightbar commands. This looks worse than it is. Since we only use one HOST
 * command to say "talk to the lightbar", we put the "and tell it to do X" part
 * into a subcommand. We'll make separate structs for subcommands with
 * different input args, so that we know how much to expect.
 */
#define EC_CMD_LIGHTBAR_CMD 0x28

struct rgb_s {
	u8 r, g, b;
};

#define LB_BATTERY_LEVELS 4
/* List of tweakable parameters. NOTE: It's __packed so it can be sent in a
 * host command, but the alignment is the same regardless. Keep it that way.
 */
struct lightbar_params_v0 {
	/* Timing */
	i32 google_ramp_up;
	i32 google_ramp_down;
	i32 s3s0_ramp_up;
	i32 s0_tick_delay[2];		/* AC=0/1 */
	i32 s0a_tick_delay[2];		/* AC=0/1 */
	i32 s0s3_ramp_down;
	i32 s3_sleep_for;
	i32 s3_ramp_up;
	i32 s3_ramp_down;

	/* Oscillation */
	u8 new_s0;
	u8 osc_min[2];			/* AC=0/1 */
	u8 osc_max[2];			/* AC=0/1 */
	u8 w_ofs[2];			/* AC=0/1 */

	/* Brightness limits based on the backlight and AC. */
	u8 bright_bl_off_fixed[2];		/* AC=0/1 */
	u8 bright_bl_on_min[2];		/* AC=0/1 */
	u8 bright_bl_on_max[2];		/* AC=0/1 */

	/* Battery level thresholds */
	u8 battery_threshold[LB_BATTERY_LEVELS - 1];

	/* Map [AC][battery_level] to color index */
	u8 s0_idx[2][LB_BATTERY_LEVELS];	/* AP is running */
	u8 s3_idx[2][LB_BATTERY_LEVELS];	/* AP is sleeping */

	/* Color palette */
	struct rgb_s color[8];			/* 0-3 are Google colors */
} __packed;

struct lightbar_params_v1 {
	/* Timing */
	i32 google_ramp_up;
	i32 google_ramp_down;
	i32 s3s0_ramp_up;
	i32 s0_tick_delay[2];		/* AC=0/1 */
	i32 s0a_tick_delay[2];		/* AC=0/1 */
	i32 s0s3_ramp_down;
	i32 s3_sleep_for;
	i32 s3_ramp_up;
	i32 s3_ramp_down;
	i32 s5_ramp_up;
	i32 s5_ramp_down;
	i32 tap_tick_delay;
	i32 tap_gate_delay;
	i32 tap_display_time;

	/* Tap-for-battery params */
	u8 tap_pct_red;
	u8 tap_pct_green;
	u8 tap_seg_min_on;
	u8 tap_seg_max_on;
	u8 tap_seg_osc;
	u8 tap_idx[3];

	/* Oscillation */
	u8 osc_min[2];			/* AC=0/1 */
	u8 osc_max[2];			/* AC=0/1 */
	u8 w_ofs[2];			/* AC=0/1 */

	/* Brightness limits based on the backlight and AC. */
	u8 bright_bl_off_fixed[2];		/* AC=0/1 */
	u8 bright_bl_on_min[2];		/* AC=0/1 */
	u8 bright_bl_on_max[2];		/* AC=0/1 */

	/* Battery level thresholds */
	u8 battery_threshold[LB_BATTERY_LEVELS - 1];

	/* Map [AC][battery_level] to color index */
	u8 s0_idx[2][LB_BATTERY_LEVELS];	/* AP is running */
	u8 s3_idx[2][LB_BATTERY_LEVELS];	/* AP is sleeping */

	/* s5: single color pulse on inhibited power-up */
	u8 s5_idx;

	/* Color palette */
	struct rgb_s color[8];			/* 0-3 are Google colors */
} __packed;

/* Lightbyte program. */
#define EC_LB_PROG_LEN 192
struct lightbar_program {
	u8 size;
	u8 data[EC_LB_PROG_LEN];
};

struct ec_params_lightbar {
	u8 cmd;		      /* Command (see enum lightbar_command) */
	union {
		struct {
			/* no args */
		} dump, off, on, init, get_seq, get_params_v0, get_params_v1,
			version, get_brightness, get_demo, suspend, resume;

		struct {
			u8 num;
		} set_brightness, seq, demo;

		struct {
			u8 ctrl, reg, value;
		} reg;

		struct {
			u8 led, red, green, blue;
		} set_rgb;

		struct {
			u8 led;
		} get_rgb;

		struct {
			u8 enable;
		} manual_suspend_ctrl;

		struct lightbar_params_v0 set_params_v0;
		struct lightbar_params_v1 set_params_v1;
		struct lightbar_program set_program;
	};
} __packed;

struct ec_response_lightbar {
	union {
		struct {
			struct {
				u8 reg;
				u8 ic0;
				u8 ic1;
			} vals[23];
		} dump;

		struct  {
			u8 num;
		} get_seq, get_brightness, get_demo;

		struct lightbar_params_v0 get_params_v0;
		struct lightbar_params_v1 get_params_v1;

		struct {
			u32 num;
			u32 flags;
		} version;

		struct {
			u8 red, green, blue;
		} get_rgb;

		struct {
			/* no return params */
		} off, on, init, set_brightness, seq, reg, set_rgb,
			demo, set_params_v0, set_params_v1,
			set_program, manual_suspend_ctrl, suspend, resume;
	};
} __packed;

/* Lightbar commands */
enum lightbar_command {
	LIGHTBAR_CMD_DUMP = 0,
	LIGHTBAR_CMD_OFF = 1,
	LIGHTBAR_CMD_ON = 2,
	LIGHTBAR_CMD_INIT = 3,
	LIGHTBAR_CMD_SET_BRIGHTNESS = 4,
	LIGHTBAR_CMD_SEQ = 5,
	LIGHTBAR_CMD_REG = 6,
	LIGHTBAR_CMD_SET_RGB = 7,
	LIGHTBAR_CMD_GET_SEQ = 8,
	LIGHTBAR_CMD_DEMO = 9,
	LIGHTBAR_CMD_GET_PARAMS_V0 = 10,
	LIGHTBAR_CMD_SET_PARAMS_V0 = 11,
	LIGHTBAR_CMD_VERSION = 12,
	LIGHTBAR_CMD_GET_BRIGHTNESS = 13,
	LIGHTBAR_CMD_GET_RGB = 14,
	LIGHTBAR_CMD_GET_DEMO = 15,
	LIGHTBAR_CMD_GET_PARAMS_V1 = 16,
	LIGHTBAR_CMD_SET_PARAMS_V1 = 17,
	LIGHTBAR_CMD_SET_PROGRAM = 18,
	LIGHTBAR_CMD_MANUAL_SUSPEND_CTRL = 19,
	LIGHTBAR_CMD_SUSPEND = 20,
	LIGHTBAR_CMD_RESUME = 21,
	LIGHTBAR_NUM_CMDS
};

/*****************************************************************************/
/* LED control commands */

#define EC_CMD_LED_CONTROL 0x29

enum ec_led_id {
	/* LED to indicate battery state of charge */
	EC_LED_ID_BATTERY_LED = 0,
	/*
	 * LED to indicate system power state (on or in suspend).
	 * May be on power button or on C-panel.
	 */
	EC_LED_ID_POWER_LED,
	/* LED on power adapter or its plug */
	EC_LED_ID_ADAPTER_LED,

	EC_LED_ID_COUNT
};

/* LED control flags */
#define EC_LED_FLAGS_QUERY (1 << 0) /* Query LED capability only */
#define EC_LED_FLAGS_AUTO  (1 << 1) /* Switch LED back to automatic control */

enum ec_led_colors {
	EC_LED_COLOR_RED = 0,
	EC_LED_COLOR_GREEN,
	EC_LED_COLOR_BLUE,
	EC_LED_COLOR_YELLOW,
	EC_LED_COLOR_WHITE,

	EC_LED_COLOR_COUNT
};

struct ec_params_led_control {
	u8 led_id;     /* Which LED to control */
	u8 flags;      /* Control flags */

	u8 brightness[EC_LED_COLOR_COUNT];
} __packed;

struct ec_response_led_control {
	/*
	 * Available brightness value range.
	 *
	 * Range 0 means color channel not present.
	 * Range 1 means on/off control.
	 * Other values means the LED is control by PWM.
	 */
	u8 brightness_range[EC_LED_COLOR_COUNT];
} __packed;

/*****************************************************************************/
/* Verified boot commands */

/*
 * Note: command code 0x29 version 0 was VBOOT_CMD in Link EVT; it may be
 * reused for other purposes with version > 0.
 */

/* Verified boot hash command */
#define EC_CMD_VBOOT_HASH 0x2A

struct ec_params_vboot_hash {
	u8 cmd;             /* enum ec_vboot_hash_cmd */
	u8 hash_type;       /* enum ec_vboot_hash_type */
	u8 nonce_size;      /* Nonce size; may be 0 */
	u8 reserved0;       /* Reserved; set 0 */
	u32 offset;         /* Offset in flash to hash */
	u32 size;           /* Number of bytes to hash */
	u8 nonce_data[64];  /* Nonce data; ignored if nonce_size=0 */
} __packed;

struct ec_response_vboot_hash {
	u8 status;          /* enum ec_vboot_hash_status */
	u8 hash_type;       /* enum ec_vboot_hash_type */
	u8 digest_size;     /* Size of hash digest in bytes */
	u8 reserved0;       /* Ignore; will be 0 */
	u32 offset;         /* Offset in flash which was hashed */
	u32 size;           /* Number of bytes hashed */
	u8 hash_digest[64]; /* Hash digest data */
} __packed;

enum ec_vboot_hash_cmd {
	EC_VBOOT_HASH_GET = 0,       /* Get current hash status */
	EC_VBOOT_HASH_ABORT = 1,     /* Abort calculating current hash */
	EC_VBOOT_HASH_START = 2,     /* Start computing a new hash */
	EC_VBOOT_HASH_RECALC = 3,    /* Synchronously compute a new hash */
};

enum ec_vboot_hash_type {
	EC_VBOOT_HASH_TYPE_SHA256 = 0, /* SHA-256 */
};

enum ec_vboot_hash_status {
	EC_VBOOT_HASH_STATUS_NONE = 0, /* No hash (not started, or aborted) */
	EC_VBOOT_HASH_STATUS_DONE = 1, /* Finished computing a hash */
	EC_VBOOT_HASH_STATUS_BUSY = 2, /* Busy computing a hash */
};

/*
 * Special values for offset for EC_VBOOT_HASH_START and EC_VBOOT_HASH_RECALC.
 * If one of these is specified, the EC will automatically update offset and
 * size to the correct values for the specified image (RO or RW).
 */
#define EC_VBOOT_HASH_OFFSET_RO 0xfffffffe
#define EC_VBOOT_HASH_OFFSET_RW 0xfffffffd

/*****************************************************************************/
/*
 * Motion sense commands. We'll make separate structs for sub-commands with
 * different input args, so that we know how much to expect.
 */
#define EC_CMD_MOTION_SENSE_CMD 0x2B

/* Motion sense commands */
enum motionsense_command {
	/*
	 * Dump command returns all motion sensor data including motion sense
	 * module flags and individual sensor flags.
	 */
	MOTIONSENSE_CMD_DUMP = 0,

	/*
	 * Info command returns data describing the details of a given sensor,
	 * including enum motionsensor_type, enum motionsensor_location, and
	 * enum motionsensor_chip.
	 */
	MOTIONSENSE_CMD_INFO = 1,

	/*
	 * EC Rate command is a setter/getter command for the EC sampling rate
	 * of all motion sensors in milliseconds.
	 */
	MOTIONSENSE_CMD_EC_RATE = 2,

	/*
	 * Sensor ODR command is a setter/getter command for the output data
	 * rate of a specific motion sensor in millihertz.
	 */
	MOTIONSENSE_CMD_SENSOR_ODR = 3,

	/*
	 * Sensor range command is a setter/getter command for the range of
	 * a specified motion sensor in +/-G's or +/- deg/s.
	 */
	MOTIONSENSE_CMD_SENSOR_RANGE = 4,

	/*
	 * Setter/getter command for the keyboard wake angle. When the lid
	 * angle is greater than this value, keyboard wake is disabled in S3,
	 * and when the lid angle goes less than this value, keyboard wake is
	 * enabled. Note, the lid angle measurement is an approximate,
	 * un-calibrated value, hence the wake angle isn't exact.
	 */
	MOTIONSENSE_CMD_KB_WAKE_ANGLE = 5,

	/* Number of motionsense sub-commands. */
	MOTIONSENSE_NUM_CMDS
};

/* List of motion sensor types. */
enum motionsensor_type {
	MOTIONSENSE_TYPE_ACCEL = 0,
	MOTIONSENSE_TYPE_GYRO = 1,
};

/* List of motion sensor locations. */
enum motionsensor_location {
	MOTIONSENSE_LOC_BASE = 0,
	MOTIONSENSE_LOC_LID = 1,
};

/* List of motion sensor chips. */
enum motionsensor_chip {
	MOTIONSENSE_CHIP_KXCJ9 = 0,
	MOTIONSENSE_CHIP_LSM6DS0 = 1,
};

/* Module flag masks used for the dump sub-command. */
#define MOTIONSENSE_MODULE_FLAG_ACTIVE (1<<0)

/* Sensor flag masks used for the dump sub-command. */
#define MOTIONSENSE_SENSOR_FLAG_PRESENT (1<<0)

/*
 * Send this value for the data element to only perform a read. If you
 * send any other value, the EC will interpret it as data to set and will
 * return the actual value set.
 */
#define EC_MOTION_SENSE_NO_VALUE -1

struct ec_params_motion_sense {
	u8 cmd;
	union {
		/* Used for MOTIONSENSE_CMD_DUMP */
		struct {
			/*
			 * Maximal number of sensor the host is expecting.
			 * 0 means the host is only interested in the number
			 * of sensors controlled by the EC.
			 */
			u8 max_sensor_count;
		} dump;

		/*
		 * Used for MOTIONSENSE_CMD_EC_RATE and
		 * MOTIONSENSE_CMD_KB_WAKE_ANGLE.
		 */
		struct {
			/* Data to set or EC_MOTION_SENSE_NO_VALUE to read. */
			i16 data;
		} ec_rate, kb_wake_angle;

		/* Used for MOTIONSENSE_CMD_INFO. */
		struct {
			u8 sensor_num;
		} info;

		/*
		 * Used for MOTIONSENSE_CMD_SENSOR_ODR and
		 * MOTIONSENSE_CMD_SENSOR_RANGE.
		 */
		struct {
			u8 sensor_num;

			/* Rounding flag, true for round-up, false for down. */
			u8 roundup;

			u16 reserved;

			/* Data to set or EC_MOTION_SENSE_NO_VALUE to read. */
			i32 data;
		} sensor_odr, sensor_range;
	};
} __packed;

struct ec_response_motion_sensor_data {
	/* Flags for each sensor. */
	u8 flags;
	u8 padding;

	/* Each sensor is up to 3-axis. */
	i16 data[3];
} __packed;

struct ec_response_motion_sense {
	union {
		/* Used for MOTIONSENSE_CMD_DUMP */
		struct {
			/* Flags representing the motion sensor module. */
			u8 module_flags;

			/* Number of sensors managed directly by the EC */
			u8 sensor_count;

			/*
			 * sensor data is truncated if response_max is too small
			 * for holding all the data.
			 */
			struct ec_response_motion_sensor_data sensor[0];
		} dump;

		/* Used for MOTIONSENSE_CMD_INFO. */
		struct {
			/* Should be element of enum motionsensor_type. */
			u8 type;

			/* Should be element of enum motionsensor_location. */
			u8 location;

			/* Should be element of enum motionsensor_chip. */
			u8 chip;
		} info;

		/*
		 * Used for MOTIONSENSE_CMD_EC_RATE, MOTIONSENSE_CMD_SENSOR_ODR,
		 * MOTIONSENSE_CMD_SENSOR_RANGE, and
		 * MOTIONSENSE_CMD_KB_WAKE_ANGLE.
		 */
		struct {
			/* Current value of the parameter queried. */
			i32 ret;
		} ec_rate, sensor_odr, sensor_range, kb_wake_angle;
	};
} __packed;

/*****************************************************************************/
/* Force lid open command */

/* Make lid event always open */
#define EC_CMD_FORCE_LID_OPEN 0x2c

struct ec_params_force_lid_open {
	u8 enabled;
} __packed;

/*****************************************************************************/
/* USB charging control commands */

/* Set USB port charging mode */
#define EC_CMD_USB_CHARGE_SET_MODE 0x30

struct ec_params_usb_charge_set_mode {
	u8 usb_port_id;
	u8 mode;
} __packed;

/*****************************************************************************/
/* Persistent storage for host */

/* Maximum bytes that can be read/written in a single command */
#define EC_PSTORE_SIZE_MAX 64

/* Get persistent storage info */
#define EC_CMD_PSTORE_INFO 0x40

struct ec_response_pstore_info {
	/* Persistent storage size, in bytes */
	u32 pstore_size;
	/* Access size; read/write offset and size must be a multiple of this */
	u32 access_size;
} __packed;

/*
 * Read persistent storage
 *
 * Response is params.size bytes of data.
 */
#define EC_CMD_PSTORE_READ 0x41

struct ec_params_pstore_read {
	u32 offset;   /* Byte offset to read */
	u32 size;     /* Size to read in bytes */
} __packed;

/* Write persistent storage */
#define EC_CMD_PSTORE_WRITE 0x42

struct ec_params_pstore_write {
	u32 offset;   /* Byte offset to write */
	u32 size;     /* Size to write in bytes */
	u8 data[EC_PSTORE_SIZE_MAX];
} __packed;

/*****************************************************************************/
/* Real-time clock */

/* RTC params and response structures */
struct ec_params_rtc {
	u32 time;
} __packed;

struct ec_response_rtc {
	u32 time;
} __packed;

/* These use ec_response_rtc */
#define EC_CMD_RTC_GET_VALUE 0x44
#define EC_CMD_RTC_GET_ALARM 0x45

/* These all use ec_params_rtc */
#define EC_CMD_RTC_SET_VALUE 0x46
#define EC_CMD_RTC_SET_ALARM 0x47

/*****************************************************************************/
/* Port80 log access */

/* Maximum entries that can be read/written in a single command */
#define EC_PORT80_SIZE_MAX 32

/* Get last port80 code from previous boot */
#define EC_CMD_PORT80_LAST_BOOT 0x48
#define EC_CMD_PORT80_READ 0x48

enum ec_port80_subcmd {
	EC_PORT80_GET_INFO = 0,
	EC_PORT80_READ_BUFFER,
};

struct ec_params_port80_read {
	u16 subcmd;
	union {
		struct {
			u32 offset;
			u32 num_entries;
		} read_buffer;
	};
} __packed;

struct ec_response_port80_read {
	union {
		struct {
			u32 writes;
			u32 history_size;
			u32 last_boot;
		} get_info;
		struct {
			u16 codes[EC_PORT80_SIZE_MAX];
		} data;
	};
} __packed;

struct ec_response_port80_last_boot {
	u16 code;
} __packed;

/*****************************************************************************/
/* Thermal engine commands. Note that there are two implementations. We'll
 * reuse the command number, but the data and behavior is incompatible.
 * Version 0 is what originally shipped on Link.
 * Version 1 separates the CPU thermal limits from the fan control.
 */

#define EC_CMD_THERMAL_SET_THRESHOLD 0x50
#define EC_CMD_THERMAL_GET_THRESHOLD 0x51

/* The version 0 structs are opaque. You have to know what they are for
 * the get/set commands to make any sense.
 */

/* Version 0 - set */
struct ec_params_thermal_set_threshold {
	u8 sensor_type;
	u8 threshold_id;
	u16 value;
} __packed;

/* Version 0 - get */
struct ec_params_thermal_get_threshold {
	u8 sensor_type;
	u8 threshold_id;
} __packed;

struct ec_response_thermal_get_threshold {
	u16 value;
} __packed;


/* The version 1 structs are visible. */
enum ec_temp_thresholds {
	EC_TEMP_THRESH_WARN = 0,
	EC_TEMP_THRESH_HIGH,
	EC_TEMP_THRESH_HALT,

	EC_TEMP_THRESH_COUNT
};

/* Thermal configuration for one temperature sensor. Temps are in degrees K.
 * Zero values will be silently ignored by the thermal task.
 */
struct ec_thermal_config {
	u32 temp_host[EC_TEMP_THRESH_COUNT]; /* levels of hotness */
	u32 temp_fan_off;		/* no active cooling needed */
	u32 temp_fan_max;		/* max active cooling needed */
} __packed;

/* Version 1 - get config for one sensor. */
struct ec_params_thermal_get_threshold_v1 {
	u32 sensor_num;
} __packed;
/* This returns a struct ec_thermal_config */

/* Version 1 - set config for one sensor.
 * Use read-modify-write for best results! */
struct ec_params_thermal_set_threshold_v1 {
	u32 sensor_num;
	struct ec_thermal_config cfg;
} __packed;
/* This returns no data */

/****************************************************************************/

/* Toggle automatic fan control */
#define EC_CMD_THERMAL_AUTO_FAN_CTRL 0x52

/* Version 1 of input params */
struct ec_params_auto_fan_ctrl_v1 {
	u8 fan_idx;
} __packed;

/* Get/Set TMP006 calibration data */
#define EC_CMD_TMP006_GET_CALIBRATION 0x53
#define EC_CMD_TMP006_SET_CALIBRATION 0x54

/*
 * The original TMP006 calibration only needed four params, but now we need
 * more. Since the algorithm is nothing but magic numbers anyway, we'll leave
 * the params opaque. The v1 "get" response will include the algorithm number
 * and how many params it requires. That way we can change the EC code without
 * needing to update this file. We can also use a different algorithm on each
 * sensor.
 */

/* This is the same struct for both v0 and v1. */
struct ec_params_tmp006_get_calibration {
	u8 index;
} __packed;

/* Version 0 */
struct ec_response_tmp006_get_calibration_v0 {
	float s0;
	float b0;
	float b1;
	float b2;
} __packed;

struct ec_params_tmp006_set_calibration_v0 {
	u8 index;
	u8 reserved[3];
	float s0;
	float b0;
	float b1;
	float b2;
} __packed;

/* Version 1 */
struct ec_response_tmp006_get_calibration_v1 {
	u8 algorithm;
	u8 num_params;
	u8 reserved[2];
	float val[0];
} __packed;

struct ec_params_tmp006_set_calibration_v1 {
	u8 index;
	u8 algorithm;
	u8 num_params;
	u8 reserved;
	float val[0];
} __packed;


/* Read raw TMP006 data */
#define EC_CMD_TMP006_GET_RAW 0x55

struct ec_params_tmp006_get_raw {
	u8 index;
} __packed;

struct ec_response_tmp006_get_raw {
	i32 t;  /* In 1/100 K */
	i32 v;  /* In nV */
};

/*****************************************************************************/
/* MKBP - Matrix KeyBoard Protocol */

/*
 * Read key state
 *
 * Returns raw data for keyboard cols; see ec_response_mkbp_info.cols for
 * expected response size.
 */
#define EC_CMD_MKBP_STATE 0x60

/* Provide information about the matrix : number of rows and columns */
#define EC_CMD_MKBP_INFO 0x61

struct ec_response_mkbp_info {
	u32 rows;
	u32 cols;
	u8 switches;
} __packed;

/* Simulate key press */
#define EC_CMD_MKBP_SIMULATE_KEY 0x62

struct ec_params_mkbp_simulate_key {
	u8 col;
	u8 row;
	u8 pressed;
} __packed;

/* Configure keyboard scanning */
#define EC_CMD_MKBP_SET_CONFIG 0x64
#define EC_CMD_MKBP_GET_CONFIG 0x65

/* flags */
enum mkbp_config_flags {
	EC_MKBP_FLAGS_ENABLE = 1,	/* Enable keyboard scanning */
};

enum mkbp_config_valid {
	EC_MKBP_VALID_SCAN_PERIOD		= 1 << 0,
	EC_MKBP_VALID_POLL_TIMEOUT		= 1 << 1,
	EC_MKBP_VALID_MIN_POST_SCAN_DELAY	= 1 << 3,
	EC_MKBP_VALID_OUTPUT_SETTLE		= 1 << 4,
	EC_MKBP_VALID_DEBOUNCE_DOWN		= 1 << 5,
	EC_MKBP_VALID_DEBOUNCE_UP		= 1 << 6,
	EC_MKBP_VALID_FIFO_MAX_DEPTH		= 1 << 7,
};

/* Configuration for our key scanning algorithm */
struct ec_mkbp_config {
	u32 valid_mask;		/* valid fields */
	u8 flags;		/* some flags (enum mkbp_config_flags) */
	u8 valid_flags;		/* which flags are valid */
	u16 scan_period_us;	/* period between start of scans */
	/* revert to interrupt mode after no activity for this long */
	u32 poll_timeout_us;
	/*
	 * minimum post-scan relax time. Once we finish a scan we check
	 * the time until we are due to start the next one. If this time is
	 * shorter this field, we use this instead.
	 */
	u16 min_post_scan_delay_us;
	/* delay between setting up output and waiting for it to settle */
	u16 output_settle_us;
	u16 debounce_down_us;	/* time for debounce on key down */
	u16 debounce_up_us;	/* time for debounce on key up */
	/* maximum depth to allow for fifo (0 = no keyscan output) */
	u8 fifo_max_depth;
} __packed;

struct ec_params_mkbp_set_config {
	struct ec_mkbp_config config;
} __packed;

struct ec_response_mkbp_get_config {
	struct ec_mkbp_config config;
} __packed;

/* Run the key scan emulation */
#define EC_CMD_KEYSCAN_SEQ_CTRL 0x66

enum ec_keyscan_seq_cmd {
	EC_KEYSCAN_SEQ_STATUS = 0,	/* Get status information */
	EC_KEYSCAN_SEQ_CLEAR = 1,	/* Clear sequence */
	EC_KEYSCAN_SEQ_ADD = 2,		/* Add item to sequence */
	EC_KEYSCAN_SEQ_START = 3,	/* Start running sequence */
	EC_KEYSCAN_SEQ_COLLECT = 4,	/* Collect sequence summary data */
};

enum ec_collect_flags {
	/*
	 * Indicates this scan was processed by the EC. Due to timing, some
	 * scans may be skipped.
	 */
	EC_KEYSCAN_SEQ_FLAG_DONE	= 1 << 0,
};

struct ec_collect_item {
	u8 flags;		/* some flags (enum ec_collect_flags) */
};

struct ec_params_keyscan_seq_ctrl {
	u8 cmd;	/* Command to send (enum ec_keyscan_seq_cmd) */
	union {
		struct {
			u8 active;		/* still active */
			u8 num_items;	/* number of items */
			/* Current item being presented */
			u8 cur_item;
		} status;
		struct {
			/*
			 * Absolute time for this scan, measured from the
			 * start of the sequence.
			 */
			u32 time_us;
			u8 scan[0];	/* keyscan data */
		} add;
		struct {
			u8 start_item;	/* First item to return */
			u8 num_items;	/* Number of items to return */
		} collect;
	};
} __packed;

struct ec_result_keyscan_seq_ctrl {
	union {
		struct {
			u8 num_items;	/* Number of items */
			/* Data for each item */
			struct ec_collect_item item[0];
		} collect;
	};
} __packed;

/*
 * Get the next pending MKBP event.
 *
 * Returns EC_RES_UNAVAILABLE if there is no event pending.
 */
#define EC_CMD_GET_NEXT_EVENT 0x67

enum ec_mkbp_event {
	/* Keyboard matrix changed. The event data is the new matrix state. */
	EC_MKBP_EVENT_KEY_MATRIX = 0,

	/* New host event. The event data is 4 bytes of host event flags. */
	EC_MKBP_EVENT_HOST_EVENT = 1,

	/* Number of MKBP events */
	EC_MKBP_EVENT_COUNT,
};

struct ec_response_get_next_event {
	u8 event_type;
	/* Followed by event data if any */
} __packed;

/*****************************************************************************/
/* Temperature sensor commands */

/* Read temperature sensor info */
#define EC_CMD_TEMP_SENSOR_GET_INFO 0x70

struct ec_params_temp_sensor_get_info {
	u8 id;
} __packed;

struct ec_response_temp_sensor_get_info {
	char sensor_name[32];
	u8 sensor_type;
} __packed;

/*****************************************************************************/

/*
 * Note: host commands 0x80 - 0x87 are reserved to avoid conflict with ACPI
 * commands accidentally sent to the wrong interface.  See the ACPI section
 * below.
 */

/*****************************************************************************/
/* Host event commands */

/*
 * Host event mask params and response structures, shared by all of the host
 * event commands below.
 */
struct ec_params_host_event_mask {
	u32 mask;
} __packed;

struct ec_response_host_event_mask {
	u32 mask;
} __packed;

/* These all use ec_response_host_event_mask */
#define EC_CMD_HOST_EVENT_GET_B         0x87
#define EC_CMD_HOST_EVENT_GET_SMI_MASK  0x88
#define EC_CMD_HOST_EVENT_GET_SCI_MASK  0x89
#define EC_CMD_HOST_EVENT_GET_WAKE_MASK 0x8d

/* These all use ec_params_host_event_mask */
#define EC_CMD_HOST_EVENT_SET_SMI_MASK  0x8a
#define EC_CMD_HOST_EVENT_SET_SCI_MASK  0x8b
#define EC_CMD_HOST_EVENT_CLEAR         0x8c
#define EC_CMD_HOST_EVENT_SET_WAKE_MASK 0x8e
#define EC_CMD_HOST_EVENT_CLEAR_B       0x8f

/*****************************************************************************/
/* Switch commands */

/* Enable/disable LCD backlight */
#define EC_CMD_SWITCH_ENABLE_BKLIGHT 0x90

struct ec_params_switch_enable_backlight {
	u8 enabled;
} __packed;

/* Enable/disable WLAN/Bluetooth */
#define EC_CMD_SWITCH_ENABLE_WIRELESS 0x91
#define EC_VER_SWITCH_ENABLE_WIRELESS 1

/* Version 0 params; no response */
struct ec_params_switch_enable_wireless_v0 {
	u8 enabled;
} __packed;

/* Version 1 params */
struct ec_params_switch_enable_wireless_v1 {
	/* Flags to enable now */
	u8 now_flags;

	/* Which flags to copy from now_flags */
	u8 now_mask;

	/*
	 * Flags to leave enabled in S3, if they're on at the S0->S3
	 * transition.  (Other flags will be disabled by the S0->S3
	 * transition.)
	 */
	u8 suspend_flags;

	/* Which flags to copy from suspend_flags */
	u8 suspend_mask;
} __packed;

/* Version 1 response */
struct ec_response_switch_enable_wireless_v1 {
	/* Flags to enable now */
	u8 now_flags;

	/* Flags to leave enabled in S3 */
	u8 suspend_flags;
} __packed;

/*****************************************************************************/
/* GPIO commands. Only available on EC if write protect has been disabled. */

/* Set GPIO output value */
#define EC_CMD_GPIO_SET 0x92

struct ec_params_gpio_set {
	char name[32];
	u8 val;
} __packed;

/* Get GPIO value */
#define EC_CMD_GPIO_GET 0x93

/* Version 0 of input params and response */
struct ec_params_gpio_get {
	char name[32];
} __packed;
struct ec_response_gpio_get {
	u8 val;
} __packed;

/* Version 1 of input params and response */
struct ec_params_gpio_get_v1 {
	u8 subcmd;
	union {
		struct {
			char name[32];
		} get_value_by_name;
		struct {
			u8 index;
		} get_info;
	};
} __packed;

struct ec_response_gpio_get_v1 {
	union {
		struct {
			u8 val;
		} get_value_by_name, get_count;
		struct {
			u8 val;
			char name[32];
			u32 flags;
		} get_info;
	};
} __packed;

enum gpio_get_subcmd {
	EC_GPIO_GET_BY_NAME = 0,
	EC_GPIO_GET_COUNT = 1,
	EC_GPIO_GET_INFO = 2,
};

/*****************************************************************************/
/* I2C commands. Only available when flash write protect is unlocked. */

/*
 * TODO(crosbug.com/p/23570): These commands are deprecated, and will be
 * removed soon.  Use EC_CMD_I2C_XFER instead.
 */

/* Read I2C bus */
#define EC_CMD_I2C_READ 0x94

struct ec_params_i2c_read {
	u16 addr; /* 8-bit address (7-bit shifted << 1) */
	u8 read_size; /* Either 8 or 16. */
	u8 port;
	u8 offset;
} __packed;
struct ec_response_i2c_read {
	u16 data;
} __packed;

/* Write I2C bus */
#define EC_CMD_I2C_WRITE 0x95

struct ec_params_i2c_write {
	u16 data;
	u16 addr; /* 8-bit address (7-bit shifted << 1) */
	u8 write_size; /* Either 8 or 16. */
	u8 port;
	u8 offset;
} __packed;

/*****************************************************************************/
/* Charge state commands. Only available when flash write protect unlocked. */

/* Force charge state machine to stop charging the battery or force it to
 * discharge the battery.
 */
#define EC_CMD_CHARGE_CONTROL 0x96
#define EC_VER_CHARGE_CONTROL 1

enum ec_charge_control_mode {
	CHARGE_CONTROL_NORMAL = 0,
	CHARGE_CONTROL_IDLE,
	CHARGE_CONTROL_DISCHARGE,
};

struct ec_params_charge_control {
	u32 mode;  /* enum charge_control_mode */
} __packed;

/*****************************************************************************/
/* Console commands. Only available when flash write protect is unlocked. */

/* Snapshot console output buffer for use by EC_CMD_CONSOLE_READ. */
#define EC_CMD_CONSOLE_SNAPSHOT 0x97

/*
 * Read next chunk of data from saved snapshot.
 *
 * Response is null-terminated string.  Empty string, if there is no more
 * remaining output.
 */
#define EC_CMD_CONSOLE_READ 0x98

/*****************************************************************************/

/*
 * Cut off battery power immediately or after the host has shut down.
 *
 * return EC_RES_INVALID_COMMAND if unsupported by a board/battery.
 *	  EC_RES_SUCCESS if the command was successful.
 *	  EC_RES_ERROR if the cut off command failed.
 */
#define EC_CMD_BATTERY_CUT_OFF 0x99

#define EC_BATTERY_CUTOFF_FLAG_AT_SHUTDOWN	(1 << 0)

struct ec_params_battery_cutoff {
	u8 flags;
} __packed;

/*****************************************************************************/
/* USB port mux control. */

/*
 * Switch USB mux or return to automatic switching.
 */
#define EC_CMD_USB_MUX 0x9a

struct ec_params_usb_mux {
	u8 mux;
} __packed;

/*****************************************************************************/
/* LDOs / FETs control. */

enum ec_ldo_state {
	EC_LDO_STATE_OFF = 0,	/* the LDO / FET is shut down */
	EC_LDO_STATE_ON = 1,	/* the LDO / FET is ON / providing power */
};

/*
 * Switch on/off a LDO.
 */
#define EC_CMD_LDO_SET 0x9b

struct ec_params_ldo_set {
	u8 index;
	u8 state;
} __packed;

/*
 * Get LDO state.
 */
#define EC_CMD_LDO_GET 0x9c

struct ec_params_ldo_get {
	u8 index;
} __packed;

struct ec_response_ldo_get {
	u8 state;
} __packed;

/*****************************************************************************/
/* Power info. */

/*
 * Get power info.
 */
#define EC_CMD_POWER_INFO 0x9d

struct ec_response_power_info {
	u32 usb_dev_type;
	u16 voltage_ac;
	u16 voltage_system;
	u16 current_system;
	u16 usb_current_limit;
} __packed;

/*****************************************************************************/
/* I2C passthru command */

#define EC_CMD_I2C_PASSTHRU 0x9e

/* Read data; if not present, message is a write */
#define EC_I2C_FLAG_READ	(1 << 15)

/* Mask for address */
#define EC_I2C_ADDR_MASK	0x3ff

#define EC_I2C_STATUS_NAK	(1 << 0) /* Transfer was not acknowledged */
#define EC_I2C_STATUS_TIMEOUT	(1 << 1) /* Timeout during transfer */

/* Any error */
#define EC_I2C_STATUS_ERROR	(EC_I2C_STATUS_NAK | EC_I2C_STATUS_TIMEOUT)

struct ec_params_i2c_passthru_msg {
	u16 addr_flags;	/* I2C slave address (7 or 10 bits) and flags */
	u16 len;		/* Number of bytes to read or write */
} __packed;

struct ec_params_i2c_passthru {
	u8 port;		/* I2C port number */
	u8 num_msgs;	/* Number of messages */
	struct ec_params_i2c_passthru_msg msg[];
	/* Data to write for all messages is concatenated here */
} __packed;

struct ec_response_i2c_passthru {
	u8 i2c_status;	/* Status flags (EC_I2C_STATUS_...) */
	u8 num_msgs;	/* Number of messages processed */
	u8 data[];		/* Data read by messages concatenated here */
} __packed;

/*****************************************************************************/
/* Power button hang detect */

#define EC_CMD_HANG_DETECT 0x9f

/* Reasons to start hang detection timer */
/* Power button pressed */
#define EC_HANG_START_ON_POWER_PRESS  (1 << 0)

/* Lid closed */
#define EC_HANG_START_ON_LID_CLOSE    (1 << 1)

 /* Lid opened */
#define EC_HANG_START_ON_LID_OPEN     (1 << 2)

/* Start of AP S3->S0 transition (booting or resuming from suspend) */
#define EC_HANG_START_ON_RESUME       (1 << 3)

/* Reasons to cancel hang detection */

/* Power button released */
#define EC_HANG_STOP_ON_POWER_RELEASE (1 << 8)

/* Any host command from AP received */
#define EC_HANG_STOP_ON_HOST_COMMAND  (1 << 9)

/* Stop on end of AP S0->S3 transition (suspending or shutting down) */
#define EC_HANG_STOP_ON_SUSPEND       (1 << 10)

/*
 * If this flag is set, all the other fields are ignored, and the hang detect
 * timer is started.  This provides the AP a way to start the hang timer
 * without reconfiguring any of the other hang detect settings.  Note that
 * you must previously have configured the timeouts.
 */
#define EC_HANG_START_NOW             (1 << 30)

/*
 * If this flag is set, all the other fields are ignored (including
 * EC_HANG_START_NOW).  This provides the AP a way to stop the hang timer
 * without reconfiguring any of the other hang detect settings.
 */
#define EC_HANG_STOP_NOW              (1 << 31)

struct ec_params_hang_detect {
	/* Flags; see EC_HANG_* */
	u32 flags;

	/* Timeout in msec before generating host event, if enabled */
	u16 host_event_timeout_msec;

	/* Timeout in msec before generating warm reboot, if enabled */
	u16 warm_reboot_timeout_msec;
} __packed;

/*****************************************************************************/
/* Commands for battery charging */

/*
 * This is the single catch-all host command to exchange data regarding the
 * charge state machine (v2 and up).
 */
#define EC_CMD_CHARGE_STATE 0xa0

/* Subcommands for this host command */
enum charge_state_command {
	CHARGE_STATE_CMD_GET_STATE,
	CHARGE_STATE_CMD_GET_PARAM,
	CHARGE_STATE_CMD_SET_PARAM,
	CHARGE_STATE_NUM_CMDS
};

/*
 * Known param numbers are defined here. Ranges are reserved for board-specific
 * params, which are handled by the particular implementations.
 */
enum charge_state_params {
	CS_PARAM_CHG_VOLTAGE,	      /* charger voltage limit */
	CS_PARAM_CHG_CURRENT,	      /* charger current limit */
	CS_PARAM_CHG_INPUT_CURRENT,   /* charger input current limit */
	CS_PARAM_CHG_STATUS,	      /* charger-specific status */
	CS_PARAM_CHG_OPTION,	      /* charger-specific options */
	/* How many so far? */
	CS_NUM_BASE_PARAMS,

	/* Range for CONFIG_CHARGER_PROFILE_OVERRIDE params */
	CS_PARAM_CUSTOM_PROFILE_MIN = 0x10000,
	CS_PARAM_CUSTOM_PROFILE_MAX = 0x1ffff,

	/* Other custom param ranges go here... */
};

struct ec_params_charge_state {
	u8 cmd;				/* enum charge_state_command */
	union {
		struct {
			/* no args */
		} get_state;

		struct {
			u32 param;		/* enum charge_state_param */
		} get_param;

		struct {
			u32 param;		/* param to set */
			u32 value;		/* value to set */
		} set_param;
	};
} __packed;

struct ec_response_charge_state {
	union {
		struct {
			int ac;
			int chg_voltage;
			int chg_current;
			int chg_input_current;
			int batt_state_of_charge;
		} get_state;

		struct {
			u32 value;
		} get_param;
		struct {
			/* no return values */
		} set_param;
	};
} __packed;


/*
 * Set maximum battery charging current.
 */
#define EC_CMD_CHARGE_CURRENT_LIMIT 0xa1

struct ec_params_current_limit {
	u32 limit; /* in mA */
} __packed;

/*
 * Set maximum external power current.
 */
#define EC_CMD_EXT_POWER_CURRENT_LIMIT 0xa2

struct ec_params_ext_power_current_limit {
	u32 limit; /* in mA */
} __packed;

/*****************************************************************************/
/* Smart battery pass-through */

/* Get / Set 16-bit smart battery registers */
#define EC_CMD_SB_READ_WORD   0xb0
#define EC_CMD_SB_WRITE_WORD  0xb1

/* Get / Set string smart battery parameters
 * formatted as SMBUS "block".
 */
#define EC_CMD_SB_READ_BLOCK  0xb2
#define EC_CMD_SB_WRITE_BLOCK 0xb3

struct ec_params_sb_rd {
	u8 reg;
} __packed;

struct ec_response_sb_rd_word {
	u16 value;
} __packed;

struct ec_params_sb_wr_word {
	u8 reg;
	u16 value;
} __packed;

struct ec_response_sb_rd_block {
	u8 data[32];
} __packed;

struct ec_params_sb_wr_block {
	u8 reg;
	u16 data[32];
} __packed;


/*****************************************************************************/
/* Battery vendor parameters
 *
 * Get or set vendor-specific parameters in the battery. Implementations may
 * differ between boards or batteries. On a set operation, the response
 * contains the actual value set, which may be rounded or clipped from the
 * requested value.
 */

#define EC_CMD_BATTERY_VENDOR_PARAM 0xb4

enum ec_battery_vendor_param_mode {
	BATTERY_VENDOR_PARAM_MODE_GET = 0,
	BATTERY_VENDOR_PARAM_MODE_SET,
};

struct ec_params_battery_vendor_param {
	u32 param;
	u32 value;
	u8 mode;
} __packed;

struct ec_response_battery_vendor_param {
	u32 value;
} __packed;

/*****************************************************************************/
/*
 * Smart Battery Firmware Update Commands
 */
#define EC_CMD_SB_FW_UPDATE 0xb5

enum ec_sb_fw_update_subcmd {
	EC_SB_FW_UPDATE_PREPARE  = 0x0,
	EC_SB_FW_UPDATE_INFO     = 0x1, /*query sb info */
	EC_SB_FW_UPDATE_BEGIN    = 0x2, /*check if protected */
	EC_SB_FW_UPDATE_WRITE    = 0x3, /*check if protected */
	EC_SB_FW_UPDATE_END      = 0x4,
	EC_SB_FW_UPDATE_STATUS   = 0x5,
	EC_SB_FW_UPDATE_PROTECT  = 0x6,
	EC_SB_FW_UPDATE_MAX      = 0x7,
};

#define SB_FW_UPDATE_CMD_WRITE_BLOCK_SIZE 32
#define SB_FW_UPDATE_CMD_STATUS_SIZE 2
#define SB_FW_UPDATE_CMD_INFO_SIZE 8

struct ec_sb_fw_update_header {
	u16 subcmd;  /* enum ec_sb_fw_update_subcmd */
	u16 fw_id;   /* firmware id */
} __packed;

struct ec_params_sb_fw_update {
	struct ec_sb_fw_update_header hdr;
	union {
		/* EC_SB_FW_UPDATE_PREPARE  = 0x0 */
		/* EC_SB_FW_UPDATE_INFO     = 0x1 */
		/* EC_SB_FW_UPDATE_BEGIN    = 0x2 */
		/* EC_SB_FW_UPDATE_END      = 0x4 */
		/* EC_SB_FW_UPDATE_STATUS   = 0x5 */
		/* EC_SB_FW_UPDATE_PROTECT  = 0x6 */
		struct {
			/* no args */
		} dummy;

		/* EC_SB_FW_UPDATE_WRITE    = 0x3 */
		struct {
			u8  data[SB_FW_UPDATE_CMD_WRITE_BLOCK_SIZE];
		} write;
	};
} __packed;

struct ec_response_sb_fw_update {
	union {
		/* EC_SB_FW_UPDATE_INFO     = 0x1 */
		struct {
			u8 data[SB_FW_UPDATE_CMD_INFO_SIZE];
		} info;

		/* EC_SB_FW_UPDATE_STATUS   = 0x5 */
		struct {
			u8 data[SB_FW_UPDATE_CMD_STATUS_SIZE];
		} status;
	};
} __packed;

/*
 * Entering Verified Boot Mode Command
 * Default mode is VBOOT_MODE_NORMAL if EC did not receive this command.
 * Valid Modes are: normal, developer, and recovery.
 */
#define EC_CMD_ENTERING_MODE 0xb6

struct ec_params_entering_mode {
	int vboot_mode;
} __packed;

#define VBOOT_MODE_NORMAL    0
#define VBOOT_MODE_DEVELOPER 1
#define VBOOT_MODE_RECOVERY  2

/*****************************************************************************/
/* System commands */

/*
 * TODO(crosbug.com/p/23747): This is a confusing name, since it doesn't
 * necessarily reboot the EC.  Rename to "image" or something similar?
 */
#define EC_CMD_REBOOT_EC 0xd2

/* Command */
enum ec_reboot_cmd {
	EC_REBOOT_CANCEL = 0,        /* Cancel a pending reboot */
	EC_REBOOT_JUMP_RO = 1,       /* Jump to RO without rebooting */
	EC_REBOOT_JUMP_RW = 2,       /* Jump to RW without rebooting */
	/* (command 3 was jump to RW-B) */
	EC_REBOOT_COLD = 4,          /* Cold-reboot */
	EC_REBOOT_DISABLE_JUMP = 5,  /* Disable jump until next reboot */
	EC_REBOOT_HIBERNATE = 6      /* Hibernate EC */
};

/* Flags for ec_params_reboot_ec.reboot_flags */
#define EC_REBOOT_FLAG_RESERVED0      (1 << 0)  /* Was recovery request */
#define EC_REBOOT_FLAG_ON_AP_SHUTDOWN (1 << 1)  /* Reboot after AP shutdown */

struct ec_params_reboot_ec {
	u8 cmd;           /* enum ec_reboot_cmd */
	u8 flags;         /* See EC_REBOOT_FLAG_* */
} __packed;

/*
 * Get information on last EC panic.
 *
 * Returns variable-length platform-dependent panic information.  See panic.h
 * for details.
 */
#define EC_CMD_GET_PANIC_INFO 0xd3

/*****************************************************************************/
/*
 * Special commands
 *
 * These do not follow the normal rules for commands.  See each command for
 * details.
 */

/*
 * Reboot NOW
 *
 * This command will work even when the EC LPC interface is busy, because the
 * reboot command is processed at interrupt level.  Note that when the EC
 * reboots, the host will reboot too, so there is no response to this command.
 *
 * Use EC_CMD_REBOOT_EC to reboot the EC more politely.
 */
#define EC_CMD_REBOOT 0xd1  /* Think "die" */

/*
 * Resend last response (not supported on LPC).
 *
 * Returns EC_RES_UNAVAILABLE if there is no response available - for example,
 * there was no previous command, or the previous command's response was too
 * big to save.
 */
#define EC_CMD_RESEND_RESPONSE 0xdb

/*
 * This header byte on a command indicate version 0. Any header byte less
 * than this means that we are talking to an old EC which doesn't support
 * versioning. In that case, we assume version 0.
 *
 * Header bytes greater than this indicate a later version. For example,
 * EC_CMD_VERSION0 + 1 means we are using version 1.
 *
 * The old EC interface must not use commands 0xdc or higher.
 */
#define EC_CMD_VERSION0 0xdc

/*****************************************************************************/
/*
 * PD commands
 *
 * These commands are for PD MCU communication.
 */

/* EC to PD MCU exchange status command */
#define EC_CMD_PD_EXCHANGE_STATUS 0x100

enum pd_charge_state {
	PD_CHARGE_NO_CHANGE = 0, /* Don't change charge state */
	PD_CHARGE_NONE,          /* No charging allowed */
	PD_CHARGE_5V,            /* 5V charging only */
	PD_CHARGE_MAX            /* Charge at max voltage */
};

/* Status of EC being sent to PD */
struct ec_params_pd_status {
	i8 batt_soc;      /* battery state of charge */
	u8 charge_state; /* charging state (from enum pd_charge_state) */
} __packed;

/* Status of PD being sent back to EC */
#define PD_STATUS_HOST_EVENT      (1 << 0) /* Forward host event to AP */
#define PD_STATUS_IN_RW           (1 << 1) /* Running RW image */
#define PD_STATUS_JUMPED_TO_IMAGE (1 << 2) /* Current image was jumped to */
struct ec_response_pd_status {
	u32 status;      /* PD MCU status */
	u32 curr_lim_ma; /* input current limit */
	i32 active_charge_port; /* active charging port */
} __packed;

/* AP to PD MCU host event status command, cleared on read */
#define EC_CMD_PD_HOST_EVENT_STATUS 0x104

/* PD MCU host event status bits */
#define PD_EVENT_UPDATE_DEVICE     (1 << 0)
#define PD_EVENT_POWER_CHANGE      (1 << 1)
#define PD_EVENT_IDENTITY_RECEIVED (1 << 2)
struct ec_response_host_event_status {
	u32 status;      /* PD MCU host event status */
} __packed;

/* Set USB type-C port role and muxes */
#define EC_CMD_USB_PD_CONTROL 0x101

enum usb_pd_control_role {
	USB_PD_CTRL_ROLE_NO_CHANGE = 0,
	USB_PD_CTRL_ROLE_TOGGLE_ON = 1, /* == AUTO */
	USB_PD_CTRL_ROLE_TOGGLE_OFF = 2,
	USB_PD_CTRL_ROLE_FORCE_SINK = 3,
	USB_PD_CTRL_ROLE_FORCE_SOURCE = 4,
	USB_PD_CTRL_ROLE_COUNT
};

enum usb_pd_control_mux {
	USB_PD_CTRL_MUX_NO_CHANGE = 0,
	USB_PD_CTRL_MUX_NONE = 1,
	USB_PD_CTRL_MUX_USB = 2,
	USB_PD_CTRL_MUX_DP = 3,
	USB_PD_CTRL_MUX_DOCK = 4,
	USB_PD_CTRL_MUX_AUTO = 5,
	USB_PD_CTRL_MUX_COUNT
};

struct ec_params_usb_pd_control {
	u8 port;
	u8 role;
	u8 mux;
} __packed;

struct ec_response_usb_pd_control {
	u8 enabled;
	u8 role;
	u8 polarity;
	u8 state;
} __packed;

struct ec_response_usb_pd_control_v1 {
	u8 enabled;
	u8 role; /* [0] power: 0=SNK/1=SRC [1] data: 0=UFP/1=DFP */
	u8 polarity;
	char state[32];
} __packed;

#define EC_CMD_USB_PD_PORTS 0x102

struct ec_response_usb_pd_ports {
	u8 num_ports;
} __packed;

#define EC_CMD_USB_PD_POWER_INFO 0x103

#define PD_POWER_CHARGING_PORT 0xff
struct ec_params_usb_pd_power_info {
	u8 port;
} __packed;

enum usb_chg_type {
	USB_CHG_TYPE_NONE,
	USB_CHG_TYPE_PD,
	USB_CHG_TYPE_C,
	USB_CHG_TYPE_PROPRIETARY,
	USB_CHG_TYPE_BC12_DCP,
	USB_CHG_TYPE_BC12_CDP,
	USB_CHG_TYPE_BC12_SDP,
	USB_CHG_TYPE_OTHER,
	USB_CHG_TYPE_VBUS,
};
enum usb_power_roles {
	USB_PD_PORT_POWER_DISCONNECTED,
	USB_PD_PORT_POWER_SOURCE,
	USB_PD_PORT_POWER_SINK,
	USB_PD_PORT_POWER_SINK_NOT_CHARGING,
};

struct usb_chg_measures {
	u16 voltage_max;
	u16 voltage_now;
	u16 current_max;
	/*
	 * this structure is used below in struct ec_response_usb_pd_power_info,
	 * and currently expects an odd number of u16 for alignment.
	 */
} __packed;

struct ec_response_usb_pd_power_info {
	u8 role;
	u8 type;
	u8 dualrole;
	u8 reserved1;
	struct usb_chg_measures meas;
	u16 reserved2;
	u32 max_power;
} __packed;

/* Write USB-PD device FW */
#define EC_CMD_USB_PD_FW_UPDATE 0x110

enum usb_pd_fw_update_cmds {
	USB_PD_FW_REBOOT,
	USB_PD_FW_FLASH_ERASE,
	USB_PD_FW_FLASH_WRITE,
	USB_PD_FW_ERASE_SIG,
};

struct ec_params_usb_pd_fw_update {
	u16 dev_id;
	u8 cmd;
	u8 port;
	u32 size;     /* Size to write in bytes */
	/* Followed by data to write */
} __packed;

/* Write USB-PD Accessory RW_HASH table entry */
#define EC_CMD_USB_PD_RW_HASH_ENTRY 0x111
/* RW hash is first 20 bytes of SHA-256 of RW section */
#define PD_RW_HASH_SIZE 20
struct ec_params_usb_pd_rw_hash_entry {
	u16 dev_id;
	u8 dev_rw_hash[PD_RW_HASH_SIZE];
	u8 reserved;        /* For alignment of current_image */
	u32 current_image;  /* One of ec_current_image */
} __packed;

/* Read USB-PD Accessory info */
#define EC_CMD_USB_PD_DEV_INFO 0x112

struct ec_params_usb_pd_info_request {
	u8 port;
} __packed;

/* Read USB-PD Device discovery info */
#define EC_CMD_USB_PD_DISCOVERY 0x113
struct ec_params_usb_pd_discovery_entry {
	u16 vid;  /* USB-IF VID */
	u16 pid;  /* USB-IF PID */
	u8 ptype; /* product type (hub,periph,cable,ama) */
} __packed;

/* Override default charge behavior */
#define EC_CMD_PD_CHARGE_PORT_OVERRIDE 0x114

/* Negative port parameters have special meaning */
enum usb_pd_override_ports {
	OVERRIDE_DONT_CHARGE = -2,
	OVERRIDE_OFF = -1,
	/* [0, PD_PORT_COUNT): Port# */
};

struct ec_params_charge_port_override {
	i16 override_port; /* Override port# */
} __packed;

/* Read (and delete) one entry of PD event log */
#define EC_CMD_PD_GET_LOG_ENTRY 0x115

struct ec_response_pd_log {
	u32 timestamp; /* relative timestamp in milliseconds */
	u8 type;       /* event type : see PD_EVENT_xx below */
	u8 size_port;  /* [7:5] port number [4:0] payload size in bytes */
	u16 data;      /* type-defined data payload */
	u8 payload[0]; /* optional additional data payload: 0..16 bytes */
} __packed;


/* The timestamp is the microsecond counter shifted to get about a ms. */
#define PD_LOG_TIMESTAMP_SHIFT 10 /* 1 LSB = 1024us */

#define PD_LOG_SIZE_MASK  0x1F
#define PD_LOG_PORT_MASK  0xE0
#define PD_LOG_PORT_SHIFT    5
#define PD_LOG_PORT_SIZE(port, size) (((port) << PD_LOG_PORT_SHIFT) | \
				      ((size) & PD_LOG_SIZE_MASK))
#define PD_LOG_PORT(size_port) ((size_port) >> PD_LOG_PORT_SHIFT)
#define PD_LOG_SIZE(size_port) ((size_port) & PD_LOG_SIZE_MASK)

/* PD event log : entry types */
/* PD MCU events */
#define PD_EVENT_MCU_BASE       0x00
#define PD_EVENT_MCU_CHARGE             (PD_EVENT_MCU_BASE+0)
#define PD_EVENT_MCU_CONNECT            (PD_EVENT_MCU_BASE+1)
/* Reserved for custom board event */
#define PD_EVENT_MCU_BOARD_CUSTOM       (PD_EVENT_MCU_BASE+2)
/* PD generic accessory events */
#define PD_EVENT_ACC_BASE       0x20
#define PD_EVENT_ACC_RW_FAIL   (PD_EVENT_ACC_BASE+0)
#define PD_EVENT_ACC_RW_ERASE  (PD_EVENT_ACC_BASE+1)
/* PD power supply events */
#define PD_EVENT_PS_BASE        0x40
#define PD_EVENT_PS_FAULT      (PD_EVENT_PS_BASE+0)
/* PD video dongles events */
#define PD_EVENT_VIDEO_BASE     0x60
#define PD_EVENT_VIDEO_DP_MODE (PD_EVENT_VIDEO_BASE+0)
#define PD_EVENT_VIDEO_CODEC   (PD_EVENT_VIDEO_BASE+1)
/* Returned in the "type" field, when there is no entry available */
#define PD_EVENT_NO_ENTRY       0xFF

/*
 * PD_EVENT_MCU_CHARGE event definition :
 * the payload is "struct usb_chg_measures"
 * the data field contains the port state flags as defined below :
 */
/* Port partner is a dual role device */
#define CHARGE_FLAGS_DUAL_ROLE         (1 << 15)
/* Port is the pending override port */
#define CHARGE_FLAGS_DELAYED_OVERRIDE  (1 << 14)
/* Port is the override port */
#define CHARGE_FLAGS_OVERRIDE          (1 << 13)
/* Charger type */
#define CHARGE_FLAGS_TYPE_SHIFT               3
#define CHARGE_FLAGS_TYPE_MASK       (0xF << CHARGE_FLAGS_TYPE_SHIFT)
/* Power delivery role */
#define CHARGE_FLAGS_ROLE_MASK         (7 <<  0)

/*
 * PD_EVENT_PS_FAULT data field flags definition :
 */
#define PS_FAULT_OCP                          1
#define PS_FAULT_FAST_OCP                     2
#define PS_FAULT_OVP                          3
#define PS_FAULT_DISCH                        4

/*
 * PD_EVENT_VIDEO_CODEC payload is "struct mcdp_info".
 */
struct mcdp_version {
	u8 major;
	u8 minor;
	u16 build;
} __packed;

struct mcdp_info {
	u8 family[2];
	u8 chipid[2];
	struct mcdp_version irom;
	struct mcdp_version fw;
} __packed;

/* struct mcdp_info field decoding */
#define MCDP_CHIPID(chipid) ((chipid[0] << 8) | chipid[1])
#define MCDP_FAMILY(family) ((family[0] << 8) | family[1])

/* Get/Set USB-PD Alternate mode info */
#define EC_CMD_USB_PD_GET_AMODE 0x116
struct ec_params_usb_pd_get_mode_request {
	u16 svid_idx; /* SVID index to get */
	u8 port;      /* port */
} __packed;

struct ec_params_usb_pd_get_mode_response {
	u16 svid;   /* SVID */
	u16 opos;    /* Object Position */
	u32 vdo[6]; /* Mode VDOs */
} __packed;

#define EC_CMD_USB_PD_SET_AMODE 0x117

enum pd_mode_cmd {
	PD_EXIT_MODE = 0,
	PD_ENTER_MODE = 1,
	/* Not a command.  Do NOT remove. */
	PD_MODE_CMD_COUNT,
};

struct ec_params_usb_pd_set_mode_request {
	u32 cmd;  /* enum pd_mode_cmd */
	u16 svid; /* SVID to set */
	u8 opos;  /* Object Position */
	u8 port;  /* port */
} __packed;

/* Ask the PD MCU to record a log of a requested type */
#define EC_CMD_PD_WRITE_LOG_ENTRY 0x118

struct ec_params_pd_write_log_entry {
	u8 type; /* event type : see PD_EVENT_xx above */
	u8 port; /* port#, or 0 for events unrelated to a given port */
} __packed;

#endif  /* !__ACPI__ */

/*****************************************************************************/
/*
 * Passthru commands
 *
 * Some platforms have sub-processors chained to each other.  For example.
 *
 *     AP <--> EC <--> PD MCU
 *
 * The top 2 bits of the command number are used to indicate which device the
 * command is intended for.  Device 0 is always the device receiving the
 * command; other device mapping is board-specific.
 *
 * When a device receives a command to be passed to a sub-processor, it passes
 * it on with the device number set back to 0.  This allows the sub-processor
 * to remain blissfully unaware of whether the command originated on the next
 * device up the chain, or was passed through from the AP.
 *
 * In the above example, if the AP wants to send command 0x0002 to the PD MCU,
 *     AP sends command 0x4002 to the EC
 *     EC sends command 0x0002 to the PD MCU
 *     EC forwards PD MCU response back to the AP
 */

/* Offset and max command number for sub-device n */
#define EC_CMD_PASSTHRU_OFFSET(n) (0x4000 * (n))
#define EC_CMD_PASSTHRU_MAX(n) (EC_CMD_PASSTHRU_OFFSET(n) + 0x3fff)

/*****************************************************************************/
/*
 * Deprecated constants. These constants have been renamed for clarity. The
 * meaning and size has not changed. Programs that use the old names should
 * switch to the new names soon, as the old names may not be carried forward
 * forever.
 */
#define EC_HOST_PARAM_SIZE      EC_PROTO2_MAX_PARAM_SIZE
#define EC_LPC_ADDR_OLD_PARAM   EC_HOST_CMD_REGION1
#define EC_OLD_PARAM_SIZE       EC_HOST_CMD_REGION_SIZE

#endif  /* __CROS_EC_COMMANDS_H */
