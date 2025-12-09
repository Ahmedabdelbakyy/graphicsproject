#define _CRT_SECURE_NO_WARNINGS

// WINDOWS AUDIO & HEADERS
#include <windows.h> // <--- MOVED TO TOP
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <time.h>

#define GLUT_DISABLE_ATEXIT_HACK
#include <glut.h> // <--- BELOW WINDOWS.H

#include "GameConfig.h"

namespace Level1 { // <--- START NAMESPACE

	// Global Variables
	float playerX = 0.0f;
	float playerZ = 0.0f;
	float playerRot = 0.0f;
	int cameraMode = 1;
	float crystalRot = 0.0f;

	// GAME LOGIC GLOBALS
	int score = 0;
	int lives = 5;
	bool levelComplete = false;
	bool gameOver = false;

	struct GameObject {
		float x, z;
		bool active;
	};

	std::vector<GameObject> crystals;
	std::vector<GameObject> traps;

	// Texture IDs
	int texGround;
	int texSky;
	int texPyramid;
	int texPortal;
	int texCrystal;
	int texTrap;
	int texPlayer;

	// 3DS Chunk IDs
#define PRIMARY       0x4D4D
#define OBJECTINFO    0x3D3D
#define VERSION       0x0002
#define EDITKEYFRAME  0xB000
#define MATERIAL	  0xAFFF
#define OBJECT		  0x4000
#define MATNAME       0xA000
#define MATDIFFUSE    0xA020
#define MATMAP        0xA200
#define MATMAPFILE    0xA300
#define OBJECT_MESH   0x4100
#define OBJECT_VERTICES 0x4110
#define OBJECT_FACES    0x4120
#define OBJECT_MATERIAL 0x4130
#define OBJECT_UV       0x4140

	struct Chunk {
		unsigned short int ID;
		unsigned int length;
		unsigned int bytesRead;
	};

	struct Point3d {
		float x, y, z;
	};

	struct Face {
		unsigned short int a, b, c;
	};

	struct TexCoord {
		float u, v;
	};

	struct MeshPart {
		int numVertices;
		int numFaces;
		int numTexCoords;

		Point3d* Vertices;
		Face* Faces;
		Point3d* Normals;
		TexCoord* TexCoords;

		MeshPart() {
			numVertices = 0; numFaces = 0; numTexCoords = 0;
			Vertices = NULL; Faces = NULL; Normals = NULL; TexCoords = NULL;
		}
	};

	int LoadTexture(const char* filename, int alpha = 0) {
		unsigned int texture;
		FILE* file;
		unsigned char header[54];
		unsigned int dataPos;
		unsigned int width, height;
		unsigned int imageSize;
		unsigned char* data;

		file = fopen(filename, "rb");
		if (!file) {
			printf("Error: Texture file not found: %s\n", filename);
			return -1;
		}

		if (fread(header, 1, 54, file) != 54) return -1;

		if (header[0] != 'B' || header[1] != 'M') return -1;

		dataPos = *(int*)&(header[0x0A]);
		imageSize = *(int*)&(header[0x22]);
		width = *(int*)&(header[0x12]);
		height = *(int*)&(header[0x16]);

		if (imageSize == 0) imageSize = width * height * 3;
		if (dataPos == 0) dataPos = 54;

		data = new unsigned char[imageSize];
		fread(data, 1, imageSize, file);
		fclose(file);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		delete[] data;
		return texture;
	}

	class Model_3DS {
	public:
		std::vector<MeshPart> parts;
		int tex;
		float pos[3];
		float rot[3];
		float scale[3];

		Model_3DS() {
			tex = -1;
			pos[0] = 0; pos[1] = 0; pos[2] = 0;
			rot[0] = 0; rot[1] = 0; rot[2] = 0;
			scale[0] = 1; scale[1] = 1; scale[2] = 1;
		}

		void Load(const char* path) {
			FILE* file = fopen(path, "rb");
			if (!file) {
				printf("!!! FAILURE: Could not open file: %s !!!\n", path);
				return;
			}

			Chunk chunk;
			ReadChunk(file, &chunk);

			if (chunk.ID != PRIMARY) {
				printf("!!! FAILURE: Invalid 3DS file format: %s !!!\n", path);
				fclose(file);
				return;
			}

			ProcessChunk(file, &chunk);
			fclose(file);

			for (size_t i = 0; i < parts.size(); i++) {
				ComputeNormals(parts[i]);
			}
			printf("SUCCESS: Loaded model %s with %d parts.\n", path, (int)parts.size());
		}

