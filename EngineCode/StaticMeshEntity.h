#include "Entity.h"

namespace vjson
{
class Value;
}
namespace Magic
{
class StaticMesh;
class StaticMeshEntity final : public IEntity
{
public:
    StaticMeshEntity();
    ~StaticMeshEntity();
    [[nodiscard]] virtual EntityType GetEntityType() const override { return EntityType::StaticMesh; }
    [[nodiscard]] virtual bool Load(vjson::Value* entity) override;
    [[nodiscard]] virtual bool Unload() override;
    StaticMesh* m_staticMesh = nullptr;
};

}