#pragma once
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/Math/Vector3f.h"

// +X left
// +Y up
// +Z pointing OUT of the camera

namespace Magic
{

class Camera
{
public:
    enum class CameraMovementDirection
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    Camera(Vector3f _position, Vector3f _forward);
    ~Camera();
    [[nodiscard]] Vector3f GetPosition() const;
    void SetPosition(Vector3f position);

    void Move(CameraMovementDirection movementDirection, float cameraSpeed);
    void Rotate(float xoffset, float yoffset, bool constrainPitch);
    [[nodiscard]] Matrix4f GetViewMatrix() const;
    [[nodiscard]] Matrix4f GetProjectionMatrix(float width, float height, float near, float far, float fovY) const;

    void PrintDebug(bool pos, bool vecs, bool yawpitch) const;
private:
    // Matrix4f m_toWorld;
    Vector3f m_position;
    Vector3f m_forward;
    Vector3f m_left;
    Vector3f m_up;
    Vector3f m_worldUp;
    float m_yaw;
    float m_pitch;
};


}