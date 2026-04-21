#pragma once

#include"Object3D.h"
#include "Model.h"
#include "Camera.h"

class Egg;
class Player;

class Goal
{
public:

    void Initialize(const Vector3& pos);

    void Finalize();

    void Update();

    void Draw();

    // 卵のポインタを取得
    void SetEgg(Egg* egg) {
        egg_ = egg;
    }

    // プレイヤーのポインタを取得
    void SetPlayer(Player* player) { player_ = player; }

    // ゴール判定関数
    void Clear();

    // ゴールに必要な素材数のsetter
    void SetNeedNestCount(int num) { needNestMaterialCount_ = num; }
    // getter
    int GetNeedNestCount() const { return needNestMaterialCount_; }

public: // 外部入出力


    void SetTranslate(const Vector3& translate) { object_->SetTranslate(translate); }


private:

    std::unique_ptr<Object3d> object_;

    Vector3 scale_ = { 1.0f,1.0f,1.0f };
    Vector3 rotate_ = { 0.0f,0.0f,0.0f };
    Vector3 translate_ = { 0.0f,0.0f,0.0f };

    // 卵のポインタ
    Egg* egg_ = nullptr;
    // プレイヤーのポインタ
    Player* player_ = nullptr;

    // ゴールに必要な素材の数
    int needNestMaterialCount_ = 0;
    // プレイヤーが持っている素材の数
    int playerNestMaterialCount_ = 0;

};

