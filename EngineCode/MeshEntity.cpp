#include "MeshEntity.h"
namespace Magic
{
MeshEntity::MeshEntity() 
    : IEntity()
{

}

MeshEntity::~MeshEntity()
{

}

void MeshEntity::AddSubMesh(SubMesh* pSubMesh)
{
    pSubMesh->m_parentMesh = this;
    m_subMeshes.push_back(pSubMesh);
}

std::span<SubMesh* const> MeshEntity::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}

}