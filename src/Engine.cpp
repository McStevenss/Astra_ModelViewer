#include "Engine.h"

# define M_PI           3.14159265358979323846  /* pi */
Engine::Engine()
{
    // cam = Camera(60.0f,0.1f,1000.0f);
    cam = Camera(60.0f,0.1f,500.0f);
    cam.yaw = 1.6f;
    cam.pitch = 0.4f;
    cam.targetPos = &targetpos;

    Initialize();

    CreateFrameBuffer();
}

void Engine::Initialize()
{
    // SDL init
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    win = SDL_CreateWindow("Astra ModelViewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    glctx = SDL_GL_CreateContext(win);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // ImGui style

    //DarkMode
    ImGui::StyleColorsDark();
    //LightMode
    // ImGui::StyleColorsLight();

     // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(win, glctx);
    ImGui_ImplOpenGL3_Init("#version 410");
    SDL_GL_SetSwapInterval(1);
    
    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);

}

float Engine::GetDeltaTime()
{
    static uint64_t lastTime = SDL_GetTicks64();
    uint64_t currentTime = SDL_GetTicks64();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    return deltaTime;
}

void Engine::LoadNewModel(string const &path, bool flipUvs)
{
    // Clean up the old model if it exists
    if (model)
    {
        delete model;   // calls Model::~Model(), freeing textures/meshes
        model = nullptr;
    }

     // Allocate new model
    model = new Model(path, false, flipUvs);

}

void Engine::Start()
{
    uint32_t prevTicks = SDL_GetTicks();
    stbi_set_flip_vertically_on_load(false);
    
    const int   GRID_SIZE   = 256;          // 128x128 height samples
    const float TILE_SIZE   = 533.333f;     // WoW ADT ~533.333m, optional
    const float CELL_SIZE   = TILE_SIZE / (GRID_SIZE - 1);
    
    Grid grid(20, CELL_SIZE); // 20x20 grid, spacing = 1.0

    Shader gridShader("shaders/grid.vs","shaders/grid.fs"); 
    Shader modelShader("shaders/model.vs","shaders/model.fs");

    // model = new Model("models/UE_Hero_Male/Superhero_Male.gltf",false,true);
    model = new Model("models/backpack/backpack.obj",false,false);

    while(running)
    {
        float dt = GetDeltaTime();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        // ImVec2 imgPos = RenderGUI(*model);
        ImVec2 imgPos = RenderGUI();
        ImGui::Render();
        
        BindFramebuffer();

        // --- Picking ---
        SDL_GetWindowSize(win,&ScreenWidth,&ScreenHeight);
        
        float localX = mx - imgPos.x;
        float localY = my - imgPos.y;
        bool insideImage = (localX >= 0 && localX <= EditorWindowWidth && localY >= 0 && localY <= EditorWindowHeight);
        
        targetpos.y = model->aabbMax.y/2.0f;
        // targetpos.y = 0;

        cam.Update(dt);
        glm::mat4 View = cam.view();
        glm::mat4 Projection = cam.proj(EditorWindowWidth/(float)EditorWindowHeight);
        glm::mat4 VP = Projection*View; 

        // --- Render ---
        glClearColor(0.32f,0.55f,0.75f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        gridShader.use();
        gridShader.setMat4("uVP", VP);
        grid.Render(gridShader);

        lightPosition.x = targetpos.x + lightOrbitRadius * cosf(lightYaw);
        lightPosition.z = targetpos.z + lightOrbitRadius * sinf(lightYaw);
        lightPosition.y = lightHeight;
        
        model->UpdateModelMatrix();
        modelShader.use();
            modelShader.setMat4("view",View);
            modelShader.setMat4("projection",Projection);
            modelShader.setMat4("model",model->ModelMatrix);
            modelShader.setVec3("viewPos",cam.position());
            modelShader.setVec3("lightPos",lightPosition);
        
        model->Draw(modelShader);

        HandleInput(dt);

        UnbindFramebuffer();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    delete model;
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(win);

}

void Engine::HandleInput(float dt)
{
    // int mx=0,my=0;
    SDL_GetMouseState(&mx,&my);
    SDL_Event e; 
    while(SDL_PollEvent(&e))
    {
        ImGui_ImplSDL2_ProcessEvent(&e);
        if(e.type==SDL_QUIT) running=false;
        if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){ cam.recalculateViewport(e); }
        if(e.type==SDL_MOUSEBUTTONDOWN){ if(e.button.button==SDL_BUTTON_RIGHT) rmb=true; if(e.button.button==SDL_BUTTON_LEFT) lmb=true; if(e.button.button==SDL_BUTTON_MIDDLE) mmb=true; }
        if(e.type==SDL_MOUSEBUTTONUP){ if(e.button.button==SDL_BUTTON_RIGHT) rmb=false; if(e.button.button==SDL_BUTTON_LEFT) lmb=false; if(e.button.button==SDL_BUTTON_MIDDLE) mmb=false; }

        if(e.type==SDL_KEYDOWN){
            if(e.key.keysym.sym==SDLK_ESCAPE) running=false;
        }     
        if(e.type==SDL_MOUSEWHEEL)
        {
            cam.Zoom(e,0.2f);
        }
        cam.HandleInput(e,mx,my);

        if(rmb){
            SDL_SetRelativeMouseMode(SDL_TRUE);
            // SDL_WarpMouseInWindow(win, ScreenWidth/2,ScreenHeight/2);
            // SDL_WarpMouseInWindow(win, ScreenWidth/2,ScreenHeight/2);
        }
        if(!rmb){
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }


    }
}

void Engine::CreateFrameBuffer()
{
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ScreenWidth, ScreenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ScreenWidth, ScreenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Engine::RescaleFramebuffer(float width, float height)
{
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);
}

void Engine::BindFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
}

