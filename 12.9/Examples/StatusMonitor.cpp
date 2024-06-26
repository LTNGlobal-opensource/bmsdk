 /* -LICENSE-START-
 ** Copyright (c) 2023 Blackmagic Design
 **  
 ** Permission is hereby granted, free of charge, to any person or organization 
 ** obtaining a copy of the software and accompanying documentation (the 
 ** "Software") to use, reproduce, display, distribute, sub-license, execute, 
 ** and transmit the Software, and to prepare derivative works of the Software, 
 ** and to permit third-parties to whom the Software is furnished to do so, in 
 ** accordance with:
 ** 
 ** (1) if the Software is obtained from Blackmagic Design, the End User License 
 ** Agreement for the Software Development Kit (“EULA”) available at 
 ** https://www.blackmagicdesign.com/EULA/DeckLinkSDK; or
 ** 
 ** (2) if the Software is obtained from any third party, such licensing terms 
 ** as notified by that third party,
 ** 
 ** and all subject to the following:
 ** 
 ** (3) the copyright notices in the Software and this entire statement, 
 ** including the above license grant, this restriction and the following 
 ** disclaimer, must be included in all copies of the Software, in whole or in 
 ** part, and all derivative works of the Software, unless such copies or 
 ** derivative works are solely in the form of machine-executable object code 
 ** generated by a source language processor.
 ** 
 ** (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 ** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 ** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
 ** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
 ** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
 ** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 ** DEALINGS IN THE SOFTWARE.
 ** 
 ** A copy of the Software is available free of charge at 
 ** https://www.blackmagicdesign.com/desktopvideo_sdk under the EULA.
 ** 
 ** -LICENSE-END-
 */

#include <atomic>
#include "platform.h"

struct FourCCNameMapping
{
	INT32_UNSIGNED fourcc;
	const char* name;
};

// Pixel format mappings
static FourCCNameMapping kPixelFormatMappings[] =
{
	{ bmdFormat8BitYUV,		"8-bit YUV" },
	{ bmdFormat10BitYUV,	"10-bit YUV" },
	{ bmdFormat8BitARGB,	"8-bit ARGB" },
	{ bmdFormat8BitBGRA,	"8-bit BGRA" },
	{ bmdFormat10BitRGB,	"10-bit RGB" },
	{ bmdFormat12BitRGB,	"12-bit RGB" },
	{ bmdFormat12BitRGBLE,	"12-bit RGBLE" },
	{ bmdFormat10BitRGBXLE,	"12-bit RGBXLE" },
	{ bmdFormat10BitRGBX,	"10-bit RGBX" },
	{ bmdFormatH265,		"H.265" },

	{ 0, NULL }
};

// Colorspace mappings
static FourCCNameMapping kColorspaceMappings[] =
{
	{ bmdColorspaceRec601,	"Rec.601" },
	{ bmdColorspaceRec709,	"Rec.709" },
	{ bmdColorspaceRec2020,	"Rec.2020" },

	{ 0, NULL }
};

// Dynamic range mappings
static FourCCNameMapping kDynamicRangeMappings[] =
{
	{ bmdDynamicRangeSDR, 			"SDR" },
	{ bmdDynamicRangeHDRStaticPQ,	"HDR PQ" },
	{ bmdDynamicRangeHDRStaticHLG,	"HDR HLG" },

	{ 0, NULL }
};

// Field dominance mappings
static FourCCNameMapping kFieldDominanceMappings[] =
{
	{ bmdUnknownFieldDominance,		"Unknown" },
	{ bmdLowerFieldFirst,			"Lower field first" },
	{ bmdUpperFieldFirst,			"Upper field first" },
	{ bmdProgressiveFrame,			"Progressive" },
	{ bmdProgressiveSegmentedFrame,	"Progressive Segmented Frame" },

	{ 0, NULL }
};

// Link configuration mappings
static FourCCNameMapping kLinkConfigurationMappings[] =
{
	{ bmdLinkConfigurationSingleLink,	"Single-link" },
	{ bmdLinkConfigurationDualLink,		"Dual-link" },
	{ bmdLinkConfigurationQuadLink,		"Quad-link" },

	{ 0, NULL }
};

// Panel type mappings
static FourCCNameMapping kPanelTypeMappings[] =
{
	{ bmdPanelNotDetected,				"Not detected" },
	{ bmdPanelTeranexMiniSmartPanel,	"Teranex Mini Smart Panel" },

	{ 0, NULL }
};

