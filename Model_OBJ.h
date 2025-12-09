#ifndef Model_OBJ_h
#define Model_OBJ_h

#include <vector>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

class Model_OBJ {
public:
    Model_OBJ();
    void Load(const char *filename, GLuint textureID); // Matches .cpp now!
    void Draw();
    
private:
    std::vector<float> vertices;
    std::vector<float> texCoords;
    std::vector<float> normals;
    
    struct Face {
        int vIndex[3];
        int tIndex[3];
        int nIndex[3];
    };
    std::vector<Face> faces;
    GLuint texID;
};
#endif