// here we unbind our framebuffer
void Engine::UnbindFramebuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ImVec2 Engine::RenderGUI()
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ScreenWidth*0.8f, ScreenHeight), ImGuiCond_Always);
    ImGui::Begin("Editor",
                 nullptr,
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoTitleBar);

    EditorWindowWidth = ImGui::GetContentRegionAvail().x;
    EditorWindowHeight = ImGui::GetContentRegionAvail().y;

    // rescale framebuffer
    RescaleFramebuffer(EditorWindowWidth, EditorWindowHeight);
    glViewport(0, 0, EditorWindowWidth, EditorWindowHeight);

    // get correct image position **inside the window**
    ImVec2 imgPos = ImGui::GetCursorScreenPos();

    // add image
    ImGui::GetWindowDrawList()->AddImage(
        (ImTextureID)(intptr_t)texture_id,
        imgPos,
        ImVec2(imgPos.x + EditorWindowWidth, imgPos.y + EditorWindowHeight),
        ImVec2(0,1),
        ImVec2(1,0)
    );

    ImGui::End();


        // --- Settings Window (20%) ---
    ImGui::SetNextWindowPos(ImVec2(ScreenWidth * 0.8f, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ScreenWidth * 0.2f, ScreenHeight), ImGuiCond_Always);
    ImGui::Begin("Settings",
                 nullptr,
                 ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse);

    //--------------------------------------------------------------------
    ImGui::SeparatorText("Camera");
    ImGui::Text("Yaw: %.1f", cam.yaw);
    ImGui::Text("Pitch: %.1f", cam.pitch);
    ImGui::Text("Distance: %.1f", cam.distance);
    ImGui::Text("Target Position: %.1f, %.1f, %.1f", targetpos.x,targetpos.y,targetpos.z);
    ImGui::Text("AABB MAX-y: %.5f", model->aabbMax.y);
    ImGui::Text("AABB MIN-y: %.5f", model->aabbMin.y);
    
    
    ImGui::SeparatorText("Model");
    ImGui::DragFloat3("Scale", &model->Scale.x, 1.0f, 0.01, 10.0f, "%.2f");
    ImGui::SeparatorText("Light");
    ImGui::SliderAngle("Orbit", &lightYaw, 0.0f, 360.0f, "%.0f°");
    ImGui::SliderFloat("Height", &lightHeight, 0.00, 25.0f, "%.2f");

    static char filepath[256] = ""; // buffer for input

    ImGui::SeparatorText("Model Loader");

    // Input field for file path
    ImGui::InputText("Filepath", filepath, IM_ARRAYSIZE(filepath));
     
    ImGui::Checkbox("Flip texture: ", &flipTexture);
    // Submit button
    if (ImGui::Button("Load Model"))
    {
        // Call your function with filepath
        std::string path(filepath);
        LoadNewModel(path, flipTexture);   // <-- replace with your own function
        // or just store it: model.currentFilepath = path;
    }


    ImGui::SeparatorText("Model IO");
    if (ImGui::Button("Save Model"))
    {
        if(model)
        {
            SaveModelBinary(*model,"SavedModel.modl");
        }
    }

    static char binaryFilepath[256] = ""; // buffer for input
    ImGui::InputText("Binary Filepath", binaryFilepath, IM_ARRAYSIZE(filepath));
    if (ImGui::Button("Load Binary Model"))
    {
        Model* binaryModel = LoadModelBinary(binaryFilepath);

        if (&model)
        {
            delete model;   // calls Model::~Model(), freeing textures/meshes
            model = nullptr;
        }

     // Allocate new model
        model = binaryModel;
    }
    
    
    ImGui::End();

    return imgPos;
}

