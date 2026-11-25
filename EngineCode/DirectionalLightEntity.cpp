#include "DirectionalLightEntity.h"
#include "Math/Matrix4f.h"
#include "Math/Vector3f.h"

#include "ThirdParty/imgui/imgui.h"
Magic::DirectionalLightEntity::DirectionalLightEntity() : m_direction(Vector4f(0.0f, 0.0f, 0.0f, 0.0f)), m_color(Vector4f(1.0f)), m_angle(0.0f), m_intensity(0.0f), m_exposure(0.0f)
{

}

bool Magic::DirectionalLightEntity::Load(const vjson::Object *params)
{
    const vjson::Array& colorJson = params->ArrayAtKeyOrEmpty("color");
    Vector4f color(colorJson.DoubleAtIndex(0, 1.0), colorJson.DoubleAtIndex(1, 1.0), colorJson.DoubleAtIndex(2, 1.0), colorJson.DoubleAtIndex(3, 1.0));
    m_color = color;
    m_angle = params->DoubleAtKey("angle", 1.0);
    m_intensity = params->DoubleAtKey("intensity", 1.0);
    m_exposure = params->DoubleAtKey("exposure", 1.0);

    const Vector4f localDirection(0.0, 0.0, -1.0, 0.0);
    Vector4f worldDirection = m_transform * localDirection;
    Vector3f direction3(worldDirection.v[0], worldDirection.v[1], worldDirection.v[2]);
    direction3.AsNormalized();
    m_direction = Vector4f(static_cast<float>(direction3.x), static_cast<float>(direction3.y), static_cast<float>(direction3.z), 0.0f); // points TOWARD direction of light
    return true;
}

bool Magic::DirectionalLightEntity::Unload()
{
    return true;
}

Matrix4f Magic::DirectionalLightEntity::GetViewMatrix(Vector3f cameraPosition)
{
    Vector3f lightForward = Vector3f(
        m_direction.v[0],
        m_direction.v[1],
        m_direction.v[2]
    ).AsNormalized();

    // Avoid crossing with an almost-parallel up vector.
    Vector3f upHint = std::abs(Dot(lightForward, Vector3f(0.0f, 0.0f, 1.0f))) > 0.99f
        ? Vector3f(0.0f, 1.0f, 0.0f)
        : Vector3f(0.0f, 0.0f, 1.0f);

    Vector3f lightRight = Cross(lightForward, upHint).AsNormalized();
    Vector3f lightUp = Cross(lightRight, lightForward).AsNormalized();

    Vector3f shadowCenter = cameraPosition; // better: camera frustum/world bounds center
    constexpr float shadowDepth = 200.0f;
    Vector3f lightPosition = shadowCenter - lightForward * (shadowDepth * 0.5f);

    Matrix4f lightWorld(
        lightRight.x,   lightForward.x,   lightUp.x,   lightPosition.x,
        lightRight.y,   lightForward.y,   lightUp.y,   lightPosition.y,
        lightRight.z,   lightForward.z,   lightUp.z,   lightPosition.z,
        0.0f,           0.0f,             0.0f,        1.0f
    );

    return lightWorld.InvertedRigid();
}

Matrix4f Magic::DirectionalLightEntity::GetProjectionMatrix(float width, float height, float nearPlane, float farPlane)
{
    auto MakeOrthographic = [](float width, float height, float nearPlane, float farPlane) -> Matrix4f
    {
        const float halfW = width * 0.5f;
        const float halfH = height * 0.5f;

        return Matrix4f(
            1.0f / halfW, 0.0f, 0.0f, 0.0f
            , 0.0f, 0.0f, -1.0f / halfH, 0.0f
            , 0.0f, 1.0f / (farPlane - nearPlane), 0.0f, -nearPlane / (farPlane - nearPlane)
            , 0.0f, 0.0f, 0.0f, 1.0f
        );
    };
    return MakeOrthographic(width, height, nearPlane, farPlane);
}

void Magic::DirectionalLightEntity::DrawInspectorElementsSpecific()
{
    ImGui::DragFloat3("Light Direction", &m_direction.v[0], 0.05f, -1.0f, 1.0f, "%.3f");
    ImGui::DragFloat3("Light Color", &m_color.v[0], 0.05f, 0.0f, 1.0f, "%.3f");
    ImGui::DragFloat("Light Intensity", &m_intensity, 0.05f, 0.0f, 100.0f, "%.3f");
    ImGui::DragFloat("Light Exposure", &m_exposure, 0.05f, 0.0f, 100.0f, "%.3f");
    ImGui::Separator(); 
}
