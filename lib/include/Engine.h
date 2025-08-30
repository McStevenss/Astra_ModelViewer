#pragma once
#include <SDL2/SDL.h>
// #include <GL/glew.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>
#include <memory>
#include <algorithm>
#include <Shader.hpp>
#include "Camera.hpp"
//ImGui + SDL
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_opengl3.h"
#include "Grid.h"
#include "Model.hpp"
#include "Mesh.hpp"
#include "CustomFileHeaders.h"
#include <unordered_map>
class Engine {

    public:
        Engine();

        
        void Initialize();
        void Start();
        
    private:
        
        Camera cam;
        void HandleInput(float dt);
        void CreateFrameBuffer();
        void RescaleFramebuffer(float width, float height);
        void BindFramebuffer();
        void UnbindFramebuffer();
        float GetDeltaTime();
        void LoadNewModel(string const &path, bool flipUvs);
        // ImVec2 RenderGUI(Model& model);
        ImVec2 RenderGUI();

        void SaveModelBinary(const Model& model, const std::string& path);
        Model* LoadModelBinary(const std::string& path);

        Shader* heightMapShader;
        Shader* heightMapColorShader;

        int ScreenWidth=1920;
        int ScreenHeight=1080;
        SDL_Window* win;
        SDL_GLContext glctx;

        GLuint ringVBO=0;
        GLuint ringVAO=0;
        glm::vec3 playerPos;

        // std::unique_ptr<Model> model;  // starts as nullptr

        Model *model;


        glm::vec3 lightPosition = glm::vec3(0.0f,0.0f,0.0f);
        float lightOrbitRadius = 10;
        float lightYaw = 0;
        float lightHeight = 25;

        std::vector<glm::vec3> ringVerts;

        glm::vec3 targetpos = glm::vec3(0.0f);

        bool running=true;
        bool editMode=false;
        float aspect=ScreenWidth/ScreenHeight;
        bool wire=false;
        bool rmb=false; 
        bool lmb=false; 
        bool mmb=false; 
        bool shift=false;
        bool flatshade=false;
        bool projectCircle=true;
        float EditorWindowWidth;
        float EditorWindowHeight;
        bool flipTexture = false;

        int mx=0,my=0;
        // ------------ Config ------------
        const int   GRID_SIZE   = 256;          // 128x128 height samples
        const float TILE_SIZE   = 533.333f;     // WoW ADT ~533.333m, optional
        const float CELL_SIZE   = TILE_SIZE / (GRID_SIZE - 1);

        GLuint FBO = 0;
        GLuint texture_id = 0;
        GLuint RBO = 0;

};