		void Draw() {
			glPushMatrix();
			glTranslatef(pos[0], pos[1], pos[2]);
			glRotatef(rot[0], 1, 0, 0);
			glRotatef(rot[1], 0, 1, 0);
			glRotatef(rot[2], 0, 0, 1);
			glScalef(scale[0], scale[1], scale[2]);

			if (tex != -1) {
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, tex);
				glColor3f(1, 1, 1);
			}
			else {
				glDisable(GL_TEXTURE_2D);
			}

			for (size_t i = 0; i < parts.size(); i++) {
				MeshPart& mesh = parts[i];
				if (mesh.numFaces == 0) continue;

				glBegin(GL_TRIANGLES);
				for (int j = 0; j < mesh.numFaces; j++) {
					if (mesh.Normals) glNormal3f(mesh.Normals[j].x, mesh.Normals[j].y, mesh.Normals[j].z);

					if (mesh.TexCoords) glTexCoord2f(mesh.TexCoords[mesh.Faces[j].a].u, mesh.TexCoords[mesh.Faces[j].a].v);
					glVertex3f(mesh.Vertices[mesh.Faces[j].a].x, mesh.Vertices[mesh.Faces[j].a].y, mesh.Vertices[mesh.Faces[j].a].z);

					if (mesh.TexCoords) glTexCoord2f(mesh.TexCoords[mesh.Faces[j].b].u, mesh.TexCoords[mesh.Faces[j].b].v);
					glVertex3f(mesh.Vertices[mesh.Faces[j].b].x, mesh.Vertices[mesh.Faces[j].b].y, mesh.Vertices[mesh.Faces[j].b].z);

					if (mesh.TexCoords) glTexCoord2f(mesh.TexCoords[mesh.Faces[j].c].u, mesh.TexCoords[mesh.Faces[j].c].v);
					glVertex3f(mesh.Vertices[mesh.Faces[j].c].x, mesh.Vertices[mesh.Faces[j].c].y, mesh.Vertices[mesh.Faces[j].c].z);
				}
				glEnd();
			}

			if (tex != -1) glDisable(GL_TEXTURE_2D);

			glPopMatrix();
		}

	private:
		void ReadChunk(FILE* file, Chunk* chunk) {
			fread(&chunk->ID, 2, 1, file);
			fread(&chunk->length, 4, 1, file);
			chunk->bytesRead = 6;
		}

		void SkipChunk(FILE* file, Chunk* chunk) {
			fseek(file, chunk->length - chunk->bytesRead, SEEK_CUR);
		}

		void ProcessChunk(FILE* file, Chunk* chunk) {
			while (chunk->bytesRead < chunk->length) {
				Chunk child;
				ReadChunk(file, &child);

				switch (child.ID) {
				case OBJECTINFO:
					ProcessChunk(file, &child);
					break;

				case OBJECT:
				{
					char name[100];
					int i = 0;
					char c;
					do {
						fread(&c, 1, 1, file);
						name[i] = c;
						i++;
					} while (c != '\0' && i < 99);
					child.bytesRead += i;
					ProcessChunk(file, &child);
				}
				break;

				case OBJECT_MESH:
				{
					MeshPart newPart;
					parts.push_back(newPart);
					ProcessChunk(file, &child);
				}
				break;

				case OBJECT_VERTICES:
					ReadVertices(file, &child);
					break;

				case OBJECT_FACES:
					ReadFaces(file, &child);
					break;

				case OBJECT_UV:
					ReadTexCoords(file, &child);
					break;

				default:
					SkipChunk(file, &child);
					break;
				}
				chunk->bytesRead += child.length;
			}
		}

		void ReadVertices(FILE* file, Chunk* chunk) {
			if (parts.empty()) return;
			MeshPart& mesh = parts.back();

			unsigned short int num;
			fread(&num, 2, 1, file);
			mesh.numVertices = num;
			mesh.Vertices = new Point3d[mesh.numVertices];
			for (int i = 0; i < mesh.numVertices; i++) {
				fread(&mesh.Vertices[i].x, 4, 1, file);
				fread(&mesh.Vertices[i].y, 4, 1, file);
				fread(&mesh.Vertices[i].z, 4, 1, file);
			}
			chunk->bytesRead += 2 + mesh.numVertices * 12;
			SkipChunk(file, chunk);
		}

