#define _CRT_SECURE_NO_WARNINGS

// --- WINDOWS & OPENGL HEADERS (ORDER MATTERS) ---
#include <windows.h>  // Must be first
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>

// GLUT (Handles gl.h and glu.h)
#define GLUT_DISABLE_ATEXIT_HACK
#include <glut.h>

// YOUR HELPERS
#include "Model_OBJ.h"  // <--- REVERTED to OBJ
#include "GameConfig.h" 
#include "TextureBuilder.h"

#define M_PI 3.14159265358979323846
namespace Level2 {

    // --- 1. GLOBAL VARIABLES ---
    GLuint texGround;
    GLuint texSky;
    GLuint texTech;
    GLuint texRobot;

    int width = 800;
    int height = 600;

    // Player State
    float playerX = 0.0f;
    float playerZ = 15.0f;
    float playerY = 0.0f;
    float playerRot = 180.0f;

    // Jump State
    bool isJumping = false;
    float jumpSpeed = 0.0f;

    // Camera Mode: 0 = First Person, 1 = Third Person
    int cameraMode = 1;

    // Robot Model
    Model_OBJ playerModel; // <--- REVERTED to OBJ

    // Game Logic
    int score = 0;
    float gameTime = 600.0f;
    bool gameOver = false;
    bool gameWon = false;

    // Teleport Pad
    float padX = 0.0f;
    float padZ = -40.0f;

    // --- GAME OBJECTS ---
    struct Orb { float x, z; bool active; float scale; bool collecting; };
    Orb orbs[10] = {
        {0, 0, true, 1.0f, false},      {5, -5, true, 1.0f, false},
        {-5, 10, true, 1.0f, false},    {15, -15, true, 1.0f, false},
        {-15, -15, true, 1.0f, false},  {20, 20, true, 1.0f, false},
        {-20, 20, true, 1.0f, false},   {25, 0, true, 1.0f, false},
        {-25, 0, true, 1.0f, false},    {0, -25, true, 1.0f, false}
    };

    struct Barrier { float x, z, angle; };
    Barrier barriers[6] = {
        {0, -5, 0},     {-5, 5, 90},    {5, 5, 90},
        {10, -10, 0},   {-10, -10, 0},  {0, 15, 90}
    };

    struct Drone { float startX, startZ, currentX, dir; };
    Drone drones[3] = {
        {0, 5, 0, 0.2f},
        {-15, -15, -15, 0.2f},
        {15, -15, 15, 0.2f}
    };

    // --- HELPER FUNCTIONS ---
    float toRad(float angle) { return angle * M_PI / 180.0f; }

    // --- NEW MOUSE FUNCTION ---
    void Mouse(int button, int state, int x, int y) {
        if (gameOver || gameWon) return;
        if (state == GLUT_DOWN) {
            if (button == GLUT_LEFT_BUTTON && !isJumping) {
                isJumping = true;
                jumpSpeed = 0.5f;
                PlaySound(TEXT("jump.wav"), NULL, SND_ASYNC | SND_FILENAME);
            }
            if (button == GLUT_RIGHT_BUTTON) {
                cameraMode = 1 - cameraMode; // Toggle 0 and 1
            }
        }
    }

    void drawText(std::string text, float x, float y) {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, width, 0, height);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        if (gameWon) glColor3f(0.0f, 1.0f, 0.0f);
        else if (gameOver) glColor3f(1.0f, 0.0f, 0.0f);
        else glColor3f(1.0f, 1.0f, 0.0f);

        glRasterPos2f(x, y);
        for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    // --- PHYSICS ---
    bool checkCollision(float objX, float objZ, float hitRadius) {
        float dist = sqrt(pow(playerX - objX, 2) + pow(playerZ - objZ, 2));
        return dist < hitRadius;
    }

