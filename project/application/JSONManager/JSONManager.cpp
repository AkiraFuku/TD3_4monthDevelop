#include "JSONManager.h"
#include <fstream>
#include <WinApp.h>

std::unique_ptr<JSONManager> JSONManager::instance_ = nullptr;

using json = nlohmann::json;

JSONManager* JSONManager::GetInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::make_unique<JSONManager>(ConstructorKey());
    }

    return instance_.get();
}

void JSONManager::Initialize()
{
    
}

void JSONManager::Finalize()
{
    instance_.reset();
}

void JSONManager::RegisterGroup(const std::string& groupName, Group group)
{


    if (datas_.contains(groupName))
    {
        datas_[groupName] = group;
    }

    Group& newGroup = datas_[groupName];

   newGroup = group;

}

void JSONManager::SaveFile(const std::string& groupName)
{
    // グループを検索
    std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);

    // 未登録チェック
    assert(itGroup != datas_.end());

    json root;

    root = json::object();

    // jsonオブジェクト登録
    root[groupName] = json::object();

    // 各項目について
    for (auto itItem = itGroup->second.items.begin();
        itItem != itGroup->second.items.end(); ++itItem)
    {
        // 項目名を取得
        const std::string& itemName = itItem->first;
        JSONManager::Item& item = itItem->second;

        // int32_t型の値を保持していれば
        if (std::holds_alternative<int32_t>(item.value))
        {
            // int32_t型の値を登録
            root[groupName][itemName] = std::get<int32_t>(item.value);
        }
        else if(std::holds_alternative<float>(item.value))
        {
            // float型の値を登録
            root[groupName][itemName] = std::get<float>(item.value);
        }
        else if (std::holds_alternative<Vector3>(item.value))
        {
            // float型のjson配列登録
            Vector3 value = std::get<Vector3>(item.value);
            root[groupName][itemName] = {{"x", value.x}, {"y", value.y}, {"z", value.z}};
        }
        else if (std::holds_alternative<std::string>(item.value))
        {
            // std::srring型の値を登録
            root[groupName][itemName] = std::get<std::string>(item.value);
        }
    }

    // vector配列を格納
    for (auto itItemVector = itGroup->second.itemVector.begin();
        itItemVector != itGroup->second.itemVector.end(); ++itItemVector)
    {
        const std::string& listName = itItemVector->first;
        const std::vector<JSONManager::Item>& list = itItemVector->second;

        // このリスト用のJSON配列を作成
        json jsonList = json::array();

        for (const auto& item : list) 
        {
            if (std::holds_alternative<int32_t>(item.value)) 
            {
                jsonList.push_back(std::get<int32_t>(item.value));
            } 
            else if (std::holds_alternative<float>(item.value)) 
            {
                jsonList.push_back(std::get<float>(item.value));
            } 
            else if (std::holds_alternative<Vector3>(item.value)) 
            {
                Vector3 value = std::get<Vector3>(item.value);
                jsonList.push_back({ {"x", value.x}, {"y", value.y}, {"z", value.z} });
            } 
            else if (std::holds_alternative<std::string>(item.value)) 
            {
                jsonList.push_back(std::get<std::string>(item.value));
            }
        }

        // 最終的なルートに追加
        root[groupName][listName] = jsonList;
    }

    // ディレクトリが無ければ作成する
    std::filesystem::path dir(kDirectoryPath);
    if (!std::filesystem::exists(dir))
    {
        std::filesystem::create_directories(dir);
    }

    // 書き込むJSONファイルのフルパスを合成する
    std::string filePath = kDirectoryPath + groupName + ".json";
    // 書き込み用ファイルストリーム
    std::ofstream ofs;
    // ファイルを書き込み用に開く
    ofs.open(filePath);

    // ファイルオープン失敗？
    if (ofs.fail())
    {
        std::string message = "Fialed open data file for wite.";
        MessageBoxA(nullptr, message.c_str(), "JSONManager", 0);
        return;
    }

    // ファイルにjson文字列を書き込む(インデント幅4)
    ofs << std::setw(4) << root << std::endl;
    // ファイルを閉じる
    ofs.close();

    std::string message = std::format("{}.json saved", groupName);
    MessageBoxA(nullptr, message.c_str(), "JSONManager", 0);
}

