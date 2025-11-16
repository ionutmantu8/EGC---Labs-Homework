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

        // Overridden functions from SimpleScene
        void Init() override;
        void FrameStart() override;
        void Update(float deltaTimeSeconds) override;
        void FrameEnd() override;

        // Overridden input callback functions
        void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
        void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
        void OnWindowResize(int width, int height) override;
        void OnInputUpdate(float dt, int mods) override;

    private:
        // Helper function to create a custom mesh
        Mesh* CreateMesh(const char* name,
            const std::vector<VertexFormat>& vertices,
            const std::vector<unsigned int>& indices);

        // Converts screen pixel coordinates to grid coordinates
        glm::ivec2 GetGridCoords(float px, float py);
        // Converts raw mouse coordinates to virtual world coordinates
        glm::vec2 ToVirtual(int mouseX, int mouseY) const;

        // Represents a draggable piece in the editor
        struct Piece {
            float x, y; // World position
            std::string meshName; // Mesh to render
        };

        // Represents a slot in the side toolbar
        struct ToolbarSlot {
            float boxX, boxY, boxW, boxH; // Clickable area (hit-box)
            float drawX, drawY; // Draw position for the piece inside
            std::string meshName;
        };

        // Editor UI layout variables
        float PANEL_PAD = 24.f;  // Padding around panels
        float TOPBAR_H = 110.f; // Height of the top bar
        float SIDEBAR_X = 24.f; // X-position of the side bar
        float SIDEBAR_W = 260.f; // Width of the side bar
        float sideSlotH = 0.f;  // Calculated slot height
        float sidebarTopY = 0.f; // Calculated Y-position of the top slot

        // Editor Grid properties
        int gridRows = 9, gridCols = 17;
        float cellSize = 40.f, cellGap = 8.f;
        float gridOffsetX = 0.f, gridOffsetY = 0.f; // Calculated top-left corner of the grid

        // Editor Top Bar (piece counter)
        int maxBlocks = 9; // Max pieces allowed
        float healthBlockSize = 48.f, healthBlockGap = 34.f;
        float healthBarOffsetX = 0.f, healthBarOffsetY = 0.f; // Calculated position
        glm::vec3 startButtonPos = glm::vec3(0); // Calculated position of the start button

        // State variables for the editor
        std::vector<ToolbarSlot> toolbarSlots; // List of all toolbar slots
        std::vector<Piece> shipPieces; // List of all pieces (in toolbar or on grid)
        std::vector<std::vector<int>> occ; // 2D grid for occupation, stores index of piece or -1
        bool dragging = false; // Is the user currently dragging a piece?
        int selectedPieceIndex = -1; // Index of the piece being dragged
        float dragOffsetX = 0.f, dragOffsetY = 0.f; // Offset from piece origin to mouse click
        int prevCellX = -1, prevCellY = -1; // Original cell of a piece being dragged

        // Editor validation functions
        void RebuildOcc(); // Recalculates the occupation grid
        bool CellOfPiece(const Piece& p, int& gx, int& gy); // Gets the grid cell for a piece
        bool CellsEdgeTouch(const std::pair<int, int>& a, const std::pair<int, int>& b); // Checks if two cells are adjacent
        bool ArePairsTouching(const std::vector<std::pair<int, int>>& A, const std::vector<std::pair<int, int>>& B); // Checks if two groups of cells touch
        bool AllValidSetup(); // Checks all editor rules to see if game can start
        bool IsInsideStartBtn(float x, float y); // AABB check for start button
        bool IsPaddleTriple(const std::vector<std::pair<int, int>>& cells); // Checks if 3 cells form a valid 1x3 or 3x1 paddle

        // Game state machine
        enum class Mode { EDITOR, GAME };
        Mode mode = Mode::EDITOR; // Start in editor mode

        // In-Game objects
        struct Paddle { glm::vec2 pos; float w, h; glm::vec3 col; float speed; };

        struct Ball {
            glm::vec2 pos; // Center position
            glm::vec2 vel; // Velocity vector
            float r = 10.f;
            float speed = 400.f;
            bool alive = true; // Is the ball still in play?
        };

        // Input state for paddles
        bool keyW = false, keyS = false, keyUp = false, keyDown = false;
        int scoreL = 0, scoreR = 0; // Player scores

        // Pong arena dimensions
        float arenaX = 140.f, arenaY = 70.f; // Bottom-left corner
        float arenaW = 980.f, arenaH = 600.f; // Width and height

        Paddle leftP{}, rightP{}; // The two paddle objects

        // Data structure for transferring editor setup to game
        struct Vehicle {
            std::pair<int, int> reds[3]; // Grid cells for red paddle
            std::pair<int, int> yellows[3]; // Grid cells for yellow paddle
            std::pair<int, int> ball; // Grid cell for the ball
            std::vector<std::pair<int, int>> bumpers; // Grid cells for bumpers
        };
        Vehicle BuildVehicleFromEditor(); // Converts shipPieces to a Vehicle
        glm::vec2 CellToWorld(int gx, int gy); // Converts grid cell to world position
        void StartGameFromVehicle(const Vehicle& v); // Initializes game state from a Vehicle

        // Game logic and rendering
        void RenderGameBasic(); // Renders all game objects
        void UpdateGame(float dt); // Runs all game logic (physics, scoring)

        // Misc variables
        glm::mat3 modelMatrix = glm::mat3(1.0f);
        float vW = 0.f, vH = 0.f; // Virtual window width and height

        // Inline AABB check for clicking on a piece in the editor
        bool HitPiece(const Piece& p, float x, float y) const {
            // Pieces are considered 'cellSize' squares for clicking
            return (x > p.x && x < p.x + cellSize &&
                y > p.y && y < p.y + cellSize);
        }
        void DrawRectCellMesh(const char* mesh, glm::vec2 center, float w, float h); // Draws a scaled mesh
        bool CircleAABB(glm::vec2 c, float r, glm::vec4 aabb); // Circle-AABB collision check
        void DrawDigit(int d, float x, float y, float s); // Renders a single score digit
        void DrawNumber(int n, float x, float y, float s); // Renders a multi-digit score
        void ResetMatch(); // Resets the score and starts a new game
        int maxScore = 10; // Score needed to win

        // Bumper logic
        void DrawBumperAt(float x, float y); // Renders a bumper in the editor
        struct AttachedBumper {
            bool leftSide = true; // Is it attached to the left (yellow) paddle?
            bool onPaddle = true;
            float relX = 0.f;  // Relative X offset from paddle center (unused)
            float relY = 0.f; // Relative Y offset from paddle center
        };
        std::vector<AttachedBumper> attachedBumpers; // List of all active in-game bumpers
        void DrawBumperFacing(bool faceRight, glm::vec2 center, glm::vec2 scale); // Renders an in-game bumper
        glm::vec2 BumperCenter(const AttachedBumper& ab); // Calculates the world position of an attached bumper
        bool PaddleVertical(const std::vector<std::pair<int, int>>& cells); // Checks paddle orientation
        bool BumperIsOnPaddle(const std::pair<int, int>& bp, const std::vector<std::pair<int, int>>& cells); // (Unused) Checks if bumper is on paddle
        bool TouchesAny(int gx, int gy, const std::vector<std::pair<int, int>>& cells); // Checks adjacency for bumper association

        // Visual feedback timers
        float leftHitTime = 0.0f; // Remaining time for left paddle hit animation
        float rightHitTime = 0.0f;

        // Camera shake effect
        float shakeTime = 0.0f; // Remaining time for shake
        float shakeDuration = 0.0f; // Total duration of the current shake
        float shakeIntensity = 0.0f; // Max offset of the shake
        glm::vec2 shakeOffset = glm::vec2(0.0f); // The calculated offset for this frame
        glm::vec2 ComputeShake(); // Calculates the shakeOffset

        // Particle system
        struct Particle {
            glm::vec2 pos;
            glm::vec2 vel;
            float size;
            float life; // Remaining time
            float life0; // Initial lifetime
            bool orange; // Color (orange or yellow)
        };
        std::vector<Particle> particles; // List of all active particles
        void SpawnBallExplosion(glm::vec2 p, bool rightSide); // Creates a particle burst
        void UpdateParticles(float dt); // Applies physics to particles
        void RenderParticles(m1::Tema1* self); // Renders particles

        // Collisions
        int bumperHitCooldownFrames = 0; // Cooldown after a bumper hit
        bool SweepCircleAABB(const glm::vec2& p0, const glm::vec2& delta, float r,
            const glm::vec4& aabb, float& tHit, glm::vec2& nHit); // Continuous collision detection

        // Multi-ball & round logic
        std::vector<Ball> balls; // All active balls in the current round
        int roundBalls = 1;   // How many balls to spawn this round
        int roundLeft = 0;  // Balls scored by left this round
        int roundRight = 0;  // Balls scored by right this round
        int roundAlive = 0; // How many balls are still in play
        void StartRound(); // Resets and spawns new balls

        // Coordinate conversion functions
        glm::vec2 EditorCellCenter(int gx, int gy); // Gets the pixel center of a grid cell
        glm::vec2 EditorPosToUV(glm::vec2 p); // Converts editor pixel pos to normalized UV [0,1]
        glm::vec2 ArenaPosFromUV(glm::vec2 uv); // Converts normalized UV [0,1] to game arena pixel pos
    };
}