    bool checkWall(float newX, float newZ) {
        bool hit = false;
        // 5 Buildings (Solid Walls)
        if (newX > -22 && newX < -18 && newZ > -22 && newZ < -18) hit = true;
        else if (newX > 16 && newX < 24 && newZ > -34 && newZ < -26) hit = true;
        else if (newX > -32.5 && newX < -27.5 && newZ > 7.5 && newZ < 12.5) hit = true;
        else if (newX > 22 && newX < 28 && newZ > 17 && newZ < 23) hit = true;
        else if (newX > -5 && newX < 5 && newZ > -55 && newZ < -45) hit = true;

        if (!hit) {
            for (int i = 0; i < 6; i++) {
                float bX = barriers[i].x;
                float bZ = barriers[i].z;
                if (barriers[i].angle == 0) {
                    if (newX > bX - 3 && newX < bX + 3 && newZ > bZ - 0.5 && newZ < bZ + 0.5) hit = true;
                }
                else {
                    if (newX > bX - 0.5 && newX < bX + 0.5 && newZ > bZ - 3 && newZ < bZ + 3) hit = true;
                }
            }
        }
        if (hit) {
            PlaySound(TEXT("hit.wav"), NULL, SND_ASYNC | SND_FILENAME);
        }
        return hit;
    }

