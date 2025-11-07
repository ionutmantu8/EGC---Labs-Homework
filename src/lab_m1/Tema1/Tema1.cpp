#include "lab_m1/Tema1/Tema1.h"
#include <vector>
#include <iostream>
#include <cmath>

using namespace m1;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Tema1::Tema1() {}
Tema1::~Tema1() {}

Mesh* Tema1::CreateMesh(const char* name,
    const std::vector<VertexFormat>& vertices,
    const std::vector<unsigned int>& indices)
{
    unsigned int VAO = 0, VBO = 0, IBO = 0;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(),
        &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(),
        &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);

    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(sizeof(glm::vec3)));

    glEnableVertexAttribArray(2); // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3)));

    glEnableVertexAttribArray(3); // color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat),
        (void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));

    glBindVertexArray(0);

    auto* mesh = new Mesh(name);
    mesh->InitFromBuffer(VAO, (unsigned int)indices.size());
    mesh->SetDrawMode(GL_TRIANGLES);
    return mesh;
}

void Tema1::Init()
{
    auto res = window->GetResolution();
    vW = (float)res.x;
    vH = (float)res.y;

    auto cam = GetSceneCamera();
    cam->SetOrthographic(0, vW, 0, vH, 0.01f, 400);
    cam->SetPosition(glm::vec3(0, 0, 50));
    cam->SetRotation(glm::vec3(0, 0, 0));
    cam->Update();
    GetCameraInput()->SetActive(false);

    // ===== Layout editor
    PANEL_PAD = 24.0f;
    TOPBAR_H = 110.0f;

    SIDEBAR_X = PANEL_PAD;
    SIDEBAR_W = 260.0f;

    float usableSideH = (float)res.y - 2.f * PANEL_PAD;
    sideSlotH = usableSideH / 3.f;
    sidebarTopY = (float)res.y - PANEL_PAD - sideSlotH;

    // Grid (dreapta)
    cellSize = 40.0f;
    cellGap = 8.0f;
    gridRows = 9;
    gridCols = 17;

    occ.assign((size_t)gridRows, std::vector<int>((size_t)gridCols, -1));
    prevCellX = prevCellY = -1;

    const float framePad = 26.0f;
    const float gridW = gridCols * (cellSize + cellGap) - cellGap;
    const float gridH = gridRows * (cellSize + cellGap) - cellGap;

    float frameX = SIDEBAR_X + SIDEBAR_W + PANEL_PAD;
    float frameY = PANEL_PAD + 10.0f;
    float frameTop = frameY + (gridH + 2 * framePad);

    float desiredTop = (float)res.y - (TOPBAR_H + 10.0f);
    float deltaY = desiredTop - frameTop;
    frameY += deltaY;

    gridOffsetX = frameX + framePad;
    gridOffsetY = frameY + framePad;

    // ===== Meshes
    AddMeshToList(object2D::CreateSquare("gridCell",
        glm::vec3(0, 0, 0), cellSize, glm::vec3(0.20f, 0.45f, 1.0f), true));

    AddMeshToList(object2D::CreateSquare("healthBlock",
        glm::vec3(0, 0, 2), 48.0f, glm::vec3(0.10f, 0.78f, 0.05f), true));

    AddMeshToList(object2D::CreateSquare("startBtn",
        glm::vec3(0, 0, 2), 48.0f, glm::vec3(0.90f, 0.15f, 0.15f), true));

    AddMeshToList(object2D::CreateSquare("startBtnReady",
        glm::vec3(0, 0, 2), 48.0f, glm::vec3(0.15f, 0.85f, 0.20f), true));

    AddMeshToList(object2D::CreateSquare("outlineBlue",
        glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.20f, 0.30f, 1.0f), false));
    AddMeshToList(object2D::CreateSquare("outlineRed",
        glm::vec3(0, 0, 0), 1.0f, glm::vec3(1, 0, 0), false));

    AddMeshToList(object2D::CreateSquare("redSquare",
        glm::vec3(0, 0, 1), cellSize, glm::vec3(1, 0, 0), true));
    AddMeshToList(object2D::CreateSquare("yellowSquare",
        glm::vec3(0, 0, 1), cellSize, glm::vec3(1, 1, 0), true));

    // cerc portocaliu (unit circle)
    {
        const int seg = 40;
        std::vector<VertexFormat> v; std::vector<unsigned> ind;
        glm::vec3 col(1.0f, 0.55f, 0.0f);
        v.emplace_back(glm::vec3(0, 0, 1), col);
        for (int i = 0; i <= seg; ++i) {
            float t = (float)i / seg * 2.f * (float)M_PI;
            v.emplace_back(glm::vec3(std::cos(t), std::sin(t), 1), col);
        }
        for (int i = 1; i <= seg; ++i) { ind.push_back(0); ind.push_back(i); ind.push_back(i + 1); }
        AddMeshToList(CreateMesh("orangeCircle", v, ind));
    }

    // triunghiul „notch” al butonului start
    {
        std::vector<VertexFormat> v; std::vector<unsigned> ind = { 0,1,2 };
        const float notchBottom = 0.10f, notchTop = 0.90f, notchDepth = -0.50f;
        v.emplace_back(glm::vec3(0.0f, notchBottom, 3), glm::vec3(0, 0, 0));
        v.emplace_back(glm::vec3(0.0f, notchTop, 3), glm::vec3(0, 0, 0));
        v.emplace_back(glm::vec3(notchDepth, 0.50f, 3), glm::vec3(0, 0, 0));
        AddMeshToList(CreateMesh("startNotch", v, ind));
    }

    // alb plin / contur – pentru terenul Pong
    AddMeshToList(object2D::CreateSquare("whiteFill",
        glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.95f, 0.95f, 0.98f), true));
    AddMeshToList(object2D::CreateSquare("whiteStroke",
        glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.95f, 0.95f, 0.98f), false));

    // top-bar positions
    maxBlocks = 5;
    healthBlockSize = 48.0f;
    healthBlockGap = 34.0f;
    healthBarOffsetX = frameX;
    healthBarOffsetY = (float)res.y - TOPBAR_H + (TOPBAR_H - healthBlockSize) * 0.5f;
    startButtonPos = glm::vec3(
        healthBarOffsetX + maxBlocks * (healthBlockSize + healthBlockGap),
        healthBarOffsetY, 0);

    // toolbar slots
    toolbarSlots.clear();
    for (int k = 0; k < 3; ++k) {
        ToolbarSlot s;
        s.boxX = SIDEBAR_X;
        s.boxY = sidebarTopY - k * sideSlotH;
        s.boxW = SIDEBAR_W;
        s.boxH = sideSlotH;
        s.drawX = s.boxX + (s.boxW - cellSize) * 0.5f;
        s.drawY = s.boxY + (s.boxH - cellSize) * 0.5f;
        s.meshName = (k == 0 ? "redSquare" : (k == 1 ? "yellowSquare" : "orangeCircle"));
        toolbarSlots.push_back(s);
    }
}

