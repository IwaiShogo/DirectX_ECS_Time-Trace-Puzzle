/*****************************************************************//**
 * @file	CreatorWindow.h
 * @brief	エンティティ作成ウィンドウ
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

#ifndef ___CREATOR_WINDOW_H___
#define ___CREATOR_WINDOW_H___

// ===== インクルード =====
#include "Editor/Editor.h"
#include "Core/Serializer.h"
#include "imgui.h"
#include <filesystem>

class CreatorWindow : public EditorWindow {
public:
	void Draw(World& world, Entity& selected, Context& ctx) override {
		ImGui::Begin("Entity Creator");

		Registry& reg = world.getRegistry();

		// 1. 新規作成
		if (ImGui::Button("Create Empty Entity")) {
			Entity e = world.create_entity()
				.add<Tag>("New Entity")
				.add<Transform>() // 必須
				.id();
			selected = e; // 選択状態にする
			Logger::Log("Created Entity ID: " + std::to_string(e));
		}

		ImGui::Separator();

		// 2. プレハブから生成
		ImGui::Text("Spawn from Prefab:");

		// Prefabsフォルダ内のjsonファイルを列挙
		namespace fs = std::filesystem;
		std::string prefabDir = "Resources/Prefabs";
		if (fs::exists(prefabDir)) {
			for (const auto& entry : fs::directory_iterator(prefabDir)) {
				if (entry.path().extension() == ".json") {
					std::string filename = entry.path().filename().string();
					if (ImGui::Button(filename.c_str())) {
						Entity e = Serializer::LoadEntity(world, entry.path().string());
						// 生成位置をカメラの前などにすると便利ですが、一旦保存された位置に出ます
						selected = e;
						Logger::Log("Spawned: " + filename);
					}
				}
			}
		}
		else {
			ImGui::TextDisabled("Resources/Prefabs folder not found");
		}

		ImGui::Separator();

		// 3. コンポーネントの追加 (選択中のエンティティに対して)
		if (selected != NullEntity && reg.has<Tag>(selected)) {
			ImGui::Text("Add Component to ID:%d", selected);

			if (!reg.has<MeshComponent>(selected) && ImGui::Button("Add Mesh")) {
				reg.emplace<MeshComponent>(selected, "cube"); // デフォルト値
			}
			if (!reg.has<BoxCollider>(selected) && ImGui::Button("Add Collider")) {
				reg.emplace<BoxCollider>(selected);
			}
			if (!reg.has<AudioSource>(selected) && ImGui::Button("Add Audio")) {
				reg.emplace<AudioSource>(selected, "jump");
			}
			if (!reg.has<Velocity>(selected) && ImGui::Button("Add Velocity")) {
				reg.emplace<Velocity>(selected);
			}
			if (!reg.has<SpriteComponent>(selected) && ImGui::Button("Add Sprite")) {
				reg.emplace<SpriteComponent>(selected, "player", 64.0f, 64.0f);
			}
			if (!reg.has<BillboardComponent>(selected) && ImGui::Button("Add Billboard")) {
				reg.emplace<BillboardComponent>(selected, "star");
			}
			if (!reg.has<PlayerInput>(selected) && ImGui::Button("Add Input")) {
				reg.emplace<PlayerInput>(selected);
			}
			if (!reg.has<Lifetime>(selected) && ImGui::Button("Add Lifetime")) {
				reg.emplace<Lifetime>(selected, 5.0f);
			}
			// ... 他のコンポーネント
		}

		ImGui::End();
	}
};

#endif // !___CREATOR_WINDOW_H___