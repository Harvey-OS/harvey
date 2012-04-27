/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    RemSmb.h

Abstract:

    Definition of descriptor strings for Net API remote calls.
    Names defined in this file follow the format:

                RemSmb_RemDescriptor

           RemDescriptor follows one of the following formats:

             StructureName_level         -  info structures
             StructureName_level_suffix  -  special info structures
             ApiName_P                   -  parameter descriptors

Notes:

    1. While the above formats should be followed, the equate names
       cannot exceed 32 characters, and abbreviated forms should be used.

    2. The remote API mechanism requires that the return parameter length
       is less than or equal to the send parameter length. This assumption
       is made in order to reduce the overhead in the buffer management
       required for the API call. This restriction is not unreasonable
       as the APIs were designed to return data in the data buffer and just
       use return parameters for data lengths & file handles etc.
       HOWEVER, if it has been spec'ed to return a large parameter field, it
       is possible to pad the size of the send parameter using a REM_FILL_BYTES
       field to meet the above restriction.

Author:

Environment:

    Portable to just about anything.
    Requires ANSI C extensions: slash-slash comments, long external
names.

Revision History:

--*/

#ifndef _REMDEF_
#define _REMDEF_
/*
 * ====================================================================
 * SMB XACT message descriptors.
 * ====================================================================
 */

#define	REMSmb_share_info_0		"B13"
#define	REMSmb_share_info_1		"B13BWz"
#define	REMSmb_share_info_2		"B13BWzWWWzB9B"

#define	REMSmb_share_info_90		"B13BWz"
#define	REMSmb_share_info_92		"zzz"
#define	REMSmb_share_info_93		"zzz"

#define	REMSmb_share_info_0_setinfo	"B13"
#define	REMSmb_share_info_1_setinfo	"B13BWz"
#define	REMSmb_share_info_2_setinfo	"B13BWzWWOB9B"

#define	REMSmb_share_info_90_setinfo	"B13BWz"
#define	REMSmb_share_info_91_setinfo	"B13BWzWWWOB9BB9BWzWWzWW"

#define	REMSmb_NetShareEnum_P		"WrLeh"
#define	REMSmb_NetShareGetInfo_P	"zWrLh"
#define	REMSmb_NetShareSetInfo_P	"zWsTP"
#define	REMSmb_NetShareAdd_P		"WsT"
#define	REMSmb_NetShareDel_P		"zW"
#define	REMSmb_NetShareCheck_P		"zh"

#define	REMSmb_session_info_0		"z"
#define	REMSmb_session_info_1		"zzWWWDDD"
#define	REMSmb_session_info_2		"zzWWWDDDz"
#define	REMSmb_session_info_10		"zzDD"

#define	REMSmb_NetSessionEnum_P		"WrLeh"
#define	REMSmb_NetSessionGetInfo_P	"zWrLh"
#define	REMSmb_NetSessionDel_P		"zW"

#define	REMSmb_connection_info_0	"W"
#define	REMSmb_connection_info_1	"WWWWDzz"

#define	REMSmb_NetConnectionEnum_P	"zWrLeh"

#define	REMSmb_file_info_0		"W"
#define	REMSmb_file_info_1		"WWWzz"
#define	REMSmb_file_info_2		"D"
#define	REMSmb_file_info_3		"DWWzz"

#define	REMSmb_NetFileEnum_P		"zWrLeh"
#define	REMSmb_NetFileEnum2_P		"zzWrLehb8g8"
#define	REMSmb_NetFileGetInfo_P		"WWrLh"
#define	REMSmb_NetFileGetInfo2_P	"DWrLh"
#define	REMSmb_NetFileClose_P		"W"
#define	REMSmb_NetFileClose2_P		"D"

#define	REMSmb_server_info_0		"B16"
#define	REMSmb_server_info_1		"B16BBDz"
#define	REMSmb_server_info_2		"B16BBDzDDDWWzWWWWWWWB21BzWWWWWWWWWWWWWWWWWWWWWWz"
#define	REMSmb_server_info_3		"B16BBDzDDDWWzWWWWWWWB21BzWWWWWWWWWWWWWWWWWWWWWWzDWz"

