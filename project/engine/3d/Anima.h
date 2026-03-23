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

        bool isLoop = true; // ループするかどうか
        bool sTransitioning = false; // 別のアニメーションに遷移中かどうか
        bool isFinished = false; // アニメーションが終了したかどうか
        float duration = -1.0f; // アニメーション期間（-1なら無制限）


    };



    // アニメーションさせる対象を登録
    void Initialize(Object3d* targetObject);

    // 毎フレーム更新
    void Update();

    void SetCurrentMove(const AnimeMove& move) {
        currentMove_ = move;
        isAnimationFinished_ = false; // リセット
    }

    bool IsAnimationFinished() const { return isAnimationFinished_; }
    bool IsAnimationPlaying() const { return isPlaying_ && !isAnimationFinished_; }
    float GetTimer() const { return timer_; } // タイマーの値を取得

    // アニメーション再生開始
    void Play() { isPlaying_ = true; isAnimationFinished_ = false; }
    // アニメーション停止
    void Stop() { isPlaying_ = false; }
    
    // アニメーションをリセット（一回きりアニメーション再生用）
    void ResetAnimation() {
        timer_ = 0.0f;
        isAnimationFinished_ = false;
        isPlaying_ = false;
    }

private:

    Object3d* target_ = nullptr;
    float timer_ = 0.0f;
    bool isPlaying_ = false;
    AnimeMove currentMove_;

    // 特定のパーツ（インスタンス）への参照をキャッシュしておくと便利です
    //インスタンスを登録する関数
    void RegisterModelInstance(const std::string& name, Object3d::ModelInstance* instance);

    std::vector<std::unique_ptr<Object3d::ModelInstance>> modelInstances_; // モデルインスタンスのリスト
    bool isAnimationFinished_ = false;  // アニメーション終了フラグ
  /*  Object3d::ModelInstance* leftArm_ = nullptr;
    Object3d::ModelInstance* rightArm_ = nullptr;*/
};