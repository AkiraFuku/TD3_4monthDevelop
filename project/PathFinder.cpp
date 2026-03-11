#include "PathFinder.h"
#include "CollisionMask.h"
#include <cmath>
#include <algorithm>

std::vector<Point> PathFinder::FindPath(Point start, Point goal, int width, int height) {
    std::vector<Node*> openList;
    std::vector<Node*> closedList;
    std::vector<Point> finalPath;

    // 1. スタートノードを作成してOpenリストへ
    openList.push_back(new Node(start, 0, CalcH(start, goal), nullptr));

    while (!openList.empty()) {
        // 2. Openリストの中で最小のFスコアを持つノードを探す
        auto it = std::min_element(openList.begin(), openList.end(), [](Node* a, Node* b) {
            return a->f < b->f;
            });
        Node* current = *it;

        // 3. ゴールに到達したか
        if (current->pos == goal) {
            Node* temp = current;
            while (temp != nullptr) {
                finalPath.push_back(temp->pos);
                temp = temp->parent;
            }
            std::reverse(finalPath.begin(), finalPath.end());
            break; // ループ終了
        }

        // 4. 現在のノードをClosedへ移動
        openList.erase(it);
        closedList.push_back(current);

        // 5. 上下左右の4方向を調べる
        Point directions[] = { {0, 1}, {0, -1}, {1, 0}, {-1, 0} };
        for (auto& dir : directions) {
            Point nextPos = { current->pos.x + dir.x, current->pos.y + dir.y };

            // マップ範囲外チェック
            if (nextPos.x < 0 || nextPos.x >= width || nextPos.y < 0 || nextPos.y >= height) continue;

            // ★重要：CollisionMask を直接参照して壁判定
            if (CollisionMask::GetInstance()->IsWall((float)nextPos.x, (float)nextPos.y)) continue;

            // 既にClosedにある場合はスキップ
            bool inClosed = false;
            for (auto n : closedList) { if (n->pos == nextPos) { inClosed = true; break; } }
            if (inClosed) continue;

            int newG = current->g + 1;

            // 既にOpenにあるかチェック
            Node* openNode = nullptr;
            for (auto n : openList) { if (n->pos == nextPos) { openNode = n; break; } }

            if (openNode == nullptr) {
                openList.push_back(new Node(nextPos, newG, CalcH(nextPos, goal), current));
            }
            else if (newG < openNode->g) {
                openNode->g = newG;
                openNode->f = openNode->g + openNode->h;
                openNode->parent = current;
            }
        }
    }

    // メモリの全解放（絶対に忘れないこと！）
    for (auto n : openList) delete n;
    for (auto n : closedList) delete n;

    return finalPath; // 見つからなければ空のvectorを返す
}

int PathFinder::CalcH(Point a, Point b) {
    // マンハッタン距離
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}