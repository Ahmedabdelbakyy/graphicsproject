#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLUT_DISABLE_ATEXIT_HACK
#include <glut.h>
#include "GameConfig.h"

int currentLevel = 1;

// --- CALLBACK DISPATCHERS ---
void Master_Display() {
	if (currentLevel == 1) Level1::Display();
	else Level2::Display();
}

void Master_Reshape(int w, int h) {
	if (currentLevel == 1) Level1::Reshape(w, h);
	else Level2::Reshape(w, h);
}

void Master_Keyboard(unsigned char key, int x, int y) {
	if (key == 27) exit(0);
	if (currentLevel == 1) Level1::Keyboard(key, x, y);
	else Level2::Keyboard(key, x, y);
}

void Master_Anim() {
	if (currentLevel == 1) Level1::Anim();
	else Level2::Anim();
}

// --- NEW: MASTER MOUSE FUNCTION ---
void Master_Mouse(int button, int state, int x, int y) {
	if (currentLevel == 1) {
		Level1::Mouse(button, state, x, y);
	}
	else if (currentLevel == 2) {
		Level2::Mouse(button, state, x, y);
	}
}

// --- MAIN ---
int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(150, 150);
	glutCreateWindow("ROBO COLLECTOR");

	glutDisplayFunc(Master_Display);
	glutReshapeFunc(Master_Reshape);
	glutKeyboardFunc(Master_Keyboard);

	// *** THIS LINE IS CRITICAL FOR MOUSE ***
	glutMouseFunc(Master_Mouse);
	// ***************************************

	glutIdleFunc(Master_Anim);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);

	Level1::Init();
	Level2::Init();

	glutMainLoop();
	return 0;
}