/* -LICENSE-START-
 ** Copyright (c) 2011 Blackmagic Design
 **
 ** Permission is hereby granted, free of charge, to any person or organization
 ** obtaining a copy of the software and accompanying documentation covered by
 ** this license (the "Software") to use, reproduce, display, distribute,
 ** execute, and transmit the Software, and to prepare derivative works of the
 ** Software, and to permit third-parties to whom the Software is furnished to
 ** do so, all subject to the following:
 ** 
 ** The copyright notices in the Software and this entire statement, including
 ** the above license grant, this restriction and the following disclaimer,
 ** must be included in all copies of the Software, in whole or in part, and
 ** all derivative works of the Software, unless such copies or derivative
 ** works are solely in the form of machine-executable object code generated by
 ** a source language processor.
 ** 
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 ** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 ** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 ** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 ** DEALINGS IN THE SOFTWARE.
 ** -LICENSE-END-
 */

#include "DeckLinkController.h"

using namespace std;


DeckLinkDevice::DeckLinkDevice(CapturePreviewAppDelegate* ui, IDeckLink* device)
	: uiDelegate(ui), deckLink(device), deckLinkInput(NULL), deckLinkConfig(NULL),
	deckLinkHDMIInputEDID(NULL), deckLinkProfileManager(NULL), supportFormatDetection(false),
	currentlyCapturing(false), deviceName(NULL), supportedInputConnections(0), refCount(1)
{
	// DeckLinkDevice owns IDeckLink instance
	// AddRef has already been called on this IDeckLink instance on our behalf in DeckLinkDeviceArrived to avoid a race
}

DeckLinkDevice::~DeckLinkDevice()
{
	if (deckLinkProfileManager)
	{
		deckLinkProfileManager->Release();
		deckLinkProfileManager = NULL;
	}
	
	if (deckLinkInput)
	{
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}
	
	if (deckLinkConfig)
	{
		deckLinkConfig->Release();
		deckLinkConfig = NULL;
	}

	if (deckLinkHDMIInputEDID)
	{
		deckLinkHDMIInputEDID->Release();
		deckLinkHDMIInputEDID = NULL;
	}
	
	if (deckLink)
	{
		deckLink->Release();
		deckLink = NULL;
	}
	
	if (deviceName)
		CFRelease(deviceName);
}

HRESULT         DeckLinkDevice::QueryInterface (REFIID iid, LPVOID *ppv)
{
	CFUUIDBytes		iunknown;
	HRESULT			result = E_NOINTERFACE;
	
	// Initialise the return result
	*ppv = NULL;
	
	// Obtain the IUnknown interface and compare it the provided REFIID
	iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (memcmp(&iid, &iunknown, sizeof(REFIID)) == 0)
	{
		*ppv = this;
		AddRef();
		result = S_OK;
	}
	else if (memcmp(&iid, &IID_IDeckLinkNotificationCallback, sizeof(REFIID)) == 0)
	{
		*ppv = (IDeckLinkNotificationCallback*)this;
		AddRef();
		result = S_OK;
	}
	
	return result;
}

ULONG       DeckLinkDevice::AddRef (void)
{
	return ++refCount;
}

ULONG       DeckLinkDevice::Release (void)
{
	ULONG newRefValue = --refCount;
	if (newRefValue == 0)
		delete this;
	
	return newRefValue;
}

