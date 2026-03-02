#include "Player.h"
#include "ModelManager.h"

void Player::Initialize()
{
    playerObject_ = std::make_unique<Object3d>();
    playerObject_->Initialize();

    ModelManager::GetInstance()->LoadModel("player.obj");
    playerObject_->SetModel("player.obj");
}

void Player::Finalize()
{
    
}

void Player::Update()
{
    playerObject_->Update();
}

void Player::Draw()
{
    playerObject_->Draw();
}