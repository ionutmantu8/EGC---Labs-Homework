#include "lab_m1/Tema1/Tema1.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdlib>

using namespace m1;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr float kBumperGameScale = 0.75f;

Tema1::Tema1() {}
Tema1::~Tema1() {}

void Tema1::Init()
{
	// Get window resolution
	auto res = window->GetResolution();
	vW = (float)res.x;
	vH = (float)res.y;

	// Set up the orthographic camera
	auto cam = GetSceneCamera();
	cam->SetOrthographic(0, vW, 0, vH, 0.01f, 400);
	cam->SetPosition(glm::vec3(0, 0, 50));
	cam->SetRotation(glm::vec3(0, 0, 0));
	cam->Update();
	GetCameraInput()->SetActive(false); // Disable default camera controls

	// Define basic padding and panel sizes
	PANEL_PAD = 24.0f;
	TOPBAR_H = 110.0f;

	SIDEBAR_X = PANEL_PAD;
	SIDEBAR_W = 260.0f;

	// Calculate dimensions for the side toolbar slots
	float usableSideH = (float)res.y - 2.f * PANEL_PAD;
	sideSlotH = usableSideH / 3.f;
	sidebarTopY = (float)res.y - PANEL_PAD - sideSlotH;

	// Grid layout properties
	cellSize = 40.0f;
	cellGap = 8.0f;
	gridRows = 9;
	gridCols = 17;

	// Initialize the occupation grid (occ) to -1 (empty)
	occ.assign((size_t)gridRows, std::vector<int>((size_t)gridCols, -1));
	prevCellX = prevCellY = -1; // Used for drag-and-drop logic

	// Calculate the physical screen position of the editor grid
	const float framePad = 26.0f;
	const float gridW = gridCols * (cellSize + cellGap) - cellGap;
	const float gridH = gridRows * (cellSize + cellGap) - cellGap;

	float frameX = SIDEBAR_X + SIDEBAR_W + PANEL_PAD;
	float frameY = PANEL_PAD + 10.0f;
	float frameTop = frameY + (gridH + 2 * framePad);

	// Adjust grid position vertically to align with the top bar
	float desiredTop = (float)res.y - (TOPBAR_H + 10.0f);
	float deltaY = desiredTop - frameTop;
	frameY += deltaY;

	// Final calculated offsets for drawing the grid
	gridOffsetX = frameX + framePad;
	gridOffsetY = frameY + framePad;

	// Create all necessary 2D meshes for the editor and game

	// Editor UI meshes
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

	// Game piece meshes
	AddMeshToList(object2D::CreateSquare("redSquare",
		glm::vec3(0, 0, 1), cellSize, glm::vec3(1, 0, 0), true));
	AddMeshToList(object2D::CreateSquare("yellowSquare",
		glm::vec3(0, 0, 1), cellSize, glm::vec3(1, 1, 0), true));

	// Particle meshes
	AddMeshToList(object2D::CreateSquare("partYellow",
		glm::vec3(0, 0, 0), 1.0f, glm::vec3(1.0f, 0.95f, 0.30f), true));
	AddMeshToList(object2D::CreateSquare("partOrange",
		glm::vec3(0, 0, 0), 1.0f, glm::vec3(1.0f, 0.55f, 0.10f), true));

	// Ball mesh (procedurally generated circle)
	{
		const int seg = 40;
		std::vector<VertexFormat> v; std::vector<unsigned> ind;
		glm::vec3 col(1.0f, 0.55f, 0.0f);
		v.emplace_back(glm::vec3(0, 0, 1), col); // Center vertex
		for (int i = 0; i <= seg; ++i) {
			float t = (float)i / seg * 2.f * (float)M_PI;
			v.emplace_back(glm::vec3(std::cos(t), std::sin(t), 1), col);
		}
		// Create triangles from center to circumference
		for (int i = 1; i <= seg; ++i) { ind.push_back(0); ind.push_back(i); ind.push_back(i + 1); }
		AddMeshToList(CreateMesh("orangeCircle", v, ind));
	}

	// Start button play icon (triangle notch)
	{
		std::vector<VertexFormat> v; std::vector<unsigned> ind = { 0,1,2 };
		const float notchBottom = 0.10f, notchTop = 0.90f, notchDepth = -0.50f;
		v.emplace_back(glm::vec3(0.0f, notchBottom, 3), glm::vec3(0, 0, 0));
		v.emplace_back(glm::vec3(0.0f, notchTop, 3), glm::vec3(0, 0, 0));
		v.emplace_back(glm::vec3(notchDepth, 0.50f, 3), glm::vec3(0, 0, 0));
		AddMeshToList(CreateMesh("startNotch", v, ind));
	}

	// Pong arena meshes (white fill and stroke)
	AddMeshToList(object2D::CreateSquare("whiteFill",
		glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.95f, 0.95f, 0.98f), true));
	AddMeshToList(object2D::CreateSquare("whiteStroke",
		glm::vec3(0, 0, 0), 1.0f, glm::vec3(0.95f, 0.95f, 0.98f), false));

	// Bumper mesh (procedural semicircle cap)
	{
		const int seg = 40;
		std::vector<VertexFormat> v; std::vector<unsigned> ind;
		glm::vec3 capCol(0.96f, 0.95f, 0.78f); // Light beige
		v.emplace_back(glm::vec3(0, 0, 1), capCol); // Center
		for (int i = 0; i <= seg; ++i) {
			float t = (float)i / seg * (float)M_PI; // 0 to PI (semicircle)
			v.emplace_back(glm::vec3(std::cos(t), std::sin(t), 1), capCol);
		}
		for (int i = 1; i <= seg; ++i) { ind.push_back(0); ind.push_back(i); ind.push_back(i + 1); }
		AddMeshToList(CreateMesh("bumperCap", v, ind));
	}
	// Bumper stem (simple square)
	AddMeshToList(object2D::CreateSquare("bumperStem",
		glm::vec3(0, 0, 1), 1.0f, glm::vec3(0.80f, 0.80f, 0.80f), true));

	// Initialize the 4 slots in the side toolbar
	toolbarSlots.clear();
	sideSlotH = ((float)res.y - 2.f * PANEL_PAD) / 4.f; // Recalculate for 4 slots
	sidebarTopY = (float)res.y - PANEL_PAD - sideSlotH;

	for (int k = 0; k < 4; ++k) {
		ToolbarSlot s;
		s.boxX = SIDEBAR_X;
		s.boxY = sidebarTopY - k * sideSlotH;
		s.boxW = SIDEBAR_W;
		s.boxH = sideSlotH;
		// Center the piece inside the slot for drawing
		s.drawX = s.boxX + (s.boxW - cellSize) * 0.5f;
		s.drawY = s.boxY + (s.boxH - cellSize) * 0.5f;

		// Assign the correct mesh to each slot
		if (k == 0) s.meshName = "redSquare";
		else if (k == 1) s.meshName = "yellowSquare";
		else if (k == 2) s.meshName = "orangeCircle";
		else s.meshName = "bumper";

		toolbarSlots.push_back(s);
	}

	// Position the "health" (remaining blocks) bar and start button
	maxBlocks = 9;
	healthBlockSize = 48.0f;
	healthBlockGap = 34.0f;
	healthBarOffsetX = SIDEBAR_X + SIDEBAR_W + PANEL_PAD; // Align with grid frame
	healthBarOffsetY = (float)res.y - TOPBAR_H + (TOPBAR_H - healthBlockSize) * 0.5f;
	startButtonPos = glm::vec3(
		healthBarOffsetX + maxBlocks * (healthBlockSize + healthBlockGap),
		healthBarOffsetY, 0);
}

