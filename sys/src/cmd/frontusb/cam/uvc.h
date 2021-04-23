typedef struct VCDescriptor {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
} VCDescriptor;

typedef struct VCHeader {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bNumFormats;
	uchar wTotalLength[2];
	uchar dwClockFrequency[4];
	uchar bInCollection;
	uchar baInterfaceNr[1];
} VCHeader;

typedef struct VCInputTerminal {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bTerminalID;
	uchar wTerminalType[2];
	uchar bAssocTerminal;
	uchar iTerminal;
} VCInputTerminal;

typedef struct VCOutputTerminal {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bTerminalID;
	uchar wTerminalType[2];
	uchar bAssocTerminal;
	uchar bSourceID;
	uchar iTerminal;
} VCOutputTerminal;

typedef struct VCCameraTerminal {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bTerminalID;
	uchar wTerminalType[2];
	uchar bAssocTerminal;
	uchar iTerminal;
	uchar wObjectiveFocalLengthMin[2];
	uchar wObjectiveFocalLengthMax[2];
	uchar wOcularFocalLength[2];
	uchar bControlSize;
	uchar bmControls[3];
} VCCameraTerminal;

typedef struct VCUnit {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bUnitID;
} VCUnit;

typedef struct VCSelectorUnit {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bUnitID;
	uchar bNrInPins;
	uchar baSourceID[1];
	/* after baSourceID: uchar iSelector; */
} VCSelectorUnit;

typedef struct VCProcessingUnit {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bUnitID;
	uchar bSourceID;
	uchar wMaxMultiplier[2];
	uchar bControlSize;
	uchar bmControls[3];
	uchar iProcessing;
	uchar bmVideoStandards;
} VCProcessingUnit;

typedef struct VCEncodingUnit {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bUnitID;
	uchar bSourceID;
	uchar iEncoding;
	uchar bControlSize;
	uchar bmControls[3];
	uchar bmControlsRuntime[3];
} VCEncodingUnit;

typedef struct VCExtensionUnit {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bUnitID;
	uchar guidExtensionCode[16];
	uchar bNumControls;
	uchar bNrInPins;
	uchar baSourceID[1];
	/*
		uchar bControlSize;
		uchar bmControls[1];
		uchar iExtension;
	*/
} VCExtensionUnit;

typedef struct VSInputHeader {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bNumFormats;
	uchar wTotalLength[2];
	uchar bEndpointAddress;
	uchar bmInfo;
	uchar bTerminalLink;
	uchar bStillCaptureMethod;
	uchar bTriggerSupport;
	uchar bTriggerUsage;
	uchar bControlSize;
	uchar bmaControls[1];
} VSInputHeader;

typedef struct VSOutputHeader {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bNumFormats;
	uchar wTotalLength[2];
	uchar bEndpointAddress;
	uchar bTerminalLink;
	uchar bControlSize;
	uchar bmaControls[1];
} VSOutputHeader;

typedef struct VSStillFrame {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bEndpointAddress;
	uchar bNumImageSizePatterns;
	uchar data[1];
} VSStillFrame;

typedef struct VSUncompressedFormat {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bFormatIndex;
	uchar bNumFrameDescriptors;
	uchar guidFormat[16];
	uchar bBitsPerPixel;
	uchar bDefaultFrameIndex;
	uchar bAspectRatioX;
	uchar bAspectRatioY;
	uchar bmInterlaceFlags;
	uchar bCopyProtect;
} VSUncompressedFormat;

typedef struct VSUncompressedFrame {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bFrameIndex;
	uchar bmCapabilities;
	uchar wWidth[2];
	uchar wHeight[2];
	uchar dwMinBitRate[4];
	uchar dwMaxBitRate[4];
	uchar dwMaxVideoFrameBufferSize[4];
	uchar dwDefaultFrameInterval[4];
	uchar bFrameIntervalType;
	uchar dwFrameInterval[1][4];
} VSUncompressedFrame;

typedef struct VSColorFormat {
	uchar bLength;
	uchar bDescriptorType;
	uchar bDescriptorSubtype;
	uchar bColorPrimaries;
	uchar bTransferCharacteristics;
	uchar bMatrixCoefficients;
} VSColorFormat;

typedef struct ProbeControl {
	uchar bmHint[2];
	uchar bFormatIndex;
	uchar bFrameIndex;
	uchar dwFrameInterval[4];
	uchar wKeyFrameRate[2];
	uchar wPFrameRate[2];
	uchar wCompQuality[2];
	uchar wCompWindowSize[2];
	uchar wDelay[2];
	uchar dwMaxVideoFrameSize[4];
	uchar dwMaxPayloadTransferSize[4];
	uchar dwClockFrequency[4];
	uchar bmFramingInfo;
	uchar bPreferedVersion;
	uchar bMinVersion;
	uchar bMaxVersion;
	uchar bBitDepthLuma;
	uchar bmSettings;
	uchar bMaxNumberOfRefFramesPlus1;
	uchar bmRateControlModes[2];
	uchar bmLayoutPerStream[8];
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
