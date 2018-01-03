/* -LICENSE-START-
 ** Copyright (c) 2010 Blackmagic Design
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
//  BMDOpenGLOutput.mm
//  OpenGLOutput
//

#include "BMDOpenGLOutput.h"
#include <libkern/OSAtomic.h>


BMDOpenGLOutput::BMDOpenGLOutput()
	: pRenderDelegate(NULL), pGLScene(NULL), pDL(NULL), pDLOutput(NULL)
{
	CGLPixelFormatAttribute attribs[] =
	{
		kCGLPFAColorSize, (CGLPixelFormatAttribute)32,
		kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
		kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
		kCGLPFAAccelerated,
		(CGLPixelFormatAttribute)0
	};
	CGLPixelFormatObj pixelFormatObj;
	GLint numPixelFormats; 
	CGLChoosePixelFormat (attribs, &pixelFormatObj, &numPixelFormats);
	CGLCreateContext (pixelFormatObj, 0, &contextObj);
	CGLDestroyPixelFormat (pixelFormatObj); 	
	pGLScene = new GLScene();
}

BMDOpenGLOutput::~BMDOpenGLOutput()
{
	if (pDLOutput != NULL)
	{
		pDLOutput->Release();
		pDLOutput = NULL;
	}
	if (pDL != NULL)
	{
		pDL->Release();
		pDL = NULL;
	}
	if (pRenderDelegate != NULL)
	{
		pRenderDelegate->Release();
		pRenderDelegate = NULL;
	}

	delete pGLScene;
	pGLScene = NULL;
	
	CGLDestroyContext (contextObj);
}

void BMDOpenGLOutput::SetPreroll()
{
	IDeckLinkMutableVideoFrame* pDLVideoFrame;
	
	// Set 3 frame preroll
	for (uint32_t i=0; i < 3; i++)
	{
		// Flip frame vertical, because OpenGL rendering starts from left bottom corner
		if (pDLOutput->CreateVideoFrame(uiFrameWidth, uiFrameHeight, uiFrameWidth*4, bmdFormat8BitBGRA, bmdFrameFlagFlipVertical, &pDLVideoFrame) != S_OK)
			goto bail;
		
		if (pDLOutput->ScheduleVideoFrame(pDLVideoFrame, (uiTotalFrames * frameDuration), frameDuration, frameTimescale) != S_OK)
			goto bail;
		
		/* The local reference to the IDeckLinkVideoFrame is released here, as the ownership has now been passed to
		 *  the DeckLinkAPI via ScheduleVideoFrame.
		 *
		 * After the API has finished with the frame, it is returned to the application via ScheduledFrameCompleted.
		 * In ScheduledFrameCompleted, this application updates the video frame and passes it to ScheduleVideoFrame,
		 * returning ownership to the DeckLink API.
		 */
		pDLVideoFrame->Release();
		pDLVideoFrame = NULL;
		
		uiTotalFrames++;
	}
	return;
	
bail:
	if (pDLVideoFrame)
	{
		pDLVideoFrame->Release();
		pDLVideoFrame = NULL;
	}
}

bool BMDOpenGLOutput::InitDeckLink()
{
	bool bSuccess = FALSE;
	IDeckLinkIterator* pDLIterator = NULL;

	pDLIterator = CreateDeckLinkIteratorInstance();
	if (pDLIterator == NULL)
	{
		NSRunAlertPanel(@"This application requires the DeckLink drivers installed.", @"Please install the Blackmagic DeckLink drivers to use the features of this application.", @"OK", nil, nil);
		goto error;
	}

	if (pDLIterator->Next(&pDL) != S_OK)
	{
		NSRunAlertPanel(@"This application requires a DeckLink device.", @"You will not be able to use the features of this application until a DeckLink PCI card is installed.", @"OK", nil, nil);
		goto error;
	}
	
	if (pDL->QueryInterface(IID_IDeckLinkOutput, (void**)&pDLOutput) != S_OK)
		goto error;
	
	pRenderDelegate = new RenderDelegate(this);
	if (pRenderDelegate == NULL)
		goto error;
	
	if (pDLOutput->SetScheduledFrameCompletionCallback(pRenderDelegate) != S_OK)
		goto error;
	
	bSuccess = TRUE;

error:
	
	if (!bSuccess)
	{
		if (pDLOutput != NULL)
		{
			pDLOutput->Release();
			pDLOutput = NULL;
		}
		if (pDL != NULL)
		{
			pDL->Release();
			pDL = NULL;
		}
		if (pRenderDelegate != NULL)
		{
			pRenderDelegate->Release();
			pRenderDelegate = NULL;
		}
	}
	
	if (pDLIterator != NULL)
	{
		pDLIterator->Release();
		pDLIterator = NULL;
	}
	
	return bSuccess;
}

bool BMDOpenGLOutput::InitGUI(IDeckLinkScreenPreviewCallback *previewCallback)
{
	// Set preview
	if (previewCallback != NULL)
	{
		pDLOutput->SetScreenPreviewCallback(previewCallback);
	}
	return true;
}

