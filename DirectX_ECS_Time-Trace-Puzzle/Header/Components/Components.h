/*****************************************************************//**
 * @file	Components.h
 * @brief	基本的なコンポーネント
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

#ifndef ___COMPONENTS_H___
#define ___COMPONENTS_H___

// ===== インクルード =====
#include <DirectXMath.h>
#include "main.h"

using namespace DirectX;

// 親子関係
struct Relationship
{
	Entity parent = NullEntity;
	std::vector<Entity> children;
};

/**
 * @struct	Transform
 * @brief	位置・回転・スケール
 */
struct Transform
{
	XMFLOAT3 position;	// x, y, z
	XMFLOAT3 rotation;	// pitch, yaw, roll (Euler angles in degrees or radians)
	XMFLOAT3 scale;		// x, y, z

	Transform(XMFLOAT3 p = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 r = { 0.0f, 0.0f, 0.0f }, XMFLOAT3 s = { 1.0f, 1.0f, 1.0f })
		: position(p), rotation(r), scale(s) {}
};

/**
 * @struct	Velocity
 * @brief	速度（物理挙動用）
 */
struct Velocity
{
	XMFLOAT3 velocity;

	Velocity(XMFLOAT3 v = { 0.0f, 0.0f, 0.0f })
		: velocity(v) {}
};

/**
 * @struct	Tag
 * @brief	名前
 */
struct Tag
{
	const char* name;

	Tag(const char* n = "Entity")
		: name(n) {}
};

/**
 * @struct	Camera
 * @brief	カメラ
 */
struct Camera
{
	float fov;			// 視野角（Radian）
	float nearZ, farZ;	// 視錐台の範囲
	float aspect;		// アスペクト比

	// コンストラクタ
	Camera(float f = XM_PIDIV4, float n = 0.1f, float r = 1000.0f, float a = static_cast<float>(Config::SCREEN_WIDTH) / static_cast<float>(Config::SCREEN_HEIGHT))
		: fov(f), nearZ(n), farZ(r), aspect(a) {}
};

/**
 * @struct	BoxCollider
 * @brief	ボックスの当たり判定
 */
struct BoxCollider
{
	XMFLOAT3 size;		// 幅・高さ・奥行
	XMFLOAT3 offset;	// 中心からのオフセット

	BoxCollider(XMFLOAT3 s = { 1.0f, 1.0f,1.0f }, XMFLOAT3 o = { 0.0f, 0.0f, 0.0f })
		: size(s), offset(o) {}
};

/**
 * @struct	PlayerInput
 * @brief	操作可能フラグ
 */
struct PlayerInput
{
	float speed;
	float jumpPower;

	PlayerInput(float s = 3.0f, float j = 5.0f)
		: speed(s), jumpPower(j) {}
};

/**
 * @struct	GridPosition
 * @brief	論理的なグリッド描画（整数）
 */
struct GridPosition
{
	int x, z;

	GridPosition(int _x = 0, int _z = 0)
		: x(_x), z(_z) {}
};

struct GridMoveState
{
	bool isMoving = false;	// 移動中か？
	XMFLOAT3 startPos;		// 移動開始時のワールド座標
	XMFLOAT3 targetPos;		// 目的地のワールド座標
	float progress = 0.0f;	// 進捗（0.0 ~ 1.0）
	float moveSpeed = 4.0f;	// 移動アニメーション速度

	GridMoveState(float speed = 4.0f)
		: isMoving(false), startPos(0, 0, 0), targetPos(0, 0, 0), progress(0.0f), moveSpeed(speed) {}
};

/**
 * @struct	SpriteComponent
 * @brief	2D描画
 */
struct SpriteComponent
{
	std::string textureKey;	// ResourceManagerで使うキー
	float width, height;	// 描画サイズ（ピクセル）
	XMFLOAT4 color;			// 色と透明度
	XMFLOAT2 pivot;			// 中心点（0.0 ~ 1.0）デフォルトは左上（0, 0）
	
	SpriteComponent(const std::string& key, float w, float h, const XMFLOAT4& c = { 1, 1, 1, 1 }, const XMFLOAT2& p = { 0, 0 })
		: textureKey(key), width(w), height(h), color(c), pivot(p) {}
};

/**
 * @struct	MeshComponent
 * @brief	モデル描画
 */
struct MeshComponent
{
	std::string modelKey;	// ResourceManagerのキー
	XMFLOAT3 scaleOffset;	// モデル固有のスケール補正（アセットが巨大/極小な場合用）
	XMFLOAT4 color;			// マテリアルカラー乗算用

	MeshComponent(const std::string& key,
				  const XMFLOAT3& scale = { 1.0f, 1.0f, 1.0f },
				  const XMFLOAT4& c = { 1.0f, 1.0f, 1.0f, 1.0f})
		: modelKey(key), scaleOffset(scale), color(c) {}
};

/**
 * @struct	Audiosource
 * @brief	音源（鳴らす側）
 */
struct AudioSource
{
	std::string soundKey;	// ResourceManagerのキー
	float volume;			// 基本音量
	float range;			// 音が聞こえる最大距離（3Dサウンド用）
	bool isLoop;			// ループするか
	bool playOnAwake;		// 生成時に即再生するか

	// 内部状態管理用
	bool isPlaying = false;

	AudioSource(const std::string& key,
				float vol = 1.0f,
				float r = 20.0f,
				bool loop = false,
				bool awake = false)
		: soundKey(key), volume(vol), range(r), isLoop(loop), playOnAwake(awake) {}
};

/**
 * @struct	AudioListener
 * @brief	聞く側（通常はカメラかプレイヤーに1つだけつける）
 */
struct AudioListener
{
	// データは不要、タグとして機能する。
};

/**
 * @struct	Lifetime
 * @brief	寿命（秒）
 */
struct Lifetime
{
	float time;	// 残り時間

	Lifetime(float t)
		: time(t) {}
};

/**
 * @struct	BillboardComponent
 * @brief	ビルボード
 */
struct BillboardComponent
{
	std::string textureKey;
	XMFLOAT2 size;	// 幅、高さ
	XMFLOAT4 color;

	BillboardComponent(const std::string& key, float w = 1.0f, float h = 1.0f, const XMFLOAT4& c = { 1, 1, 1, 1 })
		: textureKey(key), size(w, h), color(c) {}
};

#endif // !___COMPONENTS_H___