bool        DeckLinkDevice::init()
{
	IDeckLinkProfileAttributes*            deckLinkAttributes = NULL;
	
	// Get input interface
	if (deckLink->QueryInterface(IID_IDeckLinkInput, (void**) &deckLinkInput) != S_OK)
		return false;
	
	// Get attributes interface
	if (deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**) &deckLinkAttributes) != S_OK)
		return false;

	// Check if input mode detection format is supported.
	if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportFormatDetection) != S_OK)
		supportFormatDetection = false;

	// Get the supported video input connections for the device
	if (deckLinkAttributes->GetInt(BMDDeckLinkVideoInputConnections, &supportedInputConnections) != S_OK)
		supportedInputConnections = 0;
	
	deckLinkAttributes->Release();
	
	// Get configuration interface to allow changing of input connector
	// We hold onto IDeckLinkConfiguration for lifetime of DeckLinkDevice to retain input connector setting
	if (deckLink->QueryInterface(IID_IDeckLinkConfiguration, (void**) &deckLinkConfig) != S_OK)
		return false;
	
	// Enable all EDID functionality if possible
	if (deckLink->QueryInterface(IID_IDeckLinkHDMIInputEDID, (void**)&deckLinkHDMIInputEDID) == S_OK && deckLinkHDMIInputEDID)
	{
		int64_t allKnownRanges = bmdDynamicRangeSDR | bmdDynamicRangeHDRStaticPQ | bmdDynamicRangeHDRStaticHLG;
		deckLinkHDMIInputEDID->SetInt(bmdDeckLinkHDMIInputEDIDDynamicRange, allKnownRanges);
		deckLinkHDMIInputEDID->WriteToEDID();
	}
	
	// Get device name
	if (deckLink->GetDisplayName(&deviceName) != S_OK)
		deviceName = CFStringCreateCopy(NULL, CFSTR("DeckLink"));
	
	// Get the profile manager interface
	// Will return S_OK when the device has > 1 profiles
	if (deckLink->QueryInterface(IID_IDeckLinkProfileManager, (void**) &deckLinkProfileManager) != S_OK)
	{
		deckLinkProfileManager = NULL;
	}
	
	return true;
}

bool		DeckLinkDevice::startCapture(BMDDisplayMode displayMode, IDeckLinkScreenPreviewCallback* screenPreviewCallback, bool applyDetectedInputMode)
{
	BMDVideoInputFlags		videoInputFlags;
	
	// Enable input video mode detection if the device supports it
	videoInputFlags = (supportFormatDetection && applyDetectedInputMode) ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;
	
	// Set the screen preview
	deckLinkInput->SetScreenPreviewCallback(screenPreviewCallback);
	
	// Set capture callback
	deckLinkInput->SetCallback(this);
	
	// Set the video input mode
	if (deckLinkInput->EnableVideoInput(displayMode, bmdFormat10BitYUV, videoInputFlags) != S_OK)
	{
		[uiDelegate showErrorMessage:@"This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use." title:@"Error starting the capture"];
		return false;
	}
	
	// Start the capture
	if (deckLinkInput->StartStreams() != S_OK)
	{
		[uiDelegate showErrorMessage:@"This application was unable to start the capture. Perhaps, the selected device is currently in-use." title:@"Error starting the capture"];
		return false;
	}
	
	currentlyCapturing = true;
	
	return true;
}

void		DeckLinkDevice::stopCapture()
{
	// Stop the capture
	deckLinkInput->StopStreams();
	
	// Delete capture callback
	deckLinkInput->SetCallback(NULL);
	deckLinkInput->DisableVideoInput();
	
	currentlyCapturing = false;
}


HRESULT		DeckLinkDevice::VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags)
{
	UInt32				flags = bmdVideoInputEnableFormatDetection;
	BMDPixelFormat		pixelFormat = bmdFormat10BitYUV;
	
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

	if (detectedSignalFlags & bmdDetectedVideoInputRGB444)
		pixelFormat = bmdFormat10BitRGB;

	if (detectedSignalFlags & bmdDetectedVideoInputDualStream3D)
		flags |= bmdVideoInputDualStream3D;

	// Restart capture with the new video mode if told to
	if ([uiDelegate shouldRestartCaptureWithNewVideoMode] == YES)
	{
		// Stop the capture
		deckLinkInput->StopStreams();
		
		// Set the video input mode
		if (deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), pixelFormat, flags) != S_OK)
		{
			[uiDelegate stopCapture];
			[uiDelegate showErrorMessage:@"This application was unable to select the new video mode." title:@"Error restarting the capture."];
			goto bail;
		}
		
		// Start the capture
		if (deckLinkInput->StartStreams() != S_OK)
		{
			[uiDelegate stopCapture];
			[uiDelegate showErrorMessage:@"This application was unable to start the capture on the selected device." title:@"Error restarting the capture."];
			goto bail;
		}
	}
	
	// Update the UI with detected display mode
	[uiDelegate selectDetectedVideoMode: newMode->GetDisplayMode()];
	
