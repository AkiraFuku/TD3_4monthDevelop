#pragma once
#include <variant>
#include <map>
#include "StringUtility.h"
#include <memory>

#include "json.hpp"
#include "Vector3.h"

class JSONManager
{
public:

    // 項目構造体
    struct Item
    {
        std::variant<int32_t, float, Vector3, std::string> value;	// 値
    };


    // グループ構造体
    struct Group
    {
        std::map<std::string, Item> items;	// 項目
        std::map<std::string, std::vector<Item>> itemVector;    // 項目vector配列
    };

    template <typename T>
    std::vector<JSONManager::Item> ToItemVector(const std::vector<T>& source) {
        std::vector<JSONManager::Item> dest;
        dest.reserve(source.size()); // メモリ確保を1回で済ませる
        for (const auto& val : source) {
            dest.push_back(JSONManager::Item{ val });
        }
        return dest;
    }

    template <typename T>
    bool TryGetVector(const std::string& groupName, const std::string& itemName, std::vector<T>& outVector) {
        // 1. 指定されたグループと項目が存在するかチェック
        auto itGroup = datas_.find(groupName);
        if (itGroup == datas_.end()) return false;

        auto itVector = itGroup->second.itemVector.find(itemName);
        if (itVector == itGroup->second.itemVector.end()) return false;

        // 2. 中身を一つずつ取り出して、指定された型 T に変換できるか確認
        const std::vector<Item>& itemList = itVector->second;
        outVector.clear();
        outVector.reserve(itemList.size());

        for (const auto& item : itemList) {
            // variant から指定された型 T を取り出す
            if (auto pVal = std::get_if<T>(&item.value)) {
                outVector.push_back(*pVal);
            } 
            else 
            {
                // 一つでも型が合わないものがあれば、データ不整合として失敗させる（設計による）
                outVector.clear();
                return false;
            }
        }

        return true;
    }
   
public:

    // シングルトンインスタンスの取得
    static JSONManager* GetInstance();

    // 初期化
    void Initialize();
    // 終了
    void Finalize();

    /// <summary>
    /// グループを登録
    /// <param name="groupName">gyry－プ</param>
    void RegisterGroup(const std::string& groupName, Group group);


    /// <summary>
    /// ファイルに書き出し
    /// <param name="groupName">gyry－プ</param>
    void SaveFile(const std::string& groupName);

    // ファイルから読み込む
    void LoadFile(const std::string& groupName);

    // 取得ヘルパー
    bool TryGetInt(const std::string& groupName, const std::string& itemName, int32_t& out) const;
    bool TryGetFloat(const std::string& groupName, const std::string& itemName, float& out) const;
    bool TryGetVector3(const std::string& groupName, const std::string& itemName, Vector3& out) const;
    bool TryGetString(const std::string& groupName, const std::string& itemName, std::string& out) const;



public:
    // コンストラクタに渡すための鍵
    class ConstructorKey
    {
    private:
        ConstructorKey() = default;
        friend class JSONManager;
    };

    // PassKeyを受け取るコンストラクタ
    explicit JSONManager(ConstructorKey){}

private:	// シングルトンインスタンス

    static std::unique_ptr<JSONManager> instance_;

    ~JSONManager() = default;
    JSONManager(JSONManager&) = delete;
    JSONManager& operator=(JSONManager&) = delete;

    friend struct std::default_delete<JSONManager>;

private:

    std::map<std::string, Group> datas_;	// グループデータ

    // グローバル変数の保存先ファイルパス
    const std::string kDirectoryPath = "Resources/JSONManager/";

};

