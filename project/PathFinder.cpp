#include "PathFinder.h"
#include "CollisionMask.h"
#include "ThreadManager.h"
#include "ThreadPhysics.h" // PhysicsNodeの構造を知るために必要
#include <cmath>
#include <algorithm>
#include "OneWayObject.h"

std::vector<Point> PathFinder::FindPath(Point start, Point goal, int width, int height, ThreadManager* tm
    , const std::vector<std::unique_ptr<OneWayObject>>& oneWays) {
    std::vector<Node*> openList;
    std::vector<Node*> closedList;
    std::vector<Point> finalPath;

    openList.push_back(new Node(start, 0, CalcH(start, goal), nullptr));

    int nodeCount = 0;

    while (!openList.empty()) {
        nodeCount++;
        if (nodeCount > 30000) { // 5000ノード以上調べてもダメなら中断
            break;
        }
        auto it = std::min_element(openList.begin(), openList.end(), [](Node* a, Node* b) {
            return a->f < b->f;
            });
        Node* current = *it;

        if (current->pos == goal) {
            Node* temp = current;
            while (temp != nullptr) {
                finalPath.push_back(temp->pos);
                temp = temp->parent;
            }
            std::reverse(finalPath.begin(), finalPath.end());
            break;
        }

        openList.erase(it);
        closedList.push_back(current);

        Point directions[] = {
     {0, 1}, {0, -1}, {1, 0}, {-1, 0}, // 上下左右
     {1, 1}, {1, -1}, {-1, 1}, {-1, -1} // 斜めを追加
        };
        for (auto& dir : directions) {
            Point nextPos = { current->pos.x + dir.x, current->pos.y + dir.y };
            Vector3 moveDir = { (float)dir.x, 0.0f, (float)dir.y };

            if (nextPos.x < 0 || nextPos.x >= width || nextPos.y < 0 || nextPos.y >= height) continue;

            // --- 壁と糸の複合判定 ---
            float worldX = (float)nextPos.x - 256.0f;
            float worldZ = (float)nextPos.y - 256.0f;

            
            bool isWall = CollisionMask::GetInstance()->IsWall(worldX, worldZ);
            bool hasThread = false;
            Vector3 worldPos = { worldX, 0.0f, worldZ };

            // 1. 【追加】橋の判定（最優先）
            bool onOneWay = false;
            bool canPassOneWay = true;

            for (const auto& ow : oneWays) {
                if (ow->IsInside(worldPos)) {

                    if (!ow->CanPass(moveDir, Vector3{ (float)current->pos.x - 256.0f + 0.5f, 0, (float)current->pos.y - 256.0f + 0.5f })) 
                    {
                        canPassOneWay = false; // 逆走判定
                        break;
                    }

                    onOneWay = true;
                    break;
                }
            }

            if (tm) {
                // ThreadManager内の全糸ノードをチェック
                float checkRadiusSq = 0.8f; // 半径3.0の2乗。広めに設定

                for (auto& physics : tm->GetPhysicsList()) {
                    for (const auto& node : physics->GetNodes()) {
                        float dx = node.currentPos.x - worldX;
                        float dz = node.currentPos.z - worldZ;

                        if ((dx * dx + dz * dz) < checkRadiusSq) {
                            hasThread = true;
                            break;
                        }
                    }
                    if (hasThread) break;
                }
            }

            bool canPass = !isWall || hasThread || onOneWay;

            if (!canPass) {
                continue; // 通れないなら次の方向へ
            }

            bool inClosed = false;
            for (auto n : closedList) { if (n->pos == nextPos) { inClosed = true; break; } }
            if (inClosed) continue;

            float moveCost = (dir.x != 0 && dir.y != 0) ? 1.414f : 1.0f;

            // --- ここで優先度をつける ---
            if (hasThread) {
                // 糸の上は最優先（コストをそのまま、あるいは少し下げる）
                moveCost *= 1.0f;
            }
            else if (!isWall) {
                // 地面の上は次点
                moveCost *= 1.2f;
            }
            else if (onOneWay) {
                // 橋の上は「糸がない場合の最終手段」としてコストを高く設定する
                // これにより、糸があるなら糸を、なければ橋を通るようになる
                moveCost *= 5.0f;
            }

            int newG = current->g + static_cast<int>(moveCost * 10.0f);
            Node* openNode = nullptr;
            for (auto n : openList) { if (n->pos == nextPos) { openNode = n; break; } }

            if (openNode == nullptr) {
                openList.push_back(new Node(nextPos, newG, CalcH(nextPos, goal), current));
            }
            else if (newG < openNode->g) {
                openNode->g = newG;
                openNode->f = (float)openNode->g + (float)openNode->h;
                openNode->parent = current;
            }
        }
    }

    for (auto n : openList) delete n;
    for (auto n : closedList) delete n;

    if (finalPath.empty()) {
        if (openList.empty()) {
            OutputDebugStringA("Reason: No path exists (Isolated)\n"); // 物理的に繋がってない
        }
        else {
            OutputDebugStringA("Reason: Search limit reached\n"); // 遠すぎて諦めた
        }
    }

    return finalPath;
}

int PathFinder::CalcH(Point a, Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}