#include "Anima.h"
#include <fstream>
#include <cmath>

void Anima::Initialize(Object3d* targetObject)
{
    target_ = targetObject;
}

void Anima::Update()
{
    if (!isPlaying_ || !target_ || isAnimationFinished_) return;

    timer_ += 0.016f;

    if (currentMove_.duration > 0.0f && timer_ >= currentMove_.duration) {
        if (!currentMove_.isLoop) {
            isAnimationFinished_ = true;
        } else {
            timer_ = 0.0f;
        }
    }

    const auto& instances = target_->GetModelInstances();
    for (const auto& instance : instances) {
        if (currentMove_.moveFunction) {
            currentMove_.moveFunction(*instance);
        }
    }
}

bool Anima::LoadAnimationsFromJSON(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    try {
        json jsonData;
        file >> jsonData;

        if (jsonData.contains("animations") && jsonData["animations"].is_array()) {
            for (const auto& animConfig : jsonData["animations"]) {
                // JSON から AnimeMove を生成
                AnimeMove move = CreateAnimationFromJSON(animConfig);
                // 必要に応じて animationMap_ に追加
            }
            return true;
        }
    } catch (const std::exception& e) {
        // エラーハンドリング
        return false;
    }

    return false;
}

Anima::AnimeMove Anima::CreateAnimationFromJSON(const json& animConfig)
{
    AnimeMove move;
    
    if (animConfig.contains("duration")) {
        move.duration = animConfig["duration"].get<float>();
    }
    
    if (animConfig.contains("isLoop")) {
        move.isLoop = animConfig["isLoop"].get<bool>();
    }

    // アニメーションタイプに応じた moveFunction を設定
    std::string animType = animConfig.value("type", "default");
    
    if (animType == "bob") {
        // 上下に動くアニメーション
        move.moveFunction = [animConfig](Object3d::ModelInstance& instance) {
            float amplitude = animConfig.value("amplitude", 0.2f);
            float frequency = animConfig.value("frequency", 2.0f);
            float t = std::sin(frequency * 2.0f) * 0.5f + 0.5f;
            instance.transform.translate.y = amplitude * t;
        };
    } 
    else if (animType == "rotate") {
        // 回転アニメーション
        move.moveFunction = [animConfig](Object3d::ModelInstance& instance) {
            float angle = animConfig.value("angle", 0.5f);
            instance.transform.rotate.w = std::cos(angle / 2.0f);
            instance.transform.rotate.x = std::sin(angle / 2.0f);
        };
    }
    else {
        // デフォルト
        move.moveFunction = [](Object3d::ModelInstance& instance) {
            instance.transform = {
                {1.0f, 1.0f, 1.0f},
                {0.0f, 0.0f, 0.0f, 1.0f},
                {0.0f, 0.0f, 0.0f}
            };
        };
    }

    return move;
}