#define	REMSmb_server_info_1_setinfo	"B16BBDz"
#define	REMSmb_server_info_2_setinfo	"B16BBDzDDDWWzWWWWWWWB21BOWWWWWWWWWWWWWWWWWWWWWWz"

#define	REMSmb_server_admin_command	"B"

#define	REMSmb_server_diskenum_0	"B3"

#define	REMSmb_authenticator_info_0	"B8D"

#define	REMSmb_server_diskft_100	"B"
#define	REMSmb_server_diskft_101	"BBWWWWDW"
#define	REMSmb_server_diskft_102	"BBWWWWDN"
#define	REMSmb_server_diskfterr_0	"DWWDDW"
#define	REMSmb_ft_info_0		"WWW"
#define	REMSmb_ft_drivestats_0		"BBWDDDDDDD"
#define	REMSmb_ft_error_info_1		"DWWDDWBBDD"

#define	REMSmb_I_NetServerDiskEnum_P	"WrLeh"
#define	REMSmb_I_NetServerDiskGetInfo_P	"WWrLh"
#define	REMSmb_I_FTVerifyMirror_P	"Wz"
#define	REMSmb_I_FTAbortVerify_P	"W"
#define	REMSmb_I_FTGetInfo_P		"WrLh"
#define	REMSmb_I_FTSetInfo_P		"WsTP"
#define	REMSmb_I_FTLockDisk_P		"WWh"
#define	REMSmb_I_FTFixError_P		"Dzhh2"
#define	REMSmb_I_FTAbortFix_P		"D"
#define	REMSmb_I_FTDiagnoseError_P	"Dhhhh"
#define	REMSmb_I_FTGetDriveStats_P	"WWrLh"
#define	REMSmb_I_FTErrorGetInfo_P	"DWrLh"

#define	REMSmb_NetServerEnum_P		"WrLeh"
#define	REMSmb_I_NetServerEnum_P	"WrLeh"
#define	REMSmb_NetServerEnum2_P		"WrLehDz"
#define	REMSmb_I_NetServerEnum2_P	"WrLehDz"
#define	REMSmb_NetServerEnum3_P		"WrLehDzz"
#define	REMSmb_NetServerGetInfo_P	"WrLh"
#define	REMSmb_NetServerSetInfo_P	"WsTP"
#define	REMSmb_NetServerDiskEnum_P	"WrLeh"
#define	REMSmb_NetServerAdminCommand_P	"zhrLeh"
#define	REMSmb_NetServerReqChalleng_P	"zb8g8"
#define	REMSmb_NetServerAuthenticat_P	"zb8g8"
#define	REMSmb_NetServerPasswordSet_P	"zb12g12b16"

#define	REMSmb_NetAuditOpen_P		"h"
#define	REMSmb_NetAuditClear_P		"zz"
#define	REMSmb_NetAuditRead_P		"zb16g16DhDDrLeh"

#define	REMSmb_AuditLogReturnBuf	"K"

#define	REMSmb_NetErrorLogOpen_P	"h"
#define	REMSmb_NetErrorLogClear_P	"zz"
#define	REMSmb_NetErrorLogRead_P	"zb16g16DhDDrLeh"

#define	REMSmb_ErrorLogReturnBuf	"K"

#define	REMSmb_chardev_info_0		"B9"
#define	REMSmb_chardev_info_1		"B10WB22D"
#define	REMSmb_chardevQ_info_0		"B13"
#define	REMSmb_chardevQ_info_1		"B14WzWW"

#define	REMSmb_NetCharDevEnum_P		"WrLeh"
#define	REMSmb_NetCharDevGetInfo_P	"zWrLh"
#define	REMSmb_NetCharDevControl_P	"zW"
#define	REMSmb_NetCharDevQEnum_P	"zWrLeh"
#define	REMSmb_NetCharDevQGetInfo_P	"zzWrLh"
#define	REMSmb_NetCharDevQSetInfo_P	"zWsTP"
#define	REMSmb_NetCharDevQPurge_P	"z"
#define	REMSmb_NetCharDevQPurgeSelf_P	"zz"

