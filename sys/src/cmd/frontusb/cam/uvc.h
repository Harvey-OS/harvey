typedef struct VCDescriptor {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
} VCDescriptor;

typedef struct VCHeader {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bNumFormats;
	u8 wTotalLength[2];
	u8 dwClockFrequency[4];
	u8 bInCollection;
	u8 baInterfaceNr[1];
} VCHeader;

typedef struct VCInputTerminal {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bTerminalID;
	u8 wTerminalType[2];
	u8 bAssocTerminal;
	u8 iTerminal;
} VCInputTerminal;

typedef struct VCOutputTerminal {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bTerminalID;
	u8 wTerminalType[2];
	u8 bAssocTerminal;
	u8 bSourceID;
	u8 iTerminal;
} VCOutputTerminal;

typedef struct VCCameraTerminal {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bTerminalID;
	u8 wTerminalType[2];
	u8 bAssocTerminal;
	u8 iTerminal;
	u8 wObjectiveFocalLengthMin[2];
	u8 wObjectiveFocalLengthMax[2];
	u8 wOcularFocalLength[2];
	u8 bControlSize;
	u8 bmControls[3];
} VCCameraTerminal;

typedef struct VCUnit {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bUnitID;
} VCUnit;

typedef struct VCSelectorUnit {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bUnitID;
	u8 bNrInPins;
	u8 baSourceID[1];
	/* after baSourceID: uchar iSelector; */
} VCSelectorUnit;

typedef struct VCProcessingUnit {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bUnitID;
	u8 bSourceID;
	u8 wMaxMultiplier[2];
	u8 bControlSize;
	u8 bmControls[3];
	u8 iProcessing;
	u8 bmVideoStandards;
} VCProcessingUnit;

typedef struct VCEncodingUnit {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bUnitID;
	u8 bSourceID;
	u8 iEncoding;
	u8 bControlSize;
	u8 bmControls[3];
	u8 bmControlsRuntime[3];
} VCEncodingUnit;

typedef struct VCExtensionUnit {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bUnitID;
	u8 guidExtensionCode[16];
	u8 bNumControls;
	u8 bNrInPins;
	u8 baSourceID[1];
	/*
		uchar bControlSize;
		uchar bmControls[1];
		uchar iExtension;
	*/
} VCExtensionUnit;

typedef struct VSInputHeader {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bNumFormats;
	u8 wTotalLength[2];
	u8 bEndpointAddress;
	u8 bmInfo;
	u8 bTerminalLink;
	u8 bStillCaptureMethod;
	u8 bTriggerSupport;
	u8 bTriggerUsage;
	u8 bControlSize;
	u8 bmaControls[1];
} VSInputHeader;

typedef struct VSOutputHeader {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bNumFormats;
	u8 wTotalLength[2];
	u8 bEndpointAddress;
	u8 bTerminalLink;
	u8 bControlSize;
	u8 bmaControls[1];
} VSOutputHeader;

typedef struct VSStillFrame {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bEndpointAddress;
	u8 bNumImageSizePatterns;
	u8 data[1];
} VSStillFrame;

typedef struct VSUncompressedFormat {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bFormatIndex;
	u8 bNumFrameDescriptors;
	u8 guidFormat[16];
	u8 bBitsPerPixel;
	u8 bDefaultFrameIndex;
	u8 bAspectRatioX;
	u8 bAspectRatioY;
	u8 bmInterlaceFlags;
	u8 bCopyProtect;
} VSUncompressedFormat;

typedef struct VSUncompressedFrame {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bFrameIndex;
	u8 bmCapabilities;
	u8 wWidth[2];
	u8 wHeight[2];
	u8 dwMinBitRate[4];
	u8 dwMaxBitRate[4];
	u8 dwMaxVideoFrameBufferSize[4];
	u8 dwDefaultFrameInterval[4];
	u8 bFrameIntervalType;
	u8 dwFrameInterval[1][4];
} VSUncompressedFrame;

typedef struct VSColorFormat {
	u8 bLength;
	u8 bDescriptorType;
	u8 bDescriptorSubtype;
	u8 bColorPrimaries;
	u8 bTransferCharacteristics;
	u8 bMatrixCoefficients;
} VSColorFormat;

