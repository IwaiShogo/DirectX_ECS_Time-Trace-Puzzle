/*****************************************************************//**
 * @file	Editor.cpp
 * @brief	
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

// ===== インクルード =====
#include "Editor/Editor.h"
#include "Editor/HierarchyWindow.h"
#include "Editor/InspectorWindow.h"
#include "Editor/SystemWindow.h"
#include "Editor/GizmoSystem.h"
#include "Editor/CreatorWindow.h"
#include "Systems/CollisionSystem.h"

void Editor::Initialize()
{
	m_windows.push_back(std::make_unique<HierarchyWindow>());
	m_windows.push_back(std::make_unique<InspectorWindow>());
	m_windows.push_back(std::make_unique<SystemWindow>());
	m_windows.push_back(std::make_unique<CreatorWindow>());
}

void Editor::Draw(World& world, Context& ctx)
{	
	// マウス左クリックでピッキング
	if (ctx.debug.enableMousePicking && Input::GetMouseLeftButton()) { // ※マウス用ボタン定数をInputに追加するか、GetMouseLeftButtonを作る
		// 今回は簡易的に GetKey(VK_LBUTTON) でもOK
		if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !ImGui::GetIO().WantCaptureMouse) {

			// 1. マウス座標の取得
			POINT mousePos; GetCursorPos(&mousePos);
			ScreenToClient(GetActiveWindow(), &mousePos); // ウィンドウ内座標へ

			float w = (float)Config::SCREEN_WIDTH;
			float h = (float)Config::SCREEN_HEIGHT;

			// 2. NDC座標 (-1 ~ 1) への変換
			float ndcX = (2.0f * mousePos.x) / w - 1.0f;
			float ndcY = 1.0f - (2.0f * mousePos.y) / h;

			// 3. カメラ行列の取得 (RenderSystem等から取るのが正しいが、ここでも計算可)
			// ※デバッグカメラかメインカメラかで分岐する必要があります
			XMMATRIX view = XMMatrixIdentity();
			XMMATRIX proj = XMMatrixIdentity();
			XMVECTOR camPos = XMVectorZero();
			bool cameraFound = false;

			// 簡易的にメインカメラEntityを探して計算
			world.getRegistry().view<Camera, Transform>([&](Entity e, Camera& c, Transform& t) {
				if (cameraFound) return;

				camPos = XMLoadFloat3(&t.position);

				XMVECTOR eye = XMLoadFloat3(&t.position);
				// 回転行列を作成 (Pitch: X軸回転, Yaw: Y軸回転)
				// Transform.rotation.x を Pitch(上下)、y を Yaw(左右) として使います
				XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);
				// 前方ベクトル (0, 0, 1) を回転させる
				XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
				// 上方向ベクトル (0, 1, 0) を回転させる
				XMVECTOR upDir = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotationMatrix);

				// LookToLH: 位置、向き、上でビュー行列を作る
				view = XMMatrixLookToLH(eye, lookDir, upDir);
				proj = XMMatrixPerspectiveFovLH(c.fov, c.aspect, c.nearZ, c.farZ);
				cameraFound = true;
				});

			if (cameraFound)
			{
				// 4. レイの計算 (Unproject)
				XMVECTOR rayOrigin = camPos;
				XMVECTOR rayTarget = XMVector3Unproject(
					XMVectorSet(mousePos.x, mousePos.y, 1.0f, 0.0f), // Z=1 (Far)
					0, 0, w, h, 0.0f, 1.0f, // Viewport
					proj, view, XMMatrixIdentity()
				);
				XMVECTOR rayDir = XMVector3Normalize(rayTarget - rayOrigin);

				XMFLOAT3 origin, dir;
				XMStoreFloat3(&origin, rayOrigin);
				XMStoreFloat3(&dir, rayDir);

				// 5. レイキャスト実行
				float dist;
				Entity hit = CollisionSystem::Raycast(world.getRegistry(), origin, dir, dist);

				if (hit != NullEntity) {
					m_selectedEntity = hit;
					Logger::Log("Selected Entity ID: " + std::to_string(hit));
				}
				else {
					m_selectedEntity = NullEntity;
				}
			}
		}
	}

	// 各ウィンドウを描画
	for (auto& window : m_windows)
	{
		window->Draw(world, m_selectedEntity, ctx);
	}

	// デバッグUI
	ResourceManager::Instance().OnInspector();
	AudioManager::Instance().OnInspector();

	// ログウィンドウの描画
	Logger::Draw("Debug Logger");

	// ギズモ描画
	GizmoSystem::Draw(world.getRegistry(), m_selectedEntity);
}