#define	REMSmb_msg_info_0		"B16"
#define	REMSmb_msg_info_1		"B16BBB16"
#define	REMSmb_send_struct		"K"

#define	REMSmb_NetMessageNameEnum_P	"WrLeh"
#define	REMSmb_NetMessageNameGetInfo_P	"zWrLh"
#define	REMSmb_NetMessageNameAdd_P	"zW"
#define	REMSmb_NetMessageNameDel_P	"zW"
#define	REMSmb_NetMessageNameFwd_P	"zzW"
#define	REMSmb_NetMessageNameUnFwd_P	"z"
#define	REMSmb_NetMessageBufferSend_P	"zsT"
#define	REMSmb_NetMessageFileSend_P	"zz"
#define	REMSmb_NetMessageLogFileSet_P	"zW"
#define	REMSmb_NetMessageLogFileGet_P	"rLh"

#define	REMSmb_service_info_0		"B16"
#define	REMSmb_service_info_1		"B16WDW"
#define	REMSmb_service_info_2		"B16WDWB64"
#define	REMSmb_service_cmd_args		"K"

#define	REMSmb_NetServiceEnum_P		"WrLeh"
#define	REMSmb_NetServiceControl_P	"zWWrL"
#define	REMSmb_NetServiceInstall_P	"zF88sg88T"	/* See NOTE 2 */
#define	REMSmb_NetServiceGetInfo_P	"zWrLh"

#define	REMSmb_access_info_0		"z"
#define	REMSmb_access_info_0_setinfo	"z"
#define	REMSmb_access_info_1		"zWN"
#define	REMSmb_access_info_1_setinfo	"OWN"
#define	REMSmb_access_list		"B21BW"

#define	REMSmb_NetAccessEnum_P		"zWWrLeh"
#define	REMSmb_NetAccessGetInfo_P	"zWrLh"
#define	REMSmb_NetAccessSetInfo_P	"zWsTP"
#define	REMSmb_NetAccessAdd_P		"WsT"
#define	REMSmb_NetAccessDel_P		"z"
#define	REMSmb_NetAccessGetUserPerms_P	"zzh"

#define	REMSmb_group_info_0		"B21"
#define	REMSmb_group_info_1		"B21Bz"
#define	REMSmb_group_users_info_0	"B21"
#define	REMSmb_group_users_info_1	"B21BN"

#define	REMSmb_NetGroupEnum_P		"WrLeh"
#define	REMSmb_NetGroupAdd_P		"WsT"
#define	REMSmb_NetGroupDel_P		"z"
#define	REMSmb_NetGroupAddUser_P	"zz"
#define	REMSmb_NetGroupDelUser_P	"zz"
#define	REMSmb_NetGroupGetUsers_P	"zWrLeh"
#define	REMSmb_NetGroupSetUsers_P	"zWsTW"
#define	REMSmb_NetGroupGetInfo_P	"zWrLh"
#define	REMSmb_NetGroupSetInfo_P	"zWsTP"

#define	REMSmb_user_info_0		"B21"
#define	REMSmb_user_info_1		"B21BB16DWzzWz"
#define	REMSmb_user_info_2		"B21BB16DWzzWzDzzzzDDDDWb21WWzWW"
#define	REMSmb_user_info_10		"B21Bzzz"
#define	REMSmb_user_info_11		"B21BzzzWDDzzDDWWzWzDWb21W"

#define	REMSmb_user_info_100		"DWW"
#define	REMSmb_user_info_101		"B60"
#define	REMSmb_user_modals_info_0	"WDDDWW"
#define	REMSmb_user_modals_info_1	"Wz"
#define	REMSmb_user_modals_info_100	"B50"
#define	REMSmb_user_modals_info_101	"zDDzDD"
#define	REMSmb_user_logon_info_0	"B21B"
#define	REMSmb_user_logon_info_1	"WB21BWDWWDDDDDDDzzzD"
#define	REMSmb_user_logon_info_2	"B21BzzzD"
#define	REMSmb_user_logoff_info_1	"WDW"

