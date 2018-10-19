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

#include <vector>
#include "DeckLinkAPI.h"
#include "CapturePreview.h"

class DeckLinkDevice : public IDeckLinkInputCallback
{
public:
    DeckLinkDevice(CapturePreviewAppDelegate* uiDelegate, IDeckLink* device);
    virtual ~DeckLinkDevice();
    
    bool                        init();
    
    //
    // IDeckLinkInputCallback interface
	virtual HRESULT             VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newDisplayMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
	virtual HRESULT             VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket);
    
    // IUnknown needs only a dummy implementation
	virtual HRESULT             QueryInterface (REFIID iid, LPVOID *ppv);
	virtual ULONG               AddRef();
	virtual ULONG               Release();
    
    NSString*                   getDeviceName() { return (NSString*)deviceName; };
	NSMutableArray*             getDisplayModeNames();
	bool                        deviceSupportsFormatDetection() { return supportFormatDetection; };
	bool                        isCapturing() { return currentlyCapturing; };
	bool                        startCapture(int videoModeIndex, IDeckLinkScreenPreviewCallback* screenPreviewCallback);
	void                        stopCapture();
    
    // public members
    IDeckLink*                              deckLink;
	IDeckLinkInput*                         deckLinkInput;
    bool                                    supportFormatDetection;
    bool                                	currentlyCapturing;
    
private:
    
    TimecodeStruct*                         getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* frame, BMDTimecodeFormat format);
    
    
    CapturePreviewAppDelegate*              uiDelegate;
    std::vector<IDeckLinkDisplayMode*>      modeList;
    CFStringRef                             deviceName;
    int32_t                                 refCount;
};

class DeckLinkDeviceDiscovery :  public IDeckLinkDeviceNotificationCallback
{
private:
    IDeckLinkDiscovery*             deckLinkDiscovery;
    CapturePreviewAppDelegate*      uiDelegate;
    int32_t                         refCount;
public:
	DeckLinkDeviceDiscovery(CapturePreviewAppDelegate* uiDelegate);
	virtual ~DeckLinkDeviceDiscovery();
    
    bool                Enable();
    void                Disable();
			
    // IDeckLinkDeviceArrivalNotificationCallback interface
    virtual HRESULT     DeckLinkDeviceArrived (/* in */ IDeckLink* deckLinkDevice);
	virtual HRESULT     DeckLinkDeviceRemoved (/* in */ IDeckLink* deckLinkDevice);
    
    // IUnknown needs only a dummy implementation
	virtual HRESULT		QueryInterface (REFIID iid, LPVOID *ppv);
	virtual ULONG		AddRef ();
	virtual ULONG		Release ();
};