// void Engine::SaveModelBinary(const Model& model, const std::string& path)
// {
//     std::ofstream out(path, std::ios::binary);
//     if (!out.is_open()) return;

//     ModelHeader header;
//     header.numMeshes = model.meshes.size();
//     header.hasDiffuse = model.hasDiffuse;
//     header.hasSpecular = model.hasSpecular;
//     header.hasNormal = model.hasNormal;
//     out.write(reinterpret_cast<char*>(&header), sizeof(header));

//     for (const auto& mesh : model.meshes) {
//         MeshHeader meshHeader;
//         meshHeader.numVertices = mesh.vertices.size();
//         meshHeader.numIndices = mesh.indices.size();
//         meshHeader.numTextures = mesh.textures.size();
//         meshHeader.aabbMin = mesh.aabbMin;
//         meshHeader.aabbMax = mesh.aabbMax;

//         out.write(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));

//         // Write vertices
//         for (const auto& v : mesh.vertices) {
//             VertexBinary vb;
//             vb.position = v.Position;
//             vb.normal = v.Normal;
//             vb.texCoords = v.TexCoords;
//             vb.tangent = v.Tangent;
//             vb.bitangent = v.Bitangent;
//             out.write(reinterpret_cast<char*>(&vb), sizeof(vb));
//         }

//         // Write indices
//         out.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        
//         // model.directory
//         // For simplicity, we can save texture paths as fixed-length strings (e.g., 256 chars)
//         for (const auto& tex : mesh.textures) {
            
//             char buffer[256] = {};
//             std::string fullPath = model.directory + "/" + tex.path; 
//             strncpy(buffer, fullPath.c_str(), 255);
//             // strncpy(buffer, tex.path.c_str(), 255);
//             out.write(buffer, 256);
//         }
//     }

//     out.close();
// }

