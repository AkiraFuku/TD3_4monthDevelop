#pragma once

#include "Camera.h"
#include "Model.h"
#include "Object3D.h"
#include "DrawFunction.h"

class Player {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="pos">初期位置</param>
    void Initialize(const Vector3& pos);

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// 移動処理
    /// </summary>
    void UpdateMove();

    /// <summary>
    /// 移動距離確定
    /// </summary>
    void ResultMove();

public: // 外部入出力
    // ----- Getter -----

    // 位置
    Vector3 GetPosition() const { return object_->GetTranslate(); }
    void SetPosition(const Vector3& pos);

    // AABBを取得
    AABB GetAABB() const;

private:
    std::unique_ptr<Object3d> object_;

    Vector3 velocity_ = { 0.1f, 0.0f, 0.1f };

    Vector3 scale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 rotate_ = { 0.0f, 0.0f, 0.0f };
    Vector3 translate_ = { 0.0f, 0.0f, 0.0f };

    Vector3 moveVel_;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.6f;
    static inline const float kHeight = 1.6f;
};
