/*****************************************************************//**
 * @file	SceneGame.cpp
 * @brief	ゲームシーン
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/23	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

// ===== インクルード =====
#include "Scene/SceneGame.h"
#include "Scene/SceneManager.h"
#include <string>

void SceneGame::Initialize()
{
	// 親クラスの初期化
	IScene::Initialize();

	// システム追加
	m_world.registerSystem<MovementSystem>();
	m_world.registerSystem<CollisionSystem>();

	// Entityの生成
	// Camera
	m_world.create_entity()
		.add<Tag>("MainCamera")
		.add<Transform>(XMFLOAT3(10.0f, 10.0f, -10.0f))
		.add<Camera>();

	// Player
	m_world.create_entity()
		.add<Tag>("Player")
		.add<Transform>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<Velocity>(XMFLOAT3(0.05f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f));

	// Enemy
	m_world.create_entity()
		.add<Tag>("Enemy")
		.add<Transform>(XMFLOAT3(5.0f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f));
}

void SceneGame::Finalize()
{
}

void SceneGame::Update()
{
	IScene::Update();
}

void SceneGame::Render()
{
	IScene::Render();
}

void SceneGame::OnInspector()
{
	ImGui::Begin("Game Inspector");

	// ECSの状況を表示（デバッグ用）
	ImGui::Text("Entities in World");
	ImGui::Separator();

	Registry& reg = m_world.getRegistry();

	// Transformを持つ全てのEntityを表示
	reg.view<Transform>([&](Entity e, Transform& trans) {
		// Tagを持っていれば名前を表示, 無ければID
		if (reg.has<Tag>(e))
		{
			ImGui::Text("ID: %d [%s]", e, reg.get<Tag>(e).name);
		}
		else
		{
			ImGui::Text("ID: %d", e);
		}

		// 座標をImGuiで動的に変更
		std::string label = "Pos##" + std::to_string(e);
		ImGui::DragFloat3(label.c_str(), &trans.position.x, 0.1f);
	});

	ImGui::Separator();
	if (ImGui::Button("Back to Title"))
	{
		SceneManager::ChangeScene(SceneType::Title);
	}

	ImGui::End();
}