bail:
	[pool release];
	return S_OK;
}

HRESULT 	DeckLinkDevice::VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket)
{
	BOOL							hasValidInputSource = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) != 0 ? NO : YES;
	NSAutoreleasePool* 				pool = [[NSAutoreleasePool alloc] init];
	AncillaryDataStruct*			ancillaryData = [[[AncillaryDataStruct alloc] init] autorelease];
	
	// Update input source label
	[uiDelegate updateInputSourceState:hasValidInputSource];
	
	// Get the various timecodes and userbits for this frame
	ancillaryData.vitcF1 = getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC);
	ancillaryData.vitcF2 = getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITCField2);
	ancillaryData.rp188vitc1 = getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC1);
	ancillaryData.rp188ltc = getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188LTC);
	ancillaryData.rp188vitc2 = getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188VITC2);
	ancillaryData.rp188hfrtc = getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188HighFrameRate);
	ancillaryData.metadata = getMetadataFromFrame(videoFrame);

	// Update the UI
	dispatch_block_t updateAncillary = ^{
		[uiDelegate setAncillaryData:ancillaryData];
		[uiDelegate reloadAncillaryTable];
	};
	
	dispatch_async(dispatch_get_main_queue(), updateAncillary);

	[pool release];
	return S_OK;
}

TimecodeStruct*				DeckLinkDevice::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat)
{
	IDeckLinkTimecode*		timecode = NULL;
	CFStringRef				timecodeCFString;
	BMDTimecodeUserBits		userBits = 0;
	TimecodeStruct*			returnTimeCode = [[[TimecodeStruct alloc] init] autorelease];
	
	if ((videoFrame != NULL) && (videoFrame->GetTimecode(timecodeFormat, &timecode) == S_OK))
	{
		if (timecode->GetString(&timecodeCFString) == S_OK)
		{
			returnTimeCode.timecode = [NSString stringWithString: (NSString *)timecodeCFString];
			CFRelease(timecodeCFString);
		}
		else
		{
			returnTimeCode.timecode = @"";
		}
		
		timecode->GetTimecodeUserBits(&userBits);
		returnTimeCode.userBits = [NSString stringWithFormat:@"0x%08X", userBits];
		
		timecode->Release();
	}
	else
	{
		returnTimeCode.timecode = @"";
		returnTimeCode.userBits = @"";
	}
	
	return returnTimeCode;
}

