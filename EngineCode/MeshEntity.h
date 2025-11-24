#include "Entity.h"
#include "SubMesh.h"
#include <vector>
#include <span>
namespace Magic
{

class MeshEntity final : public IEntity
{
public:
    MeshEntity();
    ~MeshEntity();
    virtual EntityType GetEntityType() override { return EntityType::StaticMesh; }
    void AddSubMesh(SubMesh* pSubMesh);
    [[nodiscard]] std::span<SubMesh* const> GetSubMeshes() const;
private:
    std::vector<SubMesh*> m_subMeshes;
};




}