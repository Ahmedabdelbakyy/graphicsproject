#define GL_SILENCE_DEPRECATION
#include "TextureBuilder.h"
#include <cstdio>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

static void ensureAndBind(GLuint* id){
    if(*id == 0) glGenTextures(1,id);
    glBindTexture(GL_TEXTURE_2D, *id);
}
static void setParams(bool wrap){
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
static inline unsigned rd16(const unsigned char* p){ return p[0] | (p[1]<<8); }
static inline unsigned rd32(const unsigned char* p){ return p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); }

void loadBMP(GLuint* textureID, const char* filename, bool wrap){
    FILE* f = std::fopen(filename, "rb");
    if(!f) throw std::runtime_error(std::string("Cannot open BMP: ")+filename);

    unsigned char fileHdr[14], infoHdr[40];
    if(std::fread(fileHdr,1,14,f)!=14 || fileHdr[0]!='B' || fileHdr[1]!='M'){
        std::fclose(f); throw std::runtime_error("Not a BMP file");
    }
    if(std::fread(infoHdr,1,40,f)!=40){
        std::fclose(f); throw std::runtime_error("Bad DIB header");
    }

    int width   = (int)rd32(&infoHdr[4]);
    int height  = (int)((int)rd32(&infoHdr[8])); // negative => top-down
    int planes  = (int)rd16(&infoHdr[12]);
    int bpp     = (int)rd16(&infoHdr[14]);
    unsigned comp = rd32(&infoHdr[16]);
    if(planes!=1 || (bpp!=24 && bpp!=32) || comp!=0){
        std::fclose(f); throw std::runtime_error("Unsupported BMP (need uncompressed 24/32 bpp)");
    }

    unsigned offBits = rd32(&fileHdr[10]);
    std::fseek(f,(long)offBits,SEEK_SET);

    bool topDown = (height < 0);
    int h = std::abs(height);
    int bytesPerPixel = bpp/8;
    size_t fileStride = ((width*bytesPerPixel + 3)/4)*4;

    std::vector<unsigned char> row(fileStride);
    std::vector<unsigned char> pixels(width*h*bytesPerPixel);

    for(int y=0;y<h;++y){
        if(std::fread(row.data(),1,fileStride,f)!=fileStride){
            std::fclose(f); throw std::runtime_error("Unexpected EOF in BMP");
        }
        int dstY = topDown ? y : (h-1-y);
        unsigned char* dst = &pixels[dstY*width*bytesPerPixel];
        for(int x=0;x<width;++x){
            unsigned char b=row[x*bytesPerPixel+0];
            unsigned char g=row[x*bytesPerPixel+1];
            unsigned char r=row[x*bytesPerPixel+2];
            dst[x*bytesPerPixel+0]=r;
            dst[x*bytesPerPixel+1]=g;
            dst[x*bytesPerPixel+2]=b;
            if(bytesPerPixel==4) dst[x*bytesPerPixel+3]=row[x*bytesPerPixel+3];
        }
    }
    std::fclose(f);

    ensureAndBind(textureID);
    GLenum fmt = (bytesPerPixel==4)?GL_RGBA:GL_RGB;
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, width, h, fmt, GL_UNSIGNED_BYTE, pixels.data());
    setParams(wrap);
}