#define	REMSmb_NetUserEnum_P		"WrLeh"
#define	REMSmb_NetUserAdd_P		"WsTW"
#define	REMSmb_NetUserAdd2_P		"WsTWW"
#define	REMSmb_NetUserDel_P		"z"
#define	REMSmb_NetUserGetInfo_P		"zWrLh"
#define	REMSmb_NetUserSetInfo_P		"zWsTPW"
#define	REMSmb_NetUserSetInfo2_P	"zWsTPWW"
#define	REMSmb_NetUserPasswordSet_P	"zb16b16W"
#define	REMSmb_NetUserPasswordSet2_P	"zb16b16WW"
#define	REMSmb_NetUserGetGroups_P	"zWrLeh"
#define	REMSmb_NetUserSetGroups_P	"zWsTW"
#define	REMSmb_NetUserModalsGet_P	"WrLh"
#define	REMSmb_NetUserModalsSet_P	"WsTP"
#define	REMSmb_NetUserEnum2_P		"WrLDieh"
#define	REMSmb_NetUserValidate2_P	"Wb62WWrLhWW"

#define	REMSmb_wksta_info_0		"WDzzzzBBDWDWWWWWWWWWWWWWWWWWWWzzW"
#define	REMSmb_wksta_info_0_setinfo	"WDOOOOBBDWDWWWWWWWWWWWWWWWWWWWzzW"
#define	REMSmb_wksta_info_1		"WDzzzzBBDWDWWWWWWWWWWWWWWWWWWWzzWzzW"
#define	REMSmb_wksta_info_1_setinfo	"WDOOOOBBDWDWWWWWWWWWWWWWWWWWWWzzWzzW"
#define	REMSmb_wksta_info_10		"zzzBBzz"
#define	REMSmb_wksta_annc_info		"K"

#define	REMSmb_NetWkstaLogon_P		"zzirL"
#define	REMSmb_NetWkstaLogoff_P		"zD"
#define	REMSmb_NetWkstaSetUID_P		"zzzW"
#define	REMSmb_NetWkstaGetInfo_P	"WrLh"
#define	REMSmb_NetWkstaSetInfo_P	"WsTP"
#define	REMSmb_NetWkstaUserLogon_P	"zzWb54WrLh"
#define	REMSmb_NetWkstaUserLogoff_P	"zzWb38WrLh"

#define	REMSmb_use_info_0		"B9Bz"
#define	REMSmb_use_info_1		"B9BzzWWWW"

#define	REMSmb_use_info_2		"B9BzzWWWWWWWzB16"

#define	REMSmb_NetUseEnum_P		"WrLeh"
#define	REMSmb_NetUseAdd_P		"WsT"
#define	REMSmb_NetUseDel_P		"zW"
#define	REMSmb_NetUseGetInfo_P		"zWrLh"

#define	REMSmb_printQ_0			"B13"
#define	REMSmb_printQ_1			"B13BWWWzzzzzWW"
#define	REMSmb_printQ_2			"B13BWWWzzzzzWN"
#define	REMSmb_printQ_3			"zWWWWzzzzWWzzl"
#define	REMSmb_printQ_4			"zWWWWzzzzWNzzl"
#define	REMSmb_printQ_5			"z"

#define	REMSmb_DosPrintQEnum_P		"WrLeh"
#define	REMSmb_DosPrintQGetInfo_P	"zWrLh"
#define	REMSmb_DosPrintQSetInfo_P	"zWsTP"
#define	REMSmb_DosPrintQAdd_P		"WsT"
#define	REMSmb_DosPrintQDel_P		"z"
#define	REMSmb_DosPrintQPause_P		"z"
#define	REMSmb_DosPrintQPurge_P		"z"
#define	REMSmb_DosPrintQContinue_P	"z"

