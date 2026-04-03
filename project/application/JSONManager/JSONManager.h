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
        std::variant<int32_t, float, Vector3> value;	// 値
    };


    // グループ構造体
    struct Group
    {
        std::map<std::string, Item> items;	// 項目
    };

   
   
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

    // 取得ヘルパー（存在チェック付き）
    bool TryGetInt(const std::string& groupName, const std::string& itemName, int32_t& out) const;
    bool TryGetFloat(const std::string& groupName, const std::string& itemName, float& out) const;
    bool TryGetVector3(const std::string& groupName, const std::string& itemName, Vector3& out) const;


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

