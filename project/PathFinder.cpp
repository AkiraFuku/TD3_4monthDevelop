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
        if (nodeCount > 30000) { 
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
            float worldX = (float)nextPos.x - 256.0f + 0.5f;
            float worldZ = (float)nextPos.y - 256.0f + 0.5f;

            
            bool isWall = CollisionMask::GetInstance()->IsWall(worldX, worldZ);
            bool hasThread = false;
            Vector3 worldPos = { worldX, 0.0f, worldZ };


            std::cout << "[A*ルート精査中] 調査マス:(" << nextPos.x << ", " << nextPos.y
                << ") 壁判定(1=空中, 0=地面): " << isWall
                << " TMの存在: " << (tm != nullptr) << std::endl;

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


            // 3. 糸の判定（正確な中心座標ベース）
            if (tm) {
                // 確実に感知できるよう、判定の広さを 2.25f (1.5マス分) に設定
                float radiusSq = 2.54f;

                for (auto& physics : tm->GetPhysicsList()) {
                    const auto& nodes = physics->GetNodes();
                    if (nodes.empty()) continue;

                    const auto& edgeStartNode = nodes.front();
                    const auto& edgeEndNode = nodes.back();

                    // 今調べている次のマスが「地面（!isWall）」のとき
                    if (!isWall) {
                        // 🌟【修正】端っこだけでなく、糸のすべてのノードをチェック対象にする！
                        // 感知半径は、マスのサイズに合わせて少し広めの 2.54f にしておきます
                        float groundRadiusSq = 2.54f;

                        for (const auto& node : nodes) {
                            float dx = node.currentPos.x - worldX;
                            float dz = node.currentPos.z - worldZ;

                            if ((dx * dx + dz * dz) < groundRadiusSq) {
                                hasThread = true;
                                std::cout << "糸の探索進捗(地面で糸の胴体を感知): マス(" << nextPos.x << ", " << nextPos.y << ") 開通！" << std::endl;
                                break;
                            }
                        }
                    }
                    // 今調べている次のマスが「空中（isWall）」のとき
                    else {
                        // 1歩前のマス（current＝今いる地面）のワールド座標を正確に計算
                        float curWorldX = (float)current->pos.x - 256.0f + 0.5f;
                        float curWorldZ = (float)current->pos.y - 256.0f + 0.5f;
                        bool currentIsWall = CollisionMask::GetInstance()->IsWall(curWorldX, curWorldZ);

                        // -----------------------------------------------------------
                        // パターンA：【地面 ⇒ 空中】まさに今、崖から糸へ飛び移ろうとしている瞬間
                        // -----------------------------------------------------------
                        if (!currentIsWall) {
                            // ★【ここがポイント】
                            // 判定基準を「次の空中マスの中心」ではなく、「今いる地面（崖のフチ）の中心」にする！
                            // これにより、地面と糸が物理的に繋がっている時だけ半径内に収まるようになります。
                            float transitionRadiusSq = 1.44f; // 1.2マスの距離（これだけあればズレは100%吸収可能）

                            float dxStart = edgeStartNode.currentPos.x - curWorldX;
                            float dzStart = edgeStartNode.currentPos.z - curWorldZ;
                            float dxEnd = edgeEndNode.currentPos.x - curWorldX;
                            float dzEnd = edgeEndNode.currentPos.z - curWorldZ;

                            // 糸のどちらかの端っこ（結び目）が、今自分がいる「地面マス」のすぐ近くにあるなら進入OK！
                            if ((dxStart * dxStart + dzStart * dzStart) < transitionRadiusSq ||
                                (dxEnd * dxEnd + dzEnd * dzEnd) < transitionRadiusSq)
                            {
                                hasThread = true;
                            }
                            else {
                                // 結び目が地面の近くにない＝浮いている糸なので、絶対に進入させない
                                hasThread = false;
                            }
                        }
                        // -----------------------------------------------------------
                        // パターンB：【空中 ⇒ 空中】すでに糸の上に乗っていて、糸を伝って前進している時
                        // -----------------------------------------------------------
                        else {
                            float radiusSq = 0.64f; // 0.8マス分（空中移動用のタイトな判定）

                            if (current->isConnectedToThread) {
                                // 【並走フライング防止】1歩前がこの一本の糸に本当に乗っていたか
                                bool isSameThread = false;
                                for (const auto& node : nodes) {
                                    float cdx = node.currentPos.x - curWorldX;
                                    float cdz = node.currentPos.z - curWorldZ;
                                    if ((cdx * cdx + cdz * cdz) < radiusSq) {
                                        isSameThread = true;
                                        break;
                                    }
                                }

                                // 1歩前がこの糸に乗っていたなら、次の空中マス（worldX, worldZ）にノードがあるか
                                if (isSameThread) {
                                    for (const auto& node : nodes) {
                                        float dx = node.currentPos.x - worldX;
                                        float dz = node.currentPos.z - worldZ;

                                        if ((dx * dx + dz * dz) < radiusSq) {
                                            hasThread = true;
                                            break;
                                        }
                                    }
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
                moveCost *= 0.05f;
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

            // ★【フラグ確定ロジック】
            bool shouldBeConnected = (!isWall && hasThread) || current->isConnectedToThread;
            if (!isWall && !hasThread) {
                shouldBeConnected = false;
            }

            // openList の中から「座標」も「糸フラグの状態」も完全に一致するノードを探す
            Node* openNode = nullptr;
            for (auto n : openList) {
                if (n->pos == nextPos && n->isConnectedToThread == shouldBeConnected) {
                    openNode = n;
                    break;
                }
            }

            if (openNode == nullptr) {
                // Hコスト（残り距離）の計算結果に、糸のルートなら大ボーナスを与える
                int hCost = CalcH(nextPos, goal);
                if (shouldBeConnected) {
                    hCost -= 200; // 推定コストを大幅に下げてキューの最前に引き上げる
                }

                Node* newNode = new Node(nextPos, newG, hCost, current);
                newNode->isConnectedToThread = shouldBeConnected;
                openList.push_back(newNode);
            }
            else {
                // 完全に同じ状態のノードが存在し、かつ今回のルートの方が低コストなら更新
                if (newG < openNode->g) {
                    openNode->g = newG;
                    openNode->f = (float)openNode->g + (float)openNode->h;
                    openNode->parent = current;
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