void Tema1::FrameStart()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    auto res = window->GetResolution();
    glViewport(0, 0, res.x, res.y);
}

void Tema1::Update(float dt)
{
    if (mode == Mode::GAME) {
        // doar desenul scenei Pong (fără mișcări/logică)
        UpdateGame(dt);
        RenderGameBasic();
        return;
    }

    // ===== EDITOR =====
    const float framePad = 26.0f;
    const float gridW = gridCols * (cellSize + cellGap) - cellGap;
    const float gridH = gridRows * (cellSize + cellGap) - cellGap;

    // Rama albastră
    {
        glm::mat3 M(1);
        M *= transform2D::Translate(gridOffsetX - framePad, gridOffsetY - framePad);
        M *= transform2D::Scale(gridW + 2 * framePad, gridH + 2 * framePad);
        RenderMesh2D(meshes["outlineBlue"], shaders["VertexColor"], M);
    }

    // Sloturi
    for (const auto& s : toolbarSlots) {
        glm::mat3 R(1);
        R *= transform2D::Translate(s.boxX, s.boxY);
        R *= transform2D::Scale(s.boxW, s.boxH);
        RenderMesh2D(meshes["outlineRed"], shaders["VertexColor"], R);
    }

    // Grid
    for (int i = 0; i < gridRows; ++i)
        for (int j = 0; j < gridCols; ++j) {
            float x = gridOffsetX + j * (cellSize + cellGap);
            float y = gridOffsetY + i * (cellSize + cellGap);
            modelMatrix = glm::mat3(1);
            modelMatrix *= transform2D::Translate(x, y);
            RenderMesh2D(meshes["gridCell"], shaders["VertexColor"], modelMatrix);
        }

    // Top bar: blocuri verzi rămase
    int toDraw = maxBlocks - (int)shipPieces.size();
    if (toDraw < 0) toDraw = 0;
    if (toDraw > maxBlocks) toDraw = maxBlocks;
    for (int k = 0; k < toDraw; ++k) {
        float x = healthBarOffsetX + k * (healthBlockSize + healthBlockGap);
        float y = healthBarOffsetY;
        glm::mat3 T(1);
        T *= transform2D::Translate(x, y);
        RenderMesh2D(meshes["healthBlock"], shaders["VertexColor"], T);
    }

    // Buton Start + triunghi
    {
        glm::mat3 T(1);
        T *= transform2D::Translate(startButtonPos.x, startButtonPos.y);
        const char* btn = AllValidSetup() ? "startBtnReady" : "startBtn";
        RenderMesh2D(meshes[btn], shaders["VertexColor"], T);

        glm::mat3 N(1);
        N *= transform2D::Translate(startButtonPos.x + healthBlockSize, startButtonPos.y);
        N *= transform2D::Scale(healthBlockSize, healthBlockSize);
        RenderMesh2D(meshes["startNotch"], shaders["VertexColor"], N);
    }

    // Piese în toolbar
    for (const auto& s : toolbarSlots) {
        glm::mat3 M(1);
        M *= transform2D::Translate(s.drawX, s.drawY);
        if (s.meshName == "orangeCircle") {
            M *= transform2D::Translate(cellSize * 0.5f, cellSize * 0.5f);
            M *= transform2D::Scale(cellSize * 0.5f, cellSize * 0.5f);
        }
        RenderMesh2D(meshes[s.meshName], shaders["VertexColor"], M);
    }

    // Piese plasate pe grilă
    for (const auto& p : shipPieces) {
        glm::mat3 M(1);
        M *= transform2D::Translate(p.x, p.y);
        if (p.meshName == "orangeCircle") {
            M *= transform2D::Translate(cellSize * 0.5f, cellSize * 0.5f);
            M *= transform2D::Scale(cellSize * 0.5f, cellSize * 0.5f);
        }
        RenderMesh2D(meshes[p.meshName], shaders["VertexColor"], M);
    }
}

