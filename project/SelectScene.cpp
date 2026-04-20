#include "SelectScene.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"

void SelectScene::Initialize()
{
    camera = std::make_unique<Camera>();
    camera->SetRotate({ 0.0f,0.0f,0.0f });
    camera->SetTranslate({ 0.0f,0.0f,-5.0f });

    // テクスチャの読み込み
    TextureManager::GetInstance()->LoadTexture("resources/stage1.png");
    TextureManager::GetInstance()->LoadTexture("resources/stage2.png");
    TextureManager::GetInstance()->LoadTexture("resources/stage3.png");
    TextureManager::GetInstance()->LoadTexture("resources/arrow.png");

    background_ = std::make_unique<Sprite>();
    background_->Initialize("resources/selectScene.png");

    stagePos_ =
    {
        {0.0f,300.0f},
        {500.0f,300.0f},
        {1000.0f,300.0f}
    };

    for (uint32_t i = 0; i < kStageNum_; i++)
    {
        std::unique_ptr<Sprite> sprite = std::make_unique<Sprite>();
        std::string path = "resources/stage" + std::to_string(i + 1) + ".png";
        sprite->Initialize(path);
        sprite->SetPosition(stagePos_[i]);
        sprites_.push_back(std::move(sprite));
    }

    arrowSprite_ = std::make_unique<Sprite>();
    arrowSprite_->Initialize("resources/arrow.png");
    arrowPos_ = sprites_[0]->GetPosition();
    arrowPos_.y += 300.0f;
    arrowSprite_->SetPosition(arrowPos_);

   
    // サウンド読み込み
    handle_ = Audio::GetInstance()->LoadAudio("resources/sounds/stageSelect.wav");
    enter_ = Audio::GetInstance()->LoadAudio("resources/sounds/enter.wav");
    select_ = Audio::GetInstance()->LoadAudio("resources/sounds/select.wav");
    // サウンド再生
    Audio::GetInstance()->PlayAudio(handle_, true, 1.0f);
}

void SelectScene::Finalize()
{
    Audio::GetInstance()->StopAudio(handle_);
}

void SelectScene::Update()
{
    background_->Update();

    preIndex = stageIndex;

    if (Input::GetInstance()->TriggerKeyDown(DIK_RIGHTARROW) ||
        Input::GetInstance()->TriggerKeyDown(DIK_D))
    {
        stageIndex++;
        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

        if (stageIndex >= 3)
        {
            stageIndex = 0;
        }
    }
    else if (Input::GetInstance()->TriggerKeyDown(DIK_LEFTARROW) ||
        Input::GetInstance()->TriggerKeyDown(DIK_A))
    {
        if (stageIndex <= 0)
        {
            stageIndex = 2;
        }
        else
        {
            stageIndex--;
        }

        Audio::GetInstance()->PlayAudio(select_, false, 1.0f);

    }
    else if (Input::GetInstance()->TriggerKeyDown(DIK_SPACE))
    {
        // ゲームシーンに戻る
        Audio::GetInstance()->PlayAudio(enter_, false, 1.0f);
        SceneManager::GetInstance()->ChangeScene("GameScene");
    }

    if (preIndex != stageIndex)
    {
        Vector2 pos = sprites_[stageIndex]->GetPosition();
        pos.y += 300.0f;
        arrowSprite_->SetPosition(pos);
    }

    // スプライトの更新処理
    for (const std::unique_ptr <Sprite>& sprite : sprites_)
    {
        sprite->Update();
    }

    arrowSprite_->Update();

}

void SelectScene::Draw()
{
    
    for (const std::unique_ptr <Sprite>& sprite : sprites_)
    {
        sprite->Draw();
    }

    arrowSprite_->Draw();

    background_->Draw();
}
