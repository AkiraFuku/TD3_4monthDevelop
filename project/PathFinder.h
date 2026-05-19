#pragma once
#include <vector>
#include <deque>
#include <memory>

// 前方宣言：ヘッダの循環参照を防ぐ
class ThreadManager;
class OneWayObject;
class BrokenBlock;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

class PathFinder {
public:
    // ThreadManagerを引数で受け取り、動的な糸の橋を考慮できるようにする
    static std::vector<Point> FindPath(Point start, Point goal, int width, int height,
        ThreadManager* threadManager, const std::vector<std::unique_ptr<OneWayObject>>& oneWays,
        const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock);

private:
    struct Node {
        Point pos;
        int g, h;
        float f;
        Node* parent;
        Node(Point p, int _g, int _h, Node* _parent)
            : pos(p), g(_g), h(_h), parent(_parent) {
            // h（予測値）を 1.5倍 程度に強調する
            f = (float)g + (float)h * 1.5f;
        }

        bool isConnectedToThread = false;
    };

    static int CalcH(Point a, Point b);
};