		void ReadFaces(FILE* file, Chunk* chunk) {
			if (parts.empty()) return;
			MeshPart& mesh = parts.back();

			unsigned short int num;
			fread(&num, 2, 1, file);
			mesh.numFaces = num;
			mesh.Faces = new Face[mesh.numFaces];
			for (int i = 0; i < mesh.numFaces; i++) {
				fread(&mesh.Faces[i].a, 2, 1, file);
				fread(&mesh.Faces[i].b, 2, 1, file);
				fread(&mesh.Faces[i].c, 2, 1, file);
				unsigned short int faceFlag;
				fread(&faceFlag, 2, 1, file);
			}
			chunk->bytesRead += 2 + mesh.numFaces * 8;
			SkipChunk(file, chunk);
		}

		void ReadTexCoords(FILE* file, Chunk* chunk) {
			if (parts.empty()) return;
			MeshPart& mesh = parts.back();

			unsigned short int num;
			fread(&num, 2, 1, file);
			mesh.numTexCoords = num;
			mesh.TexCoords = new TexCoord[mesh.numTexCoords];
			for (int i = 0; i < mesh.numTexCoords; i++) {
				fread(&mesh.TexCoords[i].u, 4, 1, file);
				fread(&mesh.TexCoords[i].v, 4, 1, file);
			}
			chunk->bytesRead += 2 + mesh.numTexCoords * 8;
		}

