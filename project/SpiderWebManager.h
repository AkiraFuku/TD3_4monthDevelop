#pragma once

#include "SpiderWebRenderer.h"
#include <Vector3.h>
#include <memory>
#include <vector>

class Camera;

struct SpiderWebData {
    Vector3 position;
    float scale;
};

class SpiderWebManager {
public:
    void Initialize(int maxWebs, Camera* camera);
    void Update();
    void Draw();

    // 蜘蛛の巣を追加する（大きさを指定可能）
    void AddWeb(const Vector3& pos, float scale = 1.0f);
    void ClearWebs();

private:
    std::vector<SpiderWebData> webs_;
    std::unique_ptr<SpiderWebRenderer> renderer_;
    Camera* camera_ = nullptr;
    int maxWebs_ = 0;
};