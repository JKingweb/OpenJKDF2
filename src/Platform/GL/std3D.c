#include "Platform/std3D.h"

#include "Engine/rdCache.h"
#include "Win95/stdDisplay.h"
#include "Win95/Window.h"
#include "World/sithWorld.h"
#include "Engine/rdColormap.h"
#include "Main/jkGame.h"
#include "World/jkPlayer.h"

#ifdef ARCH_WASM
#include <emscripten.h>
#include <SDL.h>
#include <SDL_opengles2.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "Platform/GL/shader_utils.h"
#ifdef MACOS
#include <SDL.h>
#elif defined(ARCH_WASM)
// wasm
#else
#include <SDL.h>
#include <GL/gl.h>
#endif

#ifdef WIN32
#define GL_R8 GL_RED
#endif

#define TEX_MODE_TEST 0
#define TEX_MODE_WORLDPAL 1
#define TEX_MODE_BILINEAR 2
#define TEX_MODE_16BPP 5
#define TEX_MODE_BILINEAR_16BPP 6

static bool has_initted = false;
static GLuint fb;
static GLuint fbTex;
static GLuint fbRbo;

static GLuint fb1;
static GLuint fbTex1;
static GLuint fbRbo1;

static GLuint fb2;
static GLuint fbTex2;
static GLuint fbRbo2;

static void* last_overlay = NULL;

static int activeFb;

int init_once = 0;
GLuint programDefault, programMenu;
GLint attribute_coord3d, attribute_v_color, attribute_v_uv, attribute_v_norm;
GLint uniform_mvp, uniform_tex, uniform_tex_mode, uniform_blend_mode, uniform_worldPalette;

GLint programMenu_attribute_coord3d, programMenu_attribute_v_color, programMenu_attribute_v_uv, programMenu_attribute_v_norm;
GLint programMenu_uniform_mvp, programMenu_uniform_tex, programMenu_uniform_displayPalette;

GLuint worldpal_texture;
void* worldpal_data;
GLuint displaypal_texture;
void* displaypal_data;

static rdDDrawSurface* std3D_aLoadedSurfaces[1024];
static GLuint std3D_aLoadedTextures[1024];
static size_t std3D_loadedTexturesAmt = 0;
static rdTri GL_tmpTris[STD3D_MAX_TRIS];
static size_t GL_tmpTrisAmt = 0;
static rdLine GL_tmpLines[STD3D_MAX_VERTICES];
static size_t GL_tmpLinesAmt = 0;
static D3DVERTEX GL_tmpVertices[STD3D_MAX_VERTICES];
static size_t GL_tmpVerticesAmt = 0;
static size_t rendered_tris = 0;

rdDDrawSurface* last_tex = NULL;
int last_flags = 0;

#pragma pack(push, 4)
typedef struct std3DWorldVBO
{
    float x;
    float y;
    float z;
    float nx;
    uint32_t color;
    float nz;
    float tu;
    float tv;
} std3DWorldVBO;
#pragma pack(pop)

std3DWorldVBO* world_data_all = NULL;
GLushort* world_data_elements = NULL;
GLuint world_vbo_all;
GLuint world_ibo_triangle;

GLuint menu_vbo_vertices, menu_vbo_colors, menu_vbo_uvs;
GLuint menu_ibo_triangle;

