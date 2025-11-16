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

/** Converts screen pixel coordinates (px, py) to logical grid coordinates (gx, gy).
 *
 */
glm::ivec2 Tema1::GetGridCoords(float px, float py)
{
	// Find the closest grid cell center to the pixel position
	int gx = (int)std::round((px - (gridOffsetX + cellSize * 0.5f)) / (cellSize + cellGap));
	int gy = (int)std::round((py - (gridOffsetY + cellSize * 0.5f)) / (cellSize + cellGap));

	// Clamp the coordinates to be within the valid grid bounds
	gx = glm::clamp(gx, 0, gridCols - 1);
	gy = glm::clamp(gy, 0, gridRows - 1);
	return { gx, gy };
}

/**
 *  Rebuilds the 2D 'occ' (occupation) grid.
 * This function iterates over all placed shipPieces, finds their grid coordinates,
 * and marks the corresponding cell in the 'occ' matrix with the piece's index.
 * It's essential for collision detection during drag-and-drop.
 */
void Tema1::RebuildOcc()
{
	// Safety check for grid dimensions
	if (gridRows <= 0 || gridCols <= 0 || gridRows > 200 || gridCols > 200) {
		occ.clear();
		return;
	}

	// Reset the occupation grid, filling it with -1 (empty)
	occ.assign((size_t)gridRows, std::vector<int>((size_t)gridCols, -1));

	// Iterate over every piece placed in the world
	for (int idx = 0; idx < (int)shipPieces.size(); ++idx) {
		// Calculate the center of the piece
		float cx = shipPieces[idx].x + cellSize * 0.5f;
		float cy = shipPieces[idx].y + cellSize * 0.5f;

		// Skip pieces that are not on the grid
		if (cx < gridOffsetX || cy < gridOffsetY) continue;
		if (cx >= gridOffsetX + gridCols * (cellSize + cellGap)) continue;
		if (cy >= gridOffsetY + gridRows * (cellSize + cellGap)) continue;

		// Get grid coordinates and mark the 'occ' matrix
		glm::ivec2 g = GetGridCoords(cx, cy);
		if (g.y >= 0 && g.y < gridRows && g.x >= 0 && g.x < gridCols)
			occ[g.y][g.x] = idx; // Mark cell with the index of the piece
	}
}

/**
 *  Gets the grid coordinates (gx, gy) for a given Piece object.
 */
bool Tema1::CellOfPiece(const Piece& p, int& gx, int& gy)
{
	float cx = p.x + cellSize * 0.5f;
	float cy = p.y + cellSize * 0.5f;

	// Check if the piece's center is within the grid boundaries
	bool on =
		(cx > gridOffsetX && cy > gridOffsetY &&
			cx < gridOffsetX + gridCols * (cellSize + cellGap) &&
			cy < gridOffsetY + gridRows * (cellSize + cellGap));

	if (!on) return false;

	// If it is, calculate and return its coordinates
	auto g = GetGridCoords(cx, cy);
	gx = g.x; gy = g.y;
	return true;
}

/**
 * Checks if two grid cells, 'a' and 'b', are adjacent (share an edge).
 * Does not count diagonal adjacency.
 */
bool Tema1::CellsEdgeTouch(const std::pair<int, int>& a, const std::pair<int, int>& b) {
	int dx = std::abs(a.first - b.first);
	int dy = std::abs(a.second - b.second);
	// True if cells are 1 unit apart horizontally and 0 vertically, OR 0 horizontally and 1 vertically
	return (dx == 1 && dy == 0) || (dx == 0 && dy == 1);
}

/**
 * Checks if any cell in a list 'A' touches any cell in a list 'B'.
 */
bool Tema1::ArePairsTouching(const std::vector<std::pair<int, int>>& A, const std::vector<std::pair<int, int>>& B) {
	for (const auto& a : A)
		for (const auto& b : B)
			if (CellsEdgeTouch(a, b))
				return true;
	return false;
}


/**
 *  Static helper function to check if three integers are unique and consecutive.
 * (e.g., 2, 3, 4)
 */
static bool isConsecutive3(int a, int b, int c) {
	int mn = std::min(a, std::min(b, c));
	int mx = std::max(a, std::max(b, c));
	// Check if the range is 2 (e.g., 4-2=2) and all numbers are distinct
	return (mx - mn == 2) && (a != b) && (a != c) && (b != c);
}

/**
 * Checks if a list of 3 cells forms a valid 1x3 or 3x1 paddle.
 */
