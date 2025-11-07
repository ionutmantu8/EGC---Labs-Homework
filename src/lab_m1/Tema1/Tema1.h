#pragma once
#include "components/simple_scene.h"
#include "lab_m1/lab3/transform2D.h"
#include "lab_m1/lab3/object2D.h"
#include <string>
#include <vector>
#include <GLFW/glfw3.h>

namespace m1
{
    class Tema1 : public gfxc::SimpleScene
    {
    public:
        Tema1();
        ~Tema1();

        void Init() override;
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnWindowResize(int width, int height) override;

    private:
        // ===== Helper mesh
        Mesh* CreateMesh(const char* name,
            const std::vector<VertexFormat>& vertices,
            const std::vector<unsigned int>& indices);

        // ===== Grid helper
        glm::ivec2 GetGridCoords(float px, float py);
        glm::vec2  ToVirtual(int mouseX, int mouseY) const;

        struct Piece {
            float x, y;
            std::string meshName;
        };

        struct ToolbarSlot {
            float boxX, boxY, boxW, boxH;  // hit-box
            float drawX, drawY;            // poziția piesei în cutie
            std::string meshName;
        };

        // ===== Layout (EDITOR)
        float PANEL_PAD;      // 24
        float TOPBAR_H;       // 110
        float SIDEBAR_X;      // 24
        float SIDEBAR_W;      // 260
        float sideSlotH;      // calc
        float sidebarTopY;    // calc

        // Grid (EDITOR)
        int   gridRows, gridCols;
        float cellSize, cellGap;
        float gridOffsetX, gridOffsetY;

        // Top bar (EDITOR)
        int   maxBlocks;
        float healthBlockSize, healthBlockGap;
        float healthBarOffsetX, healthBarOffsetY;
        glm::vec3 startButtonPos;

        // Editor state
        std::vector<ToolbarSlot> toolbarSlots;
        std::vector<Piece>       shipPieces;
        std::vector<std::vector<int>> occ;
        bool  dragging = false;
        int   selectedPieceIndex = -1;
        float dragOffsetX = 0.f, dragOffsetY = 0.f;
        int   prevCellX = -1, prevCellY = -1;

        // ===== Verificări editor
        void RebuildOcc();
        bool CellOfPiece(const Piece& p, int& gx, int& gy);
        static bool IsPaddlePair(const std::vector<std::pair<int, int>>& cells);
        bool CellsEdgeTouch(const std::pair<int, int>& a, const std::pair<int, int>& b);
        bool ArePairsTouching(const std::vector<std::pair<int, int>>& A, const std::vector<std::pair<int, int>>& B);
        bool AllValidSetup();
        bool IsInsideStartBtn(float x, float y);

        // ===== Mod de lucru
        enum class Mode { EDITOR, GAME };
        Mode mode = Mode::EDITOR;

        // ===== Joc (doar desen, fără logică)
        struct Paddle { glm::vec2 pos; float w, h; glm::vec3 col; float speed; };
        struct Ball {
            glm::vec2 pos;
            glm::vec2 vel;   // <— NOU
            float r;
            float speed;
        };      
        bool keyW = false, keyS = false, keyUp = false, keyDown = false;
        const int maxScore = 10;
        int scoreL = 0, scoreR = 0;


        // Arena Pong
        float arenaX = 140.f, arenaY = 70.f;
        float arenaW = 980.f, arenaH = 600.f;

        Paddle leftP{}, rightP{};
        Ball   ball{};

        // Transfer din editor în joc
        struct Vehicle {
            std::pair<int, int> reds[2];
            std::pair<int, int> yellows[2];
            std::pair<int, int> ball; // (gx, gy)
        };
        Vehicle   BuildVehicleFromEditor();
        glm::vec2 CellToWorld(int gx, int gy);
        void      StartGameFromVehicle(const Vehicle& v);

        // Desen joc static
        void RenderGameStatic();
        void OnInputUpdate(float dt, int mods) override;

        // Misc
        glm::mat3 modelMatrix;
        float vW = 0.f, vH = 0.f;
        bool HitPiece(const Piece& p, float x, float y) const {
            // piesele sunt toate încadrate într-un pătrat de dimensiune cellSize
            return (x > p.x && x < p.x + cellSize &&
                y > p.y && y < p.y + cellSize);
        }
        void DrawRectCellMesh(const char* mesh, glm::vec2 center, float w, float h);
        void RenderGameBasic();
        void Tema1::UpdateGame(float dt);
        bool CircleAABB(glm::vec2 c, float r, glm::vec4 aabb); // aabb = (x,y,w,h) cu (x,y) colț stânga-jos
        void ResetBall(int dir); // dir = -1 pornește spre stânga, +1 spre dreapta
        void DrawDigit(int d, float x, float y, float s);   // digit 0..9
        void DrawNumber(int n, float x, float y, float s);  // număr din mai multe digits
    };
}
