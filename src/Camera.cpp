#include "Camera.hpp"



Camera::Camera()
{
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), *targetPos, glm::vec3(0, 1, 0));
}


glm::mat4 Camera::proj(float aspect) const
{
    return glm::perspective(glm::radians(mFov), aspect, nearP, farP);
}

glm::vec3 Camera::position() const
{
        glm::vec3 offset;
        offset.x = distance * cosf(pitch) * cosf(yaw);
        offset.y = distance * sinf(pitch);          // <-- vertical offset
        offset.z = distance * cosf(pitch) * sinf(yaw);
        return *targetPos + offset;
}

void Camera::HandleInput(SDL_Event e, int mx, int my)
{

    if(e.type==SDL_MOUSEBUTTONDOWN){ if(e.button.button==SDL_BUTTON_RIGHT) rmb=true; if(e.button.button==SDL_BUTTON_LEFT) lmb=true; if(e.button.button==SDL_BUTTON_MIDDLE) mmb=true; }
    if(e.type==SDL_MOUSEBUTTONUP){ if(e.button.button==SDL_BUTTON_RIGHT) rmb=false; if(e.button.button==SDL_BUTTON_LEFT) lmb=false; if(e.button.button==SDL_BUTTON_MIDDLE) mmb=false; }

    static int lastmx=mx, lastmy=my;
    int dx = mx-lastmx, dy = my-lastmy;
    lastmx=mx; lastmy=my;



    if (rmb) {
        yaw   += dx * 0.0035f;
        pitch += dy * 0.0035f;
        pitch = glm::clamp(pitch, -1.5f, 1.5f);
    }

}

void Camera::Zoom(SDL_Event e)
{
    if (e.wheel.y > 0) distance = glm::max(minDistance, distance - 1.0f);
    if (e.wheel.y < 0) distance = glm::min(maxDistance, distance + 1.0f);
}
void Camera::Update(float dt)
{
    forward = glm::normalize(glm::vec3(cosf(pitch)*cosf(yaw), 0.0f, cosf(pitch)*sinf(yaw)));
    // forward = glm::normalize(glm::vec3(cosf(pitch)*cosf(playerYaw), 0.0f, cosf(pitch)*sinf(playerYaw)));
    right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));
}

void Camera::recalculateViewport(SDL_Event e)
{
    float aspect = e.window.data1 / (float)e.window.data2;
    glViewport(0,0,e.window.data1,e.window.data2); 
}
