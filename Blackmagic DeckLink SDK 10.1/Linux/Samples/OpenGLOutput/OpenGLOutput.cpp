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
// OpenGLOutput.cpp
// OpenGLOutput
//

#include "OpenGLOutput.h"
#include "CDeckLinkGLWidget.h"
#include "ui_OpenGLOutput.h"

OpenGLOutput::OpenGLOutput()
:QDialog()
{
    ui = new Ui::OpenGLOutputDialog();
    ui->setupUi(this);

    layout = new QGridLayout(ui->previewContainer);
    layout->setMargin(0);

    previewView = new CDeckLinkGLWidget(this);
    previewView->resize(ui->previewContainer->size());
    previewView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(previewView, 0, 0, 0, 0);
    previewView->DrawFrame(NULL);

    pOpenGLOutput = new BMDOpenGLOutput();

	if (!pOpenGLOutput->InitDeckLink())
		exit(0);
	if (!pOpenGLOutput->InitGUI(previewView))
		exit(0);
	if (!pOpenGLOutput->InitOpenGL())
		exit(0);

	pTimer = new QTimer(this);
	connect(pTimer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	setWindowTitle("OpenGLOutput");
    show();
}

void OpenGLOutput::OnTimer()
{
    pOpenGLOutput->UpdateScene();
}

OpenGLOutput::~OpenGLOutput()
{
	if (pTimer)
	{
		pTimer->stop();
		delete pTimer;
	}

	if (pOpenGLOutput)
	{
		pOpenGLOutput->Stop();
		delete pOpenGLOutput;
		pOpenGLOutput = NULL;
	}

	delete previewView;
	previewView = NULL;

	delete layout;
	layout = NULL;

	delete ui;
	ui = NULL;
}

void OpenGLOutput::start()
{
    if (pOpenGLOutput != NULL)
	{
        if (!pOpenGLOutput->Start())
           exit(0);
		pTimer->start(1000 / pOpenGLOutput->GetFPS());
	}
}