void Engine::SaveModelBinary(const Model& model, const std::string& path)
{
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return;

    // Write model header
    ModelHeader header;
    header.numMeshes = model.meshes.size();
    header.hasDiffuse = model.hasDiffuse;
    header.hasSpecular = model.hasSpecular;
    header.hasNormal = model.hasNormal;
    out.write(reinterpret_cast<char*>(&header), sizeof(header));

    for (const auto& mesh : model.meshes) {
        // Write mesh header
        MeshHeader meshHeader;
        meshHeader.numVertices = mesh.vertices.size();
        meshHeader.numIndices = mesh.indices.size();
        meshHeader.numTextures = mesh.textures.size();
        meshHeader.aabbMin = mesh.aabbMin;
        meshHeader.aabbMax = mesh.aabbMax;
        out.write(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));

        // Write vertices (field by field to avoid padding issues)
        for (const auto& v : mesh.vertices) {
            out.write(reinterpret_cast<const char*>(&v.Position), sizeof(glm::vec3));
            out.write(reinterpret_cast<const char*>(&v.Normal), sizeof(glm::vec3));
            out.write(reinterpret_cast<const char*>(&v.TexCoords), sizeof(glm::vec2));
            out.write(reinterpret_cast<const char*>(&v.Tangent), sizeof(glm::vec3));
            out.write(reinterpret_cast<const char*>(&v.Bitangent), sizeof(glm::vec3));
        }

        // Write indices
        out.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));

        // // Write texture paths (relative only)
        // for (const auto& tex : mesh.textures) {
        //     char buffer[256] = {};
        //     strncpy(buffer, tex.path.c_str(), 255); // relative path only
        //     out.write(buffer, 256);
        // }
        for (const auto& tex : mesh.textures) {    
            char buffer[256] = {};
            std::string fullPath = model.directory + "/" + tex.path; 
            strncpy(buffer, fullPath.c_str(), 255);
            // strncpy(buffer, tex.path.c_str(), 255);
            out.write(buffer, 256);
        }
    }

    out.close();
}



Model* Engine::LoadModelBinary(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    // if (!in.is_open()) return &Model();
    if (!in.is_open()) return nullptr;

    Model* model = new Model();

    ModelHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    model->hasDiffuse = header.hasDiffuse;
    model->hasSpecular = header.hasSpecular;
    model->hasNormal = header.hasNormal;

    for (uint32_t i = 0; i < header.numMeshes; i++) {
        MeshHeader meshHeader;
        in.read(reinterpret_cast<char*>(&meshHeader), sizeof(meshHeader));

        std::vector<Vertex> vertices(meshHeader.numVertices);
        std::vector<uint32_t> indices(meshHeader.numIndices);
        std::vector<Texture> textures(meshHeader.numTextures);

        for (uint32_t j = 0; j < meshHeader.numVertices; j++) {
            VertexBinary vb;
            in.read(reinterpret_cast<char*>(&vb), sizeof(vb));
            vertices[j].Position = vb.position;
            vertices[j].Normal = vb.normal;
            vertices[j].TexCoords = vb.texCoords;
            vertices[j].Tangent = vb.tangent;
            vertices[j].Bitangent = vb.bitangent;
        }

        in.read(reinterpret_cast<char*>(indices.data()), meshHeader.numIndices * sizeof(uint32_t));

        for (uint32_t j = 0; j < meshHeader.numTextures; j++) {
            char buffer[256] = {};
            in.read(buffer, 256);
            Texture tex;
            tex.path = buffer;
            tex.id = 0; // load texture later with OpenGL
            textures[j] = tex;
        }

        Mesh mesh(vertices, indices, textures);
        mesh.aabbMin = meshHeader.aabbMin;
        mesh.aabbMax = meshHeader.aabbMax;
        model->meshes.push_back(mesh);
    }


    for (auto& mesh : model->meshes) {
    for (auto& tex : mesh.textures) {
        if (tex.path.size() > 0 && tex.id == 0) {
            tex.id = model->TextureFromFile(tex.path.c_str(), model->directory,false,true);
            model->textures_loaded.push_back(tex);
        }
        }
    }

    for (auto& mesh : model->meshes){
        glm::vec3 minPoint(FLT_MAX);
        glm::vec3 maxPoint(-FLT_MAX);
        for(auto& vertex : mesh.vertices){
            minPoint = glm::min(minPoint, vertex.Position);
            maxPoint = glm::max(maxPoint, vertex.Position);

            model->aabbMin =  glm::min(minPoint, model->aabbMin);
            model->aabbMax =  glm::max(maxPoint, model->aabbMax);         
            model->localAabbMin =  glm::min(minPoint, model->aabbMin);
            model->localAabbMax =  glm::max(maxPoint, model->aabbMax);  
        }
    }

    return model;
}