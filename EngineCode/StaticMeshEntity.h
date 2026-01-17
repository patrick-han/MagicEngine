#include "Entity.h"
#include "SubMesh.h"
#include <vector>
#include <span>
namespace Magic
{

class StaticMeshEntity final : public IEntity
{
public:
    StaticMeshEntity();
    ~StaticMeshEntity();
    virtual EntityType GetEntityType() override { return EntityType::StaticMesh; }
    void AddSubMesh(SubMesh* pSubMesh);
    [[nodiscard]] std::span<SubMesh* const> GetSubMeshes() const;
private:
    std::vector<SubMesh*> m_subMeshes;
};




}