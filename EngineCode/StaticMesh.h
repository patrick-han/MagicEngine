#include "SubMesh.h"
#include <vector>
#include <span>

namespace Magic
{

class StaticMesh final
{
public:
    StaticMesh();
    ~StaticMesh();
    void AddSubMesh(SubMesh* pSubMesh);
    [[nodiscard]] std::span<SubMesh* const> GetSubMeshes() const;
private:
    std::vector<SubMesh*> m_subMeshes;
};


}