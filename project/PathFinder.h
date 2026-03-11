#pragma once
#include <vector>
#include <deque>

// 前方宣言：ヘッダの循環参照を防ぐ
class ThreadManager;

struct Point {
    int x, y;
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

class PathFinder {
public:
    // ThreadManagerを引数で受け取り、動的な糸の橋を考慮できるようにする
    static std::vector<Point> FindPath(Point start, Point goal, int width, int height, ThreadManager* threadManager);

private:
    struct Node {
        Point pos;
        int g, h, f;
        Node* parent;
        Node(Point p, int _g, int _h, Node* _parent)
            : pos(p), g(_g), h(_h), parent(_parent) {
            f = g + h;
        }
    };

    static int CalcH(Point a, Point b);
};