void generateFramebuffer(GLuint* fbOut, GLuint* fbTexOut, GLuint* fbRboOut)
{
    // Generate the framebuffer
    *fbOut = 0;
    glGenFramebuffers(1, fbOut);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbOut);
    
    // Set up our framebuffer texture
    glGenTextures(1, fbTexOut);
    glBindTexture(GL_TEXTURE_2D, *fbTexOut);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glBindTexture(GL_TEXTURE_2D, 0);
    
    // Attach fbTex to our currently bound framebuffer fb
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *fbTexOut, 0); 
    
    // Set up our render buffer
    glGenRenderbuffers(1, fbRboOut);
    glBindRenderbuffer(GL_RENDERBUFFER, *fbRboOut);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 640, 480);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    // Bind it to our framebuffer fb
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, *fbRboOut);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("ERROR: Framebuffer is incomplete!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void deleteFramebuffer(GLuint fbIn, GLuint fbTexIn, GLuint fbRboIn)
{
    glDeleteFramebuffers(1, &fbIn);
    glDeleteTextures(1, &fbTexIn);
    glDeleteRenderbuffers(1, &fbRboIn);
}

void swap_framebuffers()
{
    if (activeFb == 2)
    {
        activeFb = 1;
        fb = fb1;
        fbTex = fbTex1;
        fbRbo = fbRbo1;
    }
    else
    {
        activeFb = 2;
        fb = fb2;
        fbTex = fbTex2;
        fbRbo = fbRbo2;
    }
}

GLuint std3D_loadProgram(const char* fpath_base)
{
    GLuint out;
    GLint link_ok = GL_FALSE;
    
    char* tmp_vert = malloc(strlen(fpath_base) + 32);
    char* tmp_frag = malloc(strlen(fpath_base) + 32);
    
    strcpy(tmp_vert, fpath_base);
    strcat(tmp_vert, "_v.glsl");
    
    strcpy(tmp_frag, fpath_base);
    strcat(tmp_frag, "_f.glsl");
    
    GLuint vs, fs;
    if ((vs = load_shader_file(tmp_vert, GL_VERTEX_SHADER))   == 0) return 0;
    if ((fs = load_shader_file(tmp_frag, GL_FRAGMENT_SHADER)) == 0) return 0;
    
    free(tmp_vert);
    free(tmp_frag);
    
    out = glCreateProgram();
    glAttachShader(out, vs);
    glAttachShader(out, fs);
    glLinkProgram(out);
    glGetProgramiv(out, GL_LINK_STATUS, &link_ok);
    if (!link_ok) 
    {
        print_log(out);
        return 0;
    }
    
    return out;
}

GLint std3D_tryFindAttribute(GLuint program, const char* attribute_name)
{
    GLint out = glGetAttribLocation(program, attribute_name);
    if (out == -1) {
        printf("Could not bind attribute %s!\n", attribute_name);
    }
    return out;
}

GLint std3D_tryFindUniform(GLuint program, const char* uniform_name)
{
    GLint out = glGetUniformLocation(program, uniform_name);
    if (out == -1) {
        printf("Could not bind uniform %s!\n", uniform_name);
    }
    return out;
}

int init_resources()
{
    printf("OpenGL init...\n");
    generateFramebuffer(&fb1, &fbTex1, &fbRbo1);
    generateFramebuffer(&fb2, &fbTex2, &fbRbo2);

    activeFb = 1;
    fb = fb1;
    fbTex = fbTex1;
    fbRbo = fbRbo1;
    
    if ((programDefault = std3D_loadProgram("resource/shaders/default")) == 0) return false;
    if ((programMenu = std3D_loadProgram("resource/shaders/menu")) == 0) return false;
    
    // Attributes/uniforms
    attribute_coord3d = std3D_tryFindAttribute(programDefault, "coord3d");
    attribute_v_color = std3D_tryFindAttribute(programDefault, "v_color");
    attribute_v_uv = std3D_tryFindAttribute(programDefault, "v_uv");
    uniform_mvp = std3D_tryFindUniform(programDefault, "mvp");
    uniform_tex = std3D_tryFindUniform(programDefault, "tex");
    uniform_worldPalette = std3D_tryFindUniform(programDefault, "worldPalette");
    uniform_tex_mode = std3D_tryFindUniform(programDefault, "tex_mode");
    uniform_blend_mode = std3D_tryFindUniform(programDefault, "blend_mode");
    
    programMenu_attribute_coord3d = std3D_tryFindAttribute(programMenu, "coord3d");
    programMenu_attribute_v_color = std3D_tryFindAttribute(programMenu, "v_color");
    programMenu_attribute_v_uv = std3D_tryFindAttribute(programMenu, "v_uv");
    programMenu_uniform_mvp = std3D_tryFindUniform(programMenu, "mvp");
    programMenu_uniform_tex = std3D_tryFindUniform(programMenu, "tex");
    programMenu_uniform_displayPalette = std3D_tryFindUniform(programMenu, "displayPalette");
    
    // World palette
    glGenTextures(1, &worldpal_texture);
    worldpal_data = malloc(0x300);
    memset(worldpal_data, 0xFF, 0x300);
    
    glBindTexture(GL_TEXTURE_2D, worldpal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, worldpal_data);
    
    
    // Display palette
    glGenTextures(1, &displaypal_texture);
    displaypal_data = malloc(0x400);
    memset(displaypal_data, 0xFF, 0x300);
    
    glBindTexture(GL_TEXTURE_2D, displaypal_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, displaypal_data);

    unsigned int vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao ); 

    world_data_all = malloc(STD3D_MAX_VERTICES * sizeof(std3DWorldVBO));
    world_data_elements = malloc(sizeof(GLushort) * 3 * STD3D_MAX_TRIS);

    glGenBuffers(1, &world_vbo_all);
    glGenBuffers(1, &world_ibo_triangle);

    glGenBuffers(1, &menu_vbo_vertices);
    glGenBuffers(1, &menu_vbo_colors);
    glGenBuffers(1, &menu_vbo_uvs);
    glGenBuffers(1, &menu_ibo_triangle);

    has_initted = true;
    return true;
}

int std3D_Startup()
{
    return 1;
}

void std3D_Shutdown()
{

}

void std3D_FreeResources()
{
    glDeleteProgram(programDefault);
    glDeleteProgram(programMenu);
    deleteFramebuffer(fb1, fbTex1, fbRbo1);
    deleteFramebuffer(fb2, fbTex2, fbRbo2);
    glDeleteTextures(1, &worldpal_texture);
    glDeleteTextures(1, &displaypal_texture);
    if (worldpal_data)
        free(worldpal_data);
    if (displaypal_data)
        free(displaypal_data);

    worldpal_data = NULL;
    displaypal_data = NULL;

    if (world_data_all)
        free(world_data_all);
    world_data_all = NULL;

    if (world_data_elements)
        free(world_data_elements);
    world_data_elements = NULL;

    glDeleteBuffers(1, &world_vbo_all);
    glDeleteBuffers(1, &world_ibo_triangle);

    has_initted = false;
}

