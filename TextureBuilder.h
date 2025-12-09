#pragma once

// --- FIX: Force Windows Header inclusion before GL ---
#ifdef _WIN32
#include <windows.h>
#endif
// ---------------------------------------------------

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

// Matches your main.cpp call:
void loadBMP(GLuint* textureID, const char* filename, bool wrap);

// (Optional) compatibility if any file still calls the old Windows signature:
inline void loadBMP(GLuint* t, char* name, int wrap) {
	loadBMP(t, const_cast<const char*>(name), wrap != 0);
}