#define	REMSmb_print_job_0		"W"
#define	REMSmb_print_job_1		"WB21BB16B10zWWzDDz"
#define	REMSmb_print_job_2		"WWzWWDDzz"
#define	REMSmb_print_job_3		"WWzWWDDzzzzzzzzzzlz"

#define	REMSmb_print_job_info_1_setinfo	"WB21BB16B10zWWODDz"
#define	REMSmb_print_job_info_3_setinfo	"WWzWWDDzzzzzOzzzzlO"

#define	REMSmb_DosPrintJobEnum_P	"zWrLeh"
#define	REMSmb_DosPrintJobGetInfo_P	"WWrLh"
#define	REMSmb_DosPrintJobSetInfo_P	"WWsTP"
#define	REMSmb_DosPrintJobAdd_P		"zsTF129g129h"	/* See note 2 */
#define	REMSmb_DosPrintJobSchedule_P	"W"
#define	REMSmb_DosPrintJobDel_P		"W"
#define	REMSmb_DosPrintJobPause_P	"W"
#define	REMSmb_DosPrintJobContinue_P	"W"

#define	REMSmb_print_dest_0		"B9"
#define	REMSmb_print_dest_1		"B9B21WWzW"
#define	REMSmb_print_dest_2		"z"
#define	REMSmb_print_dest_3		"zzzWWzzzWW"
#define	REMSmb_print_dest_info_3_setinfo "zOzWWOzzWW"

#define	REMSmb_DosPrintDestEnum_P	"WrLeh"
#define	REMSmb_DosPrintDestGetInfo_P	"zWrLh"
#define	REMSmb_DosPrintDestControl_P	"zW"
#define	REMSmb_DosPrintDestAdd_P	"WsT"
#define	REMSmb_DosPrintDestSetInfo_P	"zWsTP"
#define	REMSmb_DosPrintDestDel_P	"z"

#define	REMSmb_NetProfileSave_P		"zDW"
#define	REMSmb_NetProfileLoad_P		"zDrLD"

#define	REMSmb_profile_load_info	"WDzD"

#define	REMSmb_statistics_info		"B"

#define	REMSmb_statistics2_info_W	"B120"
#define	REMSmb_stat_workstation_0	"DDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"
#define	REMSmb_statistics2_info_S	"B68"
#define	REMSmb_stat_server_0		"DDDDDDDDDDDDDDDDD"

#define	REMSmb_NetStatisticsGet_P	"rLeh"
#define	REMSmb_NetStatisticsClear_P	""

#define	REMSmb_NetStatisticsGet2_P	"zDWDrLh"

#define	REMSmb_NetRemoteTOD_P		"rL"

#define	REMSmb_time_of_day_info		"DDBBBBWWBBWB"

#define	REMSmb_netbios_info_0		"B17"
#define	REMSmb_netbios_info_1		"B17B9BBWWDWWW"

#define	REMSmb_NetBiosEnum_P		"WrLeh"
#define	REMSmb_NetBiosGetInfo_P		"zWrLh"

#define	REMSmb_Spl_open_data		"zzlzzzzzz"
#define	REMSmb_plain_data		"K"

#define	REMSmb_NetSplQmAbort_P		"Di"
#define	REMSmb_NetSplQmClose_P		"Di"
#define	REMSmb_NetSplQmEndDoc_P		"Dhi"
#define	REMSmb_NetSplQmOpen_P		"zTsWii"
#define	REMSmb_NetSplQmStartDoc_P	"Dzi"
#define	REMSmb_NetSplQmWrite_P		"DTsi"

#define	REMSmb_configgetall_info	"B"
#define	REMSmb_configget_info		"B"
#define	REMSmb_configset_info_0		"zz"

#define	REMSmb_NetConfigGetAll_P	"zzrLeh"
#define	REMSmb_NetConfigGet_P		"zzzrLe"
#define	REMSmb_NetConfigSet_P		"zzWWsTD"

#define	REMSmb_NetBuildGetInfo_P	"DWrLh"
#define	REMSmb_build_info_0		"WD"

