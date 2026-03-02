#pragma once

#include"Object3D.h"
#include "Model.h"
#include "Camera.h"

class Player
{
public:

    void Initialize();

    void Finalize();

    void Update();
    
    void Draw();

public: // 外部入出力





private:

    std::unique_ptr<Object3d> playerObject_;
};

