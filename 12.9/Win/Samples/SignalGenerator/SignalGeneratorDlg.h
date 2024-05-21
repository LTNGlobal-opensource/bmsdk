/* -LICENSE-START-
** Copyright (c) 2020 Blackmagic Design
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

#pragma once

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include "Resource.h"

#include "DeckLinkAPI_h.h"
#include "DeckLinkDeviceDiscovery.h"
#include "DeckLinkOutputDevice.h"
#include "PreviewWindow.h"
#include "ProfileCallback.h"
#include "SignalGenerator3DVideoFrame.h"


// Custom Messages
#define WM_ADD_DEVICE_MESSAGE					(WM_APP + 1)
#define WM_REMOVE_DEVICE_MESSAGE				(WM_APP + 2)
#define WM_UPDATE_PROFILE_MESSAGE				(WM_APP + 3)


enum OutputSignal
{
	kOutputSignalPip		= 0,
	kOutputSignalDrop		= 1
};

// Forward declarations
class Timecode;

// CSignalGeneratorDlg dialog
class CSignalGeneratorDlg : public CDialog
{
	using FillFrameFunction = std::function<void(CComPtr<IDeckLinkMutableVideoFrame>&, bool)>;

// Construction
public:
	explicit CSignalGeneratorDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CSignalGeneratorDlg() {};

	// Dialog Data
	enum { IDD = IDD_SIGNALGENERATOR_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
private:
	HICON						m_hIcon;
	//
	CButton						m_startButton;
	CComboBox					m_deviceListCombo;
	CComboBox					m_outputSignalCombo;
	CComboBox					m_audioChannelCombo;
	CComboBox					m_audioSampleDepthCombo;
	CComboBox					m_videoFormatCombo;
	CComboBox					m_pixelFormatCombo;
	CSize						m_minDialogSize;

	CStatic						m_previewBox;
	CComPtr<PreviewWindow>		m_previewWindow;

	bool						m_running;
	
	int							m_frameWidth;
	int							m_frameHeight;
	BMDTimeValue				m_frameDuration;
	BMDTimeScale				m_frameTimescale;
	unsigned long				m_framesPerSecond;
	BMDFieldDominance			m_fieldDominance;

	CComPtr<SignalGenerator3DVideoFrame>	m_videoFrameBlack;
	CComPtr<SignalGenerator3DVideoFrame>	m_videoFrameBars;
	
	OutputSignal				m_outputSignal;
	std::vector<unsigned char>	m_audioBuffer;
	unsigned long				m_audioBufferOffset;
	unsigned long				m_audioBufferSampleLength;
	unsigned long				m_audioSamplesPerFrame;
	unsigned long				m_audioChannelCount;
	BMDAudioSampleRate			m_audioSampleRate;
	BMDAudioSampleType			m_audioSampleDepth;
	BMDTimeValue				m_audioStreamTime;

	CComPtr<DeckLinkOutputDevice> 		m_selectedDevice;
	CComPtr<DeckLinkDeviceDiscovery>	m_deckLinkDiscovery;
	BMDDisplayMode						m_selectedDisplayMode;
	BMDVideoOutputFlags					m_selectedVideoOutputFlags;
	CComPtr<ProfileCallback>			m_profileCallback;

	std::map<IDeckLink*, CComPtr<DeckLinkOutputDevice>>		m_outputDevices;

	bool						m_scheduledPlaybackStopped;
	std::mutex					m_stopPlaybackMutex;
	std::condition_variable		m_stopPlaybackCondition;

	std::unique_ptr<Timecode>	m_timeCode;
	BMDTimecodeFormat			m_timeCodeFormat;
	unsigned int				m_dropFrames;
	BOOL						m_hfrtcSupported;

	// Generated message map functions
	virtual BOOL	OnInitDialog();
	void			EnableInterface(BOOL enable);
	afx_msg void	OnGetMinMaxInfo(MINMAXINFO* minMaxInfo);
	afx_msg void	OnPaint();
	afx_msg void	OnClose();
	afx_msg HCURSOR	OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	
	// Signal Generator Implementation
	void			StartRunning ();
	void			StopRunning ();

	void			RefreshOutputDeviceList(void);
	void			RefreshDisplayModeMenu(void);
	void			RefreshPixelFormatMenu(void);
	void			RefreshAudioChannelMenu(void);
	void			AddDevice(CComPtr<IDeckLink>& deckLink);
	void			RemoveDevice(CComPtr<IDeckLink>& deckLink);


public:
	void			ScheduleNextFrame(bool prerolling);
	void			WriteNextAudioSamples(unsigned int samplesToWrite);
	void			ScheduledPlaybackStopped();
	void			HaltStreams(CComPtr<IDeckLinkProfile>& newProfile);
	void			HandleDeviceError(OutputDeviceError error);

	afx_msg void OnBnClickedOk();
	afx_msg void OnNewDeviceSelected();
	afx_msg void OnNewVideoFormatSelected();

	afx_msg LRESULT	OnAddDevice(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT	OnRemoveDevice(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT	OnProfileUpdate(WPARAM wParam, LPARAM lParam);

private:
	CComPtr<SignalGenerator3DVideoFrame> CreateOutputFrame(FillFrameFunction fillFrame);
};

class Timecode
{
public:
	Timecode(int f, int d)
		: m_fps(f), m_framecount(0), m_dropframes(d), m_frames(0), m_seconds(0), m_minutes(0), m_hours(0)
	{
	}
	void update()
	{
		unsigned long frameCountNormalized = ++m_framecount;

		if (m_dropframes)
		{
			int deciMins, deciMinsRemainder;

			int framesIn10mins = (60 * 10 * m_fps) - (9 * m_dropframes);
			deciMins = frameCountNormalized / framesIn10mins;
			deciMinsRemainder = frameCountNormalized - (deciMins * framesIn10mins);

			// Add drop frames for 9 minutes of every 10 minutes that have elapsed
			// AND drop frames for every minute (over the first minute) in this 10-minute block.
			frameCountNormalized += m_dropframes * 9 * deciMins;
			if (deciMinsRemainder >= m_dropframes)
				frameCountNormalized += m_dropframes * ((deciMinsRemainder - m_dropframes) / (framesIn10mins / 10));
		}

		m_frames = (int)(frameCountNormalized % m_fps);
		frameCountNormalized /= m_fps;
		m_seconds = (int)(frameCountNormalized % 60);
		frameCountNormalized /= 60;
		m_minutes = (int)(frameCountNormalized % 60);
		frameCountNormalized /= 60;
		m_hours = (int)frameCountNormalized;
	}
	int hours() const { return m_hours; }
	int minutes() const { return m_minutes; }
	int seconds() const { return m_seconds; }
	int frames() const { return m_frames; }
	unsigned long frameCount() const { return m_framecount; }
private:
	int m_fps;
	unsigned long m_framecount;
	int m_dropframes;
	int m_frames;
	int m_seconds;
	int m_minutes;
	int m_hours;
};

void	FillSine (unsigned char* audioBuffer, unsigned long samplesToWrite, unsigned long channels, unsigned long sampleDepth);
void	FillColourBars (CComPtr<IDeckLinkMutableVideoFrame>& theFrame, bool reversed);
void	FillBlack (CComPtr<IDeckLinkMutableVideoFrame>& theFrame, bool /* unused */);