int std3D_StartScene()
{
    //printf("Begin draw\n");
    if (!has_initted)
    {
        if (!init_resources()) {
            printf("Failed to init resources, exiting...");
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to init resources, exiting...", NULL);
            exit(-1);
        }
    }
    
    rendered_tris = 0;
    
    //glBindFramebuffer(GL_FRAMEBUFFER, idirect3dexecutebuffer->fb);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glCullFace(GL_FRONT);
        
    // Technically this should be from Clear2
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    if (sithWorld_pCurrentWorld && sithWorld_pCurrentWorld->colormaps && memcmp(worldpal_data, sithWorld_pCurrentWorld->colormaps->colors, 0x300))
    {
        glBindTexture(GL_TEXTURE_2D, worldpal_texture);
        memcpy(worldpal_data, sithWorld_pCurrentWorld->colormaps->colors, 0x300);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGB, GL_UNSIGNED_BYTE, worldpal_data);
    }
    
    if (memcmp(displaypal_data, stdDisplay_masterPalette, 0x300))
    {
        glBindTexture(GL_TEXTURE_2D, displaypal_texture);
        memcpy(displaypal_data, stdDisplay_masterPalette, 0x300);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 1, GL_RGB, GL_UNSIGNED_BYTE, displaypal_data);
    }

    // Describe our vertices array to OpenGL (it can't guess its format automatically)
    glBindBuffer(GL_ARRAY_BUFFER, world_vbo_all);
    glVertexAttribPointer(
        attribute_coord3d, // attribute
        3,                 // number of elements per vertex, here (x,y,z)
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // normalize fixed-point data?
        sizeof(std3DWorldVBO),                 // data stride
        (GLvoid*)offsetof(std3DWorldVBO, x)                  // offset of first element
    );
    
    glVertexAttribPointer(
        attribute_v_color, // attribute
        4,                 // number of elements per vertex, here (R,G,B,A)
        GL_UNSIGNED_BYTE,  // the type of each element
        GL_TRUE,          // normalize fixed-point data?
        sizeof(std3DWorldVBO),                 // no extra data between each position
        (GLvoid*)offsetof(std3DWorldVBO, color) // offset of first element
    );

    glVertexAttribPointer(
        attribute_v_uv,    // attribute
        2,                 // number of elements per vertex, here (U,V)
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // take our values as-is
        sizeof(std3DWorldVBO),                 // no extra data between each position
        (GLvoid*)offsetof(std3DWorldVBO, tu)                  // offset of first element
    );

    glEnableVertexAttribArray(attribute_coord3d);
    glEnableVertexAttribArray(attribute_v_color);
    glEnableVertexAttribArray(attribute_v_uv);
    
    return 1;
}

int std3D_EndScene()
{
    glDisableVertexAttribArray(attribute_v_uv);
    glDisableVertexAttribArray(attribute_v_color);
    glDisableVertexAttribArray(attribute_coord3d);

    //printf("End draw\n");
    last_tex = NULL;
    last_flags = 0;
    std3D_ResetRenderList();
    //printf("%u tris\n", rendered_tris);
    return 1;
}

void std3D_ResetRenderList()
{
    rendered_tris += GL_tmpTrisAmt;

    GL_tmpVerticesAmt = 0;
    GL_tmpTrisAmt = 0;
    GL_tmpLinesAmt = 0;
    
    //memset(GL_tmpTris, 0, sizeof(GL_tmpTris));
    //memset(GL_tmpVertices, 0, sizeof(GL_tmpVertices));
}

int std3D_RenderListVerticesFinish()
{
    return 1;
}

void std3D_DrawMenuSubrect(float x, float y, float w, float h, float dstX, float dstY, float scale)
{
    //double tex_w = (double)Window_xSize;
    //double tex_h = (double)Window_ySize;
    double tex_w = Video_menuBuffer.format.width;
    double tex_h = Video_menuBuffer.format.height;

    float w_dst = w;
    float h_dst = h;

    if (scale == 0.0)
    {
        w_dst = (w / tex_w) * (double)Window_xSize;
        h_dst = (h / tex_h) * (double)Window_ySize;

        dstX = (dstX / tex_w) * (double)Window_xSize;
        dstY = (dstY / tex_h) * (double)Window_ySize;

        scale = 1.0;
    }

    double u1 = (x / tex_w);
    double u2 = ((x+w) / tex_w);
    double v1 = (y / tex_h);
    double v2 = ((y+h) / tex_h);

    GL_tmpVertices[GL_tmpVerticesAmt+0].x = dstX;
    GL_tmpVertices[GL_tmpVerticesAmt+0].y = dstY;
    GL_tmpVertices[GL_tmpVerticesAmt+0].z = 0.0;
    GL_tmpVertices[GL_tmpVerticesAmt+0].tu = u1;
    GL_tmpVertices[GL_tmpVerticesAmt+0].tv = v1;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+0].nx = 0;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+0].ny = 0xFFFFFFFF;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+0].nz = 0;
    
    GL_tmpVertices[GL_tmpVerticesAmt+1].x = dstX;
    GL_tmpVertices[GL_tmpVerticesAmt+1].y = dstY + (scale * h_dst);
    GL_tmpVertices[GL_tmpVerticesAmt+1].z = 0.0;
    GL_tmpVertices[GL_tmpVerticesAmt+1].tu = u1;
    GL_tmpVertices[GL_tmpVerticesAmt+1].tv = v2;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+1].nx = 0;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+1].ny = 0xFFFFFFFF;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+1].nz = 0;
    
    GL_tmpVertices[GL_tmpVerticesAmt+2].x = dstX + (scale * w_dst);
    GL_tmpVertices[GL_tmpVerticesAmt+2].y = dstY + (scale * h_dst);
    GL_tmpVertices[GL_tmpVerticesAmt+2].z = 0.0;
    GL_tmpVertices[GL_tmpVerticesAmt+2].tu = u2;
    GL_tmpVertices[GL_tmpVerticesAmt+2].tv = v2;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+2].nx = 0;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+2].ny = 0xFFFFFFFF;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+2].nz = 0;
    
    GL_tmpVertices[GL_tmpVerticesAmt+3].x = dstX + (scale * w_dst);
    GL_tmpVertices[GL_tmpVerticesAmt+3].y = dstY;
    GL_tmpVertices[GL_tmpVerticesAmt+3].z = 0.0;
    GL_tmpVertices[GL_tmpVerticesAmt+3].tu = u2;
    GL_tmpVertices[GL_tmpVerticesAmt+3].tv = v1;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+3].nx = 0;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+3].ny = 0xFFFFFFFF;
    *(uint32_t*)&GL_tmpVertices[GL_tmpVerticesAmt+3].nz = 0;
    
    GL_tmpTris[GL_tmpTrisAmt+0].v1 = GL_tmpVerticesAmt+1;
    GL_tmpTris[GL_tmpTrisAmt+0].v2 = GL_tmpVerticesAmt+0;
    GL_tmpTris[GL_tmpTrisAmt+0].v3 = GL_tmpVerticesAmt+2;
    
    GL_tmpTris[GL_tmpTrisAmt+1].v1 = GL_tmpVerticesAmt+0;
    GL_tmpTris[GL_tmpTrisAmt+1].v2 = GL_tmpVerticesAmt+3;
    GL_tmpTris[GL_tmpTrisAmt+1].v3 = GL_tmpVerticesAmt+2;
    
    GL_tmpVerticesAmt += 4;
    GL_tmpTrisAmt += 2;
}

