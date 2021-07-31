/*
 *	Audio Control Class
 */
enum
{
	/* subclass codes */
	AUDIOCONTROL = 0x01,
	AUDIOSTREAMING = 0x02,
	MIDISTREAMING = 0x03,

	/* class-specific descriptor types */
	CS_UNDEFINED = 0x20,
	CS_DEVICE = 0x21,
	CS_CONFIG = 0x22,
	CS_STRING = 0x23,
	CS_INTERFACE = 0x24,
	CS_ENDPOINT = 0x25,

	/* audiocontrol descriptor subtypes */
	HEADER = 0x01,
	INPUT_TERMINAL = 0x02,
	OUTPUT_TERMINAL = 0x03,
	MIXER_UNIT = 0x04,
	SELECTOR_UNIT = 0x05,
	FEATURE_UNIT = 0x06,
	PROCESSING_UNIT = 0x07,
	EXTENSION_UNIT = 0x08,

	/* audiostreaming descriptor subtypes */
	AS_GENERAL = 0x01,
	FORMAT_TYPE = 0x02,
	FORMAT_SPECIFIC = 0x03,

	/* processing unit process types */
	UPDOWNMIX_PROCESS = 0x01,
	DOLBY_PROLOGIC_PROCESS = 0x02,
	STEREO_3D_PROCESS = 0x03,
	REVERB_PROCESS = 0x04,
	CHORUS_PROCESS = 0x05,
	DYN_RANGE_COMP_PROCESS = 0x06,

	/* endpoint descriptor subtypes */
	EP_GENERAL = 0x01,

	/* class-specific request codes */
	GET_CUR = 0x81,
	GET_MIN = 0x82,
	GET_MAX = 0x83,
	GET_RES = 0x84,
	GET_MEM = 0x85,
	GET_STAT = 0xff,
	SET_CUR = 0x01,
	SET_MIN = 0x02,
	SET_MAX = 0x03,
	SET_RES = 0x04,
	SET_MEM = 0x05,

	/* feature unit control selectors */
	MUTE_CONTROL = 0x01,
	VOLUME_CONTROL = 0x02,
	BASS_CONTROL = 0x03,
	MID_CONTROL = 0x04,
	TREBLE_CONTROL = 0x05,
	EQUALIZER_CONTROL = 0x06,
	AGC_CONTROL = 0x07,
	DELAY_CONTROL = 0x08,
	BASS_BOOST_CONTROL = 0x09,
	LOUDNESS_CONTROL = 0x0a,

	/* up/down-mix processing unit control selectors */
	UD_ENABLE_CONTROL = 0x01,
	UD_MODE_SELECT_CONTROL = 0x02,

	/* dolby pro-logic processing unit control selectors */
	DP_ENABLE_CONTROL = 0x01,
	DP_MODE_SELECT_CONTROL = 0x02,

	/* 3D stereo expander processing unit control selectors */
	STEREO_3D_ENABLE_CONTROL = 0x01,
	SPACIOUSNESS_CONTROL = 0x03,

	/* reverb processing unit control selectors */
	REVERB_ENABLE_CONTROL = 0x01,
	REVERB_LEVEL_CONTROL = 0x02,
	REVERB_TIME_CONTROL = 0x03,
	REVERB_FEEDBACK_CONTROL = 0x04,

	/* chorus processing unit control selectors */
	CHORUS_ENABLE_CONTROL = 0x01,
	CHORUS_LEVEL_CONTROL = 0x02,
	CHORUS_RATE_CONTROL = 0x03,
	CHORUS_DEPTH_CONTROL = 0x04,

	/* dynamic range compressor processing unit control selectors */
	DR_ENABLE_CONTROL = 0x01,
	COMPRESSION_RATE_CONTROL = 0x02,
	MAXAMPL_CONTROL = 0x03,
	THRESHOLD_CONTROL = 0x04,
	ATTACK_TIME = 0x05,
	RELEASE_TIME = 0x06,

	/* extension unit control selectors */
	XU_ENABLE_CONTROL = 0x01,

	/* endpoint control selectors */
	SAMPLING_FREQ_CONTROL = 0x01,
	PITCH_CONTROL = 0x01,
};

/*
 *	Audio Format Types
 */
enum
{
	/* type 1 format codes */
	PCM = 0x0001,
	PCM8 = 0x0002,
	IEEE_FLOAT = 0x0003,
	ALAW = 0x0004,
	MULAW = 0x0005,

	/* type 2 format codes */
	MPEG = 0x1001,
	AC_3 = 0x1002,

	/* type 3 format codes */
	IEC1937_AC_3 = 0x2001,
	IEC1937_MPEG1_L1 = 0x2002,
	IEC1937_MPEG1_L2_3 = 0x2003,
	IEC1937_MPEG2_NOEXT= 0x2003,
	IEC1937_MPEG2_EXT = 0x2004,
	IEC1937_MPEG2_L1_LS = 0x2005,
	IEC1937_MPEG2_L2_3_LS = 0x2006,

