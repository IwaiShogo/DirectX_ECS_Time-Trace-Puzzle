/*****************************************************************//**
 * @file	ModelRenderSystem.h
 * @brief	ModelRendererを使って描画を行うシステム
 * 
 * @details	
 * 
 * ------------------------------------------------------------
 * @author	Iwai Shogo
 * ------------------------------------------------------------
 * 
 * @date	2025/11/26	初回作成日
 * 			作業内容：	- 追加：
 * 
 * @update	2025/xx/xx	最終更新日
 * 			作業内容：	- XX：
 * 
 * @note	（省略可）
 *********************************************************************/

#ifndef ___MODEL_RENDER_SYSTEM_H___
#define ___MODEL_RENDER_SYSTEM_H___

// ===== インクルード =====
#include "ECS/ECS.h"
#include "Components/Components.h"
#include "Graphics/ModelRenderer.h"
#include "Core/ResourceManager.h"

class ModelRenderSystem
	: public ISystem
{
public:
	ModelRenderSystem(ModelRenderer* renderer)
		: m_renderer(renderer)
	{
		m_systemName = "Model Render System";
	}

	void Render(Registry& registry, const Context& context) override
	{
		if (!m_renderer) return;

		// 1. カメラ情報の取得
		XMMATRIX viewMatrix = XMMatrixIdentity();
		XMMATRIX projMatrix = XMMatrixIdentity();
		XMFLOAT3 lightDir = { 0.5f, -1.0f, 0.5f };
		bool cameraFound = false;

		registry.view<Camera, Transform>([&](Entity e, Camera& cam, Transform& trans)
			{
				if (cameraFound) return;
				XMVECTOR eye = XMLoadFloat3(&trans.position);
				XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(trans.rotation.x, trans.rotation.y, 0.0f);
				XMVECTOR lookDir = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
				XMVECTOR upDir = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rotationMatrix);

				viewMatrix = XMMatrixLookToLH(eye, lookDir, upDir);
				projMatrix = XMMatrixPerspectiveFovLH(cam.fov, cam.aspect, cam.nearZ, cam.farZ);
				cameraFound = true;
			});

		if (!cameraFound) return;

		// 2. 描画開始
		m_renderer->Begin(viewMatrix, projMatrix, lightDir);

		// 3. MeshComponentとTransformを持つEntityを描画
		registry.view<MeshComponent, Transform>([&](Entity e, MeshComponent& m, Transform& t)
			{
				auto model = ResourceManager::Instance().GetModel(m.modelKey);
				if (model)
				{
					// モデル固有のスケール補正 * Transformのスケール
					XMFLOAT3 finalScale = {
						t.scale.x * m.scaleOffset.x,
						t.scale.y * m.scaleOffset.y,
						t.scale.z * m.scaleOffset.z
					};

					// 描画
					m_renderer->Draw(model, t.position, finalScale, t.rotation);
				}
			});
	}

private:
	ModelRenderer* m_renderer;
};

#endif // !___MODEL_RENDER_SYSTEM_H___