// Ethernet link state mappings
static FourCCNameMapping kEthernetLinkMappings[] =
{
	{ bmdEthernetLinkStateDisconnected,			"Disconnected" },
	{ bmdEthernetLinkStateConnectedUnbound,		"Connected (unbound)" },
	{ bmdEthernetLinkStateConnectedBound,		"Connected (bound)" },
	
	{ 0, NULL }
};

struct FlagNameMapping
{
	INT32_UNSIGNED flag;
	const char* name;
};

// Device busy flag mappings
static FlagNameMapping kDeviceBusyFlagMappings[] =
{
	{ bmdDeviceCaptureBusy,		"Capture" },
	{ bmdDevicePlaybackBusy,	"Playback" },
	{ bmdDeviceSerialPortBusy,	"Serial-port" },

	{ 0, NULL }
};

// Detected video input format flags mappings
static FlagNameMapping kDetectedVideoInputFormatFlagMappings[] =
{
	{ bmdDetectedVideoInputYCbCr422,		"YCbCr 4:2:2" },
	{ bmdDetectedVideoInputRGB444,			"RGB 4:4:4" },
	{ bmdDetectedVideoInputDualStream3D,	"3D" },
	{ bmdDetectedVideoInput12BitDepth,		"12-bit" },
	{ bmdDetectedVideoInput10BitDepth,		"10-bit" },
	{ bmdDetectedVideoInput8BitDepth,		"8-bit" },

	{ 0, NULL }
};

// Video status flags mappings
static FlagNameMapping kVideoStatusFlagMappings[] =
{
	{ bmdDeckLinkVideoStatusPsF,			"PsF" },
	{ bmdDeckLinkVideoStatusDualStream3D,	"3D" },

	{ 0, NULL }
};

static const char* getFourCCName(FourCCNameMapping* mappings, INT32_UNSIGNED fourcc)
{
	while (mappings->name != NULL)
	{
		if (mappings->fourcc == fourcc)
			return mappings->name;
		++mappings;
	}

	return "Unknown";
}

static std::string getInputDisplayModeName(IDeckLinkStatus* deckLinkStatus, BMDDisplayMode displayMode)
{
	IDeckLinkInput*			deckLinkInput = NULL;
	IDeckLinkDisplayMode*	deckLinkDisplayMode = NULL;
	STRINGOBJ				displayModeString;
	std::string				modeName = "Unknown";

	if (deckLinkStatus->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)
		goto bail;

	if (deckLinkInput->GetDisplayMode(displayMode, &deckLinkDisplayMode) != S_OK)
		goto bail;

	if (deckLinkDisplayMode->GetName(&displayModeString) == S_OK)
	{
		StringToStdString(displayModeString, modeName);
		STRINGFREE(displayModeString);
	}

bail:
	if (deckLinkInput)
		deckLinkInput->Release();

	return modeName;
}

static std::string getOutputDisplayModeName(IDeckLinkStatus* deckLinkStatus, BMDDisplayMode displayMode)
{
	IDeckLinkOutput*		deckLinkOutput = NULL;
	IDeckLinkDisplayMode*	deckLinkDisplayMode = NULL;
	STRINGOBJ				displayModeString;
	std::string				modeName = "Unknown";

	if (deckLinkStatus->QueryInterface(IID_IDeckLinkOutput, (void**)&deckLinkOutput) != S_OK)
		goto bail;

	if (deckLinkOutput->GetDisplayMode(displayMode, &deckLinkDisplayMode) != S_OK)
		goto bail;

	if (deckLinkDisplayMode->GetName(&displayModeString) == S_OK)
	{
		StringToStdString(displayModeString, modeName);
		STRINGFREE(displayModeString);
	}

bail:
	if (deckLinkOutput)
		deckLinkOutput->Release();

	return modeName;
}

static std::string getBusyStateString(BMDDeviceBusyState busyState)
{
	std::string busyString;
	FlagNameMapping* mappings = kDeviceBusyFlagMappings;

	if (busyState == (BMDDeviceBusyState)0)
		return "Inactive";

	while (mappings->name != NULL)
	{
		if (mappings->flag & busyState)
		{
			if (!busyString.empty())
				busyString.append(" and ");
			busyString.append(mappings->name);
		}
		++mappings;
	}

	busyString.append(" active");
	return busyString;
}

