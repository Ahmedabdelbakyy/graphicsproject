// --- ADD WINDOWS HEADER FIRST ---
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OPENGL
#define GLUT_DISABLE_ATEXIT_HACK
#include <glut.h>

// INCLUDE THE BRIDGE HEADER
#include "GameConfig.h"

// Initialize the Global State Variable
int currentLevel = 1;

// ============================================
// MASTER CALLBACK FUNCTIONS
// ============================================

void Master_Display() {
	if (currentLevel == 1) {
		Level1::Display();
	}
	else if (currentLevel == 2) {
		Level2::Display();
	}
}

void Master_Reshape(int w, int h) {
	if (currentLevel == 1) {
		Level1::Reshape(w, h);
	}
	else if (currentLevel == 2) {
		Level2::Reshape(w, h);
	}
}

void Master_Keyboard(unsigned char key, int x, int y) {
	// Global Quit Key (ESC)
	if (key == 27) exit(0);

	if (currentLevel == 1) {
		Level1::Keyboard(key, x, y);
	}
	else if (currentLevel == 2) {
		Level2::Keyboard(key, x, y);
	}
}

void Master_Anim() {
	if (currentLevel == 1) {
		Level1::Anim();
	}
	else if (currentLevel == 2) {
		Level2::Anim();
	}
}

// ============================================
// MAIN ENTRY POINT
// ============================================

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(150, 150);
	glutCreateWindow("ROBO COLLECTOR - FULL GAME");

	// Register Master Callbacks
	glutDisplayFunc(Master_Display);
	glutReshapeFunc(Master_Reshape);
	glutKeyboardFunc(Master_Keyboard);
	glutIdleFunc(Master_Anim);

	// Standard OpenGL Init
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL);

	// Initialize Level 1 Assets FIRST
	Level1::Init();

	// Initialize Level 2 Assets SECOND
	Level2::Init();

	printf("----------------------------------------\n");
	printf(" ROBO COLLECTOR - GAME STARTED \n");
	printf("----------------------------------------\n");
	printf(" Level 1: Desert (Collect 10 Crystals)\n");
	printf(" Level 2: Future City (Collect 100 Orbs)\n");
	printf(" Controls: WASD to Move, 1/3 to Switch Camera\n");
	printf("----------------------------------------\n");

	glutMainLoop();
	return 0;
}