    // --- DRAWING ---
    void drawSkybox() {
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texSky);
        glColor3f(1.0f, 1.0f, 1.0f);
        glDisable(GL_LIGHTING);
        glTranslatef(playerX, 0, playerZ);
        glScalef(300.0f, 300.0f, 300.0f);
        float half = 0.5f;
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(-half, -half, half); glTexCoord2f(1, 0); glVertex3f(half, -half, half); glTexCoord2f(1, 1); glVertex3f(half, half, half);   glTexCoord2f(0, 1); glVertex3f(-half, half, half);
        glTexCoord2f(0, 0); glVertex3f(half, -half, -half); glTexCoord2f(1, 0); glVertex3f(-half, -half, -half); glTexCoord2f(1, 1); glVertex3f(-half, half, -half); glTexCoord2f(0, 1); glVertex3f(half, half, -half);
        glTexCoord2f(0, 0); glVertex3f(-half, -half, -half); glTexCoord2f(1, 0); glVertex3f(-half, -half, half); glTexCoord2f(1, 1); glVertex3f(-half, half, half);   glTexCoord2f(0, 1); glVertex3f(-half, half, -half);
        glTexCoord2f(0, 0); glVertex3f(half, -half, half);   glTexCoord2f(1, 0); glVertex3f(half, -half, -half); glTexCoord2f(1, 1); glVertex3f(half, half, -half);   glTexCoord2f(0, 1); glVertex3f(half, half, half);
        glTexCoord2f(0, 0); glVertex3f(-half, half, half);   glTexCoord2f(1, 0); glVertex3f(half, half, half); glTexCoord2f(1, 1); glVertex3f(half, half, -half);   glTexCoord2f(0, 1); glVertex3f(-half, half, -half);
        glTexCoord2f(0, 0); glVertex3f(-half, -half, -half); glTexCoord2f(1, 0); glVertex3f(half, -half, -half); glTexCoord2f(1, 1); glVertex3f(half, -half, half);   glTexCoord2f(0, 1); glVertex3f(-half, -half, half);
        glEnd();
        glEnable(GL_LIGHTING);
        glPopMatrix();
    }

    void drawGround() {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texGround);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glNormal3f(0.0, 1.0, 0.0);
        glTexCoord2f(0.0f, 0.0f);       glVertex3f(-1000.0f, 0.0f, -1000.0f);
        glTexCoord2f(200.0f, 0.0f);     glVertex3f(1000.0f, 0.0f, -1000.0f);
        glTexCoord2f(200.0f, 200.0f);   glVertex3f(1000.0f, 0.0f, 1000.0f);
        glTexCoord2f(0.0f, 200.0f);     glVertex3f(-1000.0f, 0.0f, 1000.0f);
        glEnd();
    }

    void drawBuilding(float x, float z, float w, float h, float d) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texTech);
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(x, h / 2, z);
        glScalef(w, h, d);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    void drawOrb(float x, float z, float scale) {
        if (scale <= 0.0f) return;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texGround);
        glColor3f(0.5f, 1.0f, 1.0f);

        glPushMatrix();
        glTranslatef(x, 2.0f, z);
        static float angle = 0.0f;
        angle += 2.0f;
        glRotatef(angle, 0, 1, 0);
        glScalef(scale, scale, scale);

        GLUquadric* quad = gluNewQuadric();
        gluQuadricTexture(quad, 1);
        gluSphere(quad, 0.8, 20, 20);

        glPopMatrix();
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    void drawBarrier(float x, float z, float angle) {
        glDisable(GL_TEXTURE_2D);
        glPushMatrix();
        glTranslatef(x, 1.5f, z);
        glRotatef(angle, 0, 1, 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 1.0f, 1.0f, 0.6f);
        glScalef(6.0f, 4.0f, 0.2f);
        glutSolidCube(1.0f);
        glDisable(GL_BLEND);
        glColor3f(1.0f, 1.0f, 1.0f);
        glPopMatrix();
    }

    void drawDrone(float x, float z) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texGround);
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(x, 4.0f, z);
        glPushMatrix();
        glScalef(1.5f, 0.5f, 1.5f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.0f, 0.0f);
        glutSolidSphere(0.3, 10, 10);
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    void drawPad() {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texTech);

        if (score >= 100) {
            GLfloat glow[] = { 0.0f, 1.0f, 0.0f, 1.0f };
            glMaterialfv(GL_FRONT, GL_EMISSION, glow);
            glColor3f(0.0f, 1.0f, 0.0f);
        }
        else {
            glColor3f(1.0f, 0.0f, 0.0f);
        }

        glPushMatrix();
        glTranslatef(padX, 0.1f, padZ);
        glScalef(4.0f, 0.2f, 4.0f);
        glutSolidCube(1.0f);
        glPopMatrix();

        GLfloat noGlow[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, noGlow);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    void drawPlayer() {
        glPushMatrix();
        glTranslatef(playerX, playerY, playerZ);
        glRotatef(playerRot + 180.0f, 0, 1, 0);
        glRotatef(-90, 1, 0, 0);
        glScalef(0.075f, 0.075f, 0.075f);
        glColor3f(1.0f, 1.0f, 1.0f);
        playerModel.Draw();
        glPopMatrix();
    }

    // --- DISPLAY LOOP ---
    void Display() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        static float lightPos = 0.0f;
        static float lightDir = 0.5f;
        lightPos += lightDir;
        if (lightPos > 20.0f || lightPos < -20.0f) lightDir *= -1.0f;

        static int frameCount = 0;
        frameCount++;
        GLfloat lightColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        if ((frameCount / 30) % 2 == 0) { lightColor[0] = 1.0f; lightColor[2] = 0.0f; }
        else { lightColor[0] = 0.0f; lightColor[2] = 1.0f; }

        GLfloat light0_pos[] = { lightPos, 10.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

        GLfloat whiteLight[] = { 0.6f, 0.6f, 0.6f, 1.0f };
        GLfloat light1_pos[] = { 0.0f, 50.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, whiteLight);
        glLightfv(GL_LIGHT1, GL_SPECULAR, whiteLight);

        if (cameraMode == 1) {
            float cameraDist = 14.0f;
            float cameraHeight = 5.0f;
            float angleRad = toRad(playerRot);
            float eyeX = playerX - cameraDist * sin(angleRad);
            float eyeZ = playerZ - cameraDist * cos(angleRad);
            gluLookAt(eyeX, cameraHeight + playerY, eyeZ, playerX, 4.0f + playerY, playerZ, 0.0f, 1.0f, 0.0f);
            drawPlayer();
        }
        else {
            float eyeHeight = 2.0f + playerY;
            float angleRad = toRad(playerRot);
            float targetX = playerX + 10.0f * sin(angleRad);
            float targetZ = playerZ + 10.0f * cos(angleRad);
            gluLookAt(playerX, eyeHeight, playerZ, targetX, eyeHeight, targetZ, 0.0f, 1.0f, 0.0f);
        }

        drawSkybox();
        drawGround();

        drawBuilding(-20, -20, 4, 15, 4);
        drawBuilding(20, -30, 8, 10, 8);
        drawBuilding(-30, 10, 5, 20, 5);
        drawBuilding(25, 20, 6, 8, 6);
        drawBuilding(0, -50, 10, 25, 10);

        for (int i = 0; i < 6; i++) drawBarrier(barriers[i].x, barriers[i].z, barriers[i].angle);
        for (int i = 0; i < 3; i++) drawDrone(drones[i].currentX, drones[i].startX);
        drawPad();

        for (int i = 0; i < 10; i++) {
            if (orbs[i].collecting) {
                orbs[i].scale -= 0.05f;
                if (orbs[i].scale <= 0) {
                    orbs[i].active = false;
                    orbs[i].collecting = false;
                    score += 10;
                }
            }
            else if (orbs[i].active && checkCollision(orbs[i].x, orbs[i].z, 3.0)) {
                orbs[i].collecting = true;
                PlaySound(TEXT("collect.wav"), NULL, SND_ASYNC | SND_FILENAME);
            }
            if (orbs[i].active) drawOrb(orbs[i].x, orbs[i].z, orbs[i].scale);
        }

        if (score >= 100) {
            if (checkCollision(padX, padZ, 3.0)) {
                gameWon = true;
            }
        }

        for (int i = 0; i < 3; i++) {
            if (checkCollision(drones[i].currentX, drones[i].startX, 2.0)) {
                if (playerX != 0.0f || playerZ != 15.0f) {
                    PlaySound(TEXT("hit.wav"), NULL, SND_ASYNC | SND_FILENAME);
                    playerX = 0.0f; playerZ = 15.0f; score = 0;
                    for (int j = 0; j < 10; j++) { orbs[j].active = true; orbs[j].scale = 1.0f; }
                }
            }
        }

        if (gameWon) drawText("MISSION ACCOMPLISHED!", width / 2 - 100, height / 2);
        else if (gameOver) drawText("GAME OVER!", width / 2 - 50, height / 2);

        if (score >= 100 && !gameWon) drawText("GO TO THE TELEPORT PAD!", width / 2 - 100, height - 80);

        drawText("Score: " + std::to_string(score) + "/100", 20, height - 40);
        drawText("Time: " + std::to_string((int)gameTime), width - 100, height - 40);

        glutSwapBuffers();
    }

    void Keyboard(unsigned char key, int x, int y) {
        if (gameOver || gameWon) return;

        float d = 1.0f;
        float rotSpeed = 5.0f;
        float newX = playerX;
        float newZ = playerZ;

        switch (key) {
        case 27: exit(0); break;
        case 'w': case 'W':
            newX += d * sin(toRad(playerRot));
            newZ += d * cos(toRad(playerRot));
            break;
        case 's': case 'S':
            newX -= d * sin(toRad(playerRot));
            newZ -= d * cos(toRad(playerRot));
            break;
        case 'a': case 'A': playerRot += rotSpeed; break;
        case 'd': case 'D': playerRot -= rotSpeed; break;
        case '1': cameraMode = 0; break;
        case '3': cameraMode = 1; break;
        }

        if (!checkWall(newX, newZ)) {
            playerX = newX;
            playerZ = newZ;
        }
        glutPostRedisplay();
    }

    void Anim() {
        if (currentLevel != 2) return;

        if (!gameOver && !gameWon) {
            gameTime -= 1.0f / 60.0f;
            if (gameTime < 0) gameTime = 0;
        }

        for (int i = 0; i < 3; i++) {
            drones[i].currentX += drones[i].dir;
            if (drones[i].currentX > drones[i].startX + 10.0f || drones[i].currentX < drones[i].startX - 10.0f) {
                drones[i].dir *= -1.0f;
            }
        }

        if (isJumping) {
            playerY += jumpSpeed;
            jumpSpeed -= 0.02f;
            if (playerY <= 0.0f) {
                playerY = 0.0f;
                isJumping = false;
            }
        }
        glutPostRedisplay();
    }

    void Init() {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
        glEnable(GL_COLOR_MATERIAL);

        loadBMP(&texGround, "metal.bmp", true);
        loadBMP(&texSky, "sky2.bmp", false);
        loadBMP(&texTech, "box.bmp", true);

        // --- REVERTED to OBJ loading ---
        loadBMP(&texRobot, "metal.bmp", true);
        playerModel.Load("robot.obj", texRobot);

        for (int i = 0; i < 3; i++) drones[i].currentX = drones[i].startX;
    }

    void Reshape(int w, int h) {
        if (h == 0) h = 1;
        width = w;
        height = h;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)w / (double)h, 0.1, 4000.0);
        glMatrixMode(GL_MODELVIEW);
    }

}