	/* format type codes */
	FORMAT_TYPE_UNDEF = 0x00,
	FORMAT_TYPE_I = 0x01,
	FORMAT_TYPE_II = 0x02,
	FORMAT_TYPE_III = 0x03,

	/* mpeg control selectors */
	MP_DUAL_CHANNEL_CONTROL = 0x01,
	MP_SECOND_STEREO_CONTROL = 0x02,
	MP_MULTILINGUAL_CONTROL = 0x03,
	MP_DYN_RANGE_CONTROL = 0x04,
	MP_SCALING_CONTROL = 0x05,
	MP_HILO_SCALING_CONTROL = 0x06,

	/* AC-3 control selectors */
	AC_MODE_CONTROL = 0x01,
	AC_DYN_RANGE_CONTROL = 0x02,
	AC_SCALING_CONTROL = 0x03,
	AC_HILO_SCALING_CONTROL = 0x04,
};

/*
 *	Audio Terminal Types
 */
enum
{
	/* USB terminal types */
	USB_UNDEFINED = 0x0100,
	USB_STREAMING = 0x0101,
	USB_VENDOR = 0x01FF,

	/* Input terminal types */
	INPUT_UNDEFINED = 0x0200,
	MICROPHONE = 0x0201,
	DESK_MICROPHONE = 0x0202,
	PERSONAL_MICROPHONE = 0x0203,
	OMNI_MICROPHONE = 0x0204,
	MICROPHONE_ARRAY = 0x0205,
	PROC_MICROPHONE_ARRAY = 0x0205,

	/* Output terminal types */
	OUTPUT_UNDEFINED = 0x0300,
	SPEAKER = 0x0301,
	HEADPHONES = 0x0302,
	HMD_AUDIO = 0x0303,
	DESK_SPEAKER = 0x0304,
	ROOM_SPEAKER = 0x0305,
	COMM_SPEAKER = 0x0306,
	LFE_SPEAKER = 0x0307,

	/* Bidirectional terminal types */
	BIDIRECTIONAL_UNDEFINED = 0x0400,
	HANDSET = 0x0401,
	HEADSET = 0x0402,
	SPEAKERPHONE = 0x0403,
	ECHO_SUPP_SPEAKERPHONE = 0x0404,
	ECHO_CANC_SPEAKERPHONE = 0x0405,

	/* Telephony terminal types */
	TELEPHONY_UNDEFINED = 0x0500,
	PHONELINE = 0x0501,
	TELEPHONE = 0x0502,
	DOWN_LINE_PHONE = 0x0503,

	/* External terminal types */
	EXTERNAL_UNDEFINED = 0x0600,
	ANALOG_CONNECTOR = 0x0601,
	DIGITAL_AUDIO = 0x0602,
	LINE_CONNECTOR = 0x0603,
	LEGACY_AUDIO = 0x0604,
	SPDIF_INTERFACE = 0x0605,
	I1394_DA = 0x0606,
	I1394_DV = 0x0607,

	/* Embedded function terminal types */
	EMBEDDED_UNDEFINED = 0x0700,
	LEVEL_CALIBRATION_NOICE_SOURCE = 0x0701,
	EQUALIZATION_NOISE = 0x0702,
	CD_PLAYER = 0x0703,
	DAT = 0x0704,
	DCC = 0x0705,
	MINIDISK = 0x0706,
	ANALOG_TAPE = 0x0707,
	PHONOGRAPH = 0x0708,
	VCR_AUDIO = 0x0709,
	VIDEODISC_AUDIO = 0x070A,
	DVD_AUDIO = 0x070B,
	TVTUNER_AUDIO = 0x070C,
	SATELLITE_AUDIO = 0x070D,
	CABLETUNER_AUDIO = 0x070E,
	DSS_AUDIO = 0x070F,
	RADIO_RECEIVER = 0x0710,
	RADIO_TRANSMITTER = 0x0711,
	MULTITRACK_RECORDER = 0x0712,
	SYNTHESIZER = 0x0713,
};

/*
 *	MIDI Streaming Subclass Definitions
 */
enum
{
	/* interface descriptor subtypes */
	MS_HEADER = 0x01,
	MIDI_IN_JACK = 0x02,
	MIDI_OUT_JACK = 0x03,
	ELEMENT = 0x03,

	/* endpoint descriptor subtypes */
	MS_GENERAL= 0x01,

	/* jack types */
	JACKTYPE_UNDEFINED = 0x00,
	EMBEDDED = 0x01,
	EXTERNAL = 0x02,

	/* endpoint control selectors */
	ASSOCIATION_CONTROL = 0x01,
};
