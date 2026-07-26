#include "Camera.h"
#include "../CommonCode/Math/Math.h"
#include <cmath>
#include "../CommonCode/Log.h"

namespace Magic {
Camera::Camera(Vector3f _position, Vector3f _forward)
    : m_position(_position), m_forward(_forward.AsNormalized())
    , m_pitch(rad2deg(std::asin(m_forward.z)))
    , m_yaw(rad2deg(std::atan2(-m_forward.x, m_forward.y)))
    , m_frozen(false)
{
    m_worldUp = Vector3f(0.0f, 0.0f, 1.0f);
    m_right = Cross(m_forward, m_worldUp).AsNormalized();
    m_up = Cross(m_right, m_forward).AsNormalized();
}

Camera::~Camera()
{
}

Vector3f Camera::GetPosition() const
{
    // return { m_toWorld.m03, m_toWorld.m13, m_toWorld.m23 };
    return m_position;
}

void Camera::SetPosition(Vector3f position)
{
    // m_toWorld.m03 = position.x;
    // m_toWorld.m13 = position.y;
    // m_toWorld.m23 = position.z;
    m_position = position;
}

void Camera::Move(CameraMovementDirection movementDirection, float cameraSpeed)
{
    if(m_frozen)
    {
        return;
    }
    Vector3f positionDelta = Vector3f(0.0f, 0.0f, 0.0f);
    if (movementDirection == CameraMovementDirection::FORWARD)
    {
        positionDelta += cameraSpeed * m_forward;
    }
    if (movementDirection == CameraMovementDirection::BACKWARD)
    {
        positionDelta -= cameraSpeed * m_forward;
    }
    if (movementDirection == CameraMovementDirection::LEFT)
    {
        positionDelta -= cameraSpeed * m_right;
    }
    if (movementDirection == CameraMovementDirection::RIGHT)
    {
        positionDelta += cameraSpeed * m_right;
    }
    if (movementDirection == CameraMovementDirection::UP)
    {
        positionDelta += cameraSpeed * m_worldUp;
    }
    if (movementDirection == CameraMovementDirection::DOWN)
    {
        positionDelta -= cameraSpeed * m_worldUp;
    }
    m_position = m_position + positionDelta;
}

void Camera::Rotate(float xoffset, float yoffset, bool constrainPitch)
{
    if(m_frozen)
    {
        return;
    }
    // Positive yaw is counter-clockwise about +Z. A positive mouse X offset
    // turns right, so it decreases yaw.
    m_yaw -= xoffset;
    m_pitch += yoffset;

    if (constrainPitch)
    {
        if (m_pitch > 89.0f) // +90 is looking straight up
        {
            m_pitch = 89.0f;
        }
        if (m_pitch < -89.0f)
        {
            m_pitch = -89.0f;
        }
    }
    // Update the direction the camera is looking at based on the camera yaw and pitch
    Vector3f direction;
    direction.x = -std::sin(deg2rad(m_yaw)) * std::cos(deg2rad(m_pitch));
    direction.y = std::cos(deg2rad(m_yaw)) * std::cos(deg2rad(m_pitch));
    direction.z = std::sin(deg2rad(m_pitch));
    m_forward = direction.AsNormalized();
    // Recalculate the remaining camera axes
    m_right = Cross(m_forward, m_worldUp).AsNormalized();
    m_up = Cross(m_right, m_forward).AsNormalized();
}

Matrix4f Camera::GetViewMatrix() const
{
    Matrix4f view = Matrix4f(
        m_right.x, m_forward.x, m_up.x, m_position.x
        , m_right.y, m_forward.y, m_up.y, m_position.y
        , m_right.z, m_forward.z, m_up.z, m_position.z
        , 0.0f, 0.0f, 0.0f, 1.0f
        ).InvertedRigid();
    return view;
}

Matrix4f Camera::GetProjectionMatrix(float width, float height, float near, float far, float fovY) const
{
    float aspectRatio = width / height;
    float tanHalfFovy = std::tanf(deg2rad(fovY) / 2.0f);
    // Convert the engine's +X-right, +Y-forward, +Z-up camera space directly to Vulkan clip space (+X right, +Y down, depth in [0, 1]).
    return Matrix4f(
          1.0f / (aspectRatio * tanHalfFovy), 0.0f, 0.0f, 0.0f
        , 0.0f, 0.0f, -1.0f / tanHalfFovy, 0.0f
        , 0.0f, far / (far-near), 0.0f, -(near*far) / (far-near)
        , 0.0f, 1.0f, 0.0f, 0.0f
    );
}

void Camera::PrintDebug(bool pos, bool vecs, bool yawpitch) const
{
    if (pos)
    {
        Logger::Info(std::format("Camera Position: {}, {}, {}", m_position.x, m_position.y, m_position.z));
    }
    if (vecs)
    {
        Logger::Info(std::format("Camera Right: {}, {}, {}", m_right.x, m_right.y, m_right.z));
        Logger::Info(std::format("Camera Up: {}, {}, {}", m_up.x, m_up.y, m_up.z));
        Logger::Info(std::format("Camera Forward: {}, {}, {}", m_forward.x, m_forward.y, m_forward.z));
    }
    if (yawpitch)
    {
        Logger::Info(std::format("Camera Yawpitch: {}, {}", m_yaw, m_pitch));
    }

}


} // Magic