void Tema1::FrameEnd() {}

/* ==================== Input ==================== */

glm::ivec2 Tema1::GetGridCoords(float px, float py)
{
    int gx = (int)std::round((px - (gridOffsetX + cellSize * 0.5f)) / (cellSize + cellGap));
    int gy = (int)std::round((py - (gridOffsetY + cellSize * 0.5f)) / (cellSize + cellGap));
    gx = glm::clamp(gx, 0, gridCols - 1);
    gy = glm::clamp(gy, 0, gridRows - 1);
    return { gx, gy };
}

void Tema1::OnMouseBtnPress(int mouseX, int mouseY, int button, int)
{
    glm::vec2 V = ToVirtual(mouseX, mouseY);
    float x = V.x, y = V.y;

    const bool isLeft = (button == 1);
    const bool isRight = (button == 2);

    if (mode == Mode::GAME) return;

    if (isRight) {
        for (int i = (int)shipPieces.size() - 1; i >= 0; --i) {
            if (HitPiece(shipPieces[i], x, y)) {
                shipPieces.erase(shipPieces.begin() + i);
                selectedPieceIndex = -1;
                dragging = false;
                RebuildOcc();
                return;
            }
        }
        return;
    }

    if (isLeft) {
        // dacă layout-ul e valid și se apasă Start -> intrăm în joc (static)
        if (AllValidSetup() && IsInsideStartBtn(x, y)) {
            auto v = BuildVehicleFromEditor();
            StartGameFromVehicle(v);
            mode = Mode::GAME;
            return;
        }

        // click în toolbar -> spawn + drag
        for (const auto& s : toolbarSlots) {
            if (x > s.boxX && x < s.boxX + s.boxW &&
                y > s.boxY && y < s.boxY + s.boxH)
            {
                if ((int)shipPieces.size() >= maxBlocks) return;
                dragging = true;
                shipPieces.push_back({ x, y, s.meshName });
                selectedPieceIndex = (int)shipPieces.size() - 1;

                auto& p = shipPieces[selectedPieceIndex];
                dragOffsetX = x - p.x;
                dragOffsetY = y - p.y;

                prevCellX = prevCellY = -1;
                return;
            }
        }

        // click pe o piesă existentă -> drag
        for (int i = (int)shipPieces.size() - 1; i >= 0; --i) {
            if (x > shipPieces[i].x && x < shipPieces[i].x + cellSize &&
                y > shipPieces[i].y && y < shipPieces[i].y + cellSize)
            {
                dragging = true;
                selectedPieceIndex = i;
                dragOffsetX = x - shipPieces[i].x;
                dragOffsetY = y - shipPieces[i].y;

                float cx0 = shipPieces[i].x + cellSize * 0.5f;
                float cy0 = shipPieces[i].y + cellSize * 0.5f;
                bool on0 =
                    (cx0 > gridOffsetX && cy0 > gridOffsetY &&
                        cx0 < gridOffsetX + gridCols * (cellSize + cellGap) &&
                        cy0 < gridOffsetY + gridRows * (cellSize + cellGap));

                if (on0) {
                    auto g0 = GetGridCoords(cx0, cy0);
                    prevCellX = g0.x; prevCellY = g0.y;
                }
                else {
                    prevCellX = prevCellY = -1;
                }
                return;
            }
        }
    }
}

