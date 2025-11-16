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

// Takes all the placed 'shipPieces' from the editor
// and converts them into a 'Vehicle' data structure for the game.
Tema1::Vehicle Tema1::BuildVehicleFromEditor()
{
	Vehicle out{};
	std::vector<std::pair<int, int>> r, y; // Temp lists for paddle cells
	std::pair<int, int> b{ -1,-1 }; // Temp var for the ball cell

	// Loop over all pieces and sort them by type
	for (const auto& p : shipPieces) {
		int gx, gy;
		CellOfPiece(p, gx, gy); // Get grid coordinates

		if (p.meshName == "redSquare") r.push_back({ gx, gy });
		else if (p.meshName == "yellowSquare") y.push_back({ gx, gy });
		else if (p.meshName == "orangeCircle") b = { gx, gy };
		else if (p.meshName == "bumper") out.bumpers.push_back({ gx, gy });
	}

	// Copy the paddle/ball cells into the final 'Vehicle' struct
	out.reds[0] = r[0]; out.reds[1] = r[1]; out.reds[2] = r[2];
	out.yellows[0] = y[0]; out.yellows[1] = y[1]; out.yellows[2] = y[2];
	out.ball = b;

	return out;
}

// Converts grid coordinates (gx, gy) to the world space (pixel)
// coordinates of the cell's bottom-left corner.
glm::vec2 Tema1::CellToWorld(int gx, int gy)
{
	return {
		gridOffsetX + gx * (cellSize + cellGap),
		gridOffsetY + gy * (cellSize + cellGap)
	};
}

// Checks if a grid cell (gx, gy) is adjacent to *any* cell in a given list.
bool Tema1::TouchesAny(int gx, int gy, const std::vector<std::pair<int, int>>& cells) {
	for (auto c : cells) {
		if (CellsEdgeTouch({ gx,gy }, c)) return true;
	}
	return false;
}

// Helper to check if a 3-cell paddle is oriented vertically.
bool Tema1::PaddleVertical(const std::vector<std::pair<int, int>>& cells) {
	// A vertical paddle has 3 cells all in the same column (same 'first' value)
	return cells.size() == 3 &&
		cells[0].first == cells[1].first &&
		cells[1].first == cells[2].first;
}

// (This function seems unused, but it's here)
// Checks if a bumper cell 'bp' is touching any cell in the 'cells' list.
bool Tema1::BumperIsOnPaddle(const std::pair<int, int>& bp,
	const std::vector<std::pair<int, int>>& cells)
{
	if (cells.size() != 3) return false;
	for (auto c : cells)
		if (CellsEdgeTouch(bp, c))
			return true;
	return false;
}

// Gets the world space (pixel) coordinates of a grid cell's *center*.
glm::vec2 Tema1::EditorCellCenter(int gx, int gy) {
	const float step = (cellSize + cellGap);
	return {
		gridOffsetX + gx * step + cellSize * 0.5f,
		gridOffsetY + gy * step + cellSize * 0.5f
	};
}

// Converts a world space (pixel) position 'p' from the editor grid
// into a normalized (U, V) coordinate (0.0 to 1.0).
// This is used to map editor positions to arena positions.
glm::vec2 Tema1::EditorPosToUV(glm::vec2 p) {
	// Get the world position of the first cell's center (0,0)
	const float startX = gridOffsetX + cellSize * 0.5f;
	const float startY = gridOffsetY + cellSize * 0.5f;

	// Get the total width/height of the grid in pixels
	const float step = (cellSize + cellGap);
	const float wCells = (gridCols - 1) * step;
	const float hCells = (gridRows - 1) * step;

	// Calculate the normalized position (U, V)
	float u = (p.x - startX) / (wCells <= 0 ? 1.f : wCells);
	float v = (p.y - startY) / (hCells <= 0 ? 1.f : hCells);

	// Clamp to [0, 1] range to avoid issues at the edges
	return { glm::clamp(u, 0.f, 1.f), glm::clamp(v, 0.f, 1.f) };
}

// Converts a normalized (U, V) coordinate (0.0 to 1.0)
// into a world space (pixel) position inside the game arena.
glm::vec2 Tema1::ArenaPosFromUV(glm::vec2 uv) {
	return {
		arenaX + uv.x * arenaW,
		arenaY + uv.y * arenaH
	};
}