static rdDDrawSurface* test_idk = NULL;

void std3D_DrawMenu()
{
    glDepthFunc(GL_ALWAYS);
    glUseProgram(programMenu);
    
    float menu_w, menu_h, menu_u, menu_v, menu_x;
    menu_w = (double)Window_xSize;
    menu_h = (double)Window_ySize;
    menu_u = 1.0;
    menu_v = 1.0;
    menu_x = 0.0;
    
    int bFixHudScale = 0;

    if (!jkGame_isDDraw)
    {
        //menu_w = 640.0;
        //menu_h = 480.0;

        // Stretch screen
        menu_u = (1.0 / Video_menuBuffer.format.width) * 640.0;
        menu_v = (1.0 / Video_menuBuffer.format.height) * 480.0;

        // Keep 4:3 aspect
        menu_x = (menu_w - (menu_h * (640.0 / 480.0))) / 2.0;
        menu_w = (menu_h * (640.0 / 480.0));
    }
    else
    {
        bFixHudScale = 1;

        menu_w = Video_menuBuffer.format.width;
        menu_h = Video_menuBuffer.format.height;
    }

    if (!bFixHudScale)
    {
        GL_tmpVertices[0].x = menu_x;
        GL_tmpVertices[0].y = 0.0;
        GL_tmpVertices[0].z = 0.0;
        GL_tmpVertices[0].tu = 0.0;
        GL_tmpVertices[0].tv = 0.0;
        *(uint32_t*)&GL_tmpVertices[0].nx = 0;
        *(uint32_t*)&GL_tmpVertices[0].ny = 0xFFFFFFFF;
        *(uint32_t*)&GL_tmpVertices[0].nz = 0;
        
        GL_tmpVertices[1].x = menu_x;
        GL_tmpVertices[1].y = menu_h;
        GL_tmpVertices[1].z = 0.0;
        GL_tmpVertices[1].tu = 0.0;
        GL_tmpVertices[1].tv = menu_v;
        *(uint32_t*)&GL_tmpVertices[1].nx = 0;
        *(uint32_t*)&GL_tmpVertices[1].ny = 0xFFFFFFFF;
        *(uint32_t*)&GL_tmpVertices[1].nz = 0;
        
        GL_tmpVertices[2].x = menu_x + menu_w;
        GL_tmpVertices[2].y = menu_h;
        GL_tmpVertices[2].z = 0.0;
        GL_tmpVertices[2].tu = menu_u;
        GL_tmpVertices[2].tv = menu_v;
        *(uint32_t*)&GL_tmpVertices[2].nx = 0;
        *(uint32_t*)&GL_tmpVertices[2].ny = 0xFFFFFFFF;
        *(uint32_t*)&GL_tmpVertices[2].nz = 0;
        
        GL_tmpVertices[3].x = menu_x + menu_w;
        GL_tmpVertices[3].y = 0.0;
        GL_tmpVertices[3].z = 0.0;
        GL_tmpVertices[3].tu = menu_u;
        GL_tmpVertices[3].tv = 0.0;
        *(uint32_t*)&GL_tmpVertices[3].nx = 0;
        *(uint32_t*)&GL_tmpVertices[3].ny = 0xFFFFFFFF;
        *(uint32_t*)&GL_tmpVertices[3].nz = 0;
        
        GL_tmpTris[0].v1 = 1;
        GL_tmpTris[0].v2 = 0;
        GL_tmpTris[0].v3 = 2;
        
        GL_tmpTris[1].v1 = 0;
        GL_tmpTris[1].v2 = 3;
        GL_tmpTris[1].v3 = 2;
        
        GL_tmpVerticesAmt = 4;
        GL_tmpTrisAmt = 2;
    }
    else
    {
        GL_tmpVerticesAmt = 0;
        GL_tmpTrisAmt = 0;

        // Main View
        std3D_DrawMenuSubrect(0, 128, menu_w, menu_h-256, 0, 128, 0.0);

        float hudScale = Window_ySize / 480.0;

        /*if (menu_w >= 3600)
            hudScale = 4;
        else if (menu_w >= 1800)
            hudScale = 3;
        else if (menu_w >= 1200)
            hudScale = 2;*/

        // Left and Right HUD
        std3D_DrawMenuSubrect(0, menu_h - 64, 64, 64, 0, Window_ySize - 64*hudScale, hudScale);
        std3D_DrawMenuSubrect(menu_w - 64, menu_h - 64, 64, 64, Window_xSize - 64*hudScale, Window_ySize - 64*hudScale, hudScale);

        // Items
        std3D_DrawMenuSubrect((menu_w / 2) - 128, menu_h - 64, 256, 64, (Window_xSize / 2) - (128*hudScale), Window_ySize - 64*hudScale, hudScale);

        // Text
        std3D_DrawMenuSubrect((menu_w / 2) - 128, 0, 256, 128, (Window_xSize / 2) - (128*hudScale), 0, hudScale);

        // Active forcepowers/items
        std3D_DrawMenuSubrect(menu_w - 64, 0, 64, 128, Window_xSize - (64*hudScale), 0, hudScale);
    }
    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, Video_menuTexId);

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Video_menuBuffer.format.width, Video_menuBuffer.format.height, GL_RED, GL_UNSIGNED_BYTE, Video_menuBuffer.sdlSurface->pixels);

    GLfloat data_vertices[32 * 3];
    GLubyte data_colors[32 * 4];
    GLfloat data_uvs[32 * 2];
    GLushort data_elements[32 * 3];

    // Generate vertices list
    //GLfloat* data_vertices = (GLfloat*)malloc(GL_tmpVerticesAmt * 3 * sizeof(GLfloat));
    //GLfloat* data_colors = (GLfloat*)malloc(GL_tmpVerticesAmt * 4 * sizeof(GLfloat));
    //GLfloat* data_uvs = (GLfloat*)malloc(GL_tmpVerticesAmt * 2 * sizeof(GLfloat));

    D3DVERTEX* vertexes = GL_tmpVertices;

    for (int i = 0; i < GL_tmpVerticesAmt; i++)
    {
        uint32_t v_color = *(uint32_t*)&vertexes[i].ny;
        uint32_t v_unknx = *(uint32_t*)&vertexes[i].nx;
        uint32_t v_unknz = *(uint32_t*)&vertexes[i].nz;
        
        /*printf("%f %f %f, %x %x %x, %f %f\n", vertexes[i].x, vertexes[i].y, vertexes[i].z,
                                              v_unknx, v_color, v_unknz,
                                              vertexes[i].tu, vertexes[i].tv);*/
                                             
        
        uint8_t v_a = (v_color >> 24) & 0xFF;
        uint8_t v_r = (v_color >> 16) & 0xFF;
        uint8_t v_g = (v_color >> 8) & 0xFF;
        uint8_t v_b = v_color & 0xFF;
 
        data_vertices[(i*3)+0] = vertexes[i].x;
        data_vertices[(i*3)+1] = vertexes[i].y;
        data_vertices[(i*3)+2] = vertexes[i].z;
        data_colors[(i*4)+0] = v_r;
        data_colors[(i*4)+1] = v_g;
        data_colors[(i*4)+2] = v_b;
        data_colors[(i*4)+3] = v_a;
        
        data_uvs[(i*2)+0] = vertexes[i].tu;
        data_uvs[(i*2)+1] = vertexes[i].tv;
        
        //printf("nx, ny, nz %x %x %x, %f %f, %f\n", v_unknx, v_color, v_unknz, vertexes[i].nx, vertexes[i].nz, vertexes[i].z);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_vertices);
    glBufferData(GL_ARRAY_BUFFER, GL_tmpVerticesAmt * 3 * sizeof(GLfloat), data_vertices, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_colors);
    glBufferData(GL_ARRAY_BUFFER, GL_tmpVerticesAmt * 4 * sizeof(GLubyte), data_colors, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_uvs);
    glBufferData(GL_ARRAY_BUFFER, GL_tmpVerticesAmt * 2 * sizeof(GLfloat), data_uvs, GL_STREAM_DRAW);
    
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, displaypal_texture);
    glUniform1i(programMenu_uniform_tex, 0);
    glUniform1i(programMenu_uniform_displayPalette, 1);
    
    {

    float maxX, maxY, scaleX, scaleY, width, height;

    scaleX = 1.0/((double)Window_xSize / 2.0);
    scaleY = 1.0/((double)Window_ySize / 2.0);
    maxX = 1.0;
    maxY = 1.0;
    width = Window_xSize;
    height = Window_ySize;
    
    float d3dmat[16] = {
       maxX*scaleX,      0,                                          0,      0, // right
       0,                                       -maxY*scaleY,               0,      0, // up
       0,                                       0,                                          1,     0, // forward
       -(width/2)*scaleX,  (height/2)*scaleY,     -1,      1  // pos
    };
    
    glUniformMatrix4fv(programMenu_uniform_mvp, 1, GL_FALSE, d3dmat);
    glViewport(0, 0, width, height);

    }
    
    rdTri* tris = GL_tmpTris;
    glEnableVertexAttribArray(programMenu_attribute_coord3d);
    glEnableVertexAttribArray(programMenu_attribute_v_color);
    glEnableVertexAttribArray(programMenu_attribute_v_uv);
    
    // Describe our vertices array to OpenGL (it can't guess its format automatically)
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_vertices);
    glVertexAttribPointer(
        programMenu_attribute_coord3d, // attribute
        3,                 // number of elements per vertex, here (x,y,z)
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // take our values as-is
        0,                 // no extra data between each position
        0                  // offset of first element
    );
    
    
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_colors);
    glVertexAttribPointer(
        programMenu_attribute_v_color, // attribute
        4,                 // number of elements per vertex, here (R,G,B,A)
        GL_UNSIGNED_BYTE,          // the type of each element
        GL_TRUE,          // take our values as-is
        0,                 // no extra data between each position
        0                  // offset of first element
    );
    
    
    glBindBuffer(GL_ARRAY_BUFFER, menu_vbo_uvs);
    glVertexAttribPointer(
        programMenu_attribute_v_uv,    // attribute
        2,                 // number of elements per vertex, here (U,V)
        GL_FLOAT,          // the type of each element
        GL_FALSE,          // take our values as-is
        0,                 // no extra data between each position
        0                  // offset of first element
    );
    
    rdDDrawSurface* last_tex = (void*)-1;
    int last_tex_idx = 0;
    //GLushort* data_elements = malloc(sizeof(GLushort) * 3 * GL_tmpTrisAmt);
    for (int j = 0; j < GL_tmpTrisAmt; j++)
    {
        data_elements[(j*3)+0] = tris[j].v1;
        data_elements[(j*3)+1] = tris[j].v2;
        data_elements[(j*3)+2] = tris[j].v3;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, menu_ibo_triangle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GL_tmpTrisAmt * 3 * sizeof(GLushort), data_elements, GL_STREAM_DRAW);

    int tris_size;  
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &tris_size);
    glDrawElements(GL_TRIANGLES, tris_size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(programMenu_attribute_v_uv);
    glDisableVertexAttribArray(programMenu_attribute_v_color);
    glDisableVertexAttribArray(programMenu_attribute_coord3d);
    
    //free(data_elements);
        
    //glBindTexture(GL_TEXTURE_2D, 0);
}

