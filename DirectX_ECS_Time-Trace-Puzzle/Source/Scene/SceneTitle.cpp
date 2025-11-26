/*****************************************************************//**
 * @file	SceneTitle.cpp
 * @brief	タイトルシーン
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
#include "Scene/SceneTitle.h"
#include "Scene/SceneManager.h"
#include <iostream>

void SceneTitle::Initialize()
{
	IScene::Initialize();
}

void SceneTitle::Finalize()
{
	std::cout << "Title Scene Finalized" << std::endl;
}

void SceneTitle::Update()
{
	if (Input::GetKeyDown(VK_SPACE))
	{
		SceneManager::ChangeScene(SceneType::Game);
	}
}

void SceneTitle::Render()
{
}

void SceneTitle::OnInspector()
{
#ifdef _DEBUG
	ImGui::Begin("Title Menu");
	ImGui::Text("Press Enter to Start");
	if (ImGui::Button("Go to Game Scene"))
	{
		SceneManager::ChangeScene(SceneType::Game);
	}
	ImGui::End();
#endif // _DEBUG
}