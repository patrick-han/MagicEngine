#pragma once
#include "Entity.h"
#include "../CommonCode/Math/Vector4f.h"

namespace Magic
{

class DirectionalLightEntity final : public IEntity
{
public:
    DirectionalLightEntity();
    ~DirectionalLightEntity() = default;
    [[nodiscard]] virtual EntityType GetEntityType() const override { return EntityType::DirectionalLight; }
    [[nodiscard]] virtual bool Load(const pxr::UsdPrim& entityPrim) override;
    [[nodiscard]] virtual bool Unload() override;
private:
    Vector4f m_direction;
    Vector4f m_color;
    float m_angle;
    float m_intensity;
    float m_exposure;
};

}
