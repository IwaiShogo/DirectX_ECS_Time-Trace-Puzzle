/*****************************************************************//**
 * @file	GizmoSystem.h
 * @brief	ギズモの描画ロジック
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

#ifndef ___GIZMO_SYSTEM_H___
#define ___GIZMO_SYSTEM_H___

#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Core/Input.h"
#include "Core/Context.h" // Config::SCREEN_WIDTH等用
#include "main.h"
#include "ImGuizmo.h"
#include <DirectXMath.h>

class GizmoSystem
{
public:
	static void Draw(Registry& reg, Entity& selected)
	{
		// --- ギズモ処理 ---
		// 選択中のEntityがあり、Transformを持っている場合
		if (selected != NullEntity && reg.has<Transform>(selected)) {

			ImGuizmo::BeginFrame();

			// カメラ情報の取得
			XMMATRIX view = XMMatrixIdentity();
			XMMATRIX proj = XMMatrixIdentity();

			// メインカメラを探して計算
			reg.view<Tag, Camera, Transform>([&](Entity e, Tag& tag, Camera& cam, Transform& t) {
				if (tag.name == "MainCamera") {

					// 1. ビュー行列 (LookToLH)
					XMVECTOR eye = XMLoadFloat3(&t.position);

					// 回転情報から向きを計算
					XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);
					XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
					XMVECTOR upDir = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotationMatrix);

					view = XMMatrixLookToLH(eye, lookDir, upDir);

					// 2. プロジェクション行列
					proj = XMMatrixPerspectiveFovLH(cam.fov, cam.aspect, cam.nearZ, cam.farZ);
				}
				});

			// ImGuizmoの設定
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

			// 行列をfloat配列に変換
			float viewM[16], projM[16];
			MatrixToFloat16(view, viewM);
			MatrixToFloat16(proj, projM);

			// 対象のTransformを取得
			Transform& t = reg.get<Transform>(selected);

			// Transform -> Matrix
			XMMATRIX worldMat = XMMatrixScaling(t.scale.x, t.scale.y, t.scale.z) *
				XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, t.rotation.z) *
				XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
			float worldM[16];
			MatrixToFloat16(worldMat, worldM);

			// 操作 (移動: TRANSLATE, 回転: ROTATE, 拡大: SCALE)
			// ※キーボードショートカットで切り替えられるようにすると便利 (W, E, Rなど)
			static ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (Input::GetKeyDown('1')) mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
			if (Input::GetKeyDown('2')) mCurrentGizmoOperation = ImGuizmo::ROTATE;
			if (Input::GetKeyDown('3')) mCurrentGizmoOperation = ImGuizmo::SCALE;

			// ギズモ描画と操作判定
			if (ImGuizmo::Manipulate(viewM, projM, mCurrentGizmoOperation, ImGuizmo::LOCAL, worldM)) {
				// 操作されたら値を書き戻す
				Float16ToTransform(worldM, t);
			}
		}

	}

private:
	// ヘルパー: XMMATRIX -> float[16]
	static void MatrixToFloat16(const DirectX::XMMATRIX& m, float* out) {
		DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)out, m);
	}

	// ヘルパー: float[16] -> XMMATRIX, Decompose
	static void Float16ToTransform(const float* in, Transform& t) {
		DirectX::XMFLOAT4X4 m;
		memcpy(&m, in, sizeof(float) * 16);
		DirectX::XMMATRIX mat = DirectX::XMLoadFloat4x4(&m);

		DirectX::XMVECTOR scale, rotQuat, trans;
		DirectX::XMMatrixDecompose(&scale, &rotQuat, &trans, mat);

		DirectX::XMStoreFloat3(&t.position, trans);
		DirectX::XMStoreFloat3(&t.scale, scale);

		// クォータニオンからオイラー角への変換は少し複雑ですが、
		// 簡易的にImGuizmoの機能を使って分解します
		float translation[3], rotation[3], scale_f[3];
		ImGuizmo::DecomposeMatrixToComponents(in, translation, rotation, scale_f);

		// ImGuizmoは度数法(Degrees)で返すので、ラジアンに戻す
		t.rotation.x = DirectX::XMConvertToRadians(rotation[0]);
		t.rotation.y = DirectX::XMConvertToRadians(rotation[1]);
		t.rotation.z = DirectX::XMConvertToRadians(rotation[2]);
	}
};

#endif