bool Tema1::IsPaddleTriple(const std::vector<std::pair<int, int>>& cells)
{
	if (cells.size() != 3) return false;

	// Check for horizontal paddle (same row, consecutive columns)
	if (cells[0].second == cells[1].second && cells[1].second == cells[2].second) {
		int x1 = cells[0].first, x2 = cells[1].first, x3 = cells[2].first;
		return isConsecutive3(x1, x2, x3);
	}
	// Check for vertical paddle (same column, consecutive rows)
	if (cells[0].first == cells[1].first && cells[1].first == cells[2].first) {
		int y1 = cells[0].second, y2 = cells[1].second, y3 = cells[2].second;
		return isConsecutive3(y1, y2, y3);
	}
	return false;
}

/**
 *  Main validation function to check if the current editor setup is valid to start the game.
 * This function enforces all the rules for the vehicle construction.
 */
bool Tema1::AllValidSetup()
{
	// Must have 7, 8, or 9 pieces total
	if ((int)shipPieces.size() < 7 || (int)shipPieces.size() > 9) return false;

	// Categorize all pieces on the grid
	std::vector<std::pair<int, int>> reds, yellows, bumpers;
	int orangeCountOnGrid = 0;

	for (const auto& p : shipPieces) {
		int gx, gy;
		// All pieces must be placed on the grid
		if (!CellOfPiece(p, gx, gy)) return false; // This piece is not on the grid

		if (p.meshName == "redSquare") {
			reds.push_back({ gx, gy });
		}
		else if (p.meshName == "yellowSquare") {
			yellows.push_back({ gx, gy });
		}
		else if (p.meshName == "orangeCircle") {
			orangeCountOnGrid++;
		}
		else if (p.meshName == "bumper") {
			bumpers.push_back({ gx, gy });
		}
		else return false; // Unknown piece type
	}

	// Must have exactly 3 red, 3 yellow, and 1 ball
	if (reds.size() != 3 || yellows.size() != 3 || orangeCountOnGrid != 1) return false;

	// Red and Yellow pieces must form valid 1x3 or 3x1 paddles
	if (!IsPaddleTriple(reds)) { return false; }
	if (!IsPaddleTriple(yellows)) return false;

	// The two paddles cannot be touching each other
	if (ArePairsTouching(reds, yellows)) return false;

	
	//Maximum of 2 bumpers
	if (bumpers.size() > 2) return false;

	// Bumpers cannot be touching each other
	for (size_t i = 0; i < bumpers.size(); ++i)
		for (size_t j = i + 1; j < bumpers.size(); ++j)
			if (CellsEdgeTouch(bumpers[i], bumpers[j])) return false;

	// Collect all paddle cells into one list for easier checking
	std::vector<std::pair<int, int>> paddlesAll;
	paddlesAll.insert(paddlesAll.end(), reds.begin(), reds.end());
	paddlesAll.insert(paddlesAll.end(), yellows.begin(), yellows.end());

	// Every bumper must be touching at least one paddle cell
	for (auto b : bumpers) {
		bool touchesPaddle = false;
		for (auto p : paddlesAll) {
			if (CellsEdgeTouch(b, p)) { touchesPaddle = true; break; }
		}
		if (!touchesPaddle) return false;
	}

	// No other piece can be in the 3-wide, infinitely-tall column above a bumper.
	for (auto b : bumpers) {
		int cmin = std::max(0, b.first - 1); // Column to the left (clamped)
		int cmax = std::min(gridCols - 1, b.first + 1); // Column to the right (clamped)

		for (int c = cmin; c <= cmax; ++c) // For each of the 3 columns
			for (int r = b.second + 1; r < gridRows; ++r) { // For each row *above* the bumper
				int idx = occ[r][c];
				if (idx != -1) {
					// This cell is occupied. Is it the bumper itself? (This check seems redundant but safe)
					const auto& P = shipPieces[(size_t)idx];
					int gx, gy; CellOfPiece(P, gx, gy);
					// If the piece found is not the bumper itself, the rule is violated
					if (!(gx == b.first && gy == b.second)) return false;
				}
			}
	}

	// If all rules pass
	return true;
}

/**
 *Simple AABB (Axis-Aligned Bounding Box) check for mouse click on the start button.
 */
bool Tema1::IsInsideStartBtn(float x, float y)
{
	return x >= startButtonPos.x && x <= startButtonPos.x + healthBlockSize &&
		y >= startButtonPos.y && y <= startButtonPos.y + healthBlockSize;
}