void Tema1::OnMouseMove(int mouseX, int mouseY, int, int)
{
    if (mode == Mode::GAME) return;
    if (!dragging || selectedPieceIndex == -1) return;

    glm::vec2 V = ToVirtual(mouseX, mouseY);
    auto& p = shipPieces[selectedPieceIndex];
    p.x = V.x - dragOffsetX;
    p.y = V.y - dragOffsetY;
}

void Tema1::OnMouseBtnRelease(int mouseX, int mouseY, int, int)
{
    if (mode == Mode::GAME) return;
    if (!dragging || selectedPieceIndex == -1) return;
    dragging = false;

    glm::vec2 V = ToVirtual(mouseX, mouseY);
    auto& p = shipPieces[selectedPieceIndex];

    float cx = p.x + cellSize * 0.5f;
    float cy = p.y + cellSize * 0.5f;

    bool overGrid =
        (cx > gridOffsetX &&
            cy > gridOffsetY &&
            cx < gridOffsetX + gridCols * (cellSize + cellGap) &&
            cy < gridOffsetY + gridRows * (cellSize + cellGap));

    if (overGrid) {
        glm::ivec2 gc = GetGridCoords(cx, cy);
        int occIdx = occ[gc.y][gc.x];
        bool sameCellAsBefore = (prevCellX == gc.x && prevCellY == gc.y);

        if (occIdx == -1 || sameCellAsBefore) {
            p.x = gridOffsetX + gc.x * (cellSize + cellGap);
            p.y = gridOffsetY + gc.y * (cellSize + cellGap);
            RebuildOcc();
        }
        else {
            if (prevCellX == -1 && prevCellY == -1) {
                shipPieces.erase(shipPieces.begin() + selectedPieceIndex);
            }
            else {
                p.x = gridOffsetX + prevCellX * (cellSize + cellGap);
                p.y = gridOffsetY + prevCellY * (cellSize + cellGap);
            }
            RebuildOcc();
        }
    }
    else {
        shipPieces.erase(shipPieces.begin() + selectedPieceIndex);
        RebuildOcc();
    }

    selectedPieceIndex = -1;
    prevCellX = prevCellY = -1;
}

void Tema1::OnWindowResize(int, int)
{
    auto cam = GetSceneCamera();
    cam->SetOrthographic(0, vW, 0, vH, 0.01f, 400);
    cam->Update();
}