// This is the main function called when the "Start" button is pressed.
// It takes the 'Vehicle' data and sets up the entire game state.
void Tema1::StartGameFromVehicle(const Vehicle& v)
{
	// Lambda helper to find the average center position of a 3-cell paddle
	auto mid3Editor = [&](std::pair<int, int> a,
		std::pair<int, int> b,
		std::pair<int, int> c) -> glm::vec2 {
			glm::vec2 pa = EditorCellCenter(a.first, a.second);
			glm::vec2 pb = EditorCellCenter(b.first, b.second);
			glm::vec2 pc = EditorCellCenter(c.first, c.second);
			return (pa + pb + pc) / 3.0f; // Average of the 3 centers
		};

	//Set up Paddles

	// Find the center-of-mass for the paddles in the editor
	glm::vec2 cLeftEditor = mid3Editor(v.yellows[0], v.yellows[1], v.yellows[2]);
	glm::vec2 cRightEditor = mid3Editor(v.reds[0], v.reds[1], v.reds[2]);

	// Convert those editor positions to game arena positions using UV mapping
	glm::vec2 leftArena = ArenaPosFromUV(EditorPosToUV(cLeftEditor));
	glm::vec2 rightArena = ArenaPosFromUV(EditorPosToUV(cRightEditor));

	// Define paddle game properties
	const float paddleW = 16.f;
	const float paddleH = 110.f;
	const float wallPad = 60.f; // Distance from side walls

	// Lambda helper to clamp the paddle's Y-position within the arena
	auto clampY = [&](float y) {
		float minY = arenaY + paddleH * 0.5f + 8.f;
		float maxY = arenaY + arenaH - paddleH * 0.5f - 8.f;
		return glm::clamp(y, minY, maxY);
		};

	// Set up the Left Paddle
	leftP.pos = { arenaX + wallPad, clampY(leftArena.y) }; // Fixed X, mapped Y
	leftP.w = paddleW;
	leftP.h = paddleH;
	leftP.col = glm::vec3(1, 1, 0); // Yellow
	leftP.speed = 600.f;

	// Set up the Right Paddle
	rightP.pos = { arenaX + arenaW - wallPad, clampY(rightArena.y) }; // Fixed X, mapped Y
	rightP.w = paddleW;
	rightP.h = paddleH;
	rightP.col = glm::vec3(1, 0, 0); // Red
	rightP.speed = 600.f;

	// Set up Bumpers 

	attachedBumpers.clear(); // Clear any bumpers from a previous game

	// Create lists of the paddle cells for easier association
	std::vector<std::pair<int, int>> reds = { v.reds[0], v.reds[1], v.reds[2] };
	std::vector<std::pair<int, int>> yellows = { v.yellows[0], v.yellows[1], v.yellows[2] };

	// Process each bumper from the editor
	for (auto bp : v.bumpers) {
		glm::vec2 cEditor = EditorCellCenter(bp.first, bp.second);

		// Check which paddle this bumper is touching
		bool nearLeft = TouchesAny(bp.first, bp.second, yellows);
		bool nearRight = TouchesAny(bp.first, bp.second, reds);

		// Lambda helper to find the average X position of a paddle
		auto avgX = [&](const std::vector<std::pair<int, int>>& vCells) {
			glm::vec2 a = EditorCellCenter(vCells[0].first, vCells[0].second);
			glm::vec2 b = EditorCellCenter(vCells[1].first, vCells[1].second);
			glm::vec2 c = EditorCellCenter(vCells[2].first, vCells[2].second);
			return (a.x + b.x + c.x) / 3.0f;
			};

		// Bumper Association Logic
		// If it's touching both (or neither), assign it to the *closest* paddle
		if (nearLeft && nearRight) {
			// Find distance to average X of each paddle
			float dxR = std::abs(cEditor.x - avgX(reds));
			float dxY = std::abs(cEditor.x - avgX(yellows));
			// Assign to the closer one
			nearRight = dxR < dxY;
			nearLeft = !nearRight;
		}
		if (!nearLeft && !nearRight) { // Same logic if it's touching neither
			float dxR = std::abs(cEditor.x - avgX(reds));
			float dxY = std::abs(cEditor.x - avgX(yellows));
			nearRight = dxR < dxY;
			nearLeft = !nearRight;
		}

		// Create the in-game bumper object
		AttachedBumper ab{};
		ab.leftSide = nearLeft; // Assign it to a side

		const Paddle& P0 = ab.leftSide ? leftP : rightP;
		const auto& cells = ab.leftSide ? yellows : reds;

		bool vertical = PaddleVertical(cells);

		//  Bumper Slot Logic
		// We need to find the bumper's *relative* Y position on the paddle.
		// We map its editor grid position to one of 3 slots: top (0), middle (1), or bottom (2).
		int idxSlot = 1; // Default to middle slot

		if (!vertical) {
			// Horizontal paddle
			int row = cells[0].second;
			int c0 = cells[0].first, c1 = cells[1].first, c2 = cells[2].first;
			int minCol = std::min(c0, std::min(c1, c2));
			int maxCol = std::max(c0, std::max(c1, c2));

			// Check if bumper is at the 'ends' of the paddle
			if (bp.second == row && bp.first == minCol - 1) idxSlot = 0; // Left end
			else if (bp.second == row && bp.first == maxCol + 1) idxSlot = 2; // Right end
			else {
				// Otherwise, find its relative position along the paddle's length
				int relCol = bp.first - minCol;
				idxSlot = glm::clamp(relCol, 0, 2);
			}
		}
		else {
			// Vertical paddle
			int col = cells[0].first;
			int r0 = cells[0].second, r1 = cells[1].second, r2 = cells[2].second;
			int minRow = std::min(r0, std::min(r1, r2));
			int maxRow = std::max(r0, std::max(r1, r2));

			// Check if bumper is at the 'ends'
			if (bp.first == col && bp.second == minRow - 1) idxSlot = 0; // Bottom end
			else if (bp.first == col && bp.second == maxRow + 1) idxSlot = 2; // Top end
			else {
				// Find its relative position along the paddle's length
				int relRow = bp.second - minRow;
				idxSlot = glm::clamp(relRow, 0, 2);
			}
		}

		// Convert the slot index (0, 1, 2) to a relative Y offset
		float perSlot = P0.h / 3.0f;
		ab.relY = (1 - idxSlot) * perSlot; // 0=top, 1=mid, 2=bottom 
		ab.relX = 0.0f; // Not used

		attachedBumpers.push_back(ab);
	}


	// Clear any old game state
	balls.clear();
	roundLeft = roundRight = 0;
	roundAlive = 0;

	// Start the first round
	StartRound();
}