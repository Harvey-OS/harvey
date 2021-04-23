#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "usb.h"
#include "uvc.h"

void
printVCHeader(Fmt *fmt, void *vp)
{
	VCHeader *p;
	int i;

	p = vp;
	fmtprint(fmt, "VCHeader:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbNumFormats = %d\n", p->bNumFormats);
	fmtprint(fmt, "\twTotalLength = %d\n", GET2(p->wTotalLength));
	fmtprint(fmt, "\tdwClockFrequency = %d\n", GET2(p->dwClockFrequency));
	fmtprint(fmt, "\tbInCollection = %d\n", p->bInCollection);
	for(i = 0; i < p->bInCollection; i++)
		fmtprint(fmt, "\tbaInterfaceNr(%d) = %d\n", i+1, p->baInterfaceNr[i]);
}

void
printVCInputTerminal(Fmt *fmt, void *vp)
{
	VCInputTerminal *p;

	p = vp;
	fmtprint(fmt, "VCInputTerminal:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbTerminalID = %d\n", p->bTerminalID);
	fmtprint(fmt, "\twTerminalType = %#.4x\n", GET2(p->wTerminalType));
	fmtprint(fmt, "\tbAssocTerminal = %d\n", p->bAssocTerminal);
	fmtprint(fmt, "\tiTerminal = %d\n", p->iTerminal);
}

void
printVCOutputTerminal(Fmt *fmt, void *vp)
{
	VCOutputTerminal *p;

	p = vp;
	fmtprint(fmt, "VCOutputTerminal:\n");
	fmtprint(fmt, "\tbLength = %#.2x\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbTerminalID = %#.2x\n", p->bTerminalID);
	fmtprint(fmt, "\twTerminalType = %#.4x\n", GET2(p->wTerminalType));
	fmtprint(fmt, "\tbAssocTerminal = %#.2x\n", p->bAssocTerminal);
	fmtprint(fmt, "\tbSourceID = %#.2x\n", p->bSourceID);
	fmtprint(fmt, "\tiTerminal = %#.2x\n", p->iTerminal);
}

void
printVCCameraTerminal(Fmt *fmt, void *vp)
{
	VCCameraTerminal *p;

	p = vp;
	fmtprint(fmt, "VCCameraTerminal:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbTerminalID = %d\n", p->bTerminalID);
	fmtprint(fmt, "\twTerminalType = %#.4x\n", GET2(p->wTerminalType));
	fmtprint(fmt, "\tbAssocTerminal = %d\n", p->bAssocTerminal);
	fmtprint(fmt, "\tiTerminal = %d\n", p->iTerminal);
	fmtprint(fmt, "\twObjectiveFocalLengthMin = %#.4x\n", GET2(p->wObjectiveFocalLengthMin));
	fmtprint(fmt, "\twObjectiveFocalLengthMax = %#.4x\n", GET2(p->wObjectiveFocalLengthMax));
	fmtprint(fmt, "\twOcularFocalLength = %#.4x\n", GET2(p->wOcularFocalLength));
	fmtprint(fmt, "\tbControlSize = %d\n", p->bControlSize);
	fmtprint(fmt, "\tbmControls = %#.6x\n", GET3(p->bmControls));
}

void
printVCSelectorUnit(Fmt *fmt, void *vp)
{
	VCSelectorUnit *p;
	int i;

	p = vp;
	fmtprint(fmt, "VCSelectorUnit:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbUnitID = %d\n", p->bUnitID);
	fmtprint(fmt, "\tbNrInPins = %d\n", p->bNrInPins);
	for(i = 0; i < p->bNrInPins; i++)
		fmtprint(fmt, "\tbaSourceID(%d) = %d\n", i+1, p->baSourceID[i]);
	fmtprint(fmt, "\tiSelector = %d\n", p->baSourceID[i]);
}

void
printVCProcessingUnit(Fmt *fmt, void *vp)
{
	VCProcessingUnit *p;

	p = vp;
	fmtprint(fmt, "VCProcessingUnit:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbUnitID = %d\n", p->bUnitID);
	fmtprint(fmt, "\tbSourceID = %d\n", p->bSourceID);
	fmtprint(fmt, "\twMaxMultiplier = %d\n", GET2(p->wMaxMultiplier));
	fmtprint(fmt, "\tbControlSize = %d\n", p->bControlSize);
	fmtprint(fmt, "\tbmControls = %#.6x\n", GET3(p->bmControls));
	fmtprint(fmt, "\tiProcessing = %d\n", p->iProcessing);
	fmtprint(fmt, "\tbmVideoStandards = %#.2x\n", p->bmVideoStandards);
}

