#pragma once

// Global State Variable to track active level
// 1 = Desert (Your Level), 2 = Future City (Friend's Level)
extern int currentLevel;

// ===========================
// LEVEL 1 DECLARATIONS
// ===========================
namespace Level1 {
	void Init();
	void Display();
	void Reshape(int w, int h);
	void Keyboard(unsigned char key, int x, int y);
	void Anim();
}

// ===========================
// LEVEL 2 DECLARATIONS
// ===========================
namespace Level2 {
	void Init();
	void Display();
	void Reshape(int w, int h);
	void Keyboard(unsigned char key, int x, int y);
	void Anim();
}