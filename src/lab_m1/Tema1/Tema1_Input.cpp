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


/**
 * Called when a mouse button is pressed.
 * Handles all click-based logic for the editor.
 */
void Tema1::OnMouseBtnPress(int mouseX, int mouseY, int button, int)
{
	// Convert screen pixel coordinates to virtual world coordinates
	glm::vec2 V = ToVirtual(mouseX, mouseY);
	float x = V.x, y = V.y;

	const bool isLeft = (button == 1);
	const bool isRight = (button == 2);

	// Don't handle editor input if we are in game mode
	if (mode == Mode::GAME) return;

	//Right Click Logic
	if (isRight) {
		// Iterate backwards (so we delete the top-most piece first)
		for (int i = (int)shipPieces.size() - 1; i >= 0; --i) {
			// Check if the click hit this piece
			if (HitPiece(shipPieces[i], x, y)) {
				// Delete the piece
				shipPieces.erase(shipPieces.begin() + i);
				selectedPieceIndex = -1; // Stop any drag operation
				dragging = false;
				RebuildOcc(); // Update the occupation grid
				return; // Stop after deleting one piece
			}
		}
		return;
	}

	// --- Left Click Logic ---
	if (isLeft) {
		// Check for Start Button click
		// If the setup is valid and the click is inside the button...
		if (AllValidSetup() && IsInsideStartBtn(x, y)) {
			// Build the vehicle data structure from the editor setup
			auto v = BuildVehicleFromEditor();
			// Initialize the game state using this vehicle
			StartGameFromVehicle(v);
			// Switch the mode to GAME
			mode = Mode::GAME;

			// Clean up the editor state
			shipPieces.clear();
			RebuildOcc();
			return;
		}

		// Check for Toolbar click (spawning a new piece)
		for (const auto& s : toolbarSlots) {
			// Check if the click is inside this slot's bounding box
			if (x > s.boxX && x < s.boxX + s.boxW &&
				y > s.boxY && y < s.boxY + s.boxH)
			{
				// Don't spawn if we've reached the max number of blocks
				if ((int)shipPieces.size() >= maxBlocks) return;

				// Start dragging immediately
				dragging = true;
				// Create a new piece at the mouse location
				shipPieces.push_back({ x, y, s.meshName });
				selectedPieceIndex = (int)shipPieces.size() - 1; // Select the new piece

				// Calculate the drag offset (where on the piece the mouse was clicked)
				auto& p = shipPieces[selectedPieceIndex];
				dragOffsetX = x - p.x;
				dragOffsetY = y - p.y;

				// Mark the 'previous cell' as invalid (since it came from the toolbar)
				prevCellX = prevCellY = -1;
				return;
			}
		}

		//Check for click on an existing piece (dragging a placed piece)
		// Iterate backwards to pick the one on top
		for (int i = (int)shipPieces.size() - 1; i >= 0; --i) {
			// A simple AABB check for the piece
			if (x > shipPieces[i].x && x < shipPieces[i].x + cellSize &&
				y > shipPieces[i].y && y < shipPieces[i].y + cellSize)
			{
				// Start dragging
				dragging = true;
				selectedPieceIndex = i; // Select this piece
				// Calculate the drag offset
				dragOffsetX = x - shipPieces[i].x;
				dragOffsetY = y - shipPieces[i].y;

				// Check if the piece was on the grid before we started dragging
				float cx0 = shipPieces[i].x + cellSize * 0.5f;
				float cy0 = shipPieces[i].y + cellSize * 0.5f;
				bool on0 =
					(cx0 > gridOffsetX && cy0 > gridOffsetY &&
						cx0 < gridOffsetX + gridCols * (cellSize + cellGap) &&
						cy0 < gridOffsetY + gridRows * (cellSize + cellGap));

				if (on0) {
					// If it was on the grid, store its cell location.
					// This is used for "snapping back" if the drop location is invalid.
					auto g0 = GetGridCoords(cx0, cy0);
					prevCellX = g0.x; prevCellY = g0.y;
				}
				else {
					// It wasn't on the grid, so there's no previous cell.
					prevCellX = prevCellY = -1;
				}
				return;
			}
		}
	}
}

/**
 * Called when the mouse is moved.
 * Updates the position of a piece being dragged.
 */
void Tema1::OnMouseMove(int mouseX, int mouseY, int, int)
{
	if (mode == Mode::GAME) return; // Not in editor
	if (!dragging || selectedPieceIndex == -1) return; // Not dragging anything

	// Convert to world coordinates
	glm::vec2 V = ToVirtual(mouseX, mouseY);

	// Update the piece's position based on mouse and the saved offset
	auto& p = shipPieces[selectedPieceIndex];
	p.x = V.x - dragOffsetX;
	p.y = V.y - dragOffsetY;
}

/**
 * Called when a mouse button is released.
 * Handles the "drop" logic for drag-and-drop.
 */
void Tema1::OnMouseBtnRelease(int mouseX, int mouseY, int, int)
{
	if (mode == Mode::GAME) return; // Not in editor
	if (!dragging || selectedPieceIndex == -1) return; // Weren't dragging anything

	dragging = false; // Stop dragging

	glm::vec2 V = ToVirtual(mouseX, mouseY);
	auto& p = shipPieces[selectedPieceIndex];

	// Calculate the center of the piece
	float cx = p.x + cellSize * 0.5f;
	float cy = p.y + cellSize * 0.5f;

	// Check if the piece's center is over the grid
	bool overGrid =
		(cx > gridOffsetX &&
			cy > gridOffsetY &&
			cx < gridOffsetX + gridCols * (cellSize + cellGap) &&
			cy < gridOffsetY + gridRows * (cellSize + cellGap));

	if (overGrid) {
		//Dropped on the grid
		glm::ivec2 gc = GetGridCoords(cx, cy); // Get the target cell
		int occIdx = occ[gc.y][gc.x]; // Check if that cell is occupied
		// Check if we are dropping it in the same cell we picked it up from
		bool sameCellAsBefore = (prevCellX == gc.x && prevCellY == gc.y);

		// The drop is valid if the cell is empty (occIdx == -1)
		// OR if we're dropping it back where we got it
		if (occIdx == -1 || sameCellAsBefore) {
			// Snap the piece to the center of the valid grid cell
			p.x = gridOffsetX + gc.x * (cellSize + cellGap);
			p.y = gridOffsetY + gc.y * (cellSize + cellGap);
			RebuildOcc(); // Update the grid
		}
		else {
			// Dropped on an INVALID (occupied) cell

			// Check if the piece came from *off-grid* (e.g., the toolbar)
			if (prevCellX == -1 && prevCellY == -1) {
				// It came from the toolbar and was dropped on an invalid spot, so delete it.
				shipPieces.erase(shipPieces.begin() + selectedPieceIndex);
			}
			else {
				// It came from a valid grid cell, so snap it back to its original position.
				p.x = gridOffsetX + prevCellX * (cellSize + cellGap);
				p.y = gridOffsetY + prevCellY * (cellSize + cellGap);
			}
			RebuildOcc(); // Update the grid
		}
	}
	else {
		//Dropped off the grid
		// Delete the piece
		shipPieces.erase(shipPieces.begin() + selectedPieceIndex);
		RebuildOcc(); // Update the grid
	}

	// Reset dragging state
	selectedPieceIndex = -1;
	prevCellX = prevCellY = -1;
}