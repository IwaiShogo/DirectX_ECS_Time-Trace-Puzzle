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
#include "Core/Prefab.h"
#include "ImGuizmo.h"
#include <DirectXMath.h>
#include <string>

// ヘルパー: XMMATRIX -> float[16]
void MatrixToFloat16(const DirectX::XMMATRIX& m, float* out) {
	DirectX::XMStoreFloat4x4((DirectX::XMFLOAT4X4*)out, m);
}

// ヘルパー: float[16] -> XMMATRIX, Decompose
void Float16ToTransform(const float* in, Transform& t) {
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

void SceneGame::Initialize()
{
	// 親クラスの初期化
	IScene::Initialize();

	// システム追加
	m_world.registerSystem<MovementSystem>();
	m_world.registerSystem<CollisionSystem>();
	auto inputSys = m_world.registerSystem<InputSystem>();
	inputSys->SetContext(m_context);
	m_world.registerSystem<LifetimeSystem>();

	// Entityの生成
	// Camera
	m_world.create_entity()
		.add<Tag>("MainCamera")
		.add<Transform>(XMFLOAT3(2.0f, 10.0f, -10.0f), XMFLOAT3(0.78f, 0.0f, 0.0f))
		.add<Camera>()
		.add<AudioListener>();

	// Player
	m_world.create_entity()
		.add<Tag>("Player")
		.add<Transform>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<Velocity>(XMFLOAT3(0.0f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f))
		.add<PlayerInput>()
		.add<MeshComponent>("hero", XMFLOAT3(0.1f, 0.1f, 0.1f));

	// Enemy
	m_world.create_entity()
		.add<Tag>("Enemy")
		.add<Transform>(XMFLOAT3(5.0f, 0.0f, 0.0f))
		.add<BoxCollider>(XMFLOAT3(1.0f, 1.0f, 1.0f));

	// UI
	m_world.create_entity()
		.add<Tag>("UI")
		.add<Transform>(XMFLOAT3(50.0f, 50.0f, 0.0f))
		.add<SpriteComponent>("test", 64.0f, 64.0f);
}

void SceneGame::Finalize()
{
}

void SceneGame::Update()
{
	IScene::Update();

	if (Input::GetButtonDown(Button::A))
	{
		XMFLOAT3 playerPos = { 0, 0, 0 };
		m_world.getRegistry().view<Tag, Transform>([&](Entity e, Tag& tag, Transform& t)
			{
				if (strcmp(tag.name, "Player") == 0)
				{
					Prefab::CreateSoundEffect(m_world, "jump", t.position, 1.0f, 30.0f);

					Logger::Log("Spawned Jump Sound!");
				}
			});
	}

	if (m_context && m_context->debug.useDebugCamera) {

		Registry& reg = m_world.getRegistry();

		// MainCameraを探す
		reg.view<Tag, Transform, Camera>([&](Entity e, Tag& tag, Transform& t, Camera& c) {
			if (strcmp(tag.name, "MainCamera") == 0) {

				// ------------------------------------------------
				// 1. マウス回転 (右クリック中のみ)
				// ------------------------------------------------
				if (Input::GetMouseRightButton()) {
					float rotSpeed = 0.005f; // 感度

					// X移動 -> Y軸回転(Yaw)
					t.rotation.y += Input::GetMouseDeltaX() * rotSpeed;
					// Y移動 -> X軸回転(Pitch)
					t.rotation.x += Input::GetMouseDeltaY() * rotSpeed;
				}

				// ------------------------------------------------
				// 2. カメラ視点での移動 (WASD)
				// ------------------------------------------------
				float moveSpeed = 10.0f * Time::DeltaTime();

				// 左Shiftで倍速
				if (Input::GetKey(VK_SHIFT)) moveSpeed *= 3.0f;

				// 現在の回転から、前方・右方ベクトルを計算
				XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(t.rotation.x, t.rotation.y, 0.0f);

				XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotationMatrix);
				XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotationMatrix);
				// 上昇下降用に上ベクトル
				XMVECTOR up = XMVectorSet(0, 1, 0, 0); // ワールドY軸

				XMVECTOR pos = XMLoadFloat3(&t.position);

				// W/S (前後)
				if (Input::GetKey('W')) pos += forward * moveSpeed;
				if (Input::GetKey('S')) pos -= forward * moveSpeed;

				// D/A (右左)
				if (Input::GetKey('D')) pos += right * moveSpeed;
				if (Input::GetKey('A')) pos -= right * moveSpeed;

				// E/Q (上昇下降: Unityスタイル)
				if (Input::GetKey('E')) pos += up * moveSpeed;
				if (Input::GetKey('Q')) pos -= up * moveSpeed;

				// 結果を保存
				XMStoreFloat3(&t.position, pos);
			}
			});
	}

