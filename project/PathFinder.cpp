#include "PathFinder.h"
#include "CollisionMask.h"
#include "ThreadManager.h"
#include "ThreadPhysics.h" // PhysicsNodeの構造を知るために必要
#include <cmath>
#include <algorithm>
#include "OneWayObject.h"
#include "BrokenBlock.h"
#include <iostream>

std::vector<Point> PathFinder::FindPath(Point start, Point goal, int width, int height, ThreadManager* tm
    , const std::vector<std::unique_ptr<OneWayObject>>& oneWays,const std::vector < std::unique_ptr <BrokenBlock>>& brokenBlock) {
    std::vector<Node*> openList;
    std::vector<Node*> closedList;
    std::vector<Point> finalPath;

    Node* startNode = new Node(start, 0, CalcH(start, goal), nullptr);

    // 【安全対策】スタート地点のワールド座標を計算
    float startWorldX = (float)start.x - 256.0f + 0.5f;
    float startWorldZ = (float)start.y - 256.0f + 0.5f;

    // 今いる場所が空中（IsWall）の場合のみ、足元の糸チェックを行う
    if (CollisionMask::GetInstance()->IsWall(startWorldX, startWorldZ)) {
        bool isStartOnThread = false;

        if (tm) {
            // 空中移動の時と同じく、少し広め（1.5マス分 = 2.25f）の判定で足元の糸を探す
            float checkRadiusSq = 0.64f;

            for (auto& physics : tm->GetPhysicsList()) {
                for (const auto& node : physics->GetNodes()) {
                    float dx = node.currentPos.x - startWorldX;
                    float dz = node.currentPos.z - startWorldZ;

                    if ((dx * dx + dz * dz) < checkRadiusSq) {
                        isStartOnThread = true; // 確かに足元に糸がある！
                        break;
                    }
                }
                if (isStartOnThread) break;
            }
        }

        // 物理的に糸が存在している場合のみ、空中スタートのフラグをONにする
        startNode->isConnectedToThread = isStartOnThread;
    }
    else {
        // 地面スタートの場合は、これまで通り「端っこを踏むまで」はフラグ false
        startNode->isConnectedToThread = false;
    }

    openList.push_back(startNode);

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

                    isWall = false; // 壁を無視して通れるようにする
                    onOneWay = true;
                    break;
                }
            }

            
            // 壊れるブロックの判定
            bool onBrokenBlock = false;
            for (const auto& br : brokenBlock)
            {
                if (br->IsInside(worldPos) && !br->IsBroken())
                {
                    Vector3 currentWorldPos = { (float)current->pos.x - 256.0f + 0.5f, 0, (float)current->pos.y - 256.0f + 0.5f };

                    if (br->IsImpassable() && !br->IsInside(currentWorldPos))
                    {
                        continue;
                    }

                    isWall = false;
                    onBrokenBlock = true;
                    break;
                }
            }


            // 3. 糸の判定（あなたの想定した「フラグ連動方式」の完全版）
            if (tm) {
                float checkRadiusSq = 0.64f;

                for (auto& physics : tm->GetPhysicsList()) {
                    const auto& nodes = physics->GetNodes();
                    if (nodes.empty()) continue;

                    for (size_t i = 0; i < nodes.size(); ++i) {
                        float dx = nodes[i].currentPos.x - worldX;
                        float dz = nodes[i].currentPos.z - worldZ;

                        if ((dx * dx + dz * dz) < checkRadiusSq) {

                            // 今調べている次のマスが「地面（!isWall）」のとき
                            if (!isWall) {
                                // 地面の上を歩いているときは、かすり飛び乗りを防ぐため
                                // 「まさにそのマスに糸の端っこがある時」だけしか認めない
                                bool isEdge = (i == 0 || i == nodes.size() - 1);
                                if (isEdge) {
                                    hasThread = true;
                                    // 🌟【あなたの設計】端っこを踏んだので、このルートの開通フラグをONにする！
                                    // （ただし、まだ openList に入れる前なので、一旦ローカル変数等で記憶するか、
                                    // 下の Node 作成時にこのフラグを引き継ぎます）

                                    if (hasThread) {
                                        std::cout << "糸の探索進捗: マス(" << nextPos.x << ", " << nextPos.y << ") まで開通！ フラグ:" << current->isConnectedToThread << std::endl;
                                    }

                                    break;
                                }
                            }
                            // 今調べている次のマスが「空中（isWall）」のとき
                            else {
                                // 空中の一歩を踏み出す、あるいは進むときは、
                                // 🌟【あなたの設計】1手前のマス（current）が、すでに「糸の端」を踏んで開通フラグを持っていた場合のみ通行を許可！
                                // または、すでに空中を移動中でフラグが維持されている場合のみ許可！
                                if (current->isConnectedToThread) {
                                    hasThread = true;
                                    if (hasThread) {
                                        std::cout << "糸の探索進捗: マス(" << nextPos.x << ", " << nextPos.y << ") まで開通！ フラグ:" << current->isConnectedToThread << std::endl;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    if (hasThread) break;
                }
            }


            bool canPass = !isWall || hasThread || onOneWay || onBrokenBlock;

            
            if (!canPass) {
               continue; // 通れないなら次の方向へ
            }

            bool inClosed = false;
            for (auto n : closedList) {
                if (n->pos == nextPos) {
                    // 座標が同じでも、既存のデータが「フラグなし」で、今回のルートが「フラグあり」なら、
                    // 過去のデータを無視して、探索を上書き・続行させる！
                    if (!n->isConnectedToThread && current->isConnectedToThread) {
                        inClosed = false;
                    }
                    else {
                        inClosed = true;
                    }
                    break;
                }
            }
            if (inClosed) continue;

            float moveCost = (dir.x != 0 && dir.y != 0) ? 1.414f : 1.0f;

            // --- ここで優先度をつける ---
            if (hasThread) {
                // 糸の上は最優先（コストをそのまま、あるいは少し下げる）
                moveCost *= 0.5f;
            }
            else if (!isWall) {
                // 地面の上は次点
                moveCost *= 1.2f;
            }
            else if (onBrokenBlock)
            {
                moveCost *= 2.0f;
            }
            else if (onOneWay) {
                // 橋の上は「糸がない場合の最終手段」としてコストを高く設定する
                // これにより、糸があるなら糸を、なければ橋を通るようになる
                moveCost *= 2.0f;
            }

            // --- 新しい Node を openList に追加する処理 ---
            int newG = current->g + static_cast<int>(moveCost * 10.0f);
            Node* openNode = nullptr;
            for (auto n : openList) {
                if (n->pos == nextPos) {
                    // 座標が同じでも、待機中のデータが「フラグなし」で、今回が「フラグあり」なら、
                    // それは別ルート（上位互換ルート）なので、別物として新しくノードを作らせる
                    if (!n->isConnectedToThread && current->isConnectedToThread) {
                        openNode = nullptr;
                    }
                    else {
                        openNode = n;
                    }
                    break;
                }
            }

            // ★【フラグ確定ロジックの修正】
            // ① 今まさに地面で端っこを踏んだ（新規開通）
            // ② 1歩前のマス（current）がすでにフラグを持っている（空中での前進）
            // このどちらかであれば、このルートは「糸の上の正当なルート」です。
            bool shouldBeConnected = (!isWall && hasThread) || current->isConnectedToThread;

            if (openNode == nullptr) {
                Node* newNode = new Node(nextPos, newG, CalcH(nextPos, goal), current);

                // 確実に true を引き継ぐ
                newNode->isConnectedToThread = shouldBeConnected;

                openList.push_back(newNode);
            }
            else {
                // 🌟【最重要】すでに openList にあるノードを更新する場合のガード
                // より低コストなルートが見つかって親（parent）が切り替わるとき、
                // 「新ルートがフラグを持っている」なら当然 true に更新しますが、
                // もし新ルートがフラグを持っていなくても、既存のノードがすでに true だったなら、
                // 絶対に false に上書き（リセット）させず、true を維持させます！
                if (newG < openNode->g) {
                    openNode->g = newG;
                    openNode->f = (float)openNode->g + (float)openNode->h;
                    openNode->parent = current;

                    if (shouldBeConnected || openNode->isConnectedToThread) {
                        openNode->isConnectedToThread = true;
                    }
                    else {
                        openNode->isConnectedToThread = false;
                    }
                }
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
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    // 斜めを 14, 直進を 10 とする整数計算が一般的
    return 10 * (dx + dy) + (14 - 2 * 10) * std::min(dx, dy);
}