void std3D_DrawRenderList()
{
    glUseProgram(programDefault);
    
    last_tex = NULL;

    // Generate vertices list
    D3DVERTEX* vertexes = GL_tmpVertices;

    float maxX, maxY, scaleX, scaleY, width, height;

    float internalWidth = Video_menuBuffer.format.width;
    float internalHeight = Video_menuBuffer.format.height;

    maxX = 1.0;
    maxY = 1.0;
    scaleX = 1.0/((double)internalWidth / 2.0);
    scaleY = 1.0/((double)internalHeight / 2.0);
    width = Window_xSize;
    height = Window_ySize;

    // JKDF2's vertical FOV is fixed with their projection, for whatever reason. 
    // This ends up resulting in the view looking squished vertically at wide/ultrawide aspect ratios.
    // To compensate, we zoom the y axis here.
    // I also went ahead and fixed vertical displays in the same way because it seems to look better.
    float zoom_yaspect = (width/height);
    float zoom_xaspect = (height/width);

    if (height > width)
    {
        zoom_yaspect = 1.0;
    }

    if (width > height)
    {
        zoom_xaspect = 1.0;
    }

    glBindBuffer(GL_ARRAY_BUFFER, world_vbo_all);
    glBufferData(GL_ARRAY_BUFFER, GL_tmpVerticesAmt * sizeof(std3DWorldVBO), vertexes, GL_STREAM_DRAW);
    
    glUniform1i(uniform_tex_mode, TEX_MODE_TEST);
    glUniform1i(uniform_blend_mode, 2);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, worldpal_texture);
    glActiveTexture(GL_TEXTURE0 + 0);
    
    glUniform1i(uniform_tex, 0);
    glUniform1i(uniform_worldPalette, 1);
    
    {
    
    float d3dmat[16] = {
       maxX*scaleX*zoom_xaspect,      0,                                          0,      0, // right
       0,                                       -maxY*scaleY*zoom_yaspect,               0,      0, // up
       0,                                       0,                                          1,     0, // forward
       -(internalWidth/2)*scaleX*zoom_xaspect,  (internalHeight/2)*scaleY*zoom_yaspect,     (!rdCamera_pCurCamera || rdCamera_pCurCamera->projectType == rdCameraProjectType_Perspective) ? -1 : 1,      1  // pos
    };
    
    glUniformMatrix4fv(uniform_mvp, 1, GL_FALSE, d3dmat);
    glViewport(0, 0, width, height);
    
    }
    
    rdTri* tris = GL_tmpTris;
    rdLine* lines = GL_tmpLines;
    
    //glEnableVertexAttribArray(attribute_v_norm);

    

    int last_tex_idx = 0;
    //GLushort* world_data_elements = malloc(sizeof(GLushort) * 3 * GL_tmpTrisAmt);
    for (int j = 0; j < GL_tmpTrisAmt; j++)
    {
        world_data_elements[(j*3)+0] = tris[j].v1;
        world_data_elements[(j*3)+1] = tris[j].v2;
        world_data_elements[(j*3)+2] = tris[j].v3;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world_ibo_triangle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GL_tmpTrisAmt * 3 * sizeof(GLushort), world_data_elements, GL_STREAM_DRAW);
    
    int do_batch = 0;
    
    //glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glCullFace(GL_FRONT);

    if (!(tris[0].flags & 0x800)) {
        //glDepthFunc(GL_ALWAYS);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    
    for (int j = 0; j < GL_tmpTrisAmt; j++)
    {
        if (tris[j].texture != last_tex || tris[j].flags != last_flags)
        {
            do_batch = 1;
        }
        
        if (do_batch)
        {
            int num_tris_batch = j - last_tex_idx;
            rdDDrawSurface* tex = tris[j].texture;
            
            test_idk = tex;

            if (num_tris_batch)
            {
                //printf("batch %u~%u\n", last_tex_idx, j);
                glDrawElements(GL_TRIANGLES, num_tris_batch * 3, GL_UNSIGNED_SHORT, (GLvoid*)((intptr_t)&world_data_elements[last_tex_idx * 3] - (intptr_t)&world_data_elements[0]));
            }

            if (tex)
            {
                int tex_id = tex->texture_id;
                glActiveTexture(GL_TEXTURE0 + 0);
                if (tex_id == 0)
                    glBindTexture(GL_TEXTURE_2D, worldpal_texture);
                else
                    glBindTexture(GL_TEXTURE_2D, tex_id);

                if (!jkPlayer_enableTextureFilter)
                    glUniform1i(uniform_tex_mode, tex->is_16bit ? TEX_MODE_16BPP : TEX_MODE_WORLDPAL);
                else
                    glUniform1i(uniform_tex_mode, tex->is_16bit ? TEX_MODE_BILINEAR_16BPP : TEX_MODE_BILINEAR);
                
                if (jkPlayer_enableTextureFilter && tex->is_16bit)
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                }
                else
                {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }

                if (tex_id == 0)
                    glUniform1i(uniform_tex_mode, TEX_MODE_TEST);
            }
            else
            {
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_2D, worldpal_texture);
                glUniform1i(uniform_tex_mode, TEX_MODE_TEST);
            }
            
            int changed_flags = (last_flags ^ tris[j].flags);

            if (changed_flags & 0x600)
            {
                if (tris[j].flags & 0x600)
                    glUniform1i(uniform_blend_mode, 5);
                else
                    glUniform1i(uniform_blend_mode, 2);
            }
            
            if (changed_flags & 0x1800)
            {
                if (tris[j].flags & 0x800)
                {
                    glDepthFunc(GL_LESS);
                }
                else
                {
                    //glDepthFunc(GL_ALWAYS);
                    glClear(GL_DEPTH_BUFFER_BIT);
                }
                
                if (changed_flags & 0x1000)
                {
                    glDepthMask(GL_TRUE);
                }
                else
                {
                    //glDepthMask(GL_FALSE);
                }
            }

            if (changed_flags & 0x10000)
            {
                if (tris[j].flags & 0x10000) {
                    glCullFace(GL_BACK);
                }
                else
                {
                    glCullFace(GL_FRONT);
                }
            }
            
            last_tex = tris[j].texture;
            last_flags = tris[j].flags;
            last_tex_idx = j;

            do_batch = 0;
        }
        //printf("tri %u: %u,%u,%u, flags %x\n", j, tris[j].v1, tris[j].v2, tris[j].v3, tris[j].flags);
        
        
        /*int vert = tris[j].v1;
        printf("%u: %f %f %f, %f %f %f, %f %f\n", vert, vertexes[vert].x, vertexes[vert].y, vertexes[vert].z,
                                      vertexes[vert].nx, vertexes[vert].ny, vertexes[vert].nz,
                                      vertexes[vert].tu, vertexes[vert].tv);
        
        vert = tris[j].v2;
        printf("%u: %f %f %f, %f %f %f, %f %f\n", vert, vertexes[vert].x, vertexes[vert].y, vertexes[vert].z,
                                      vertexes[vert].nx, vertexes[vert].ny, vertexes[vert].nz,
                                      vertexes[vert].tu, vertexes[vert].tv);
        
        vert = tris[j].v3;
        printf("%u: %f %f %f, %f %f %f, %f %f\n", vert, vertexes[vert].x, vertexes[vert].y, vertexes[vert].z,
                                      vertexes[vert].nx, vertexes[vert].ny, vertexes[vert].nz,
                                      vertexes[vert].tu, vertexes[vert].tv);*/
    }
    
    int remaining_batch = GL_tmpTrisAmt - last_tex_idx;

    if (remaining_batch)
    {
        glDrawElements(GL_TRIANGLES, remaining_batch * 3, GL_UNSIGNED_SHORT, (GLvoid*)((intptr_t)&world_data_elements[last_tex_idx * 3] - (intptr_t)&world_data_elements[0]));
    }
    
    
