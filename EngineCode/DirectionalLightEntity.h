#pragma once
#include "Entity.h"
#include "../CommonCode/Math/Vector4f.h"

namespace Magic
{

struct DirectionalLightData
{
    Vector4f m_direction;
    Vector4f m_color;
    float m_angle = 0.0f;
    float m_intensity = 0.0f;
    float m_exposure = 0.0f;
    float data0 = 0.0f;
};

class DirectionalLightEntity final : public IEntity
{
public:
    DirectionalLightEntity();
    ~DirectionalLightEntity() = default;
    [[nodiscard]] virtual EntityType GetEntityType() const override { return EntityType::DirectionalLight; }
    [[nodiscard]] virtual bool Load(const pxr::UsdPrim& entityPrim) override;
    [[nodiscard]] virtual bool Unload() override;
protected:
    virtual void DrawInspectorElementsSpecific() override;
public:

    Vector4f m_direction;
    Vector4f m_color;
    float m_angle;
    float m_intensity;
    float m_exposure;
};

}