static std::string getFlagsString(FlagNameMapping* mappings, INT32_UNSIGNED flags)
{
	std::string flagsString;

	if (flags == 0)
		return "None";

	while (mappings->name != NULL)
	{
		if (mappings->flag & flags)
		{
			if (!flagsString.empty())
				flagsString.append(" ");
			flagsString.append(mappings->name);
		}
		++mappings;
	}

	return flagsString;
}

static void printHex(INT8_UNSIGNED* buffer, INT32_UNSIGNED size)
{
	for (INT32_UNSIGNED i = 0; i < size; )
	{
		printf("%02x ", buffer[i]);
		++i;
		if ((i % 16) == 0)
			printf("\n");
	}
}

static void printStatus(IDeckLinkStatus* deckLinkStatus, BMDDeckLinkStatusID statusId)
{
	HRESULT        result;
	INT64_SIGNED   intVal;
	BOOL           boolVal;
	INT8_UNSIGNED* bytesVal = NULL;
	INT32_UNSIGNED bytesSize = 0;
	STRINGOBJ      stringVal;
	std::string    stdStringVal;

	switch (statusId)
	{
		case bmdDeckLinkStatusDetectedVideoInputMode:
		case bmdDeckLinkStatusDetectedVideoInputColorspace:
		case bmdDeckLinkStatusDetectedVideoInputDynamicRange:
		case bmdDeckLinkStatusDetectedVideoInputFormatFlags:
		case bmdDeckLinkStatusDetectedVideoInputFieldDominance:
		case bmdDeckLinkStatusDetectedSDILinkConfiguration:
		case bmdDeckLinkStatusCurrentVideoInputMode:
		case bmdDeckLinkStatusCurrentVideoInputPixelFormat:
		case bmdDeckLinkStatusCurrentVideoInputFlags:
		case bmdDeckLinkStatusCurrentVideoOutputMode:
		case bmdDeckLinkStatusCurrentVideoOutputFlags:
		case bmdDeckLinkStatusPCIExpressLinkWidth:
		case bmdDeckLinkStatusPCIExpressLinkSpeed:
		case bmdDeckLinkStatusLastVideoOutputPixelFormat:
		case bmdDeckLinkStatusReferenceSignalMode:
		case bmdDeckLinkStatusReferenceSignalFlags:
		case bmdDeckLinkStatusBusy:
		case bmdDeckLinkStatusInterchangeablePanelType:
		case bmdDeckLinkStatusDeviceTemperature:
		case bmdDeckLinkStatusEthernetLink:
		case bmdDeckLinkStatusEthernetLinkMbps:
			result = deckLinkStatus->GetInt(statusId, &intVal);
			break;

		case bmdDeckLinkStatusVideoInputSignalLocked:
		case bmdDeckLinkStatusReferenceSignalLocked:
			result = deckLinkStatus->GetFlag(statusId, &boolVal);
			break;

		case bmdDeckLinkStatusReceivedEDID:
			result = deckLinkStatus->GetBytes(statusId, NULL, &bytesSize);
			if (result == S_OK)
			{
				bytesVal = (INT8_UNSIGNED*)malloc(bytesSize * sizeof(INT8_UNSIGNED));
				if (bytesVal)
					result = deckLinkStatus->GetBytes(statusId, bytesVal, &bytesSize);
				else
					result = E_OUTOFMEMORY;
			}
			break;

		case bmdDeckLinkStatusEthernetLocalIPAddress:
		case bmdDeckLinkStatusEthernetSubnetMask:
		case bmdDeckLinkStatusEthernetGatewayIPAddress:
		case bmdDeckLinkStatusEthernetPrimaryDNS:
		case bmdDeckLinkStatusEthernetSecondaryDNS:
		case bmdDeckLinkStatusEthernetPTPGrandmasterIdentity:
		case bmdDeckLinkStatusEthernetVideoOutputAddress:
		case bmdDeckLinkStatusEthernetAudioOutputAddress:
		case bmdDeckLinkStatusEthernetAncillaryOutputAddress:
		case bmdDeckLinkStatusEthernetAudioInputChannelOrder:
			result = deckLinkStatus->GetString(statusId, &stringVal);
			if (result == S_OK)
			{
				StringToStdString(stringVal, stdStringVal);
				STRINGFREE(stringVal);
			}
			break;

		default:
			// We can safely ignore any unneeded or unknown status IDs
			return;
	}

	if (FAILED(result))
	{
		/*
		 * Failed to retrieve the status value. Don't complain as this is
		 * expected for different hardware configurations.
		 *
		 * e.g.
		 * A device that doesn't support automatic mode detection will fail
		 * a request for bmdDeckLinkStatusDetectedVideoInputMode.
		 */
		return;
	}

	switch (statusId)
	{
		case bmdDeckLinkStatusDeviceTemperature:
			printf("%-40s %d\n", "Device Temperature is:", (BMDDisplayMode)intVal);
			break;

		case bmdDeckLinkStatusDetectedVideoInputMode:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected Video Input Mode:",
				getInputDisplayModeName(deckLinkStatus, (BMDDisplayMode)intVal).c_str());
			break;


		case bmdDeckLinkStatusDetectedVideoInputColorspace:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected Video Input Colorspace:",
				getFourCCName(kColorspaceMappings, (BMDColorspace)intVal));
			break;

		case bmdDeckLinkStatusDetectedVideoInputDynamicRange:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected Video Input Dynamic Range:",
				getFourCCName(kDynamicRangeMappings, (BMDDynamicRange)intVal));
			break;

		case bmdDeckLinkStatusDetectedVideoInputFormatFlags:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected Video Input Format Flags:",
				getFlagsString(kDetectedVideoInputFormatFlagMappings, (BMDDetectedVideoInputFormatFlags)intVal).c_str());
			break;

		case bmdDeckLinkStatusDetectedVideoInputFieldDominance:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected Video Input Field Dominance:",
				getFourCCName(kFieldDominanceMappings, (BMDFieldDominance)intVal));
			break;

		case bmdDeckLinkStatusDetectedSDILinkConfiguration:
			if (result == S_FALSE)
				break;

			printf("%-40s %s\n", "Detected SDI Link Configuration:",
				getFourCCName(kLinkConfigurationMappings, (BMDLinkConfiguration)intVal));
			break;

		case bmdDeckLinkStatusCurrentVideoInputMode:
			printf("%-40s %s\n", "Current Video Input Mode:",
				getInputDisplayModeName(deckLinkStatus, (BMDDisplayMode)intVal).c_str());
			break;

		case bmdDeckLinkStatusCurrentVideoInputFlags:
			printf("%-40s %s\n", "Current Video Input Flags:",
				getFlagsString(kVideoStatusFlagMappings, (BMDDeckLinkVideoStatusFlags)intVal).c_str());
			break;

		case bmdDeckLinkStatusCurrentVideoInputPixelFormat:
			printf("%-40s %s\n", "Current Video Input Pixel Format:",
				getFourCCName(kPixelFormatMappings, (BMDPixelFormat)intVal));
			break;

		case bmdDeckLinkStatusCurrentVideoOutputMode:
			printf("%-40s %s\n", "Current Video Output Mode:",
				getOutputDisplayModeName(deckLinkStatus, (BMDDisplayMode)intVal).c_str());
			break;

		case bmdDeckLinkStatusCurrentVideoOutputFlags:
			printf("%-40s %s\n", "Current Video Output Flags:",
				getFlagsString(kVideoStatusFlagMappings, (BMDDeckLinkVideoStatusFlags)intVal).c_str());
			break;

		case bmdDeckLinkStatusPCIExpressLinkWidth:
			printf("%-40s %ux\n", "PCIe Link Width:",
				(unsigned)intVal);
			break;

		case bmdDeckLinkStatusPCIExpressLinkSpeed:
			printf("%-40s Gen. %u\n", "PCIe Link Speed:",
				(unsigned)intVal);
			break;

		case bmdDeckLinkStatusLastVideoOutputPixelFormat:
			printf("%-40s %s\n", "Last Video Output Pixel Format:",
				getFourCCName(kPixelFormatMappings, (BMDPixelFormat)intVal));
			break;

		case bmdDeckLinkStatusReferenceSignalMode:
			printf("%-40s %s\n", "Reference Signal Mode:",
				getOutputDisplayModeName(deckLinkStatus, (BMDDisplayMode)intVal).c_str());
			break;

		case bmdDeckLinkStatusBusy:
			printf("%-40s %s\n", "Busy:",
				getBusyStateString((BMDDeviceBusyState)intVal).c_str());
			break;

		case bmdDeckLinkStatusVideoInputSignalLocked:
			printf("%-40s %s\n", "Video Input Signal Locked:",
				boolVal ? "Yes" : "No");
			break;

		case bmdDeckLinkStatusReferenceSignalLocked:
			printf("%-40s %s\n", "Reference Signal Locked:",
				boolVal ? "Yes" : "No");
			break;

		case bmdDeckLinkStatusReferenceSignalFlags:
			printf("%-40s %s\n", "Reference Signal Flags:",
				getFlagsString(kVideoStatusFlagMappings, (BMDDeckLinkVideoStatusFlags)intVal).c_str());
			break;

		case bmdDeckLinkStatusInterchangeablePanelType:
			printf("%-40s %s\n", "Interchangable Panel Installed:",
				getFourCCName(kPanelTypeMappings, (BMDPanelType)intVal));
			break;

		case bmdDeckLinkStatusEthernetLink:
			printf("%-40s %s\n", "Ethernet Link State:",
				getFourCCName(kEthernetLinkMappings, (BMDEthernetLinkState)intVal));
			break;

		case bmdDeckLinkStatusEthernetLinkMbps:
			printf("%-40s %u Mbps\n", "Ethernet Link Speed:",
				(unsigned)intVal);
			break;
			
		case bmdDeckLinkStatusReceivedEDID:
			if (result == S_FALSE)
			{
				printf("%-40s %s\n", "EDID Received:", "Not Available");
				break;
			}
			printf("EDID Received:\n");
			printHex(bytesVal, bytesSize);
			break;
			
		case bmdDeckLinkStatusEthernetLocalIPAddress:
			printf("%-40s %s\n", "Ethernet Local IP Address:",
				stdStringVal.c_str());
			break;
			
		case bmdDeckLinkStatusEthernetSubnetMask:
			printf("%-40s %s\n", "Ethernet Subnet Mask:",
				stdStringVal.c_str());
			break;
			
		case bmdDeckLinkStatusEthernetGatewayIPAddress:
			printf("%-40s %s\n", "Ethernet Gateway IP Address:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetPrimaryDNS:
			printf("%-40s %s\n", "Ethernet Primary DNS IP Address:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetSecondaryDNS:
			printf("%-40s %s\n", "Ethernet Secondary DNS IP Address:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetPTPGrandmasterIdentity:
			printf("%-40s %s\n", "Ethernet PTP Grandmaster Identity:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetVideoOutputAddress:
			printf("%-40s %s\n", "Ethernet Video Output Address:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetAudioOutputAddress:
			printf("%-40s %s\n", "Ethernet Audio Output Address:",
				stdStringVal.c_str());
			break;

		case bmdDeckLinkStatusEthernetAncillaryOutputAddress:
			printf("%-40s %s\n", "Ethernet Ancillary Output Address:",
				stdStringVal.c_str());
			break;
			
		case bmdDeckLinkStatusEthernetAudioInputChannelOrder:
			printf("%-40s %s\n", "Ethernet Audio Input SDP Channel Order:",
				stdStringVal.c_str());
			break;

		default:
			break;
	}
}

class NotificationCallback : public IDeckLinkNotificationCallback
{
public:
	NotificationCallback(IDeckLinkStatus* deckLinkStatus) : m_refCount(1)
	{
		m_deckLinkStatus = deckLinkStatus;
		m_deckLinkStatus->AddRef();
	}

	// Implement the IDeckLinkNotificationCallback interface
	HRESULT STDMETHODCALLTYPE Notify(BMDNotifications topic, INT64_UNSIGNED param1, INT64_UNSIGNED param2)
	{
		// Check whether the notification we received as a status notification
		if (topic != bmdStatusChanged)
			return S_OK;

		// Print the updated status value
		BMDDeckLinkStatusID statusId = (BMDDeckLinkStatusID)param1;
		printStatus(m_deckLinkStatus, statusId);

		return S_OK;
	}

	// IUnknown needs only a dummy implementation
	HRESULT	STDMETHODCALLTYPE QueryInterface (REFIID iid, LPVOID *ppv)
	{
		return E_NOINTERFACE;
	}
	
	ULONG STDMETHODCALLTYPE AddRef()
	{
		return ++m_refCount;
	}
	
	ULONG STDMETHODCALLTYPE Release()
	{
		ULONG newRefValue = --m_refCount;

		if (newRefValue == 0)
			delete this;

		return newRefValue;
	}

private:
	IDeckLinkStatus*    m_deckLinkStatus;
	std::atomic<ULONG>	m_refCount;

	virtual ~NotificationCallback()
	{
		m_deckLinkStatus->Release();
	}
};

static bool isDeviceActive(IDeckLink* deckLink)
{
	HRESULT                         result;
	IDeckLinkProfileAttributes*     deckLinkAttributes = NULL;
	INT64_SIGNED                    duplexModeAttribute;
	
	result = deckLink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&deckLinkAttributes);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkProfileAttributes interface - result = %08x\n", result);
		return false;
	}

	result = deckLinkAttributes->GetInt(BMDDeckLinkDuplex, &duplexModeAttribute);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not get duplex mode attribute - result = %08x\n", result);
	}

	deckLinkAttributes->Release();
	
	return result == S_OK && (BMDDuplexMode)duplexModeAttribute != bmdDuplexInactive;
}

