#include "Camera.h"
#include "Common/Math/Math.h"
#include <cmath>
#include "Common/Log.h"

namespace Magic {
Camera::Camera(Vector3f _position, Vector3f _forward)
    : m_position(_position), m_forward(_forward.AsNormalized())
    , m_pitch(0.0f), m_yaw(0.0f)
    , m_frozen(false)
{
    m_worldUp = Vector3f(0.0f, 1.0f, 0.0f); // Default +Y up
    m_left = Cross(m_worldUp, m_forward).AsNormalized();
    m_up = Cross(m_forward, m_left).AsNormalized();
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
        positionDelta += cameraSpeed * m_left;
    }
    if (movementDirection == CameraMovementDirection::RIGHT)
    {
        positionDelta -= cameraSpeed * m_left;
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
    // Moving the mouse left results in a negative offset. We want a left mouse movement to be a CCW rotation when viewed from above according to RHR.
    // Subtracting a negative results in a positive increase (positive angle is CCW)
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
    direction.x = std::sin(deg2rad(m_yaw)) * std::cos(deg2rad(m_pitch));
    direction.y = std::sin(deg2rad(m_pitch));
    direction.z = std::cos(deg2rad(m_yaw)) * std::cos(deg2rad(m_pitch));
    m_forward = direction.AsNormalized();
    // also re-calculate the left and up vectors
    m_left = Cross(m_worldUp, m_forward).AsNormalized();// normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    m_up = Cross(m_forward, m_left).AsNormalized();
}

Matrix4f Camera::GetViewMatrix() const
{
    Matrix4f view = Matrix4f(
        m_left.x, m_up.x, m_forward.x, m_position.x
        , m_left.y, m_up.y, m_forward.y, m_position.y
        , m_left.z, m_up.z, m_forward.z, m_position.z
        , 0.0f, 0.0f, 0.0f, 1.0f
        ).InvertedRigid();
    return view;
}

Matrix4f Camera::GetProjectionMatrix(float width, float height, float near, float far, float fovY) const
{
    float aspectRatio = width / height;
    float tanHalfFovy = std::tanf(deg2rad(fovY) / 2.0f);
    // (void)(far);
    // Projection matrix for a view space already in the same orientation as Vulkan clip space (+Z away from camera, +X right, +Y down)
    Matrix4f projection = Matrix4f(
          1.0f / (aspectRatio * tanHalfFovy), 0.0f, 0.0f, 0.0f
        , 0.0f, 1.0f / (tanHalfFovy), 0.0f, 0.0f
        , 0.0f, 0.0f, far / (far-near), -(near*far) / (far-near)
        // , 0.0f, 0.0f, 1.0f, -near // Infinite far plane
        , 0.0f, 0.0f, 1.0f, 0.0f
    );

    // Essentially a 180 degree CW rotation about +Z to align view space with the expected vulkan clip space
    return projection * Matrix4f::MakeRotateZ(deg2rad(180.f));
}

void Camera::PrintDebug(bool pos, bool vecs, bool yawpitch) const
{
    if (pos)
    {
        Logger::Info(std::format("Camera Position: {}, {}, {}", m_position.x, m_position.y, m_position.z));
    }
    if (vecs)
    {
        Logger::Info(std::format("Camera Left: {}, {}, {}", m_left.x, m_left.y, m_left.z));
        Logger::Info(std::format("Camera Up: {}, {}, {}", m_up.x, m_up.y, m_up.z));
        Logger::Info(std::format("Camera Forward: {}, {}, {}", m_forward.x, m_forward.y, m_forward.z));
    }
    if (yawpitch)
    {
        Logger::Info(std::format("Camera Yawpitch: {}, {}", m_yaw, m_pitch));
    }

}


} // Magic