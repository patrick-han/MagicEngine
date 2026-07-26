#pragma once
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/Math/Vector3f.h"

// Right-handed camera space:
// +X right
// +Y forward (away from the camera)
// +Z up

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
    void Freeze() { m_frozen = true; }
    void UnFreeze() { m_frozen = false; }

    [[nodiscard]] Vector3f GetForward() const { return m_forward; }
    [[nodiscard]] Vector3f GetRight() const { return m_right; }
    [[nodiscard]] Vector3f GetUp() const { return m_up; }
private:
    Vector3f m_position;
    Vector3f m_forward;
    Vector3f m_right;
    Vector3f m_up;
    Vector3f m_worldUp;
    float m_yaw;
    float m_pitch;
    bool m_frozen;
};


}