static void displayHelp()
{
	HRESULT             result;
	IDeckLinkIterator*  deckLinkIterator  = NULL;
	IDeckLink*          deckLink          = NULL;
	int                 i = 0;

	result = GetDeckLinkIterator(&deckLinkIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.\n");
		return;
	}

	printf("Usage: StatusMonitor [-d <device id>] [-h/?]\n\n");
	printf("Options:\n");
	printf("    -d <device id> (default = 0):\n");

	while (true)
	{
		result = deckLinkIterator->Next(&deckLink);
		
		if (result != S_OK)
			break;
		
		if (isDeviceActive(deckLink))
		{
			// Only display devices that are active
			STRINGOBJ deckLinkDisplayName;

			if (deckLink->GetDisplayName(&deckLinkDisplayName) == S_OK)
			{
				std::string deckLinkDisplayNameString;
				StringToStdString(deckLinkDisplayName, deckLinkDisplayNameString);
				STRINGFREE(deckLinkDisplayName);

				printf("       %d: %s\n", i++, deckLinkDisplayNameString.c_str());
			}
		}

		deckLink->Release();
	}

	printf("    -h/? help\n\n");
}

IDeckLink* getDeckLinkAtIndex(int index)
{
	HRESULT             result;
	IDeckLinkIterator*  deckLinkIterator  = NULL;
	IDeckLink*          deckLink          = NULL;
	int                 i = 0;

	result = GetDeckLinkIterator(&deckLinkIterator);
	if (result != S_OK)
	{
		fprintf(stderr, "A DeckLink iterator could not be created.  The DeckLink drivers may not be installed.\n");
		return NULL;
	}

	while (true)
	{
		result = deckLinkIterator->Next(&deckLink);
		
		if (result != S_OK)
			break;
		
		// Skip over devices that are inactive
		if (isDeviceActive(deckLink))
		{
			if (index == i++)
				break;
		}

		deckLink->Release();
	}
	
	deckLinkIterator->Release();

	if (result != S_OK)
		return NULL;

	return deckLink;
}