#ifdef _DEBUG
	// マウス左クリックでピッキング
	if (m_context->debug.enableMousePicking && Input::GetMouseLeftButton()) { // ※マウス用ボタン定数をInputに追加するか、GetMouseLeftButtonを作る
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
			m_world.getRegistry().view<Camera, Transform>([&](Entity e, Camera& c, Transform& t) {
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
				Entity hit = CollisionSystem::Raycast(m_world.getRegistry(), origin, dir, dist);

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
#endif
}

void SceneGame::Render()
{
	IScene::Render();
}

// 再帰的にツリーを表示する関数
void DrawEntityNode(Registry& reg, Entity e, Entity& selected)
{
	Tag& tag = reg.get<Tag>(e);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (e == selected) flags |= ImGuiTreeNodeFlags_Selected;

	// 子がいるかチェック
	bool hasChildren = false;
	if (reg.has<Relationship>(e)) {
		if (!reg.get<Relationship>(e).children.empty()) hasChildren = true;
	}
	if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;

	// ノード描画
	bool opened = ImGui::TreeNodeEx((void*)(uint64_t)e, flags, "%s (ID:%d)", tag.name, e);

	if (ImGui::IsItemClicked()) {
		selected = e;
	}

	// 子要素の描画
	if (opened) {
		if (hasChildren) {
			for (Entity child : reg.get<Relationship>(e).children) {
				DrawEntityNode(reg, child, selected);
			}
		}
		ImGui::TreePop();
	}
}

void SceneGame::OnInspector()
{
#ifdef _DEBUG
	// メインのInspectorウィンドウ
	ImGui::Begin("Scene Inspector");

	Registry& reg = m_world.getRegistry();

	// --------------------------------------------------------
	// 1. システム稼働状況 (System Monitor)
	// --------------------------------------------------------
	if (ImGui::CollapsingHeader("System Monitor", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Columns(2, "systems"); // 2列表示
		ImGui::Text("System Name"); ImGui::NextColumn();
		ImGui::Text("Time (ms)"); ImGui::NextColumn();
		ImGui::Separator();

		for (const auto& sys : m_world.getSystems())
		{
			ImGui::Text("%s", sys->m_systemName.c_str());
			ImGui::NextColumn();

			// 処理時間をバーで表示 (最大16ms = 60fps基準)
			float fraction = (float)sys->m_lastExecutionTime / 16.0f;
			char buf[32];
			sprintf_s(buf, "%.3f ms", sys->m_lastExecutionTime);

			// 色分け (緑 -> 黄 -> 赤)
			ImVec4 col = ImVec4(0, 1, 0, 1);
			if (sys->m_lastExecutionTime > 2.0) col = ImVec4(1, 1, 0, 1);
			if (sys->m_lastExecutionTime > 5.0) col = ImVec4(1, 0, 0, 1);

			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
			ImGui::ProgressBar(fraction, ImVec2(-1, 0), buf);
			ImGui::PopStyleColor();

			ImGui::NextColumn();
		}
		ImGui::Columns(1); // 列解除
	}

	// --------------------------------------------------------
	// 2. エンティティリスト (Entity Inspector)
	// --------------------------------------------------------
	if (ImGui::CollapsingHeader("Entity List", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 全Entityを列挙するために Tag コンポーネントで回す
		// (Tagを持たないEntityは表示されない制限がありますが、今は全て持っているのでOK)
		int count = 0;
		reg.view<Tag>([&](Entity e, Tag& tag)
			{
			count++;

			// ツリーノードで表示
			// IDと名前を表示
			std::string label = std::to_string(e) + ": " + tag.name;
			if (ImGui::TreeNode(label.c_str()))
			{

				// --- コンポーネント情報の詳細表示 ---
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 1.0f, 1.0f), "[Components]");

				// Transform
				if (reg.has<Transform>(e))
				{
					Transform& t = reg.get<Transform>(e);
					if (ImGui::TreeNode("Transform"))
					{
						ImGui::DragFloat3("Position", &t.position.x, 0.1f);
						ImGui::DragFloat3("Rotation", &t.rotation.x, 1.0f);
						ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
						ImGui::TreePop();
					}
				}

				// Velocity
				if (reg.has<Velocity>(e))
				{
					Velocity& v = reg.get<Velocity>(e);
					ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", v.velocity.x, v.velocity.y, v.velocity.z);
				}

				// BoxCollider
				if (reg.has<BoxCollider>(e))
				{
					BoxCollider& b = reg.get<BoxCollider>(e);
					if (ImGui::TreeNode("BoxCollider"))
					{
						ImGui::DragFloat3("Size", &b.size.x, 0.1f);
						ImGui::DragFloat3("Offset", &b.offset.x, 0.1f);
						ImGui::TreePop();
					}
				}

				// PlayerInput
				if (reg.has<PlayerInput>(e))
				{
					PlayerInput& p = reg.get<PlayerInput>(e);
					ImGui::Text("Input: Speed=%.1f, Jump=%.1f", p.speed, p.jumpPower);
				}

				// 削除ボタン
				if (ImGui::Button("Destroy"))
				{
					// 削除処理（簡易実装：Tagを消すことでリストから見えなくする）
					// 本来は m_world.destroy(e) のようなメソッドが必要
					reg.remove<Tag>(e);
				}

				ImGui::TreePop();
			}
			});

		ImGui::Separator();
		ImGui::Text("Total Entities: %d", count);
	}

	// --------------------------------------------------------
	// 3. プレイヤー詳細情報 (Player Watcher)
	// --------------------------------------------------------
	// Tagが"Player"であるものを探す
	bool playerFound = false;
	reg.view<Tag>([&](Entity e, Tag& tag)
		{
			if (!playerFound && strcmp(tag.name, "Player") == 0)
			{
				playerFound = true;

				if (ImGui::CollapsingHeader("Player Watcher", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Text("ID: %d", e);

					// 座標
					Transform& t = reg.get<Transform>(e);
					ImGui::Text("Pos: (%.2f, %.2f, %.2f)", t.position.x, t.position.y, t.position.z);

					// 速度
					Velocity& v = reg.get<Velocity>(e);
					ImGui::Text("Vel: (%.2f, %.2f, %.2f)", v.velocity.x, v.velocity.y, v.velocity.z);
					// スピードメーター（長さ）
					float speed = std::sqrt(v.velocity.x * v.velocity.x + v.velocity.z * v.velocity.z);
					ImGui::ProgressBar(speed / 10.0f, ImVec2(0, 0), "Speed");

					// 接地判定 (簡易)
					bool isGrounded = (t.position.y <= 0.001f);
					ImGui::Text("Grounded: %s", isGrounded ? "YES" : "NO");
				}
			}
		});

	ImGui::End();

	// 2. 選択されたEntityの詳細インスペクター
	// Entityが存在し、かつ無効になっていないかチェック
	if (m_selectedEntity != NullEntity && m_world.getRegistry().has<Tag>(m_selectedEntity))
	{
		ImGui::Begin("Selected Entity Inspector");

		Registry& reg = m_world.getRegistry();

		// --- ヘッダー情報 (ID & Tag) ---
		Tag& tag = reg.get<Tag>(m_selectedEntity);
		ImGui::Text("ID: %d", m_selectedEntity);

		// 名前を編集できるようにする (char配列のバッファが必要ですが、簡易的に表示のみか、std::string対応が必要)
		// ここでは表示のみにします
		ImGui::LabelText("Name", "%s", tag.name);

		ImGui::Separator();

		// --- Transform (位置・回転・拡大縮小) ---
		if (reg.has<Transform>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Transform& t = reg.get<Transform>(m_selectedEntity);

				// Position
				ImGui::DragFloat3("Position", &t.position.x, 0.1f);

				// Rotation (ラジアン <-> 度数法の変換を入れると親切ですが、ここでは生の値を表示)
				// 度数法で扱いたい場合は: float deg[3] = { XMConvertToDegrees(t.rotation.x), ... };
				ImGui::DragFloat3("Rotation (Rad)", &t.rotation.x, 0.1f);

				// Scale
				ImGui::DragFloat3("Scale", &t.scale.x, 0.1f);
			}
		}

		// --- Mesh (見た目) ---
		if (reg.has<MeshComponent>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
			{
				MeshComponent& m = reg.get<MeshComponent>(m_selectedEntity);

				ImGui::Text("Model Key: %s", m.modelKey.c_str());
				ImGui::ColorEdit4("Color", &m.color.x); // 色変更
				ImGui::DragFloat3("Scale Offset", &m.scaleOffset.x, 0.01f);
			}
		}

		// --- BoxCollider (当たり判定) ---
		if (reg.has<BoxCollider>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_DefaultOpen))
			{
				BoxCollider& b = reg.get<BoxCollider>(m_selectedEntity);
				ImGui::DragFloat3("Size", &b.size.x, 0.1f);
				ImGui::DragFloat3("Offset", &b.offset.x, 0.1f);
			}
		}

		// --- Velocity (物理・移動) ---
		if (reg.has<Velocity>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Velocity", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Velocity& v = reg.get<Velocity>(m_selectedEntity);
				ImGui::DragFloat3("Vector", &v.velocity.x, 0.1f);

				// 停止ボタン
				if (ImGui::Button("Stop")) {
					v.velocity = { 0, 0, 0 };
				}
			}
		}

		// --- PlayerInput (操作パラメータ) ---
		if (reg.has<PlayerInput>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Player Input", ImGuiTreeNodeFlags_DefaultOpen))
			{
				PlayerInput& p = reg.get<PlayerInput>(m_selectedEntity);
				ImGui::DragFloat("Walk Speed", &p.speed, 0.1f);
				ImGui::DragFloat("Jump Power", &p.jumpPower, 0.1f);
			}
		}

		// --- AudioSource (音源) ---
		if (reg.has<AudioSource>(m_selectedEntity))
		{
			if (ImGui::CollapsingHeader("Audio Source", ImGuiTreeNodeFlags_DefaultOpen))
			{
				AudioSource& a = reg.get<AudioSource>(m_selectedEntity);
				ImGui::Text("Sound Key: %s", a.soundKey.c_str());
				ImGui::SliderFloat("Volume", &a.volume, 0.0f, 1.0f);
				ImGui::DragFloat("Range", &a.range, 0.1f);
				ImGui::Checkbox("Loop", &a.isLoop);

				if (ImGui::Button("Play Test")) {
					// その場で再生テスト (3D再生)
					// プレイヤー位置（リスナー）が必要ですが、簡易的に2D再生で確認
					AudioManager::Instance().PlaySE(a.soundKey, a.volume);
				}
			}
		}

		ImGui::Separator();

		// --- 削除機能 ---
		// 赤いボタンにする
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button("Destroy Entity", ImVec2(-1, 30))) // 幅いっぱい
		{
			// 削除実行
			reg.destroy(m_selectedEntity);
			m_selectedEntity = NullEntity; // 選択解除
		}
		ImGui::PopStyleColor();

		// --- 選択解除 ---
		if (ImGui::Button("Deselect", ImVec2(-1, 0)))
		{
			m_selectedEntity = NullEntity;
		}

		ImGui::End();
	}

	// ------------------------------------------------------------
	// Hierarchy Window
	// ------------------------------------------------------------
	ImGui::Begin("Hierarchy");

	// 親を持たない（ルート）Entityだけを起点に描画する
	reg.view<Tag>([&](Entity e, Tag& tag) {
		bool isRoot = true;
		if (reg.has<Relationship>(e)) {
			if (reg.get<Relationship>(e).parent != NullEntity) isRoot = false;
		}

		if (isRoot) {
			DrawEntityNode(reg, e, m_selectedEntity);
		}
		});

	ImGui::End();

	// --- ギズモ処理 ---
	// 選択中のEntityがあり、Transformを持っている場合
	if (m_selectedEntity != NullEntity && m_world.getRegistry().has<Transform>(m_selectedEntity)) {

		ImGuizmo::BeginFrame();

		// カメラ情報の取得
		XMMATRIX view = XMMatrixIdentity();
		XMMATRIX proj = XMMatrixIdentity();

		// メインカメラを探して計算
		m_world.getRegistry().view<Tag, Camera, Transform>([&](Entity e, Tag& tag, Camera& cam, Transform& t) {
			if (strcmp(tag.name, "MainCamera") == 0) {

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
		Transform& t = m_world.getRegistry().get<Transform>(m_selectedEntity);

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
#endif // _DEBUG
}