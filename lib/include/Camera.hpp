#pragma once 
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SDL2/SDL.h>

class Camera
{
    public:
        // Camera(float fov, float nearPlane, float farPlane);
        Camera(float fov, float nearPlane, float farPlane)
        : mFov(fov), nearP(nearPlane), farP(farPlane) {}
        Camera();
        glm::mat4 view() const;
        glm::mat4 proj(float aspect) const;
        glm::vec3 position() const;
        void recalculateViewport(SDL_Event e);
        void HandleInput(SDL_Event e, int mx, int my);
        void Zoom(SDL_Event e);
        void Update(float dt);
        // glm::vec3 positionWithCollision(TerrainMap* terrain) const;
        // glm::mat4 view(TerrainMap* terrain) const;
        glm::vec3 pos = glm::vec3(0.0f);
        glm::vec3* targetPos;
        
        
        glm::vec3 forward;
        glm::vec3 right;

        // int lastmx = 0;
        // int lastmy = 0;
        
        float mFov=60.0f; 
        float yaw=1.00f;
        float pitch=-0.35f; // radians
        float nearP=0.1f; 
        float farP=2000.0f;
        float distance = 5.0f;
        float minDistance = 2.0f;
        float maxDistance = 30.0f;
        float playerYaw = yaw;
    private:
        bool lmb=false;
        bool rmb=false;
        bool mmb=false;

};