MetadataStruct*	DeckLinkDevice::getMetadataFromFrame(IDeckLinkVideoInputFrame* frame)
{
	MetadataStruct* returnMetadata = [[[MetadataStruct alloc] init] autorelease];

	returnMetadata.electroOpticalTransferFunction = @"";
	returnMetadata.displayPrimariesRedX = @"";
	returnMetadata.displayPrimariesRedY = @"";
	returnMetadata.displayPrimariesGreenX = @"";
	returnMetadata.displayPrimariesGreenY = @"";
	returnMetadata.displayPrimariesBlueX = @"";
	returnMetadata.displayPrimariesBlueY = @"";
	returnMetadata.whitePointX = @"";
	returnMetadata.whitePointY = @"";
	returnMetadata.maxDisplayMasteringLuminance = @"";
	returnMetadata.minDisplayMasteringLuminance = @"";
	returnMetadata.maximumContentLightLevel = @"";
	returnMetadata.maximumFrameAverageLightLevel = @"";
	returnMetadata.colorspace = @"";

	IDeckLinkVideoFrameMetadataExtensions* metadataExtensions = NULL;
	if (frame->QueryInterface(IID_IDeckLinkVideoFrameMetadataExtensions, (void**) &metadataExtensions) == S_OK)
	{
		double doubleValue = 0.0;
		int64_t intValue = 0;

		if (metadataExtensions->GetInt(bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc, &intValue) == S_OK)
		{
			switch (intValue)
			{
				case 0:
					returnMetadata.electroOpticalTransferFunction = [NSString stringWithFormat:@"SDR"];
					break;
				case 1:
					returnMetadata.electroOpticalTransferFunction = [NSString stringWithFormat:@"HDR"];
					break;
				case 2:
					returnMetadata.electroOpticalTransferFunction = [NSString stringWithFormat:@"PQ (ST2084)"];
					break;
				case 3:
					returnMetadata.electroOpticalTransferFunction = [NSString stringWithFormat:@"HLG"];
					break;
				default:
					returnMetadata.electroOpticalTransferFunction = [NSString stringWithFormat:@"Unknown EOTF: %d", (int32_t)intValue];
					break;
			}
		}

		if (frame->GetFlags() & bmdFrameContainsHDRMetadata)
		{
			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesRedX = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesRedY = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesGreenX = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesGreenY = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesBlueX = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY, &doubleValue) == S_OK)
				returnMetadata.displayPrimariesBlueY = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRWhitePointX, &doubleValue) == S_OK)
				returnMetadata.whitePointX = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRWhitePointY, &doubleValue) == S_OK)
				returnMetadata.whitePointY = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance, &doubleValue) == S_OK)
				returnMetadata.maxDisplayMasteringLuminance = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance, &doubleValue) == S_OK)
				returnMetadata.minDisplayMasteringLuminance = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel, &doubleValue) == S_OK)
				returnMetadata.maximumContentLightLevel = [NSString stringWithFormat:@"%.04f", doubleValue];

			if (metadataExtensions->GetFloat(bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel, &doubleValue) == S_OK)
				returnMetadata.maximumFrameAverageLightLevel = [NSString stringWithFormat:@"%.04f", doubleValue];
		}

		if (metadataExtensions->GetInt(bmdDeckLinkFrameMetadataColorspace, &intValue) == S_OK)
		{
			switch (intValue)
			{
				case bmdColorspaceRec601:
					returnMetadata.colorspace = [NSString stringWithFormat:@"Rec.601"];
					break;

				case bmdColorspaceRec709:
					returnMetadata.colorspace = [NSString stringWithFormat:@"Rec.709"];
					break;

				case bmdColorspaceRec2020:
					returnMetadata.colorspace = [NSString stringWithFormat:@"Rec.2020"];
					break;
			}
		}

		metadataExtensions->Release();
	}
	return returnMetadata;
}


ProfileCallback::ProfileCallback(CapturePreviewAppDelegate* delegate)
	: uiDelegate(delegate), refCount(1)
{
}

HRESULT		ProfileCallback::ProfileChanging (IDeckLinkProfile *profileToBeActivated, bool streamsWillBeForcedToStop)
{
	// When streamsWillBeForcedToStop is true, the profile to be activated is incompatible with the current
	// profile and capture will be stopped by the DeckLink driver. It is better to notify the
	// controller to gracefully stop capture, so that the UI is set to a known state.
	if (streamsWillBeForcedToStop)
		[uiDelegate haltStreams:profileToBeActivated];
	return S_OK;
}

HRESULT		ProfileCallback::ProfileActivated (IDeckLinkProfile *activatedProfile)
{
	// New profile activated, inform owner to update popup menus
	// Ensure that reference is added to new profile before handing to main thread
	activatedProfile->AddRef();
	dispatch_async(dispatch_get_main_queue(), ^{
		[uiDelegate updateProfile:activatedProfile];
	});
	
	return S_OK;
}