bool BMDOpenGLOutput::InitOpenGL()
{
	const GLubyte * strExt;
	GLboolean isFBO;
	
    CGLSetCurrentContext (contextObj);
	
	strExt = glGetString (GL_EXTENSIONS);
	isFBO = gluCheckExtension ((const GLubyte*)"GL_EXT_framebuffer_object", strExt);
	
	if (!isFBO)
	{
		NSRunAlertPanel(@"OpenGL initialization error.", @"OpenGL extention \"GL_EXT_framebuffer_object\" is not supported.", @"OK", nil, nil);
		return false;
	}

	return true;
}

bool BMDOpenGLOutput::Start()
{
	bool								bSuccess = false;
	IDeckLinkDisplayModeIterator*		pDLDisplayModeIterator;
	IDeckLinkDisplayMode*				pDLDisplayMode = NULL;
	
	// Get first available video mode for Output
	if (pDLOutput->GetDisplayModeIterator(&pDLDisplayModeIterator) == S_OK)
	{
		if (pDLDisplayModeIterator->Next(&pDLDisplayMode) != S_OK)
		{
			NSRunAlertPanel(@"DeckLink error.", @"Cannot find video mode.", @"OK", nil, nil);
			goto bail;
		}
	}
	
	uiFrameWidth = pDLDisplayMode->GetWidth();
	uiFrameHeight = pDLDisplayMode->GetHeight();
	pDLDisplayMode->GetFrameRate(&frameDuration, &frameTimescale);
	
	uiFPS = ((frameTimescale + (frameDuration-1))  /  frameDuration);
	
	if (pDLOutput->EnableVideoOutput(pDLDisplayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) != S_OK)
		goto bail;
	
	uiTotalFrames = 0;
	
	SetPreroll();

	CGLSetCurrentContext (contextObj);
		
	pGLScene->InitScene();

	glGenFramebuffersEXT(1, &idFrameBuf);
	glGenRenderbuffersEXT(1, &idColorBuf);
	glGenRenderbuffersEXT(1, &idDepthBuf);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, idFrameBuf);
	
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, idColorBuf);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, uiFrameWidth, uiFrameHeight);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, idDepthBuf);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, uiFrameWidth, uiFrameHeight);
	
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, idColorBuf);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, idDepthBuf);
	
	glStatus = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (glStatus != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		NSRunAlertPanel(@"OpenGL initialization error.", @"Cannot initialize framebuffer.", @"OK", nil, nil);
		goto bail;
	}

	pDLOutput->StartScheduledPlayback(0, 100, 1.0);
	bSuccess = true;
	
bail:
	if (pDLDisplayMode)
	{
		pDLDisplayMode->Release();
		pDLDisplayMode = NULL;
	}
	if (pDLDisplayModeIterator)
	{
		pDLDisplayModeIterator->Release();
		pDLDisplayModeIterator = NULL;
	}
	
	return bSuccess;
}

bool BMDOpenGLOutput::Stop()
{
	if (pDLOutput == NULL)
		return false;
	
	pDLOutput->StopScheduledPlayback(0, NULL, 0);
	pDLOutput->DisableVideoOutput();
	

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		
	glDeleteRenderbuffersEXT(1, &idDepthBuf);
	glDeleteRenderbuffersEXT(1, &idColorBuf);
	glDeleteFramebuffersEXT(1, &idFrameBuf);	

	return true;
}

void BMDOpenGLOutput::RenderToDevice(IDeckLinkVideoFrame* pDLVideoFrame)
{
	void*	pFrame;

	pDLVideoFrame->GetBytes((void**)&pFrame);	

	old_contextObj = CGLGetCurrentContext();
    CGLSetCurrentContext (contextObj);

	pGLScene->DrawScene(0, 0, uiFrameWidth, uiFrameHeight);

	glReadPixels(0, 0, uiFrameWidth, uiFrameHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pFrame);

	if (pDLOutput->ScheduleVideoFrame(pDLVideoFrame, (uiTotalFrames * frameDuration), frameDuration, frameTimescale) == S_OK)
		uiTotalFrames++;
	
	CGLSetCurrentContext(old_contextObj);
}

////////////////////////////////////////////
// Render Delegate Class
////////////////////////////////////////////
RenderDelegate::RenderDelegate (BMDOpenGLOutput* pOwner)
{
	m_refCount = 1;
	m_pOwner = pOwner;
}

HRESULT	RenderDelegate::QueryInterface (REFIID iid, LPVOID *ppv)
{
	*ppv = NULL;
	return E_NOINTERFACE;
}

ULONG	RenderDelegate::AddRef ()
{
	return OSAtomicIncrement32(&m_refCount);
}

ULONG	RenderDelegate::Release ()
{
	int32_t		newRefValue;
	
	newRefValue = OSAtomicDecrement32(&m_refCount);
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}
	
	return newRefValue;
}

HRESULT	RenderDelegate::ScheduledFrameCompleted (IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result)
{
	m_pOwner->RenderToDevice(completedFrame);
	return S_OK;
}


HRESULT	RenderDelegate::ScheduledPlaybackHasStopped ()
{
	return S_OK;
}
