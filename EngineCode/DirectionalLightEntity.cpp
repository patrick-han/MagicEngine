#include "DirectionalLightEntity.h"

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/base/tf/token.h>

Magic::DirectionalLightEntity::DirectionalLightEntity() : m_direction(Vector4f(0.0f, 0.0f, 0.0f, 0.0f)), m_color(Vector4f(1.0f)), m_angle(0.0f), m_intensity(0.0f), m_exposure(0.0f)
{

}

bool Magic::DirectionalLightEntity::Load(const pxr::UsdPrim &entityPrim)
{
    pxr::UsdGeomXformCache cache;
    const pxr::GfMatrix4d worldTransform = cache.GetLocalToWorldTransform(entityPrim);

    
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
