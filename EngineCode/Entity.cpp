#include "Entity.h"
#include "ThirdParty/imgui/imgui.h"
namespace Magic
{

void IEntity::DrawInspectorElements()
{
    ImGui::TextWrapped("UUID");
    std::string uuidStr = m_uuid.ToString();
    ImGui::TextWrapped("%s", uuidStr.c_str());
    ImGui::Separator();


    ImGui::TextWrapped("Transform Matrix");
    ImGui::TextWrapped("[%f, %f, %f, %f]", m_transform.m[0], m_transform.m[1], m_transform.m[2], m_transform.m[3]);
    ImGui::TextWrapped("[%f, %f, %f, %f]", m_transform.m[4], m_transform.m[5], m_transform.m[6], m_transform.m[7]);
    ImGui::TextWrapped("[%f, %f, %f, %f]", m_transform.m[8], m_transform.m[9], m_transform.m[10], m_transform.m[11]);
    ImGui::TextWrapped("[%f, %f, %f, %f]", m_transform.m[12], m_transform.m[13], m_transform.m[14], m_transform.m[15]);
    ImGui::Separator();


    ImGui::TextWrapped("Position");
    float avail_width = ImGui::GetContentRegionAvail().x;
    float item_width = (avail_width - ImGui::GetStyle().ItemSpacing.x * 2.0f) / 3.0f;
    ImGui::PushItemWidth(item_width);
    ImGui::DragFloat("##PositionX", &m_transform.m[3], 0.1f, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
    ImGui::DragFloat("##PositionY", &m_transform.m[7], 0.1f, 0.0f, 0.0f, "%.3f"); ImGui::SameLine();
    ImGui::DragFloat("##PositionZ", &m_transform.m[11], 0.1f, 0.0f, 0.0f, "%.3f");
    ImGui::PopItemWidth();
    ImGui::Separator();

    
    DrawInspectorElementsSpecific();
}

}