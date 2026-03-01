#include "Entity.h"
namespace Magic
{
class StaticMesh;
class StaticMeshEntity final : public IEntity
{
public:
    StaticMeshEntity();
    ~StaticMeshEntity();
    virtual EntityType GetEntityType() override { return EntityType::StaticMesh; }
    StaticMesh* m_staticMesh = nullptr;
};

}