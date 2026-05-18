#pragma once
#include "Vector4.h"
#include "ParticleManager.h"
#include "Transform.h"
class ParicleEmitter
{
public:
    /// <summary>
    /// パーティクル生成
    /// </summary>
    /// <param name="name">パーティクルグループ名</param>
    /// <param name="transfom">位置</param>
    /// <param name="count"></param>
    /// <param name="frequency"></param>
    /// <param name="frequencyTime"></param>
    ParicleEmitter(const  std::string name, EulerTransform transfom, uint32_t count, float frequency, float frequencyTime);
    void Update();
    void Emit();

      void SetTranslate(const Vector3& translate) {
        transform_.translate = translate;//位置

    };

private:
    EulerTransform transform_;//位置
    uint32_t count_;//パーティクル数
    float frequency_;//範囲
    float frequencyTime_;//発生時間
    std::string name_;
};