glm::vec2 Tema1::ToVirtual(int mouseX, int mouseY) const
{
    auto res = window->GetResolution();
    float sx = vW / (float)res.x;
    float sy = vH / (float)res.y;
    float vx = mouseX * sx;
    float vy = ((float)res.y - mouseY) * sy; // invers Y
    return glm::vec2(vx, vy);
}

void Tema1::RebuildOcc()
{
    if (gridRows <= 0 || gridCols <= 0 || gridRows > 200 || gridCols > 200) {
        occ.clear();
        return;
    }
    occ.assign((size_t)gridRows, std::vector<int>((size_t)gridCols, -1));

    for (int idx = 0; idx < (int)shipPieces.size(); ++idx) {
        float cx = shipPieces[idx].x + cellSize * 0.5f;
        float cy = shipPieces[idx].y + cellSize * 0.5f;

        if (cx < gridOffsetX || cy < gridOffsetY) continue;
        if (cx >= gridOffsetX + gridCols * (cellSize + cellGap)) continue;
        if (cy >= gridOffsetY + gridRows * (cellSize + cellGap)) continue;

        glm::ivec2 g = GetGridCoords(cx, cy);
        if (g.y >= 0 && g.y < gridRows && g.x >= 0 && g.x < gridCols)
            occ[g.y][g.x] = idx;
    }
}

bool Tema1::CellOfPiece(const Piece& p, int& gx, int& gy)
{
    float cx = p.x + cellSize * 0.5f;
    float cy = p.y + cellSize * 0.5f;
    bool on =
        (cx > gridOffsetX && cy > gridOffsetY &&
            cx < gridOffsetX + gridCols * (cellSize + cellGap) &&
            cy < gridOffsetY + gridRows * (cellSize + cellGap));
    if (!on) return false;
    auto g = GetGridCoords(cx, cy);
    gx = g.x; gy = g.y;
    return true;
}

bool Tema1::IsPaddlePair(const std::vector<std::pair<int, int>>& cells)
{
    if (cells.size() != 2) return false;
    const int x1 = cells[0].first, y1 = cells[0].second;
    const int x2 = cells[1].first, y2 = cells[1].second;
    const bool sameRow = (y1 == y2) && (std::abs(x1 - x2) == 1);
    const bool sameCol = (x1 == x2) && (std::abs(y1 - y2) == 1);
    return sameRow || sameCol;
}

