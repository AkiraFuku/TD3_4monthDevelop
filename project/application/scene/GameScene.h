#pragma once
#include "BaseGameScene.h"

class GameScene : public BaseGameScene 
{
public:

    void Initialize();
  
    void Finalize();

    void UpdateExtra() override;

    void DrawExtra() override;

    void LoadStage() override;

    void OnClear() override;

private:

    void Clear();
};
