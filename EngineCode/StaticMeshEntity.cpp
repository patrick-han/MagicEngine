#include "StaticMeshEntity.h"
namespace Magic
{
StaticMeshEntity::StaticMeshEntity() 
    : IEntity()
{

}

StaticMeshEntity::~StaticMeshEntity()
{

}

void StaticMeshEntity::AddSubMesh(SubMesh* pSubMesh)
{
    pSubMesh->m_parentMesh = this;
    m_subMeshes.push_back(pSubMesh);
}

std::span<SubMesh* const> StaticMeshEntity::GetSubMeshes() const
{
    return std::span<SubMesh* const>(m_subMeshes);
}

}