bool Tema1::CellsEdgeTouch(const std::pair<int, int>& a, const std::pair<int, int>& b) {
    int dx = std::abs(a.first - b.first);
    int dy = std::abs(a.second - b.second);
    return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

bool Tema1::ArePairsTouching(const std::vector<std::pair<int, int>>& A, const std::vector<std::pair<int, int>>& B) {
    for (const auto& a : A)
        for (const auto& b : B)
            if (CellsEdgeTouch(a, b))
                return true;
    return false;
}

bool Tema1::AllValidSetup()
{
    if ((int)shipPieces.size() != maxBlocks) return false;

    std::vector<std::pair<int, int>> reds, yellows;
    int orangeCountOnGrid = 0;

    for (const auto& p : shipPieces) {
        int gx, gy;
        if (!CellOfPiece(p, gx, gy)) return false;

        if (p.meshName == "redSquare")        reds.push_back({ gx, gy });
        else if (p.meshName == "yellowSquare") yellows.push_back({ gx, gy });
        else if (p.meshName == "orangeCircle") orangeCountOnGrid++;
        else return false;
    }

    if (ArePairsTouching(reds, yellows)) return false;
    if (reds.size() != 2 || yellows.size() != 2 || orangeCountOnGrid != 1) return false;
    if (!IsPaddlePair(reds))    return false;
    if (!IsPaddlePair(yellows)) return false;

    return true;
}

bool Tema1::IsInsideStartBtn(float x, float y)
{
    return x >= startButtonPos.x && x <= startButtonPos.x + healthBlockSize &&
        y >= startButtonPos.y && y <= startButtonPos.y + healthBlockSize;
}

/* ======= Transfer editor -> joc (static) ======= */

Tema1::Vehicle Tema1::BuildVehicleFromEditor()
{
    Vehicle out{};
    std::vector<std::pair<int, int>> r, y;
    std::pair<int, int> b{ -1,-1 };

    for (const auto& p : shipPieces) {
        int gx, gy; CellOfPiece(p, gx, gy);
        if (p.meshName == "redSquare")         r.push_back({ gx, gy });
        else if (p.meshName == "yellowSquare") y.push_back({ gx, gy });
        else if (p.meshName == "orangeCircle") b = { gx, gy };
    }
    out.reds[0] = r[0]; out.reds[1] = r[1];
    out.yellows[0] = y[0]; out.yellows[1] = y[1];
    out.ball = b;
    return out;
}

glm::vec2 Tema1::CellToWorld(int gx, int gy)
{
    return {
        gridOffsetX + gx * (cellSize + cellGap),
        gridOffsetY + gy * (cellSize + cellGap)
    };
}

void Tema1::StartGameFromVehicle(const Vehicle& v)
{
    // — 1) convertim celulele din editor la poziții world (pentru Y)
    auto cellCenter = [&](std::pair<int, int> c) {
        glm::vec2 p = CellToWorld(c.first, c.second);
        return p + glm::vec2(cellSize * 0.5f, cellSize * 0.5f);
        };
    auto mid = [](glm::vec2 a, glm::vec2 b) { return (a + b) * 0.5f; };

    glm::vec2 L0 = cellCenter(v.yellows[0]);
    glm::vec2 L1 = cellCenter(v.yellows[1]);
    glm::vec2 R0 = cellCenter(v.reds[0]);
    glm::vec2 R1 = cellCenter(v.reds[1]);

    float yLeft = mid(L0, L1).y;   // din editor luăm DOAR Y
    float yRight = mid(R0, R1).y;

    // — 2) dimensiuni paddle + margini față de perete
    const float paddleW = 16.f;
    const float paddleH = 110.f;
    const float wallPad = 60.f;   // cât de “în spate” stau

    // — 3) “lipim” pe stânga/dreapta și clampăm pe verticală în teren
    auto clampY = [&](float y) {
        float minY = arenaY + paddleH * 0.5f + 8.f;
        float maxY = arenaY + arenaH - paddleH * 0.5f - 8.f;
        return glm::clamp(y, minY, maxY);
        };

    leftP.pos = { arenaX + wallPad,           clampY(yLeft) };
    rightP.pos = { arenaX + arenaW - wallPad,  clampY(yRight) };

    leftP.w = rightP.w = paddleW;
    leftP.h = rightP.h = paddleH;
    leftP.col = glm::vec3(1, 1, 0);   // galben
    rightP.col = glm::vec3(1, 0, 0);   // roșu

    // — 4) bila: centru teren (doar desen deocamdată)
    ball.pos = { arenaX + arenaW * 0.5f, arenaY + arenaH * 0.5f };
    ball.r = 10.f;
    ball.speed = 520.f;
    // pornește ușor în sus spre dreapta
    ball.vel = glm::normalize(glm::vec2(1.f, 0.1f)) * ball.speed;

    // viteza paletelor
    leftP.speed = rightP.speed = 600.f;

    
}
bool Tema1::CircleAABB(glm::vec2 c, float r, glm::vec4 aabb)
{
    // aabb: (x,y,w,h) cu (x,y) colțul stânga-jos
    float x = aabb.x, y = aabb.y, w = aabb.z, h = aabb.w;
    float cx = std::max(x, std::min(c.x, x + w));
    float cy = std::max(y, std::min(c.y, y + h));
    float dx = c.x - cx, dy = c.y - cy;
    return (dx * dx + dy * dy) <= (r * r);
}
void Tema1::ResetBall(int dir)
{
    ball.pos = { arenaX + arenaW * 0.5f, arenaY + arenaH * 0.5f };
    // mic unghi ca să nu fie perfect orizontal
    glm::vec2 d = glm::normalize(glm::vec2((float)dir, 0.15f));
    ball.vel = d * ball.speed;
}

/* ======= Desen joc static ======= */


void Tema1::DrawRectCellMesh(const char* mesh, glm::vec2 center, float w, float h) {
    glm::mat3 M(1);
    M *= transform2D::Translate(center.x - w * 0.5f, center.y - h * 0.5f);
    M *= transform2D::Scale(w / cellSize, h / cellSize); // <- normalizare
    RenderMesh2D(meshes[mesh], shaders["VertexColor"], M);
}
void Tema1::DrawDigit(int d, float x, float y, float s)
{
    // font 3x5 (1 = pixel aprins)
    static const int D[10][15] = {
        // 0
        {1,1,1,
         1,0,1,
         1,0,1,
         1,0,1,
         1,1,1},
         // 1
         {0,1,0,
          1,1,0,
          0,1,0,
          0,1,0,
          1,1,1},
          // 2
          {1,1,1,
           0,0,1,
           1,1,1,
           1,0,0,
           1,1,1},
           // 3
           {1,1,1,
            0,0,1,
            1,1,1,
            0,0,1,
            1,1,1},
            // 4
            {1,0,1,
             1,0,1,
             1,1,1,
             0,0,1,
             0,0,1},
             // 5
             {1,1,1,
              1,0,0,
              1,1,1,
              0,0,1,
              1,1,1},
              // 6
              {1,1,1,
               1,0,0,
               1,1,1,
               1,0,1,
               1,1,1},
               // 7
               {1,1,1,
                0,0,1,
                0,1,0,
                0,1,0,
                0,1,0},
                // 8
                {1,1,1,
                 1,0,1,
                 1,1,1,
                 1,0,1,
                 1,1,1},
                 // 9
                 {1,1,1,
                  1,0,1,
                  1,1,1,
                  0,0,1,
                  1,1,1}
    };

    const float px = s;           // mărimea “pixelului”
    const float gap = px * 0.25f; // spațiere mică între pixeli

    for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 3; ++c) {
            if (D[d][r * 3 + c]) {
                glm::mat3 M(1);
                float dx = x + c * (px + gap);
                float dy = y + (4 - r) * (px + gap); // (0 sus)
                M *= transform2D::Translate(dx, dy);
                M *= transform2D::Scale(px, px);
                RenderMesh2D(meshes["whiteFill"], shaders["VertexColor"], M);
            }
        }
}