#if 0
    // Draw all lines
    world_data_elements = malloc(sizeof(GLushort) * 2 * GL_tmpLinesAmt);
    for (int j = 0; j < GL_tmpLinesAmt; j++)
    {
        world_data_elements[(j*2)+0] = lines[j].v1;
        world_data_elements[(j*2)+1] = lines[j].v2;
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, world_ibo_triangle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, GL_tmpLinesAmt * 2 * sizeof(GLushort), world_data_elements, GL_STREAM_DRAW);

    int lines_size;
    glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &lines_size);
    glDrawElements(GL_LINES, lines_size / sizeof(GLushort), GL_UNSIGNED_SHORT, 0);
#endif
        
    // Done drawing    
    glBindTexture(GL_TEXTURE_2D, worldpal_texture);
    glCullFace(GL_FRONT);
    
    std3D_ResetRenderList();
}

int std3D_SetCurrentPalette(rdColor24 *a1, int a2)
{
    return 1;
}

void std3D_GetValidDimension(unsigned int inW, unsigned int inH, unsigned int *outW, unsigned int *outH)
{
    // TODO hack for JKE? I don't know what they're doing
    *outW = inW > 256 ? 256 : inW;
    *outH = inH > 256 ? 256 : inH;
}

int std3D_DrawOverlay()
{
    return 1;
}

