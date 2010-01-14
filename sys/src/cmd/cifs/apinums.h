/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1987-1991             **/
/********************************************************************/

#define API_WShareEnum			0
#define API_WShareGetInfo		1
#define API_WShareSetInfo		2
#define API_WShareAdd			3
#define API_WShareDel			4
#define API_NetShareCheck		5
#define API_WSessionEnum		6
#define API_WSessionGetInfo		7
#define API_WSessionDel			8
#define API_WConnectionEnum		9
#define API_WFileEnum			10
#define API_WFileGetInfo		11
#define API_WFileClose			12
#define API_WServerGetInfo		13
#define API_WServerSetInfo		14
#define API_WServerDiskEnum		15
#define API_WServerAdminCommand		16
#define API_NetAuditOpen		17
#define API_WAuditClear			18
#define API_NetErrorLogOpen		19
#define API_WErrorLogClear		20
#define API_NetCharDevEnum		21
#define API_NetCharDevGetInfo		22
#define API_WCharDevControl		23
#define API_NetCharDevQEnum		24
#define API_NetCharDevQGetInfo		25
#define API_WCharDevQSetInfo		26
#define API_WCharDevQPurge		27
#define API_WCharDevQPurgeSelf		28
#define API_WMessageNameEnum		29
#define API_WMessageNameGetInfo		30
#define API_WMessageNameAdd		31
#define API_WMessageNameDel		32
#define API_WMessageNameFwd		33
#define API_WMessageNameUnFwd		34
#define API_WMessageBufferSend		35
#define API_WMessageFileSend		36
#define API_WMessageLogFileSet		37
#define API_WMessageLogFileGet		38
#define API_WServiceEnum		39
#define API_WServiceInstall		40
#define API_WServiceControl		41
#define API_WAccessEnum			42
#define API_WAccessGetInfo		43
#define API_WAccessSetInfo		44
#define API_WAccessAdd			45
#define API_WAccessDel			46
#define API_WGroupEnum			47
#define API_WGroupAdd			48
#define API_WGroupDel			49
#define API_WGroupAddUser		50
#define API_WGroupDelUser		51
#define API_WGroupGetUsers		52
#define API_WUserEnum			53
#define API_WUserAdd			54
#define API_WUserDel			55
#define API_WUserGetInfo		56
#define API_WUserSetInfo		57
#define API_WUserPasswordSet		58
#define API_WUserGetGroups		59
#define API_DeadTableEntry		60
/* This line and number replaced a Dead Entry */
#define API_WWkstaSetUID		62
#define API_WWkstaGetInfo		63
#define API_WWkstaSetInfo		64
#define API_WUseEnum			65
#define API_WUseAdd			66
#define API_WUseDel			67
#define API_WUseGetInfo			68
#define API_WPrintQEnum			69
#define API_WPrintQGetInfo		70
#define API_WPrintQSetInfo		71
#define API_WPrintQAdd			72
#define API_WPrintQDel			73
#define API_WPrintQPause		74
#define API_WPrintQContinue		75
#define API_WPrintJobEnum		76
#define API_WPrintJobGetInfo		77
#define API_WPrintJobSetInfo_OLD	78
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
#define API_WPrintJobDel		81
#define API_WPrintJobPause		82
#define API_WPrintJobContinue		83
#define API_WPrintDestEnum		84
#define API_WPrintDestGetInfo		85
#define API_WPrintDestControl		86
#define API_WProfileSave		87
#define API_WProfileLoad		88
#define API_WStatisticsGet		89
#define API_WStatisticsClear		90
#define API_NetRemoteTOD		91
#define API_WNetBiosEnum		92
#define API_WNetBiosGetInfo		93
#define API_NetServerEnum		94
#define API_I_NetServerEnum		95
#define API_WServiceGetInfo		96
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
#define API_WPrintQPurge		103
#define API_NetServerEnum2		104
#define API_WAccessGetUserPerms		105
#define API_WGroupGetInfo		106
#define API_WGroupSetInfo		107
#define API_WGroupSetUsers		108
#define API_WUserSetGroups		109
#define API_WUserModalsGet		110
#define API_WUserModalsSet		111
#define API_WFileEnum2			112
#define API_WUserAdd2			113
#define API_WUserSetInfo2		114
#define API_WUserPasswordSet2		115
#define API_I_NetServerEnum2		116
#define API_WConfigGet2			117
#define API_WConfigGetAll2		118
#define API_WGetDCName			119
#define API_NetHandleGetInfo		120
#define API_NetHandleSetInfo		121
#define API_WStatisticsGet2		122
#define API_WBuildGetInfo		123
#define API_WFileGetInfo2		124
#define API_WFileClose2			125
#define API_WNetServerReqChallenge	126
#define API_WNetServerAuthenticate	127
#define API_WNetServerPasswordSet	128
#define API_WNetAccountDeltas		129
#define API_WNetAccountSync		130
#define API_WUserEnum2			131
#define API_WWkstaUserLogon		132
#define API_WWkstaUserLogoff		133
#define API_WLogonEnum			134
#define API_WErrorLogRead		135
#define API_WI_NetPathType		136
#define API_WI_NetPathCanonicalize	137
#define API_WI_NetPathCompare		138
#define API_WI_NetNameValidate		139
#define API_WI_NetNameCanonicalize	140
#define API_WI_NetNameCompare		141
#define API_WAuditRead			142
#define API_WPrintDestAdd		143
#define API_WPrintDestSetInfo		144
#define API_WPrintDestDel		145
#define API_WUserValidate2		146
#define API_WPrintJobSetInfo		147
#define API_TI_NetServerDiskEnum	148
#define API_TI_NetServerDiskGetInfo	149
#define API_TI_FTVerifyMirror		150
#define API_TI_FTAbortVerify		151
#define API_TI_FTGetInfo		152
#define API_TI_FTSetInfo		153
#define API_TI_FTLockDisk		154
#define API_TI_FTFixError		155
#define API_TI_FTAbortFix		156
#define API_TI_FTDiagnoseError		157
#define API_TI_FTGetDriveStats		158
/* This line and number replaced a Dead Entry */
#define API_TI_FTErrorGetInfo		160
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
#define API_NetAccessCheck		163
#define API_NetAlertRaise		164
#define API_NetAlertStart		165
#define API_NetAlertStop		166
#define API_NetAuditWrite		167
#define API_NetIRemoteAPI		168
#define API_NetServiceStatus		169
#define API_I_NetServerRegister		170
#define API_I_NetServerDeregister	171
#define API_I_NetSessionEntryMake	172
#define API_I_NetSessionEntryClear	173
#define API_I_NetSessionEntryGetInfo	174
#define API_I_NetSessionEntrySetInfo	175
#define API_I_NetConnectionEntryMake	176
#define API_I_NetConnectionEntryClear	177
#define API_I_NetConnectionEntrySetInfo	178
#define API_I_NetConnectionEntryGetInfo	179
#define API_I_NetFileEntryMake		180
#define API_I_NetFileEntryClear		181
#define API_I_NetFileEntrySetInfo	182
#define API_I_NetFileEntryGetInfo	183
#define API_AltSrvMessageBufferSend	184
#define API_AltSrvMessageFileSend	185
#define API_wI_NetRplWkstaEnum		186
#define API_wI_NetRplWkstaGetInfo	187
#define API_wI_NetRplWkstaSetInfo	188
#define API_wI_NetRplWkstaAdd		189
#define API_wI_NetRplWkstaDel		190
#define API_wI_NetRplProfileEnum	191
#define API_wI_NetRplProfileGetInfo	192
#define API_wI_NetRplProfileSetInfo	193
#define API_wI_NetRplProfileAdd		194
#define API_wI_NetRplProfileDel		195
#define API_wI_NetRplProfileClone	196
#define API_wI_NetRplBaseProfileEnum	197
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
#define API_WIServerSetInfo		201
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
/* This line and number replaced a Dead Entry */
#define API_WPrintDriverEnum		205
#define API_WPrintQProcessorEnum	206
#define API_WPrintPortEnum		207
#define API_WNetWriteUpdateLog		208
#define API_WNetAccountUpdate		209
#define API_WNetAccountConfirmUpdate	210
#define API_WConfigSet			211
#define API_WAccountsReplicate		212
/*   213 is used by WfW  */
#define API_SamOEMChgPasswordUser2_P	214
#define API_NetServerEnum3		215
#define MAX_API				215
