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
    [[nodiscard]] std::size_t GetSubMeshCount() const { return m_subMeshes.size(); }
private:
    std::vector<SubMesh*> m_subMeshes;
};


}