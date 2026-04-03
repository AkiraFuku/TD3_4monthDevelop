#pragma once
#include "Audio.h"
class SceneManager;
class Scene
{
public:
    virtual ~Scene() = default;
    virtual void Initialize() = 0;
    virtual void Finalize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    virtual void SetSceneManager(SceneManager* sceneManager) {
        sceneManager_ = sceneManager;
    }
    SceneManager*  GetSceneManager() {
        return sceneManager_;
    }

    uint32_t GetBGMHandle() const {
        return BGMhandle_;
    }
private:
    SceneManager* sceneManager_ = nullptr;
protected:
     uint32_t BGMhandle_ = 0;

};

