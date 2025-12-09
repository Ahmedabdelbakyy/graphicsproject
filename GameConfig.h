#pragma once

// Global State Variable
extern int currentLevel;

// LEVEL 1
namespace Level1 {
	void Init();
	void Display();
	void Reshape(int w, int h);
	void Keyboard(unsigned char key, int x, int y);
	void Mouse(int button, int state, int x, int y); // <--- REQUIRED
	void Anim();
}

// LEVEL 2
namespace Level2 {
	void Init();
	void Display();
	void Reshape(int w, int h);
	void Keyboard(unsigned char key, int x, int y);
	void Mouse(int button, int state, int x, int y); // <--- REQUIRED
	void Anim();
}