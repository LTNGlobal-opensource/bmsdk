/* -LICENSE-START-
** Copyright (c) 2017 Blackmagic Design
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
//
//  DeckLinkDeviceDiscovery.cpp
//  DeckLink Device Discovery Callback
//

#include "DeckLinkDeviceDiscovery.h"

DeckLinkDeviceDiscovery::DeckLinkDeviceDiscovery(SignalGenerator* delegate)
	: m_uiDelegate(delegate), m_refCount(1)
{
	m_deckLinkDiscovery = CreateDeckLinkDiscoveryInstance();
}


DeckLinkDeviceDiscovery::~DeckLinkDeviceDiscovery()
{
	if (m_deckLinkDiscovery != NULL)
	{
		// Uninstall device arrival notifications and release discovery object
		m_deckLinkDiscovery->UninstallDeviceNotifications();
		m_deckLinkDiscovery->Release();
		m_deckLinkDiscovery = NULL;
	}
}

bool DeckLinkDeviceDiscovery::enable()
{
	HRESULT result = E_FAIL;

	// Install device arrival notifications
	if (m_deckLinkDiscovery != NULL)
		result = m_deckLinkDiscovery->InstallDeviceNotifications(this);

	return result == S_OK;
}

void DeckLinkDeviceDiscovery::disable()
{
	// Uninstall device arrival notifications
	if (m_deckLinkDiscovery != NULL)
		m_deckLinkDiscovery->UninstallDeviceNotifications();
}

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceArrived(/* in */ IDeckLink* deckLink)
{
	// Update UI (add new device to menu) from main thread
	QCoreApplication::postEvent( m_uiDelegate, new SignalGeneratorEvent( ADD_DEVICE_EVENT, deckLink ) );
	return S_OK;
}

HRESULT DeckLinkDeviceDiscovery::DeckLinkDeviceRemoved(/* in */ IDeckLink* deckLink)
{
	// Update UI (remove new device to menu) from main thread
	QCoreApplication::postEvent( m_uiDelegate, new SignalGeneratorEvent( REMOVE_DEVICE_EVENT, deckLink ) );
	return S_OK;
}


HRESULT DeckLinkDeviceDiscovery::QueryInterface(REFIID iid, LPVOID *ppv)
{
	CFUUIDBytes		iunknown;
	HRESULT			result = E_NOINTERFACE;

	if (ppv == NULL)
		return E_INVALIDARG;

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

ULONG DeckLinkDeviceDiscovery::AddRef(void)
{
	return (ULONG) m_refCount.fetchAndAddAcquire(1);
}

ULONG DeckLinkDeviceDiscovery::Release(void)
{
	ULONG		newRefValue;

	newRefValue = (ULONG) m_refCount.fetchAndAddAcquire(-1);
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}

	return newRefValue;
}