HRESULT		ProfileCallback::QueryInterface (REFIID iid, LPVOID *ppv)
{
	CFUUIDBytes		iunknown;
	HRESULT			result = E_NOINTERFACE;
	
	// Initialise the return result
	*ppv = NULL;
	
	// Obtain the IUnknown interface and compare it the provided REFIID
	iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (memcmp(&iid, &iunknown, sizeof(REFIID)) == 0)
	{
		*ppv = this;
		AddRef();
		result = S_OK;
	}
	
	return result;
}

ULONG		ProfileCallback::AddRef (void)
{
	return ++refCount;
}

ULONG		ProfileCallback::Release (void)
{
	ULONG newRefValue = --refCount;
	if (newRefValue == 0)
		delete this;
	
	return newRefValue;
}


DeckLinkDeviceDiscovery::DeckLinkDeviceDiscovery(CapturePreviewAppDelegate* delegate)
: uiDelegate(delegate), deckLinkDiscovery(NULL), refCount(1)
{
	deckLinkDiscovery = CreateDeckLinkDiscoveryInstance();
}


DeckLinkDeviceDiscovery::~DeckLinkDeviceDiscovery()
{
	if (deckLinkDiscovery != NULL)
	{
		// Uninstall device arrival notifications and release discovery object
		deckLinkDiscovery->UninstallDeviceNotifications();
		deckLinkDiscovery->Release();
		deckLinkDiscovery = NULL;
	}
}

bool        DeckLinkDeviceDiscovery::Enable()
{
	HRESULT     result = E_FAIL;
	
	// Install device arrival notifications
	if (deckLinkDiscovery != NULL)
		result = deckLinkDiscovery->InstallDeviceNotifications(this);
	
	return result == S_OK;
}

void        DeckLinkDeviceDiscovery::Disable()
{
	// Uninstall device arrival notifications
	if (deckLinkDiscovery != NULL)
		deckLinkDiscovery->UninstallDeviceNotifications();
}

HRESULT     DeckLinkDeviceDiscovery::DeckLinkDeviceArrived (/* in */ IDeckLink* deckLink)
{
	// Update UI (add new device to menu) from main thread
	// AddRef the IDeckLink instance before handing it off to the main thread
	deckLink->AddRef();
	dispatch_async(dispatch_get_main_queue(), ^{
		[uiDelegate addDevice:deckLink];
	});
	
	return S_OK;
}

HRESULT     DeckLinkDeviceDiscovery::DeckLinkDeviceRemoved (/* in */ IDeckLink* deckLink)
{
	dispatch_async(dispatch_get_main_queue(), ^{ [uiDelegate removeDevice:deckLink]; });
	return S_OK;
}

HRESULT         DeckLinkDeviceDiscovery::QueryInterface (REFIID iid, LPVOID *ppv)
{
	CFUUIDBytes		iunknown;
	HRESULT			result = E_NOINTERFACE;
	
	// Initialise the return result
	*ppv = NULL;
	
	// Obtain the IUnknown interface and compare it the provided REFIID
	iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
	if (memcmp(&iid, &iunknown, sizeof(REFIID)) == 0)
	{
		*ppv = this;
		AddRef();
		result = S_OK;
	}
	else if (memcmp(&iid, &IID_IDeckLinkDeviceNotificationCallback, sizeof(REFIID)) == 0)
	{
		*ppv = (IDeckLinkDeviceNotificationCallback*)this;
		AddRef();
		result = S_OK;
	}
	
	return result;
}

ULONG           DeckLinkDeviceDiscovery::AddRef (void)
{
	return ++refCount;
}

ULONG           DeckLinkDeviceDiscovery::Release (void)
{
	ULONG newRefValue = --refCount;
	if (newRefValue == 0)
		delete this;
	
	return newRefValue;
}