void
printVCEncodingUnit(Fmt *fmt, void *vp)
{
	VCEncodingUnit *p;

	p = vp;
	fmtprint(fmt, "VCEncodingUnit:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbUnitID = %d\n", p->bUnitID);
	fmtprint(fmt, "\tbSourceID = %d\n", p->bSourceID);
	fmtprint(fmt, "\tiEncoding = %d\n", p->iEncoding);
	fmtprint(fmt, "\tbControlSize = %d\n", p->bControlSize);
	fmtprint(fmt, "\tbmControls = %#.6x\n", GET3(p->bmControls));
	fmtprint(fmt, "\tbmControlsRuntime = %#.6x\n", GET3(p->bmControlsRuntime));
}

void
printVCExtensionUnit(Fmt *fmt, void *vp)
{
	VCExtensionUnit *p;
	int i, i0, e;

	p = vp;
	fmtprint(fmt, "VCExtensionUnit:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbUnitID = %d\n", p->bUnitID);
	fmtprint(fmt, "\tbNumControls = %d\n", p->bNumControls);
	fmtprint(fmt, "\tbNrInPins = %d\n", p->bNrInPins);
	for(i = 0; i < p->bNrInPins; i++)
		fmtprint(fmt, "\tbaSourceID(%d) = %d\n", i+1, p->baSourceID[i]);
	fmtprint(fmt, "\tbControlSize = %d\n", p->baSourceID[i]);
	i0 = i;
	e = i + p->baSourceID[i] + 1;
	for(; i < e; i++)
		fmtprint(fmt, "\tbmControls(%d) = %d\n", i - i0 + 1, p->baSourceID[i]);
	fmtprint(fmt, "\tiExtension = %d\n", p->baSourceID[i]);
}

void
printVSInputHeader(Fmt *fmt, void *vp)
{
	VSInputHeader *p;
	int i;

	p = vp;
	fmtprint(fmt, "VSInputHeader:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbNumFormats = %d\n", p->bNumFormats);
	fmtprint(fmt, "\twTotalLength = %d\n", GET2(p->wTotalLength));
	fmtprint(fmt, "\tbEndpointAddress = %#x\n", p->bEndpointAddress);
	fmtprint(fmt, "\tbmInfo = %#.2x\n", p->bmInfo);
	fmtprint(fmt, "\tbTerminalLink = %d\n", p->bTerminalLink);
	fmtprint(fmt, "\tbStillCaptureMethod = %d\n", p->bStillCaptureMethod);
	fmtprint(fmt, "\tbTriggerSupport = %#.2x\n", p->bTriggerSupport);
	fmtprint(fmt, "\tbTriggerUsage = %#.2x\n", p->bTriggerUsage);
	fmtprint(fmt, "\tbControlSize = %d\n", p->bControlSize);
	for(i = 0; i < p->bControlSize; i++)
		fmtprint(fmt, "\tbmaControls(%d) = %d\n", i+1, p->bmaControls[i]);
}

void
printVSOutputHeader(Fmt *fmt, void *vp)
{
	VSOutputHeader *p;
	int i;

	p = vp;
	fmtprint(fmt, "VSOutputHeader:\n");
	fmtprint(fmt, "\tbLength = %#.2x\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbNumFormats = %#.2x\n", p->bNumFormats);
	fmtprint(fmt, "\twTotalLength = %#.4x\n", GET2(p->wTotalLength));
	fmtprint(fmt, "\tbEndpointAddress = %#.2x\n", p->bEndpointAddress);
	fmtprint(fmt, "\tbTerminalLink = %#.2x\n", p->bTerminalLink);
	fmtprint(fmt, "\tbControlSize = %#.2x\n", p->bControlSize);
	for(i = 0; i < p->bControlSize; i++)
		fmtprint(fmt, "\tbmaControls(%d) = %d\n", i+1, p->bmaControls[i]);
}