void std3D_UnloadAllTextures()
{
    glDeleteTextures(std3D_loadedTexturesAmt, std3D_aLoadedTextures);
    std3D_loadedTexturesAmt = 0;
}

void std3D_AddRenderListTris(rdTri *tris, unsigned int num_tris)
{
    if (GL_tmpTrisAmt + num_tris > STD3D_MAX_TRIS)
    {
        return;
    }
    
    memcpy(&GL_tmpTris[GL_tmpTrisAmt], tris, sizeof(rdTri) * num_tris);
    
    GL_tmpTrisAmt += num_tris;
}

void std3D_AddRenderListLines(rdLine* lines, uint32_t num_lines)
{
    if (GL_tmpLinesAmt + num_lines > STD3D_MAX_VERTICES)
    {
        return;
    }
    
    memcpy(&GL_tmpLines[GL_tmpLinesAmt], lines, sizeof(rdLine) * num_lines);
    GL_tmpLinesAmt += num_lines;
}

int std3D_AddRenderListVertices(D3DVERTEX *vertices, int count)
{
    if (GL_tmpVerticesAmt + count >= STD3D_MAX_VERTICES)
    {
        return 0;
    }
    
    memcpy(&GL_tmpVertices[GL_tmpVerticesAmt], vertices, sizeof(D3DVERTEX) * count);
    
    GL_tmpVerticesAmt += count;
    
    return 1;
}

