#include "StaticMesh.h"


namespace Magic
{

StaticMesh::StaticMesh()
{
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::AddSubMesh(SubMesh* pSubMesh)
{
    pSubMesh->m_parentMesh = this;
    m_subMeshes.push_back(pSubMesh);
}

std::span<SubMesh* const> StaticMesh::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}


}