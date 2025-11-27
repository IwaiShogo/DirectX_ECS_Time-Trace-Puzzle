/*****************************************************************//**
 * @file	InspectorWindow.h
 * @brief	インスペクターウィンドウ
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date   2025/11/27	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___INSPECTOR_WINDOW_H___
#define ___INSPECTOR_WINDOW_H___

// ===== インクルード =====
#include "Editor/Editor.h"
#include "Components/Components.h"
#include "Core/AudioManager.h"
#include "Core/Serializer.h"
#include "imgui.h"

class InspectorWindow
	: public EditorWindow
{
public:
	void Draw(World& world, Entity& selected, Context& ctx) override
	{
		Registry& reg = world.getRegistry();

		// 選択されていない、または無効な場合は何もしない
		if (selected == NullEntity || !reg.has<Tag>(selected)) return;

		ImGui::Begin("Inspector");

		// Tag (Header)
		Tag& tag = reg.get<Tag>(selected);
		ImGui::Text("ID: %d", selected);
		ImGui::LabelText("Name", "%s", tag.name.c_str());
		ImGui::Separator();

		// --- Transform ---
		if (reg.has<Transform>(selected)) {
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
				Transform& t = reg.get<Transform>(selected);
				ImGui::DragFloat3("Position", &t.position.x, 0.1f);
				ImGui::DragFloat3("Rotation", &t.rotation.x, 0.1f);
				ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
			}
		}

		// --- Mesh ---
		if (reg.has<MeshComponent>(selected)) {
			if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
				MeshComponent& m = reg.get<MeshComponent>(selected);
				ImGui::Text("Model: %s", m.modelKey.c_str());
				ImGui::ColorEdit4("Color", &m.color.x);
				ImGui::DragFloat3("Scale Offset", &m.scaleOffset.x, 0.01f);
			}
		}

		// --- Collider ---
		if (reg.has<BoxCollider>(selected)) {
			if (ImGui::CollapsingHeader("BoxCollider", ImGuiTreeNodeFlags_DefaultOpen)) {
				BoxCollider& b = reg.get<BoxCollider>(selected);
				ImGui::DragFloat3("Size", &b.size.x, 0.1f);
				ImGui::DragFloat3("Offset", &b.offset.x, 0.1f);
			}
		}

		// --- Velocity ---
		if (reg.has<Velocity>(selected)) {
			if (ImGui::CollapsingHeader("Velocity", ImGuiTreeNodeFlags_DefaultOpen)) {
				Velocity& v = reg.get<Velocity>(selected);
				ImGui::DragFloat3("Vector", &v.velocity.x, 0.1f);
				if (ImGui::Button("Stop")) v.velocity = { 0,0,0 };
			}
		}

		// --- PlayerInput ---
		if (reg.has<PlayerInput>(selected)) {
			if (ImGui::CollapsingHeader("Player Input", ImGuiTreeNodeFlags_DefaultOpen)) {
				PlayerInput& p = reg.get<PlayerInput>(selected);
				ImGui::DragFloat("Speed", &p.speed, 0.1f);
				ImGui::DragFloat("Jump", &p.jumpPower, 0.1f);
			}
		}

		// --- AudioSource ---
		if (reg.has<AudioSource>(selected)) {
			if (ImGui::CollapsingHeader("AudioSource", ImGuiTreeNodeFlags_DefaultOpen)) {
				AudioSource& a = reg.get<AudioSource>(selected);
				ImGui::Text("Key: %s", a.soundKey.c_str());
				ImGui::SliderFloat("Volume", &a.volume, 0.0f, 1.0f);
				ImGui::DragFloat("Range", &a.range, 0.1f);
				if (ImGui::Button("Test Play")) {
					AudioManager::Instance().PlaySE(a.soundKey, a.volume);
				}
			}
		}

		ImGui::Separator();

		// プレハブ保存
		if (ImGui::Button("Save as Prefab")) {
			// タグ名をファイル名にする（例: "Enemy" -> "Assets/Prefabs/Enemy.json"）
			// ※フォルダは事前に作っておいてください
			std::string path = "Resources/Prefabs/" + std::string(reg.get<Tag>(selected).name) + ".json";
			Serializer::SaveEntity(reg, selected, path);
			Logger::Log("Saved Prefab: " + path);
		}

		// --- 削除ボタン ---
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) {
			reg.destroy(selected);
			selected = NullEntity;
		}
		ImGui::PopStyleColor();

		ImGui::End();
	}
};

#endif // !___INSPECTOR_WINDOW_H___