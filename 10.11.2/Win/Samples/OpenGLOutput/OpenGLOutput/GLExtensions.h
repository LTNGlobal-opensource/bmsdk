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
//  GLExtensions.h
//  OpenGLOutput
//

#ifndef __GLExtensions_h__
#define __GLExtensions_h__

#include <windows.h>
#include <gl/gl.h>

#define GL_UNSIGNED_INT_8_8_8_8_REV	0x8367

#define GL_BGRA						0x80E1
#define GL_RENDERBUFFER_EXT			0x8D41
#define GL_FRAMEBUFFER_EXT			0x8D40
#define GL_FRAMEBUFFER_COMPLETE_EXT	0x8CD5
#define GL_COLOR_ATTACHMENT0_EXT	0x8CE0
#define GL_DEPTH_ATTACHMENT_EXT		0x8D00

typedef void (APIENTRY *BMD_glGenFramebuffersEXT) (GLsizei, GLuint *);
typedef void (APIENTRY *BMD_glGenRenderbuffersEXT) (GLsizei, GLuint *);
typedef void (APIENTRY *BMD_glBindRenderbufferEXT) (GLenum, GLuint);
typedef void (APIENTRY *BMD_glRenderbufferStorageEXT) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (APIENTRY *BMD_glDeleteFramebuffersEXT) (GLsizei, const GLuint*);
typedef void (APIENTRY *BMD_glDeleteRenderbuffersEXT) (GLsizei, const GLuint*);
typedef void (APIENTRY *BMD_glBindFramebufferEXT) (GLenum, GLuint);
typedef void (APIENTRY *BMD_glFramebufferTexture2DEXT) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (APIENTRY *BMD_glFramebufferRenderbufferEXT) (GLenum, GLenum, GLenum, GLuint);
typedef GLenum (APIENTRY *BMD_glCheckFramebufferStatusEXT) (GLenum);

struct GLExtensions
{
        bool ResolveExtensions();

        BMD_glGenFramebuffersEXT pGenFramebuffersEXT;
        BMD_glGenRenderbuffersEXT pGenRenderbuffersEXT;
        BMD_glBindRenderbufferEXT pBindRenderbufferEXT;
        BMD_glRenderbufferStorageEXT pRenderbufferStorageEXT;
        BMD_glDeleteFramebuffersEXT pDeleteFramebuffersEXT;
        BMD_glDeleteRenderbuffersEXT pDeleteRenderbuffersEXT;
        BMD_glBindFramebufferEXT pBindFramebufferEXT;
        BMD_glFramebufferTexture2DEXT pFramebufferTexture2DEXT;
        BMD_glFramebufferRenderbufferEXT pFramebufferRenderbufferEXT;
        BMD_glCheckFramebufferStatusEXT pCheckFramebufferStatusEXT;
};

inline GLExtensions &getGLExtensions()
{
        static GLExtensions glext;
        return glext;
}

#define glGenFramebuffersEXT getGLExtensions().pGenFramebuffersEXT
#define glGenRenderbuffersEXT getGLExtensions().pGenRenderbuffersEXT
#define glBindRenderbufferEXT getGLExtensions().pBindRenderbufferEXT
#define glRenderbufferStorageEXT getGLExtensions().pRenderbufferStorageEXT
#define glDeleteFramebuffersEXT getGLExtensions().pDeleteFramebuffersEXT
#define glDeleteRenderbuffersEXT getGLExtensions().pDeleteRenderbuffersEXT
#define glBindFramebufferEXT getGLExtensions().pBindFramebufferEXT
#define glFramebufferTexture2DEXT getGLExtensions().pFramebufferTexture2DEXT
#define glFramebufferRenderbufferEXT getGLExtensions().pFramebufferRenderbufferEXT
#define glCheckFramebufferStatusEXT getGLExtensions().pCheckFramebufferStatusEXT

#endif // __GLExtensions_h__

