#include "Player.h"
#include "Input.h"
#include "ModelManager.h"
#include "imgui.h"
#include "CollisionMask.h"

/// <summary>
/// ������
/// </summary>
/// <param name="pos">�����ʒu</param>
void Player::Initialize(const Vector3& pos) {
    // ���f���̏�����
    object_ = std::make_unique<Object3d>();
    object_->Initialize();
     translate_ = pos;
    object_->SetTranslate(translate_);
    ModelManager::GetInstance()->LoadModel("resources","player/player.obj");
    object_->SetModel("player/player.obj");
}
/// <summary>
/// �I��
/// </summary>
void Player::Finalize() {}
/// <summary>
/// �X�V
/// </summary>
void Player::Update() {

    moveVel_ = { 0.0f, 0.0f, 0.0f };

    // �ړ�����
    UpdateMove();

    

#ifdef USE_IMGUI

    ImGui::Begin("Player Window");

    Vector3 scale = object_->GetScale();
    if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.1f, 10.0f)) {
        object_->SetScale(scale);
    }

    Vector3 rotate = object_->GetRotate();
    if (ImGui::DragFloat3("Rotate", &rotate.x, 0.1f, -360.0f, 360.0f)) {
        object_->SetRotate(rotate);
    }

    Vector3 translate = object_->GetTranslate();
    if (ImGui::DragFloat3("Translate", &translate.x, 0.1f, -100.0f, 100.0f)) {
        object_->SetTranslate(translate);
    }

    ImGui::End();

#endif

    // �Փ˔���
    IsCollision();

    // �ړ������m��
    ResultMove();

    // ���f���̍X�V
    object_->Update();
}
/// <summary>
/// �`��
/// </summary>
void Player::Draw() {
    // ���f���̕`��
    object_->Draw();
}

/// <summary>
/// �ړ�����
/// </summary>
void Player::UpdateMove() {
    // �ړ�������~�ς���ϐ�
    Vector3 moveDirection = { 0.0f, 0.0f, 0.0f };

    // �L�[���͂ɉ����ĕ����x�N�g�������
    if (Input::GetInstance()->PushedKeyDown(DIK_D)) {
        moveDirection.x += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_A)) {
        moveDirection.x -= 1.0f;
    }
    else
    {
        moveDirection.x = 0.0f;
    }

    if (Input::GetInstance()->PushedKeyDown(DIK_W)) {
        moveDirection.z += 1.0f;
    } else if (Input::GetInstance()->PushedKeyDown(DIK_S)) {
        moveDirection.z -= 1.0f;
    }
    else
    {
        moveDirection.z = 0.0f;
    }

    if (moveDirection.x != 0.0f || moveDirection.z != 0.0f) {
        // �x�N�g���̒�����v�Z
        float length = std::sqrtf(moveDirection.x * moveDirection.x +
                                  moveDirection.z * moveDirection.z);
        // ���K��
        if (length != 0.0f) {
            moveDirection.x /= length;
            moveDirection.z /= length;
        }

        // ���K�����������Ɏw�葬�x��|���ĉ��Z
        moveVel_.x += moveDirection.x * velocity_.x;
        moveVel_.z += moveDirection.z * velocity_.z;
    }

    
}

void Player::IsCollision()
{
    float nextX = translate_.x + moveVel_.x;
    if(CollisionMask::GetInstance()->IsWall(nextX, translate_.z))
    {
        moveVel_.x = 0.0f;
    }

    float nextZ = translate_.z + moveVel_.z;
    if(CollisionMask::GetInstance()->IsWall(translate_.x, nextZ))
    {
        moveVel_.z = 0.0f;
    }

}

void Player::ResultMove()
{
    translate_ += moveVel_;
    object_->SetTranslate(translate_);
}

void Player::SetPosition(const Vector3& pos)
{
    translate_ = pos;
    object_->SetTranslate(translate_);
}

AABB Player::GetAABB() const
{
    Vector3 worldPos = GetPosition();
    AABB aabb;

    aabb.min = { worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f };
    aabb.max = { worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f };

    return aabb;
}