int main(int argc, const char * argv[])
{
	IDeckLink*              deckLink             = NULL;
	IDeckLinkStatus*        deckLinkStatus       = NULL;
	IDeckLinkNotification*  deckLinkNotification = NULL;
	NotificationCallback*   notificationCallback = NULL;
	int                     deckLinkIndex;
	STRINGOBJ               deckLinkDisplayName;
	HRESULT                 result               = E_FAIL;
	
	Initialize();

	if (argc == 1)
	{
		// Use first device as default
		deckLinkIndex = 0;
	}
	else if (argc > 2 && strcmp(argv[1], "-d") == 0)
	{
		deckLinkIndex = atoi(argv[2]);
	}
	else
	{
		displayHelp();
		return EXIT_SUCCESS;
	}
	
	deckLink = getDeckLinkAtIndex(deckLinkIndex);
	
	if (!deckLink)
	{
		fprintf(stderr, "Could not find DeckLink device at index %d\n\n", deckLinkIndex);
		displayHelp();
		return EXIT_FAILURE;
	}

	// Obtain the status interface
	result = deckLink->QueryInterface(IID_IDeckLinkStatus, (void**)&deckLinkStatus);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkStatus interface - result = %08x\n", result);
		goto bail;
	}
	
	result = deckLink->GetDisplayName(&deckLinkDisplayName);
	if (result == S_OK)
	{
		std::string deckLinkDisplayNameString;
		StringToStdString(deckLinkDisplayName, deckLinkDisplayNameString);
		STRINGFREE(deckLinkDisplayName);
		printf("%-40s %s\n", "DeckLink Display Name:", deckLinkDisplayNameString.c_str());
	}

	// Print general status values
	printStatus(deckLinkStatus, bmdDeckLinkStatusBusy);
	printStatus(deckLinkStatus, bmdDeckLinkStatusPCIExpressLinkWidth);
	printStatus(deckLinkStatus, bmdDeckLinkStatusPCIExpressLinkSpeed);
	printStatus(deckLinkStatus, bmdDeckLinkStatusInterchangeablePanelType);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDeviceTemperature);

	// Print video input status values
	printStatus(deckLinkStatus, bmdDeckLinkStatusVideoInputSignalLocked);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedVideoInputMode);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedVideoInputFormatFlags);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedVideoInputFieldDominance);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedSDILinkConfiguration);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedVideoInputColorspace);
	printStatus(deckLinkStatus, bmdDeckLinkStatusDetectedVideoInputDynamicRange);
	printStatus(deckLinkStatus, bmdDeckLinkStatusCurrentVideoInputMode);
	printStatus(deckLinkStatus, bmdDeckLinkStatusCurrentVideoInputFlags);
	printStatus(deckLinkStatus, bmdDeckLinkStatusCurrentVideoInputPixelFormat);

	// Print video output status values
	printStatus(deckLinkStatus, bmdDeckLinkStatusCurrentVideoOutputMode);
	printStatus(deckLinkStatus, bmdDeckLinkStatusCurrentVideoOutputFlags);
	printStatus(deckLinkStatus, bmdDeckLinkStatusLastVideoOutputPixelFormat);
	printStatus(deckLinkStatus, bmdDeckLinkStatusReferenceSignalLocked);
	printStatus(deckLinkStatus, bmdDeckLinkStatusReferenceSignalMode);
	printStatus(deckLinkStatus, bmdDeckLinkStatusReferenceSignalFlags);
	printStatus(deckLinkStatus, bmdDeckLinkStatusReceivedEDID);
	
	// Print Ethernet status values
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetLink);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetLinkMbps);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetLocalIPAddress);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetSubnetMask);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetGatewayIPAddress);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetPrimaryDNS);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetSecondaryDNS);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetPTPGrandmasterIdentity);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetVideoOutputAddress);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetAudioOutputAddress);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetAncillaryOutputAddress);
	printStatus(deckLinkStatus, bmdDeckLinkStatusEthernetAudioInputChannelOrder);

	// Obtain the notification interface
	result = deckLink->QueryInterface(IID_IDeckLinkNotification, (void**)&deckLinkNotification);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not obtain the IDeckLinkNotification interface - result = %08x\n", result);
		goto bail;
	}

	notificationCallback = new NotificationCallback(deckLinkStatus);
	if (notificationCallback == NULL)
	{
		fprintf(stderr, "Could not create notification callback object\n");
		goto bail;
	}

	// Register for notification changes
	result = deckLinkNotification->Subscribe(bmdStatusChanged, notificationCallback);
	if (result != S_OK)
	{
		fprintf(stderr, "Could not subscribe to the status change notification - result = %08x\n", result);
		goto bail;
	}

	// Wait until user presses Enter
	printf("Monitoring... Press <RETURN> to exit\n");
	
	getchar();
	
	printf("Exiting.\n");

	// Release resources
bail:

	// Release the notification interface
	if (deckLinkNotification != NULL)
	{
		if (notificationCallback != NULL)
			deckLinkNotification->Unsubscribe(bmdStatusChanged, notificationCallback);
		deckLinkNotification->Release();
	}

	// Release the notification callback
	if (notificationCallback != NULL)
		notificationCallback->Release();

	// Release the status interface
	if (deckLinkStatus)
		deckLinkStatus->Release();

	// Release the DeckLink object
	if (deckLink)
		deckLink->Release();

	return(result == S_OK) ? 0 : 1;
}
