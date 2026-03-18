#pragma once
#include "Object3d.h"
#include <vector>
#include <memory>

#include <functional>

class Anima {
public:

    struct AnimeMove
    {
        std::function<void(Object3d::ModelInstance&)> moveFunction;


    };



    // アニメーションさせる対象を登録
    void Initialize(Object3d* targetObject);

    // 毎フレーム更新
    void Update();

    void SetCurrentMove(const AnimeMove& move) {
        currentMove_ = move;
    }

    // アニメーションの再生制御
    void Play() { isPlaying_ = true; }
    void Stop() { isPlaying_ = false; }

private:

    Object3d* target_ = nullptr;
    float timer_ = 0.0f;
    bool isPlaying_ = false;
    AnimeMove currentMove_;

    // 特定のパーツ（インスタンス）への参照をキャッシュしておくと便利です
    //インスタンスを登録する関数
    void RegisterModelInstance(const std::string& name, Object3d::ModelInstance* instance);

    std::vector<std::unique_ptr<Object3d::ModelInstance>> modelInstances_; // モデルインスタンスのリスト
  /*  Object3d::ModelInstance* leftArm_ = nullptr;
    Object3d::ModelInstance* rightArm_ = nullptr;*/
};