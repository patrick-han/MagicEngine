#include "DirectionalLightEntity.h"
#include "../CommonCode/Math/Matrix4f.h"
#include "../CommonCode/Math/Vector3f.h"

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/base/tf/token.h>
#include "ThirdParty/imgui/imgui.h"
Magic::DirectionalLightEntity::DirectionalLightEntity() : m_direction(Vector4f(0.0f, 0.0f, 0.0f, 0.0f)), m_color(Vector4f(1.0f)), m_angle(0.0f), m_intensity(0.0f), m_exposure(0.0f)
{

}

bool Magic::DirectionalLightEntity::Load(const pxr::UsdPrim &entityPrim)
{
    pxr::UsdGeomXformCache cache;
    const pxr::GfMatrix4d worldTransform = cache.GetLocalToWorldTransform(entityPrim);
    SetName(entityPrim.GetName().GetText());

    
    const pxr::UsdPrim sunPrim = entityPrim.GetChild(pxr::TfToken("Sun"));

    if (!sunPrim || !sunPrim.IsA<pxr::UsdLuxDistantLight>())
    {
        return false;
    }

    const pxr::UsdLuxDistantLight sun(sunPrim);

    pxr::GfVec3f color;
    sun.GetColorAttr().Get(&color);
    m_color.v[0] = color[0];
    m_color.v[1] = color[1];
    m_color.v[2] = color[2];
    sun.GetAngleAttr().Get(&m_angle);
    sun.GetIntensityAttr().Get(&m_intensity);
    sun.GetExposureAttr().Get(&m_exposure);

    m_intensity = m_intensity * 4.0f; // USD divides the Blender intensity value by 4 (From Blender's USD exporter: "/* Unclear why, but approximately matches Karma. */")

    const pxr::GfVec4d localDirection(0.0, 0.0, -1.0, 0.0); // "An intrinsic light that provides light from a distant source, along the -Z axis"
    pxr::GfVec4d worldDirection = localDirection * worldTransform; // USD uses row vectors
    pxr::GfVec3d direction3(worldDirection[0], worldDirection[1], worldDirection[2]);
    direction3.Normalize();

    m_direction = Vector4f(static_cast<float>(direction3[0]), static_cast<float>(direction3[1]), static_cast<float>(direction3[2]), 0.0f); // points TOWARD direction of light
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