void Tema1::DrawNumber(int n, float x, float y, float s)
{
    // desenează până la 3 cifre (0..999) – suficient pentru scor
    if (n == 0) { DrawDigit(0, x, y, s); return; }
    int digits[4]; int k = 0;
    while (n > 0 && k < 4) { digits[k++] = n % 10; n /= 10; }
    float advance = 3 * s + 2 * (s * 0.25f) + s; // lățime digit + extra spațiu
    for (int i = k - 1; i >= 0; --i) {
        DrawDigit(digits[i], x, y, s);
        x += advance;
    }
}
void Tema1::RenderGameBasic() {
    // fundal închis (opțional poți lăsa glClearColor negru)
    // chenar teren
    {
        glm::mat3 M(1);
        M *= transform2D::Translate(arenaX, arenaY);
        M *= transform2D::Scale(arenaW, arenaH);
        RenderMesh2D(meshes["whiteStroke"], shaders["VertexColor"], M);
    }
    // linie punctată la mijloc
    {
        const int dashCount = 12;
        float dashH = arenaH / (dashCount * 1.6f);
        float gap = dashH * 0.6f;
        float xMid = arenaX + arenaW * 0.5f - 2.0f; // ușor centrat
        for (int i = 0; i < dashCount; ++i) {
            float y = arenaY + 10.f + i * (dashH + gap);
            glm::mat3 M(1);
            M *= transform2D::Translate(xMid, y);
            M *= transform2D::Scale(4.f, dashH);      // 4px grosime
            RenderMesh2D(meshes["whiteFill"], shaders["VertexColor"], M);
        }
    }
    // palete din centrul perechilor plasate în editor
   // palete
    DrawRectCellMesh("yellowSquare", leftP.pos, leftP.w, leftP.h);
    DrawRectCellMesh("redSquare", rightP.pos, rightP.w, rightP.h);

    // bila (doar desen, fără mișcare)
    {
        glm::mat3 M(1);
        M *= transform2D::Translate(ball.pos.x, ball.pos.y);
        M *= transform2D::Scale(ball.r, ball.r);
        RenderMesh2D(meshes["orangeCircle"], shaders["VertexColor"], M);
    }
    float s = 6.0f; // mărimea pixelului
    DrawNumber(scoreL, arenaX + 8.f, arenaY + arenaH + 18.f, s);
    DrawNumber(scoreR, arenaX + arenaW - 90.f, arenaY + arenaH + 18.f, s);
}
void Tema1::OnInputUpdate(float dt, int mods) {
    if (mode != Mode::GAME) return;

    keyW = window->KeyHold(GLFW_KEY_W);
    keyS = window->KeyHold(GLFW_KEY_S);
    keyUp = window->KeyHold(GLFW_KEY_UP);
    keyDown = window->KeyHold(GLFW_KEY_DOWN);
}
void Tema1::UpdateGame(float dt)
{
    // ===== 1) input palete
    int dirL = (keyW ? 1 : 0) + (keyS ? -1 : 0);
    int dirR = (keyUp ? 1 : 0) + (keyDown ? -1 : 0);

    leftP.pos.y += dirL * leftP.speed * dt;
    rightP.pos.y += dirR * rightP.speed * dt;

    float minY = arenaY + leftP.h * 0.5f + 8.f;
    float maxY = arenaY + arenaH - leftP.h * 0.5f - 8.f;
    leftP.pos.y = glm::clamp(leftP.pos.y, minY, maxY);
    rightP.pos.y = glm::clamp(rightP.pos.y, minY, maxY);

    // ===== 2) mișcare bilă
    ball.pos += ball.vel * dt;

    // ===== 3) pereții sus/jos – reflexie pe Y
    if (ball.pos.y - ball.r < arenaY) {
        ball.pos.y = arenaY + ball.r;
        ball.vel.y = std::abs(ball.vel.y);
    }
    if (ball.pos.y + ball.r > arenaY + arenaH) {
        ball.pos.y = arenaY + arenaH - ball.r;
        ball.vel.y = -std::abs(ball.vel.y);
    }

    // ===== 4) goluri stânga/dreapta
    if (ball.pos.x + ball.r < arenaX) {
        // a trecut de linia din stânga -> Right marchează
        scoreR++;
        ResetBall(+1);      // pornește spre dreapta (spre marcator)
        return;
    }
    if (ball.pos.x - ball.r > arenaX + arenaW) {
        // a trecut de linia din dreapta -> Left marchează
        scoreL++;
        ResetBall(-1);      // pornește spre stânga (spre marcator)
        return;
    }

    // ===== 5) coliziuni cu paletele (AABB)
    auto leftAABB = glm::vec4(leftP.pos.x - leftP.w * 0.5f, leftP.pos.y - leftP.h * 0.5f, leftP.w, leftP.h);
    auto rightAABB = glm::vec4(rightP.pos.x - rightP.w * 0.5f, rightP.pos.y - rightP.h * 0.5f, rightP.w, rightP.h);

    // stânga – lovește doar când vine dinspre stânga
    if (ball.vel.x < 0.f && CircleAABB(ball.pos, ball.r, leftAABB)) {
        // scoate bila din paletă
        ball.pos.x = leftP.pos.x + leftP.w * 0.5f + ball.r + 1.f;

        // unghiul în [-1,1] rad (~±57°)
        float t = (ball.pos.y - leftP.pos.y) / (leftP.h * 0.5f);
        t = glm::clamp(t, -1.0f, 1.0f);
        float angle = t; // deja în radiani conform cerinței

        ball.vel.x = ball.speed * std::cos(angle);  // pozitiv (spre dreapta)
        ball.vel.y = ball.speed * std::sin(angle);
    }

    // dreapta – lovește doar când vine dinspre dreapta
    if (ball.vel.x > 0.f && CircleAABB(ball.pos, ball.r, rightAABB)) {
        // scoate bila din paletă
        ball.pos.x = rightP.pos.x - rightP.w * 0.5f - ball.r - 1.f;

        float t = (ball.pos.y - rightP.pos.y) / (rightP.h * 0.5f);
        t = glm::clamp(t, -1.0f, 1.0f);
        float angle = t;

        ball.vel.x = -ball.speed * std::cos(angle);  // negativ (spre stânga)
        ball.vel.y = ball.speed * std::sin(angle);
    }
}
