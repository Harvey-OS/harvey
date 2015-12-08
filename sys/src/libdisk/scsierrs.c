/* machine generated. Once. TODO: write the vendor support to bring this in as we should.
 * But the errors almost never change, so there's no hurry.
 */
#include <u.h>
#include <libc.h>

typedef struct Err Err;
struct Err
{
	int n;
	char *s;
};

static Err scsierrs[] = {
	{0x0000,	"no additional sense information"},

	{0x0001,	"filemark detected"},

	{0x0002,	"end-of-partition/medium detected"},

	{0x0003,	"setmark detected"},

	{0x0004,	"beginning-of-partition/medium detected"},

	{0x0005,	"end-of-data detected"},

	{0x0006,	"i/o process terminated"},

	{0x0007,	"programmable early warning detected"},

	{0x0011,	"audio play operation in progress"},

	{0x0012,	"audio play operation paused"},

	{0x0013,	"audio play operation successfully completed"},

	{0x0014,	"audio play operation stopped due to error"},

	{0x0015,	"no current audio status to return"},

	{0x0016,	"operation in progress"},

	{0x0017,	"cleaning requested"},

	{0x0018,	"erase operation in progress"},

	{0x0019,	"locate operation in progress"},

	{0x001a,	"rewind operation in progress"},

	{0x001b,	"set capacity operation in progress"},

	{0x001c,	"verify operation in progress"},

	{0x0100,	"no index/sector signal"},

	{0x0200,	"no seek complete"},

	{0x0300,	"peripheral device write fault"},

	{0x0301,	"no write current"},

	{0x0302,	"excessive write errors"},

	{0x0400,	"logical unit not ready, cause not reportable"},

	{0x0401,	"logical unit is in process of becoming ready"},

	{0x0402,	"logical unit not ready, initializing command required"},

	{0x0403,	"logical unit not ready, manual intervention required"},

	{0x0404,	"logical unit not ready, format in progress"},

	{0x0405,	"logical unit not ready, rebuild in progress"},

	{0x0406,	"logical unit not ready, recalculation in progress"},

	{0x0407,	"logical unit not ready, operation in progress"},

	{0x0408,	"logical unit not ready, long write in progress"},

	{0x0409,	"logical unit not ready, self-test in progress"},

	{0x040a,	"logical unit not accessible, asymmetric access state transition"},

	{0x040b,	"logical unit not accessible, target port in standby state"},

	{0x040c,	"logical unit not accessible, target port in unavailable state"},

	{0x0410,	"logical unit not ready, auxiliary memory not accessible"},

	{0x0411,	"logical unit not ready, notify (enable spinup) required"},

	{0x0412,	"logical unit not ready, offline"},

	{0x0413,	"logical unit not ready, sa creation in progress"},

	{0x0414,	"logical unit not ready, space allocation in progress"},

	{0x0415,	"logical unit not ready, robotics disabled"},

	{0x0416,	"logical unit not ready, configuration required"},

	{0x0417,	"logical unit not ready, calibration required"},

	{0x0418,	"logical unit not ready, a door is open"},

	{0x0419,	"logical unit not ready, operating in sequential mode"},

	{0x0500,	"logical unit does not respond to selection"},

	{0x0600,	"no reference position found"},

	{0x0700,	"multiple peripheral devices selected"},

	{0x0800,	"logical unit communication failure"},

	{0x0801,	"logical unit communication time-out"},

	{0x0802,	"logical unit communication parity error"},

	{0x0803,	"logical unit communication crc error (ultra-dma/32)"},

	{0x0804,	"unreachable copy target"},

	{0x0900,	"track following error"},

	{0x0901,	"tracking servo failure"},

	{0x0902,	"focus servo failure"},

	{0x0903,	"spindle servo failure"},

	{0x0904,	"head select fault"},

	{0x0a00,	"error log overflow"},

	{0x0b00,	"warning"},

	{0x0b01,	"warning - specified temperature exceeded"},

	{0x0b02,	"warning - enclosure degraded"},

	{0x0b03,	"warning - background self-test failed"},

	{0x0b04,	"warning - background pre-scan detected medium error"},

	{0x0b05,	"warning - background medium scan detected medium error"},

	{0x0b06,	"warning - non-volatile cache now volatile"},

	{0x0b07,	"warning - degraded power to non-volatile cache"},

	{0x0b08,	"warning - power loss expected"},

	{0x0c00,	"write error"},

	{0x0c01,	"write error - recovered with auto reallocation"},

	{0x0c02,	"write error - auto reallocation failed"},

	{0x0c03,	"write error - recommend reassignment"},

	{0x0c04,	"compression check miscompare error"},

	{0x0c05,	"data expansion occurred during compression"},

	{0x0c06,	"block not compressible"},

	{0x0c07,	"write error - recovery needed"},

	{0x0c08,	"write error - recovery failed"},

	{0x0c09,	"write error - loss of streaming"},

	{0x0c0a,	"write error - padding blocks added"},

	{0x0c0b,	"auxiliary memory write error"},

	{0x0c0c,	"write error - unexpected unsolicited data"},

	{0x1000,	"id crc or ecc error"},

	{0x1001,	"logical block guard check failed"},

	{0x1002,	"logical block application tag check failed"},

	{0x1003,	"logical block reference tag check failed"},

	{0x1100,	"unrecovered read error"},

	{0x1101,	"read retries exhausted"},

	{0x1102,	"error too long to correct"},

	{0x1103,	"multiple read errors"},

	{0x1104,	"unrecovered read error - auto reallocate failed"},

	{0x1105,	"l-ec uncorrectable error"},

	{0x1106,	"circ unrecovered error"},

	{0x1107,	"data re-synchronization error"},

	{0x1108,	"incomplete block read"},

	{0x1109,	"no gap found"},

	{0x110a,	"miscorrected error"},

	{0x110b,	"unrecovered read error - recommend reassignment"},

	{0x110c,	"unrecovered read error - recommend rewrite the data"},

	{0x1110,	"error reading isrc number"},

	{0x1111,	"read error - loss of streaming"},

	{0x1112,	"auxiliary memory read error"},

	{0x1113,	"read error - failed retransmission request"},

	{0x1114,	"read error - lba marked bad by application client"},

	{0x1200,	"address mark not found for id field"},

	{0x1300,	"address mark not found for data field"},

	{0x1400,	"recorded entity not found"},

	{0x1401,	"record not found"},

	{0x1402,	"filemark or setmark not found"},

	{0x1403,	"end-of-data not found"},

	{0x1404,	"block sequence error"},

	{0x1405,	"record not found - recommend reassignment"},

	{0x1406,	"record not found - data auto-reallocated"},

	{0x1407,	"locate operation failure"},

	{0x1500,	"random positioning error"},

	{0x1501,	"mechanical positioning error"},

	{0x1502,	"positioning error detected by read of medium"},

	{0x1600,	"data synchronization mark error"},

	{0x1601,	"data sync error - data rewritten"},

	{0x1602,	"data sync error - recommend rewrite"},

	{0x1603,	"data sync error - data auto-reallocated"},

	{0x1604,	"data sync error - recommend reassignment"},

	{0x1700,	"recovered data with no error correction applied"},

	{0x1701,	"recovered data with retries"},

	{0x1702,	"recovered data with positive head offset"},

	{0x1703,	"recovered data with negative head offset"},

	{0x1704,	"recovered data with retries and/or circ applied"},

	{0x1705,	"recovered data using previous sector id"},

	{0x1706,	"recovered data without ecc - data auto-reallocated"},

	{0x1707,	"recovered data without ecc - recommend reassignment"},

	{0x1708,	"recovered data without ecc - recommend rewrite"},

	{0x1709,	"recovered data without ecc - data rewritten"},

	{0x1800,	"recovered data with error correction applied"},

	{0x1801,	"recovered data with error corr. & retries applied"},

	{0x1802,	"recovered data - data auto-reallocated"},

	{0x1803,	"recovered data with circ"},

	{0x1804,	"recovered data with l-ec"},

	{0x1805,	"recovered data - recommend reassignment"},

	{0x1806,	"recovered data - recommend rewrite"},

	{0x1807,	"recovered data with ecc - data rewritten"},

	{0x1808,	"recovered data with linking"},

	{0x1900,	"defect list error"},

	{0x1901,	"defect list not available"},

	{0x1902,	"defect list error in primary list"},

	{0x1903,	"defect list error in grown list"},

	{0x1a00,	"parameter list length error"},

	{0x1b00,	"synchronous data transfer error"},

	{0x1c00,	"defect list not found"},

	{0x1c01,	"primary defect list not found"},

	{0x1c02,	"grown defect list not found"},

	{0x2000,	"invalid command operation code"},

	{0x2001,	"access denied - initiator pending-enrolled"},

	{0x2002,	"access denied - no access rights"},

	{0x2003,	"access denied - invalid mgmt id key"},

	{0x2004,	"illegal command while in write capable state"},

	{0x2005,	"obsolete"},

	{0x2006,	"illegal command while in explicit address mode"},

	{0x2007,	"illegal command while in implicit address mode"},

	{0x2008,	"access denied - enrollment conflict"},

	{0x2009,	"access denied - invalid lu identifier"},

	{0x200a,	"access denied - invalid proxy token"},

	{0x200b,	"access denied - acl lun conflict"},

	{0x2100,	"logical block address out of range"},

	{0x2101,	"invalid element address"},

	{0x2102,	"invalid address for write"},

	{0x2103,	"invalid write crossing layer jump"},

	{0x2200,	"illegal function (use 20 00, 24 00, or 26 00)"},

	{0x2400,	"invalid field in cdb"},

	{0x2401,	"cdb decryption error"},

	{0x2402,	"obsolete"},

	{0x2403,	"obsolete"},

	{0x2404,	"security audit value frozen"},

	{0x2405,	"security working key frozen"},

	{0x2406,	"nonce not unique"},

	{0x2407,	"nonce timestamp out of range"},

	{0x2408,	"invalid xcdb"},

	{0x2500,	"logical unit not supported"},

	{0x2600,	"invalid field in parameter list"},

	{0x2601,	"parameter not supported"},

	{0x2602,	"parameter value invalid"},

	{0x2603,	"threshold parameters not supported"},

	{0x2604,	"invalid release of persistent reservation"},

	{0x2605,	"data decryption error"},

	{0x2606,	"too many target descriptors"},

	{0x2607,	"unsupported target descriptor type code"},

	{0x2608,	"too many segment descriptors"},

	{0x2609,	"unsupported segment descriptor type code"},

	{0x260a,	"unexpected inexact segment"},

	{0x260b,	"inline data length exceeded"},

	{0x260c,	"invalid operation for copy source or destination"},

	{0x2610,	"data decryption key fail limit reached"},

	{0x2611,	"incomplete key-associated data set"},

	{0x2612,	"vendor specific key reference not found"},

	{0x2700,	"write protected"},

	{0x2701,	"hardware write protected"},

	{0x2702,	"logical unit software write protected"},

	{0x2703,	"associated write protect"},

	{0x2704,	"persistent write protect"},

	{0x2705,	"permanent write protect"},

	{0x2706,	"conditional write protect"},

	{0x2707,	"space allocation failed write protect"},

	{0x2800,	"not ready to ready change, medium may have changed"},

	{0x2801,	"import or export element accessed"},

	{0x2802,	"format-layer may have changed"},

	{0x2803,	"import/export element accessed, medium changed"},

	{0x2900,	"power on, reset, or bus device reset occurred"},

	{0x2901,	"power on occurred"},

	{0x2902,	"scsi bus reset occurred"},

	{0x2903,	"bus device reset function occurred"},

	{0x2904,	"device internal reset"},

	{0x2905,	"transceiver mode changed to single-ended"},

	{0x2906,	"transceiver mode changed to lvd"},

	{0x2907,	"i_t nexus loss occurred"},

	{0x2a00,	"parameters changed"},

	{0x2a01,	"mode parameters changed"},

	{0x2a02,	"log parameters changed"},

	{0x2a03,	"reservations preempted"},

	{0x2a04,	"reservations released"},

	{0x2a05,	"registrations preempted"},

	{0x2a06,	"asymmetric access state changed"},

	{0x2a07,	"implicit asymmetric access state transition failed"},

	{0x2a08,	"priority changed"},

	{0x2a09,	"capacity data has changed"},

	{0x2a0a,	"error history i_t nexus cleared"},

	{0x2a0b,	"error history snapshot released"},

	{0x2a0c,	"error recovery attributes have changed"},

	{0x2a10,	"timestamp changed"},

	{0x2a11,	"data encryption parameters changed by another i_t nexus"},

	{0x2a12,	"data encryption parameters changed by vendor specific event"},

	{0x2a13,	"data encryption key instance counter has changed"},

	{0x2a14,	"sa creation capabilities data has changed"},

	{0x2b00,	"copy cannot execute since host cannot disconnect"},

	{0x2c00,	"command sequence error"},

	{0x2c01,	"too many windows specified"},

	{0x2c02,	"invalid combination of windows specified"},

	{0x2c03,	"current program area is not empty"},

	{0x2c04,	"current program area is empty"},

	{0x2c05,	"illegal power condition request"},

	{0x2c06,	"persistent prevent conflict"},

	{0x2c07,	"previous busy status"},

	{0x2c08,	"previous task set full status"},

	{0x2c09,	"previous reservation conflict status"},

	{0x2c0a,	"partition or collection contains user objects"},

	{0x2c0b,	"not reserved"},

	{0x3000,	"incompatible medium installed"},

	{0x3001,	"cannot read medium - unknown format"},

	{0x3002,	"cannot read medium - incompatible format"},

	{0x3003,	"cleaning cartridge installed"},

	{0x3004,	"cannot write medium - unknown format"},

	{0x3005,	"cannot write medium - incompatible format"},

	{0x3006,	"cannot format medium - incompatible medium"},

	{0x3007,	"cleaning failure"},

	{0x3008,	"cannot write - application code mismatch"},

	{0x3009,	"current session not fixated for append"},

	{0x300a,	"cleaning request rejected"},

	{0x300c,	"worm medium - overwrite attempted"},

	{0x3010,	"medium not formatted"},

	{0x3011,	"incompatible volume type"},

	{0x3012,	"incompatible volume qualifier"},

	{0x3013,	"cleaning volume expired"},

	{0x3100,	"medium format corrupted"},

	{0x3101,	"format command failed"},

	{0x3102,	"zoned formatting failed due to spare linking"},

	{0x3200,	"no defect spare location available"},

	{0x3201,	"defect list update failure"},

	{0x3300,	"tape length error"},

	{0x3400,	"enclosure failure"},

	{0x3500,	"enclosure services failure"},

	{0x3501,	"unsupported enclosure function"},

	{0x3502,	"enclosure services unavailable"},

	{0x3503,	"enclosure services transfer failure"},

	{0x3504,	"enclosure services transfer refused"},

	{0x3505,	"enclosure services checksum error"},

	{0x3600,	"ribbon, ink, or toner failure"},

	{0x3700,	"rounded parameter"},

	{0x3800,	"event status notification"},

	{0x3802,	"esn - power management class event"},

	{0x3804,	"esn - media class event"},

	{0x3806,	"esn - device busy class event"},

	{0x3807,	"thin provisioning soft threshold reached"},

	{0x3900,	"saving parameters not supported"},

	{0x3a00,	"medium not present"},

	{0x3a01,	"medium not present - tray closed"},

	{0x3a02,	"medium not present - tray open"},

	{0x3a03,	"medium not present - loadable"},

	{0x3a04,	"medium not present - medium auxiliary memory accessible"},

	{0x3b00,	"sequential positioning error"},

	{0x3b01,	"tape position error at beginning-of-medium"},

	{0x3b02,	"tape position error at end-of-medium"},

	{0x3b03,	"tape or electronic vertical forms unit not ready"},

	{0x3b04,	"slew failure"},

	{0x3b05,	"paper jam"},

	{0x3b06,	"failed to sense top-of-form"},

	{0x3b07,	"failed to sense bottom-of-form"},

	{0x3b08,	"reposition error"},

	{0x3b09,	"read past end of medium"},

	{0x3b0a,	"read past beginning of medium"},

	{0x3b0b,	"position past end of medium"},

	{0x3b0c,	"position past beginning of medium"},

	{0x3b11,	"medium magazine not accessible"},

	{0x3b12,	"medium magazine removed"},

	{0x3b13,	"medium magazine inserted"},

	{0x3b14,	"medium magazine locked"},

	{0x3b15,	"medium magazine unlocked"},

	{0x3b16,	"mechanical positioning or changer error"},

	{0x3b17,	"read past end of user object"},

	{0x3b18,	"element disabled"},

	{0x3b19,	"element enabled"},

	{0x3b1a,	"data transfer device removed"},

	{0x3b1b,	"data transfer device inserted"},

	{0x4000,	"ram failure (should use 40 nn)"},

	{0x4100,	"data path failure (should use 40 nn)"},

	{0x4200,	"power-on or self-test failure (should use 40 nn)"},

	{0x4300,	"message error"},

	{0x4400,	"internal target failure"},

	{0x4471,	"ata device failed set features"},

	{0x4500,	"select or reselect failure"},

	{0x4600,	"unsuccessful soft reset"},

	{0x4700,	"scsi parity error"},

	{0x4701,	"data phase crc error detected"},

	{0x4702,	"scsi parity error detected during st data phase"},

	{0x4703,	"information unit iucrc error detected"},

	{0x4704,	"asynchronous information protection error detected"},

	{0x4705,	"protocol service crc error"},

	{0x4706,	"phy test function in progress"},

	{0x4800,	"initiator detected error message received"},

	{0x4900,	"invalid message error"},

	{0x4a00,	"command phase error"},

	{0x4b00,	"data phase error"},

	{0x4b01,	"invalid target port transfer tag received"},

	{0x4b02,	"too much write data"},

	{0x4b03,	"ack/nak timeout"},

	{0x4b04,	"nak received"},

	{0x4b05,	"data offset error"},

	{0x4b06,	"initiator response timeout"},

	{0x4b07,	"connection lost"},

	{0x4c00,	"logical unit failed self-configuration"},

	{0x5000,	"write append error"},

	{0x5001,	"write append position error"},

	{0x5002,	"position error related to timing"},

	{0x5100,	"erase failure"},

	{0x5101,	"erase failure - incomplete erase operation detected"},

	{0x5200,	"cartridge fault"},

	{0x5300,	"media load or eject failed"},

	{0x5301,	"unload tape failure"},

	{0x5302,	"medium removal prevented"},

	{0x5303,	"medium removal prevented by data transfer element"},

	{0x5304,	"medium thread or unthread failure"},

	{0x5400,	"scsi to host system interface failure"},

	{0x5500,	"system resource failure"},

	{0x5501,	"system buffer full"},

	{0x5502,	"insufficient reservation resources"},

	{0x5503,	"insufficient resources"},

	{0x5504,	"insufficient registration resources"},

	{0x5505,	"insufficient access control resources"},

	{0x5506,	"auxiliary memory out of space"},

	{0x5507,	"quota error"},

	{0x5508,	"maximum number of supplemental decryption keys exceeded"},

	{0x5509,	"medium auxiliary memory not accessible"},

	{0x550a,	"data currently unavailable"},

	{0x550b,	"insufficient power for operation"},

	{0x5700,	"unable to recover table-of-contents"},

	{0x5800,	"generation does not exist"},

	{0x5900,	"updated block read"},

	{0x5a00,	"operator request or state change input"},

	{0x5a01,	"operator medium removal request"},

	{0x5a02,	"operator selected write protect"},

	{0x5a03,	"operator selected write permit"},

	{0x5b00,	"log exception"},

	{0x5b01,	"threshold condition met"},

	{0x5b02,	"log counter at maximum"},

	{0x5b03,	"log list codes exhausted"},

	{0x5c00,	"rpl status change"},

	{0x5c01,	"spindles synchronized"},

	{0x5c02,	"spindles not synchronized"},

	{0x6000,	"lamp failure"},

	{0x6100,	"video acquisition error"},

	{0x6101,	"unable to acquire video"},

	{0x6102,	"out of focus"},

	{0x6200,	"scan head positioning error"},

	{0x6300,	"end of user area encountered on this track"},

	{0x6301,	"packet does not fit in available space"},

	{0x6400,	"illegal mode for this track"},

	{0x6401,	"invalid packet size"},

	{0x6500,	"voltage fault"},

	{0x6600,	"automatic document feeder cover up"},

	{0x6601,	"automatic document feeder lift up"},

	{0x6602,	"document jam in automatic document feeder"},

	{0x6603,	"document miss feed automatic in document feeder"},

	{0x6700,	"configuration failure"},

	{0x6701,	"configuration of incapable logical units failed"},

	{0x6702,	"add logical unit failed"},

	{0x6703,	"modification of logical unit failed"},

	{0x6704,	"exchange of logical unit failed"},

	{0x6705,	"remove of logical unit failed"},

	{0x6706,	"attachment of logical unit failed"},

	{0x6707,	"creation of logical unit failed"},

	{0x6708,	"assign failure occurred"},

	{0x6709,	"multiply assigned logical unit"},

	{0x670a,	"set target port groups command failed"},

	{0x670b,	"ata device feature not enabled"},

	{0x6800,	"logical unit not configured"},

	{0x6900,	"data loss on logical unit"},

	{0x6901,	"multiple logical unit failures"},

	{0x6902,	"parity/data mismatch"},

	{0x6a00,	"informational, refer to log"},

	{0x6b00,	"state change has occurred"},

	{0x6b01,	"redundancy level got better"},

	{0x6b02,	"redundancy level got worse"},

	{0x6c00,	"rebuild failure occurred"},

	{0x7100,	"decompression exception long algorithm id"},

	{0x7200,	"session fixation error"},

	{0x7201,	"session fixation error writing lead-in"},

	{0x7202,	"session fixation error writing lead-out"},

	{0x7203,	"session fixation error - incomplete track in session"},

	{0x7204,	"empty or partially written reserved track"},

	{0x7205,	"no more track reservations allowed"},

	{0x7206,	"rmz extension is not allowed"},

	{0x7207,	"no more test zone extensions are allowed"},

	{0x7300,	"cd control error"},

	{0x7301,	"power calibration area almost full"},

	{0x7302,	"power calibration area is full"},

	{0x7303,	"power calibration area error"},

	{0x7304,	"program memory area update failure"},

	{0x7305,	"program memory area is full"},

	{0x7306,	"rma/pma is almost full"},

	{0x7310,	"current power calibration area almost full"},

	{0x7311,	"current power calibration area is full"},

	{0x7317,	"rdz is full"},

	{0x7400,	"security error"},

	{0x7401,	"unable to decrypt data"},

	{0x7402,	"unencrypted data encountered while decrypting"},

	{0x7403,	"incorrect data encryption key"},

	{0x7404,	"cryptographic integrity validation failed"},

	{0x7405,	"error decrypting data"},

	{0x7406,	"unknown signature verification key"},

	{0x7407,	"encryption parameters not useable"},

	{0x7408,	"digital signature validation failure"},

	{0x7409,	"encryption mode mismatch on read"},

	{0x740a,	"encrypted block not raw read enabled"},

	{0x740b,	"incorrect encryption parameters"},

	{0x740c,	"unable to decrypt parameter list"},

	{0x7410,	"sa creation parameter value invalid"},

	{0x7411,	"sa creation parameter value rejected"},

	{0x7412,	"invalid sa usage"},

	{0x7421,	"data encryption configuration prevented"},

	{0x7430,	"sa creation parameter not supported"},

	{0x7440,	"authentication failed"},

	{0x7461,	"external data encryption key manager access error"},

	{0x7462,	"external data encryption key manager error"},

	{0x7463,	"external data encryption key not found"},

	{0x7464,	"external data encryption request not authorized"},

	{0x7471,	"logical unit access not authorized"},

	{0x7479,	"security conflict in translated device"},

};

char*
scsierrmsg(int n)
{
	int i;

	for(i = 0; i < nelem(scsierrs); i++)
		if(scsierrs[i].n == n)
			return scsierrs[i].s;
	return "scsi error";
}