int std3D_ClearZBuffer()
{
    return 1;
}

int std3D_AddToTextureCache(stdVBuffer *vbuf, rdDDrawSurface *texture, int is_alpha_tex, int no_alpha)
{
    //printf("Add to texture cache\n");
    
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    uint8_t* image_8bpp = vbuf->sdlSurface->pixels;
    uint16_t* image_16bpp = vbuf->sdlSurface->pixels;
    uint8_t* pal = vbuf->palette;
    
    uint32_t width, height;
    width = vbuf->format.width;
    height = vbuf->format.height;

    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    if (vbuf->format.format.is16bit)
    {
        texture->is_16bit = 1;
        if (!is_alpha_tex)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0,  GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, image_8bpp);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,  GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, image_8bpp);
    }
    else
    {
        texture->is_16bit = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_8bpp);
    }

#if 0    
    void* image_data = malloc(width*height*4);
    
    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            uint32_t index = (i*height) + j;
            uint32_t val_rgba = 0xFF000000;
            
            if (pal)
            {
                uint8_t val = image_8bpp[index];
                val_rgba |= (pal[(val * 3) + 2] << 16);
                val_rgba |= (pal[(val * 3) + 1] << 8);
                val_rgba |= (pal[(val * 3) + 0] << 0);
            }
            else
            {
                uint8_t val = image_8bpp[index];
                rdColor24* pal_master = (rdColor24*)sithWorld_pCurrentWorld->colormaps->colors;//stdDisplay_gammaPalette;
                rdColor24* color = &pal_master[val];
                val_rgba |= (color->r << 16);
                val_rgba |= (color->g << 8);
                val_rgba |= (color->b << 0);
            }
            
            *(uint32_t*)(image_data + index*4) = val_rgba;
        }
        
        
    }
    
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
#endif

    
    
    
    std3D_aLoadedSurfaces[std3D_loadedTexturesAmt] = texture;
    std3D_aLoadedTextures[std3D_loadedTexturesAmt++] = image_texture;
    /*ext->surfacebuf = image_data;
    ext->surfacetex = image_texture;
    ext->surfacepaltex = pal_texture;*/
    
    texture->texture_id = image_texture;
    texture->texture_loaded = 1;
    
    return 1;
}

void std3D_UpdateFrameCount(rdDDrawSurface *surface)
{
}

// Added helpers
int std3D_HasAlpha()
{
    return 1;
}

int std3D_HasModulateAlpha()
{
    return 1;
}

int std3D_HasAlphaFlatStippled()
{
    return 1;
}

void std3D_PurgeTextureCache()
{
    for (int i = 0; i < 1024; i++)
    {
        rdDDrawSurface* tex = std3D_aLoadedSurfaces[i];
        if (!tex) continue;

        if (std3D_aLoadedTextures[i])
            glDeleteTextures(1, &std3D_aLoadedTextures[i]);

        std3D_aLoadedTextures[i] = 0;

        tex->texture_loaded = 0;
        tex->texture_id = 0;

        std3D_aLoadedSurfaces[i] = NULL;
    }
    std3D_loadedTexturesAmt = 0;
}