// ファイルを読み込み、datas_ に登録する
void JSONManager::LoadFile(const std::string& groupName)
{
    std::string filePath = kDirectoryPath + groupName + ".json";
    if (!std::filesystem::exists(filePath)) {
        return;
    }

    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return;
    }

    json j;
    try 
    {
        ifs >> j;
    } 
    catch (const std::exception&) 
    {
        return;
    }

    if (!j.contains(groupName)) 
    {
        return;
    }

    Group group;
    for (auto it = j[groupName].begin(); it != j[groupName].end(); ++it) {
        const std::string itemName = it.key();
        const json& val = it.value();
        Item item;

        if (val.is_number_integer()) 
        {
            // nlohmann::json の整数は大きさが int64_t の場合があるためキャスト
            item.value = static_cast<int32_t>(val.get<int64_t>());
        } 
        else if (val.is_number_float()) 
        {
            item.value = val.get<float>();
        } 
        else if (val.is_object())
        {
            Vector3 v{};
            v.x = val["x"].get<float>();
            v.y = val["y"].get<float>();
            v.z = val["z"].get<float>();
            item.value = v;
        }
        else if (val.is_string())
        {
            item.value = val.get<std::string>();
        }
        else if (val.is_array())
        {
            std::vector<Item> itemList;
            for (const auto& subVal : val) {
                Item subItem;
             
                if (subVal.is_number_integer()) 
                {
                    subItem.value = static_cast<int32_t>(subVal.get<int64_t>());
                }
                else if (subVal.is_number_float())
                {
                    subItem.value = subVal.get<float>();
                } 
                else if (subVal.is_object())
                {
                    Vector3 v{};
                    v.x = subVal["x"].get<float>();
                    v.y = subVal["y"].get<float>();
                    v.z = subVal["z"].get<float>();
                    subItem.value = v;
                } 
                else if (subVal.is_string())
                {
                    subItem.value = subVal.get<std::string>();
                }
                // ... 他の型の判定
                itemList.push_back(subItem);
            }
            group.itemVector[itemName] = std::move(itemList);
        }
       

        group.items[itemName] = item;
    }

    // 上書きして登録
    datas_[groupName] = std::move(group);
}

bool JSONManager::TryGetInt(const std::string& groupName, const std::string& itemName, int32_t& out) const
{
    auto itGroup = datas_.find(groupName);
    if (itGroup == datas_.end()) return false;
    auto itItem = itGroup->second.items.find(itemName);
    if (itItem == itGroup->second.items.end()) return false;

    if (std::holds_alternative<int32_t>(itItem->second.value)) 
    {
        out = std::get<int32_t>(itItem->second.value);
        return true;
    }
    
    if (std::holds_alternative<float>(itItem->second.value)) {
        out = static_cast<int32_t>(std::get<float>(itItem->second.value));
        return true;
    }
    return false;
}

bool JSONManager::TryGetFloat(const std::string& groupName, const std::string& itemName, float& out) const
{
    auto itGroup = datas_.find(groupName);
    if (itGroup == datas_.end()) return false;
    auto itItem = itGroup->second.items.find(itemName);
    if (itItem == itGroup->second.items.end()) return false;

    if (std::holds_alternative<float>(itItem->second.value)) 
    {
        out = std::get<float>(itItem->second.value);
        return true;
    }

    if (std::holds_alternative<int32_t>(itItem->second.value)) 
    {
        out = static_cast<float>(std::get<int32_t>(itItem->second.value));
        return true;
    }
    return false;
}

bool JSONManager::TryGetVector3(const std::string& groupName, const std::string& itemName, Vector3& out) const
{
    auto itGroup = datas_.find(groupName);
    if (itGroup == datas_.end()) return false;
    auto itItem = itGroup->second.items.find(itemName);
    if (itItem == itGroup->second.items.end()) return false;

    if (std::holds_alternative<Vector3>(itItem->second.value)) 
    {
        out = std::get<Vector3>(itItem->second.value);
        return true;
    }
    return false;
}

bool JSONManager::TryGetString(const std::string& groupName, const std::string& itemName, std::string& out) const
{
    auto itGroup = datas_.find(groupName);
    if (itGroup == datas_.end()) return false;
    auto itItem = itGroup->second.items.find(itemName);
    if (itItem == itGroup->second.items.end()) return false;

    if (std::holds_alternative<std::string>(itItem->second.value))
    {
        out = std::get<std::string>(itItem->second.value);
        return true;
    }

    return false;
}
