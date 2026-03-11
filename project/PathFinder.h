#pragma once
#include <vector>
#include <deque>

// グリッド座標を扱うシンプルな構造体
struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

class PathFinder {
public:
    /// <summary>
    /// 経路探索のメイン関数
    /// </summary>
    /// <param name="start">開始グリッド座標</param>
    /// <param name="goal">目的グリッド座標</param>
    /// <param name="width">探索範囲の幅(Maskの解像度等)</param>
    /// <param name="height">探索範囲の高さ(Maskの解像度等)</param>
    /// <returns>目的地までの座標リスト（見つからなければ空）</returns>
    static std::vector<Point> FindPath(Point start, Point goal, int width, int height);

private:
    // A*の計算で使用する一時的なノード情報
    struct Node {
        Point pos;
        int g; // スタートからの実コスト
        int h; // ゴールまでの推定コスト
        int f; // 合計スコア (g + h)
        Node* parent;

        Node(Point p, int _g, int _h, Node* _parent)
            : pos(p), g(_g), h(_h), parent(_parent) {
            f = g + h;
        }
    };

    // 推定距離（マンハッタン距離）の計算
    static int CalcH(Point a, Point b);
};