void Tema1::FrameStart()
{
	// Clears the color and depth buffers at the start of each frame
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	auto res = window->GetResolution();
	// Sets the viewport to the entire window
	glViewport(0, 0, res.x, res.y);
}

void Tema1::Update(float dt)
{
	// Check the current mode (EDITOR or GAME) and call the appropriate functions
	if (mode == Mode::GAME) {
		UpdateGame(dt);       // Run game logic
		UpdateParticles(dt);  // Update particle physics
		RenderGameBasic();    // Render the game state
		return;
	}

	// If not in GAME mode, render the editor interface

	// Calculate grid dimensions for rendering
	const float framePad = 26.0f;
	const float gridW = gridCols * (cellSize + cellGap) - cellGap;
	const float gridH = gridRows * (cellSize + cellGap) - cellGap;

	//Draw the blue frame around the grid
	{
		glm::mat3 M(1);
		M *= transform2D::Translate(gridOffsetX - framePad, gridOffsetY - framePad);
		M *= transform2D::Scale(gridW + 2 * framePad, gridH + 2 * framePad);
		RenderMesh2D(meshes["outlineBlue"], shaders["VertexColor"], M);
	}

	//Draw the red outlines for the toolbar slots
	for (const auto& s : toolbarSlots) {
		glm::mat3 R(1);
		R *= transform2D::Translate(s.boxX, s.boxY);
		R *= transform2D::Scale(s.boxW, s.boxH);
		RenderMesh2D(meshes["outlineRed"], shaders["VertexColor"], R);
	}

	//Draw all the individual grid cells
	for (int i = 0; i < gridRows; ++i)
		for (int j = 0; j < gridCols; ++j) {
			float x = gridOffsetX + j * (cellSize + cellGap);
			float y = gridOffsetY + i * (cellSize + cellGap);
			modelMatrix = glm::mat3(1);
			modelMatrix *= transform2D::Translate(x, y);
			RenderMesh2D(meshes["gridCell"], shaders["VertexColor"], modelMatrix);
		}

	// Draw the top bar "health" (remaining blocks counter)
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

	//Draw the Start Button
	{
		glm::mat3 T(1);
		T *= transform2D::Translate(startButtonPos.x, startButtonPos.y);
		// Button color changes to green if the setup is valid
		const char* btn = AllValidSetup() ? "startBtnReady" : "startBtn";
		RenderMesh2D(meshes[btn], shaders["VertexColor"], T);

		// Draw the play icon (triangle) on the button
		glm::mat3 N(1);
		N *= transform2D::Translate(startButtonPos.x + healthBlockSize, startButtonPos.y);
		N *= transform2D::Scale(healthBlockSize, healthBlockSize);
		RenderMesh2D(meshes["startNotch"], shaders["VertexColor"], N);
	}

	//Draw the pieces inside the toolbar slots
	for (const auto& s : toolbarSlots) {
		if (s.meshName == "orangeCircle") {
			glm::mat3 M(1);
			M *= transform2D::Translate(s.drawX + cellSize * 0.5f, s.drawY + cellSize * 0.5f);
			M *= transform2D::Scale(cellSize * 0.5f, cellSize * 0.5f);
			RenderMesh2D(meshes["orangeCircle"], shaders["VertexColor"], M);
		}
		else if (s.meshName == "bumper") {
			DrawBumperAt(s.drawX, s.drawY); // Special draw call for bumper
		}
		else {
			glm::mat3 M(1);
			M *= transform2D::Translate(s.drawX, s.drawY);
			RenderMesh2D(meshes[s.meshName], shaders["VertexColor"], M);
		}
	}

	//Draw the pieces placed on the grid (or being dragged)
	for (const auto& p : shipPieces) {
		if (p.meshName == "orangeCircle") {
			glm::mat3 M(1);
			M *= transform2D::Translate(p.x + cellSize * 0.5f, p.y + cellSize * 0.5f);
			M *= transform2D::Scale(cellSize * 0.5f, cellSize * 0.5f);
			RenderMesh2D(meshes["orangeCircle"], shaders["VertexColor"], M);
		}
		else if (p.meshName == "bumper") {
			DrawBumperAt(p.x, p.y); // Special draw call for bumper
		}
		else {
			glm::mat3 M(1);
			M *= transform2D::Translate(p.x, p.y);
			RenderMesh2D(meshes[p.meshName], shaders["VertexColor"], M);
		}
	}
}

void Tema1::FrameEnd() {}


void Tema1::OnInputUpdate(float dt, int)
{
	// Handle continuous key presses (keyboard input)
	if (mode != Mode::GAME) return; // Only in game mode

	// Check which keys are held down for paddle movement
	keyW = window->KeyHold(GLFW_KEY_W);
	keyS = window->KeyHold(GLFW_KEY_S);
	keyUp = window->KeyHold(GLFW_KEY_UP);
	keyDown = window->KeyHold(GLFW_KEY_DOWN);
}

void Tema1::OnWindowResize(int, int)
{
	// Update the camera's orthographic projection when the window is resized
	auto cam = GetSceneCamera();
	cam->SetOrthographic(0, vW, 0, vH, 0.01f, 400);
	cam->Update();
}