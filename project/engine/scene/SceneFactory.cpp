#include "SceneFactory.h"
#include "GameScene.h"
#include "TutorialScene.h"
#include "TitleScene.h"
#include "SelectScene.h"
std::unique_ptr<Scene> SceneFactory::CreateScene(const std::string& sceneName)
{
    std::unique_ptr<Scene> scene = nullptr;

    if (sceneName == "TitleScene") {
         scene = std::make_unique<TitleScene>();
    }
    else if (sceneName == "GameScene") {
         scene = std::make_unique<GameScene>();
    }
    else if (sceneName == "TutorialScene") {
        scene = std::make_unique<TutorialScene>();
    }
    else if (sceneName == "ResultScene") {
        // scene = std::make_unique<ResultScene>();
    }
    else if (sceneName == "SelectScene")
    {
        scene = std::make_unique<SelectScene>();
    }

    return scene;
}