		void ComputeNormals(MeshPart& mesh) {
			if (mesh.numFaces == 0) return;
			mesh.Normals = new Point3d[mesh.numFaces];
			for (int i = 0; i < mesh.numFaces; i++) {
				Point3d v1 = mesh.Vertices[mesh.Faces[i].a];
				Point3d v2 = mesh.Vertices[mesh.Faces[i].b];
				Point3d v3 = mesh.Vertices[mesh.Faces[i].c];

				Point3d u = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
				Point3d v = { v3.x - v1.x, v3.y - v1.y, v3.z - v1.z };

				mesh.Normals[i].x = u.y * v.z - u.z * v.y;
				mesh.Normals[i].y = u.z * v.x - u.x * v.z;
				mesh.Normals[i].z = u.x * v.y - u.y * v.x;

				float len = sqrt(mesh.Normals[i].x * mesh.Normals[i].x + mesh.Normals[i].y * mesh.Normals[i].y + mesh.Normals[i].z * mesh.Normals[i].z);
				if (len > 0) {
					mesh.Normals[i].x /= len;
					mesh.Normals[i].y /= len;
					mesh.Normals[i].z /= len;
				}
			}
		}
	};

	// Global Models
	Model_3DS modelPlayer;
	Model_3DS modelPyramid;
	Model_3DS modelPortal;
	Model_3DS modelCrystal;
	Model_3DS modelTrap;

	void Keyboard(unsigned char key, int x, int y) {
		// NEW: Reset Logic
		if (gameOver && (key == 'r' || key == 'R')) {
			score = 0;
			lives = 5;
			playerX = 0;
			playerZ = 0;
			playerRot = 0;
			gameOver = false;
			levelComplete = false;
			for (size_t i = 0; i < crystals.size(); i++) crystals[i].active = true;
			glutPostRedisplay();
			return;
		}

		if (levelComplete || gameOver) return;

		float d = 4.0f;

		switch (key) {
		case 'w': case 'W': playerX += d * sin(playerRot * 3.14159 / 180); playerZ += d * cos(playerRot * 3.14159 / 180); break;
		case 's': case 'S': playerX -= d * sin(playerRot * 3.14159 / 180); playerZ -= d * cos(playerRot * 3.14159 / 180); break;
		case 'a': case 'A': playerRot += 5.0f; break;
		case 'd': case 'D': playerRot -= 5.0f; break;
		case '1': cameraMode = 0; break;
		case '3': cameraMode = 1; break;
		}
		glutPostRedisplay();
	}

	void drawBitmapText(const char* string, float x, float y, float z) {
		const char* c;
		glRasterPos3f(x, y, z);
		for (c = string; *c != '\0'; c++) {
			glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
		}
	}

	void Anim() {
		if (levelComplete || gameOver) return;

		crystalRot += 0.5f;

		for (size_t i = 0; i < crystals.size(); i++) {
			if (crystals[i].active) {
				float dist = sqrt(pow(playerX - crystals[i].x, 2) + pow(playerZ - crystals[i].z, 2));
				if (dist < 3.0f) {
					crystals[i].active = false;
					score++;
					PlaySound(TEXT("collect.wav"), NULL, SND_ASYNC | SND_FILENAME);
				}
			}
		}

		for (size_t i = 0; i < traps.size(); i++) {
			float dist = sqrt(pow(playerX - traps[i].x, 2) + pow(playerZ - traps[i].z, 2));
			if (dist < 10.0f) {
				PlaySound(TEXT("hit.wav"), NULL, SND_ASYNC | SND_FILENAME);
				lives--;
				playerX = 0; playerZ = 0; playerRot = 0;
				printf("OUCH! Lives left: %d\n", lives);
				if (lives <= 0) {
					gameOver = true;
					printf("GAME OVER\n");
				}
			}
		}

		// *** THIS IS THE CRITICAL CHANGE FOR THE MERGE ***
		float portalDist = sqrt(pow(playerX - 0, 2) + pow(playerZ - (-200), 2));
		if (portalDist < 10.0f && score >= 10) {
			levelComplete = true;
			printf("YOU WIN! LOADING LEVEL 2...\n");
			PlaySound(TEXT("win.wav"), NULL, SND_ASYNC | SND_FILENAME);

			// SWITCH TO LEVEL 2
			currentLevel = 2;
			Level2::Init(); // Start the next level
		}

		glutPostRedisplay();
	}

	void Reshape(int w, int h) {
		if (h == 0) h = 1;
		float ratio = (float)w / h;
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glViewport(0, 0, w, h);
		gluPerspective(45.0f, ratio, 0.1f, 4000.0f);
		glMatrixMode(GL_MODELVIEW);
	}

	void setupLighting() {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		GLfloat lightPos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
		GLfloat lightColor[] = { 1.0f, 1.0f, 0.9f, 1.0f };
		GLfloat ambientColor[] = { 0.4f, 0.4f, 0.4f, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambientColor);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	}

	void DrawGround() {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texGround);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glNormal3f(0, 1, 0);
		glTexCoord2f(0, 0);   glVertex3f(-1000, 0, -1000);
		glTexCoord2f(200, 0); glVertex3f(1000, 0, -1000);
		glTexCoord2f(200, 200); glVertex3f(1000, 0, 1000);
		glTexCoord2f(0, 200); glVertex3f(-1000, 0, 1000);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	void DrawSky() {
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texSky);
		glColor3f(1, 1, 1);
		float d = 500.0f;
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex3f(-d, -d, -d); glTexCoord2f(1, 0); glVertex3f(d, -d, -d); glTexCoord2f(1, 1); glVertex3f(d, d, -d); glTexCoord2f(0, 1); glVertex3f(-d, d, -d);
		glTexCoord2f(0, 0); glVertex3f(d, -d, d); glTexCoord2f(1, 0); glVertex3f(-d, -d, d); glTexCoord2f(1, 1); glVertex3f(-d, d, d); glTexCoord2f(0, 1); glVertex3f(d, d, d);
		glTexCoord2f(0, 0); glVertex3f(-d, -d, d); glTexCoord2f(1, 0); glVertex3f(-d, -d, -d); glTexCoord2f(1, 1); glVertex3f(-d, d, -d); glTexCoord2f(0, 1); glVertex3f(-d, d, d);
		glTexCoord2f(0, 0); glVertex3f(d, -d, -d); glTexCoord2f(1, 0); glVertex3f(d, -d, d); glTexCoord2f(1, 1); glVertex3f(d, d, d); glTexCoord2f(0, 1); glVertex3f(d, d, -d);
		glTexCoord2f(0, 0); glVertex3f(-d, d, -d); glTexCoord2f(1, 0); glVertex3f(d, d, -d); glTexCoord2f(1, 1); glVertex3f(d, d, d); glTexCoord2f(0, 1); glVertex3f(-d, d, d);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
	}

	void drawScore() {
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D(0, 800, 0, 600);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 0.0f);
		char scoreText[50];
		sprintf(scoreText, "Score: %d / 10", score);
		drawBitmapText(scoreText, 20, 560, 0);
		char livesText[50];
		sprintf(livesText, "Lives: %d", lives);
		drawBitmapText(livesText, 20, 530, 0);
		if (levelComplete) {
			glColor3f(0.0f, 1.0f, 0.0f);
			drawBitmapText("CONGRATULATIONS!", 300, 320, 0);
			drawBitmapText("LEVEL 1 COMPLETE", 300, 280, 0);
		}
		else if (gameOver) {
			glColor3f(0.0f, 0.0f, 0.0f);
			drawBitmapText("GAME OVER", 330, 320, 0);
			drawBitmapText("Press 'R' to Retry", 300, 280, 0);
		}
		else if (score >= 10) {
			glColor3f(0.0f, 1.0f, 1.0f);
			drawBitmapText("Go to the Portal!", 300, 50, 0);
		}
		glEnable(GL_LIGHTING);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	void Display() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		GLfloat lightPos[] = { 0.0f, 100.0f, 0.0f, 1.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

		if (cameraMode == 1) {
			float cameraDist = 14.0f;
			float cameraHeight = 5.0f;
			float angleRad = playerRot * 3.14159f / 180.0f;
			float eyeX = playerX - cameraDist * sin(angleRad);
			float eyeZ = playerZ - cameraDist * cos(angleRad);
			float eyeY = cameraHeight;
			gluLookAt(eyeX, eyeY, eyeZ, playerX, 4.0f, playerZ, 0.0f, 1.0f, 0.0f);
		}
		else {
			float eyeHeight = 1.6f;
			float angleRad = playerRot * 3.14159f / 180.0f;
			float targetDist = 10.0f;
			float targetX = playerX + targetDist * sin(angleRad);
			float targetZ = playerZ + targetDist * cos(angleRad);
			gluLookAt(playerX, eyeHeight, playerZ, targetX, eyeHeight, targetZ, 0.0f, 1.0f, 0.0f);
		}

		DrawGround();
		DrawSky();

		glColor3f(1.0f, 1.0f, 1.0f);
		glPushMatrix(); glTranslatef(200, 0, 200); glRotatef(-90, 1, 0, 0); glScalef(3.0f, 3.0f, 3.0f); modelPyramid.Draw(); glPopMatrix();
		glPushMatrix(); glTranslatef(-200, 0, 200); glRotatef(-90, 1, 0, 0); glScalef(3.0f, 3.0f, 3.0f); modelPyramid.Draw(); glPopMatrix();
		glPushMatrix(); glTranslatef(200, 0, -200); glRotatef(-90, 1, 0, 0); glScalef(3.0f, 3.0f, 3.0f); modelPyramid.Draw(); glPopMatrix();
		glPushMatrix(); glTranslatef(-200, 0, -200); glRotatef(-90, 1, 0, 0); glScalef(3.0f, 3.0f, 3.0f); modelPyramid.Draw(); glPopMatrix();

		glPushMatrix(); glTranslatef(0, 0, -200); glRotatef(-90, 1, 0, 0); glScalef(0.5f, 0.5f, 0.5f); modelPortal.Draw(); glPopMatrix();

		for (size_t i = 0; i < crystals.size(); i++) {
			if (crystals[i].active) {
				glPushMatrix(); glTranslatef(crystals[i].x, 3.0f, crystals[i].z); glRotatef(crystalRot, 0, 1, 0); glRotatef(-90, 1, 0, 0); glScalef(0.01f, 0.01f, 0.01f); modelCrystal.Draw(); glPopMatrix();
			}
		}
		for (size_t i = 0; i < traps.size(); i++) {
			glPushMatrix(); glTranslatef(traps[i].x, 1.5f, traps[i].z); glScalef(0.05f, 0.05f, 0.05f); modelTrap.Draw(); glPopMatrix();
		}

		if (cameraMode == 1) {
			glPushMatrix(); glTranslatef(playerX, 0, playerZ); glRotatef(playerRot, 0, 1, 0); glRotatef(-90, 1, 0, 0); glScalef(0.075f, 0.075f, 0.075f); glColor3f(0.7f, 0.7f, 0.7f); modelPlayer.Draw(); glPopMatrix();
		}

		drawScore();
		glutSwapBuffers(); // USE SWAP BUFFERS
	}

	// RENAMED from main() to Init()
	void Init() {
		setupLighting();
		srand(time(NULL));

		modelPlayer.Load("Bobafett.3DS");
		modelPyramid.Load("pyramid.3DS");
		modelPortal.Load("portal.3DS");
		modelCrystal.Load("crystal.3DS");
		modelTrap.Load("trap.3DS");

		texGround = LoadTexture("sand.bmp", 255);
		texSky = LoadTexture("sky.bmp", 255);
		texPyramid = LoadTexture("pyramid.bmp", 255); modelPyramid.tex = texPyramid;
		texPortal = LoadTexture("portal.bmp", 255); modelPortal.tex = texPortal;
		texCrystal = LoadTexture("crystal.bmp", 255);
		texTrap = LoadTexture("trap.bmp", 255); modelTrap.tex = texTrap;
		texPlayer = LoadTexture("robot.bmp", 255); modelPlayer.tex = texPlayer;

		for (int i = 0; i < 50; i++) {
			GameObject c; c.x = (rand() % 600) - 300; c.z = (rand() % 600) - 300; c.active = true; crystals.push_back(c);
		}
		float tPos[5][2] = { {0, -100}, {0, -150}, {50, -120}, {-50, -120}, {0, 50} };
		for (int i = 0; i < 5; i++) {
			GameObject t; t.x = tPos[i][0]; t.z = tPos[i][1]; t.active = true; traps.push_back(t);
		}
	}

} // END NAMESPACE