#pragma once
#include "Entity.h"
#include "Buffer.h"
#include "../CommonCode/AABB.h"

#include <span>

namespace Magic
{

struct SubMesh
{
    Matrix4f m_transform;
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    uint32_t indexCount = 0;
    int diffuseTextureBindlessArraySlot = -1;
    bool hasTexture = false; // To distinguish from meshes that only have vertex colors
    AABB3f aabb;
};

class StaticMeshEntity final : public IEntity
{
public:
    StaticMeshEntity();
    ~StaticMeshEntity();
    [[nodiscard]] virtual EntityType GetEntityType() const override { return EntityType::StaticMesh; }
    [[nodiscard]] virtual bool Load(const pxr::UsdPrim& entityPrim) override;
    [[nodiscard]] virtual bool Unload() override;
    [[nodiscard]] std::span<SubMesh* const> GetSubMeshes() const;
    [[nodiscard]] std::size_t GetSubMeshCount() const { return m_subMeshes.size(); }
    virtual void DrawInspectorElements() override;
private:
    std::vector<SubMesh*> m_subMeshes;
};

}