void
printVSStillFrame(Fmt *fmt, void *vp)
{
	VSStillFrame *p;
	int i, j;

	p = vp;
	fmtprint(fmt, "VSStillFrame:\n");
	fmtprint(fmt, "\tbLength = %#.2x\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbEndpointAddress = %#.2x\n", p->bEndpointAddress);
	fmtprint(fmt, "\tbNumImageSizePatterns = %#.2x\n", p->bNumImageSizePatterns);
	for(i = 0; i < p->bNumImageSizePatterns; i++){
		fmtprint(fmt, "\twWidth(%d) = %d\n", i+1, GET2(&p->data[4 * i]));
		fmtprint(fmt, "\twHeight(%d) = %d\n", i+1, GET2(&p->data[4 * i + 2]));
	}
	fmtprint(fmt, "\tbNumCompressionPatterns = %#.2x\n", p->data[4 * i]);
	for(j = 0; j < p->data[4 * i]; i++)
		fmtprint(fmt, "\tbCompression(%d) = %#.2x", j + 1,GET2(&p->data[4 * i + 1 + j]));
}

void
printVSUncompressedFormat(Fmt *fmt, void *vp)
{
	VSUncompressedFormat *p;

	p = vp;
	fmtprint(fmt, "VSUncompressedFormat:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbFormatIndex = %d\n", p->bFormatIndex);
	fmtprint(fmt, "\tbNumFrameDescriptors = %d\n", p->bNumFrameDescriptors);
	fmtprint(fmt, "\tbBitsPerPixel = %d\n", p->bBitsPerPixel);
	fmtprint(fmt, "\tbDefaultFrameIndex = %d\n", p->bDefaultFrameIndex);
	fmtprint(fmt, "\tbAspectRatioX = %d\n", p->bAspectRatioX);
	fmtprint(fmt, "\tbAspectRatioY = %d\n", p->bAspectRatioY);
	fmtprint(fmt, "\tbmInterlaceFlags = %#.2x\n", p->bmInterlaceFlags);
	fmtprint(fmt, "\tbCopyProtect = %#.2x\n", p->bCopyProtect);
}

void
printVSUncompressedFrame(Fmt *fmt, void *vp)
{
	VSUncompressedFrame *p;
	int i;

	p = vp;
	fmtprint(fmt, "VSUncompressedFrame:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbFrameIndex = %d\n", p->bFrameIndex);
	fmtprint(fmt, "\tbmCapabilities = %#.2x\n", p->bmCapabilities);
	fmtprint(fmt, "\twWidth = %d\n", GET2(p->wWidth));
	fmtprint(fmt, "\twHeight = %d\n", GET2(p->wHeight));
	fmtprint(fmt, "\tdwMinBitRate = %d\n", GET4(p->dwMinBitRate));
	fmtprint(fmt, "\tdwMaxBitRate = %d\n", GET4(p->dwMaxBitRate));
	fmtprint(fmt, "\tdwMaxVideoFrameBufferSize = %d\n", GET4(p->dwMaxVideoFrameBufferSize));
	fmtprint(fmt, "\tdwDefaultFrameInterval = %d\n", GET4(p->dwDefaultFrameInterval));
	fmtprint(fmt, "\tbFrameIntervalType = %d\n", p->bFrameIntervalType);
	if(p->bFrameIntervalType == 0){
		fmtprint(fmt, "\tdwMinFrameInterval = %d\n", GET4(p->dwFrameInterval[0]));
		fmtprint(fmt, "\tdwMaxFrameInterval = %d\n", GET4(p->dwFrameInterval[1]));
		fmtprint(fmt, "\tdwFrameIntervalStep = %d\n", GET4(p->dwFrameInterval[2]));
	}
	for(i = 0; i < p->bFrameIntervalType; i++)
		fmtprint(fmt, "\tdwFrameInterval = %d\n", GET4(p->dwFrameInterval[i]));
}

void
printVSColorFormat(Fmt *fmt, void *vp)
{
	VSColorFormat *p;

	p = vp;
	fmtprint(fmt, "VSColorFormat:\n");
	fmtprint(fmt, "\tbLength = %d\n", p->bLength);
	fmtprint(fmt, "\tbDescriptorType = %#.2x\n", p->bDescriptorType);
	fmtprint(fmt, "\tbDescriptorSubtype = %#.2x\n", p->bDescriptorSubtype);
	fmtprint(fmt, "\tbColorPrimaries = %d\n", p->bColorPrimaries);
	fmtprint(fmt, "\tbTransferCharacteristics = %d\n", p->bTransferCharacteristics);
	fmtprint(fmt, "\tbMatrixCoefficients = %d\n", p->bMatrixCoefficients);
}