typedef struct ProbeControl {
	u8 bmHint[2];
	u8 bFormatIndex;
	u8 bFrameIndex;
	u8 dwFrameInterval[4];
	u8 wKeyFrameRate[2];
	u8 wPFrameRate[2];
	u8 wCompQuality[2];
	u8 wCompWindowSize[2];
	u8 wDelay[2];
	u8 dwMaxVideoFrameSize[4];
	u8 dwMaxPayloadTransferSize[4];
	u8 dwClockFrequency[4];
	u8 bmFramingInfo;
	u8 bPreferedVersion;
	u8 bMinVersion;
	u8 bMaxVersion;
	u8 bBitDepthLuma;
	u8 bmSettings;
	u8 bMaxNumberOfRefFramesPlus1;
	u8 bmRateControlModes[2];
	u8 bmLayoutPerStream[8];
} ProbeControl;

enum {
	CC_VIDEO = 0x0E,
	SC_UNDEFINED = 0x00,
	SC_VIDEOCONTROL = 0x01,
	SC_VIDEOSTREAMING = 0x02,
	SC_VIDEO_INTERFACE_COLLECTION = 0x03,
	PC_PROTOCOL_UNDEFINED = 0x00,
	PC_PROTOCOL_15 = 0x01,
	CS_UNDEFINED = 0x20,
	CS_DEVICE = 0x21,
	CS_CONFIGURATION = 0x22,
	CS_STRING = 0x23,
	CS_INTERFACE = 0x24,
	CS_ENDPOINT = 0x25,
	VC_DESCRIPTOR_UNDEFINED = 0x00,
	VC_HEADER = 0x01,
	VC_INPUT_TERMINAL = 0x02,
	VC_OUTPUT_TERMINAL = 0x03,
	VC_SELECTOR_UNIT = 0x04,
	VC_PROCESSING_UNIT = 0x05,
	VC_EXTENSION_UNIT = 0x06,
	VC_ENCODING_UNIT = 0x07,
	VS_UNDEFINED = 0x00,
	VS_INPUT_HEADER = 0x01,
	VS_OUTPUT_HEADER = 0x02,
	VS_STILL_IMAGE_FRAME = 0x03,
	VS_FORMAT_UNCOMPRESSED = 0x04,
	VS_FRAME_UNCOMPRESSED = 0x05,
	VS_FORMAT_MJPEG = 0x06,
	VS_FRAME_MJPEG = 0x07,
	VS_FORMAT_MPEG2TS = 0x0A,
	VS_FORMAT_DV = 0x0C,
	VS_COLORFORMAT = 0x0D,
	VS_FORMAT_FRAME_BASED = 0x10,
	VS_FRAME_FRAME_BASED = 0x11,
	VS_FORMAT_STREAM_BASED = 0x12,
	VS_FORMAT_H264 = 0x13,
	VS_FRAME_H264 = 0x14,
	VS_FORMAT_H264_SIMULCAST = 0x15,
	VS_FORMAT_VP8 = 0x16,
	VS_FRAME_VP8 = 0x17,
	VS_FORMAT_VP8_SIMULCAST = 0x18,
	RC_UNDEFINED = 0x00,
	SET_CUR = 0x01,
	SET_CUR_ALL = 0x11,
	GET_CUR = 0x81,
	GET_MIN = 0x82,
	GET_MAX = 0x83,
	GET_RES = 0x84,
	GET_LEN = 0x85,
	GET_INFO = 0x86,
	GET_DEF = 0x87,
	GET_CUR_ALL = 0x91,
	GET_MIN_ALL = 0x92,
	GET_MAX_ALL = 0x93,
	GET_RES_ALL = 0x94,
	GET_DEF_ALL = 0x97,
	VC_CONTROL_UNDEFINED = 0x00,
	VC_VIDEO_POWER_MODE_CONTROL = 0x01,
	VC_REQUEST_ERROR_CODE_CONTROL = 0x02,
	TE_CONTROL_UNDEFINED = 0x00,
	SU_CONTROL_UNDEFINED = 0x00,
	SU_INPUT_SELECT_CONTROL = 0x01,
	CT_CONTROL_UNDEFINED = 0x00,
	CT_SCANNING_MODE_CONTROL = 0x01,
	CT_AE_MODE_CONTROL = 0x02,
	CT_AE_PRIORITY_CONTROL = 0x03,
	CT_EXPOSURE_TIME_ABSOLUTE_CONTROL = 0x04,
	CT_EXPOSURE_TIME_RELATIVE_CONTROL = 0x05,
	CT_FOCUS_ABSOLUTE_CONTROL = 0x06,
	CT_FOCUS_RELATIVE_CONTROL = 0x07,
	CT_FOCUS_AUTO_CONTROL = 0x08,
	CT_IRIS_ABSOLUTE_CONTROL = 0x09,
	CT_IRIS_RELATIVE_CONTROL = 0x0a,
	CT_ZOOM_ABSOLUTE_CONTROL = 0x0b,
	CT_ZOOM_RELATIVE_CONTROL = 0x0c,
	CT_PANTILT_ABSOLUTE_CONTROL = 0x0d,
	CT_PANTILT_RELATIVE_CONTROL = 0x0e,
	CT_ROLL_ABSOLUTE_CONTROL = 0x0f,
	CT_ROLL_RELATIVE_CONTROL = 0x10,
	CT_PRIVACY_CONTROL = 0x11,
	CT_FOCUS_SIMPLE_CONTROL = 0x12,
	CT_WINDOW_CONTROL = 0x13,
	CT_REGION_OF_INTEREST_CONTROL = 0x14,
	PU_CONTROL_UNDEFINED = 0x00,
	PU_BACKLIGHT_COMPENSATION_CONTROL = 0x01,
	PU_BRIGHTNESS_CONTROL = 0x02,
	PU_CONTRAST_CONTROL = 0x03,
	PU_GAIN_CONTROL = 0x04,
	PU_POWER_LINE_FREQUENCY_CONTROL = 0x05,
	PU_HUE_CONTROL = 0x06,
	PU_SATURATION_CONTROL = 0x07,
	PU_SHARPNESS_CONTROL = 0x08,
	PU_GAMMA_CONTROL = 0x09,
	PU_WHITE_BALANCE_TEMPERATURE_CONTROL = 0x0a,
	PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL = 0x0b,
	PU_WHITE_BALANCE_COMPONENT_CONTROL = 0x0c,
	PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL = 0x0d,
	PU_DIGITAL_MULTIPLIER_CONTROL = 0x0e,
	PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL = 0x0f,
	PU_HUE_AUTO_CONTROL = 0x10,
	PU_ANALOG_VIDEO_STANDARD_CONTROL = 0x11,
	PU_ANALOG_LOCK_STATUS_CONTROL = 0x12,
	PU_CONTRAST_AUTO_CONTROL = 0x13,
	EU_CONTROL_UNDEFINED = 0x00,
	EU_SELECT_LAYER_CONTROL = 0x01,
	EU_PROFILE_TOOLSET_CONTROL = 0x02,
	EU_VIDEO_RESOLUTION_CONTROL = 0x03,
	EU_MIN_FRAME_INTERVAL_CONTROL = 0x04,
	EU_SLICE_MODE_CONTROL = 0x05,
	EU_RATE_CONTROL_MODE_CONTROL = 0x06,
	EU_AVERAGE_BITRATE_CONTROL = 0x07,
	EU_CPB_SIZE_CONTROL = 0x08,
	EU_PEAK_BIT_RATE_CONTROL = 0x09,
	EU_QUANTIZATION_PARAMS_CONTROL = 0x0a,
	EU_SYNC_REF_FRAME_CONTROL = 0x0b,
	EU_LTR_BUFFER_ = 0x0c,
	EU_LTR_PICTURE_CONTROL = 0x0d,
	EU_LTR_VALIDATION_CONTROL = 0x0e,
	EU_LEVEL_IDC_LIMIT_CONTROL = 0x0f,
	EU_SEI_PAYLOADTYPE_CONTROL = 0x10,
	EU_QP_RANGE_CONTROL = 0x11,
	EU_PRIORITY_CONTROL = 0x12,
	EU_START_OR_STOP_LAYER_CONTROL = 0x13,
	EU_ERROR_RESILIENCY_CONTROL = 0x14,
	XU_CONTROL_UNDEFINED = 0x00,
	VS_CONTROL_UNDEFINED = 0x00,
	VS_PROBE_CONTROL = 0x01,
	VS_COMMIT_CONTROL = 0x02,
	VS_STILL_PROBE_CONTROL = 0x03,
	VS_STILL_COMMIT_CONTROL = 0x04,
	VS_STILL_IMAGE_TRIGGER_CONTROL = 0x05,
	VS_STREAM_ERROR_CODE_CONTROL = 0x06,
	VS_GENERATE_KEY_FRAME_CONTROL = 0x07,
	VS_UPDATE_FRAME_SEGMENT_CONTROL = 0x08,
	VS_SYNCH_DELAY_CONTROL = 0x09,
	ITT_VENDOR_SPECIFIC = 0x0200,
	ITT_CAMERA = 0x0201,
	ITT_MEDIA_TRANSPORT_INPUT,
};

#define	GET3(p)		(((p)[2] & 0xFF)<<16 | ((p)[1] & 0xFF)<<8 | ((p)[0] & 0xFF))