#define	REMSmb_NetGetDCName_P		"zrL"
#define	REMSmb_dc_name			"B18"

#define	REMSmb_challenge_info_0	"B8"
#define	REMSmb_account_delta_info_0	"K"
#define	REMSmb_account_sync_info_0	"K"

#define	REMSmb_NetAccountDeltas_P	"zb12g12b24WWrLehg24"
#define	REMSmb_NetAccountSync_P		"zb12g12DWrLehig24"

#define	REMSmb_NetLogonEnum_P		"WrLeh"

#define	REMSmb_I_NetPathType_P		"ziD"
#define	REMSmb_I_NetPathCanonicalize_P	"zrLziDD"
#define	REMSmb_I_NetPathCompare_P	"zzDD"
#define	REMSmb_I_NetNameValidate_P	"zWD"
#define	REMSmb_I_NetNameCanonicalize_P	"zrLWD"
#define	REMSmb_I_NetNameCompare_P	"zzWD"

#define	REMSmb_LocalOnlyCall		""

/*
 * The following definitions exist for DOS LANMAN--Windows 3.0.
 * Normally, there is a  const char far * servername
 * as the first parameter, but this will be ignored (sort of).
 */
#define	REMSmb_DosPrintJobGetId_P	"WrL"
#define	REMSmb_GetPrintId		"WB16B13B"
#define	REMSmb_NetRemoteCopy_P		"zzzzWWrL"
#define	REMSmb_copy_info		"WB1"
#define	REMSmb_NetRemoteMove_P		"zzzzWWrL"
#define	REMSmb_move_info		"WB1"
#define	REMSmb_NetHandleGetInfo_P	"WWrLh"
#define	REMSmb_NetHandleSetInfo_P	"WWsTP"
#define	REMSmb_handle_info_1		"DW"
#define	REMSmb_handle_info_2		"z"
#define	REMSmb_WWkstaGetInfo_P		"WrLhOW"

/* The following strings are defined for RIPL APIs */

#define	REMSmb_RplWksta_info_0		"z"
#define	REMSmb_RplWksta_info_1		"zz"
#define	REMSmb_RplWksta_info_2		"b13b16b15b15zN"
#define	REMSmb_RplWksta_info_3		"b16b49"

#define	REMSmb_RplWkstaEnum_P		"WzWrLehb4g4"
#define	REMSmb_RplWkstaGetInfo_P	"zWrLh"
#define	REMSmb_RplWkstaSetInfo_P	"zWsTPW"
#define	REMSmb_RplWkstaAdd_P		"WsTW"
#define	REMSmb_RplWkstaDel_P		"zW"

#define	REMSmb_RplProfile_info_0	"z"
#define	REMSmb_RplProfile_info_1	"zz"
#define	REMSmb_RplProfile_info_2	"b16b47"
#define	REMSmb_RplProfile_info_3	"b16b47b16"

#define	REMSmb_RplProfileEnum_P		"WzWrLehb4g4"
#define	REMSmb_RplProfileGetInfo_P	"zWrLh"
#define	REMSmb_RplProfileSetInfo_P	"zWsTP"
#define	REMSmb_RplProfileAdd_P		"WzsTW"
#define	REMSmb_RplProfileDel_P		"zW"
#define	REMSmb_RplProfileClone_P	"WzsTW"
#define	REMSmb_RplBaseProfileEnum_P	"WrLehb4g4"


/* LAN Manager 3.0 API strings go here */

#define	REMSmb_I_GuidGetAgent_P	"g6i"
#define	REMSmb_I_GuidSetAgent_P	"b6D"


/* update support */

#define	REMSmb_NetAccountUpdate_P	"b12g12WWrLh"
#define	REMSmb_NetAccountConfirmUpd_P	"b12g12D"
#define	REMSmb_update_info_0	"K"

/*
 * SamrOemChangePasswordUser2 api support
 */
#define	REMSmb_SamOEMChgPasswordUser2	"B516B16"  /* data that is passed */

#endif	/* ndef	_REMDEF_ */