void
printProbeControl(Fmt *fmt, void *vp)
{
	ProbeControl *p;
	int i;

	p = vp;
	fmtprint(fmt, "ProbeControl:\n");
	fmtprint(fmt, "\tbmHint = %#.4ux\n", GET2(p->bmHint));
	fmtprint(fmt, "\tbFormatIndex = %#.2ux\n", p->bFormatIndex);
	fmtprint(fmt, "\tbFrameIndex = %#.2ux\n", p->bFrameIndex);
	fmtprint(fmt, "\tdwFrameInterval = %#.8ux\n", GET4(p->dwFrameInterval));
	fmtprint(fmt, "\twKeyFrameRate = %#.4ux\n", GET2(p->wKeyFrameRate));
	fmtprint(fmt, "\twPFrameRate = %#.4ux\n", GET2(p->wPFrameRate));
	fmtprint(fmt, "\twCompQuality = %#.4ux\n", GET2(p->wCompQuality));
	fmtprint(fmt, "\twCompWindowSize = %#.4ux\n", GET2(p->wCompWindowSize));
	fmtprint(fmt, "\twDelay = %#.4ux\n", GET2(p->wDelay));
	fmtprint(fmt, "\tdwMaxVideoFrameSize = %#.8ux\n", GET4(p->dwMaxVideoFrameSize));
	fmtprint(fmt, "\tdwMaxPayloadTransferSize = %#.8ux\n", GET4(p->dwMaxPayloadTransferSize));
	fmtprint(fmt, "\tdwClockFrequency = %#.8ux\n", GET4(p->dwClockFrequency));
	fmtprint(fmt, "\tbmFramingInfo = %#.2ux\n", p->bmFramingInfo);
	fmtprint(fmt, "\tbPreferedVersion = %#.2ux\n", p->bPreferedVersion);
	fmtprint(fmt, "\tbMinVersion = %#.2ux\n", p->bMinVersion);
	fmtprint(fmt, "\tbMaxVersion = %#.2ux\n", p->bMaxVersion);
	fmtprint(fmt, "\tbBitDepthLuma = %#.2ux\n", p->bBitDepthLuma);
	fmtprint(fmt, "\tbmSettings = %#.2ux\n", p->bmSettings);
	fmtprint(fmt, "\tbMaxNumberOfRefFramesPlus1 = %#.2ux\n", p->bMaxNumberOfRefFramesPlus1);
	fmtprint(fmt, "\tbmRateControlModes = %#.4ux\n", GET2(p->bmRateControlModes));
	for(i = 0; i < 4; i++)
		fmtprint(fmt, "\tbmLayoutPerStream(%d) = %#.4ux\n", i + 1, GET2(&p->bmLayoutPerStream[2 * i]));
}

void
printDescriptor(Fmt *fmt, Iface *iface, void *vp)
{
	VCDescriptor *vdp;
	
	vdp = vp;
	switch(Subclass(iface->csp)){
	case SC_VIDEOCONTROL:
		switch(vdp->bDescriptorSubtype){
		case VC_HEADER: printVCHeader(fmt, vdp); break;
		case VC_INPUT_TERMINAL:
			if(GET2(((VCInputTerminal*)vdp)->wTerminalType) == ITT_CAMERA)
				printVCCameraTerminal(fmt, vdp);
			else
				printVCInputTerminal(fmt, vdp);
			break;
		case VC_OUTPUT_TERMINAL: printVCOutputTerminal(fmt, vdp); break;
		case VC_SELECTOR_UNIT: printVCSelectorUnit(fmt, vdp); break;
		case VC_PROCESSING_UNIT: printVCProcessingUnit(fmt, vdp); break;
		case VC_ENCODING_UNIT: printVCEncodingUnit(fmt, vdp); break;
		case VC_EXTENSION_UNIT: printVCExtensionUnit(fmt, vdp); break;
		default: fmtprint(fmt, "unknown video control descriptor type %#.2x\n", vdp->bDescriptorSubtype);
		}
		break;
	case SC_VIDEOSTREAMING:
		switch(vdp->bDescriptorSubtype){
		case VS_INPUT_HEADER: printVSInputHeader(fmt, vdp); break;
		case VS_OUTPUT_HEADER: printVSOutputHeader(fmt, vdp); break;
		case VS_STILL_IMAGE_FRAME: printVSStillFrame(fmt, vdp); break;
		case VS_FORMAT_UNCOMPRESSED: printVSUncompressedFormat(fmt, vdp); break;
		case VS_FRAME_UNCOMPRESSED: printVSUncompressedFrame(fmt, vdp); break;
		case VS_COLORFORMAT: printVSColorFormat(fmt, vdp); break;
		default: fmtprint(fmt, "unknown video streaming descriptor type %#.2x\n", vdp->bDescriptorSubtype);
		}
		break;
	}
}
