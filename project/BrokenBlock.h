#pragma once
#pragma once
#include "Vector3.h"
#include "Object3d.h"
#include <memory>
#include <set>
#include "Audio.h"
#include "DrawFunction.h"
#include <vector>

class BrokenBlock
{
public:
    void Initialize(const Vector3& pos, float width, float depth);

    void Update();
    void Draw();

    bool IsInside(const Vector3& pos) const;

    void CheckRiding(const Vector3& pos, const void* entityPtr);
    bool IsRider(const void* entityPtr) const;

    // ゲッター
    Vector3 GetPosition() const { return position_; }
    bool IsBroken() const { return isBroken_; }
    bool IsImpassable() const { return isImpassable_; }
    // setter
    void SetIsBroken(bool isBroken) { isBroken_ = isBroken; }
    void SetIsImpassable(bool isIm) { isImpassable_ = isIm; }

    AABB GetAABB() const;

private:
    Vector3 position_;

    // キャラクターの当たり判定サイズ
    float width = 1.6f;
    float height = 1.6f;

    // 3Dオブジェクト
    std::vector<std::unique_ptr <Object3d>> objects_;

    // 破壊フラグ
    bool isBroken_ = false;
    bool isImpassable_ = false;
    // 壊れるまでの回数
    int maxbreakCount_ = 2;
    int currentCount_ = 0;
    // 乗っているキャラのリスト
    std::set<const void*> riders_;

    // サウンド
    Audio::SoundHandle broken_ = 0;
};