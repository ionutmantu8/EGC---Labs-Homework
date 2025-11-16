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

// Helper function to create a new mesh by setting up its VAO, VBO, and IBO.
Mesh* Tema1::CreateMesh(const char* name,
	const std::vector<VertexFormat>& vertices,
	const std::vector<unsigned int>& indices)
{
	unsigned int VAO = 0, VBO = 0, IBO = 0;
	// Generate and bind a new Vertex Array Object (VAO)
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Generate and bind the Vertex Buffer Object (VBO)
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Send the vertex data to the GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(),
		&vertices[0], GL_STATIC_DRAW);

	// Generate and bind the Index Buffer Object (IBO)
	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	// Send the index data to the GPU
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * indices.size(),
		&indices[0], GL_STATIC_DRAW);

	// Set up attribute 0: Vertex Position (vec3)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)0);

	// Set up attribute: Vertex Normal (vec3)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(sizeof(glm::vec3)));

	// Set up attribute: Texture Coordinate (vec2)
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexFormat), (void*)(2 * sizeof(glm::vec3)));

	// Set up attribute: Vertex Color (vec3)
	glEnableVertexAttribArray(3); // color
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexFormat),
		(void*)(2 * sizeof(glm::vec3) + sizeof(glm::vec2)));

	// Unbind the VAO to prevent accidental changes
	glBindVertexArray(0);

	// Create the new Mesh object and initialize it from the created buffers
	auto* mesh = new Mesh(name);
	mesh->InitFromBuffer(VAO, (unsigned int)indices.size());
	mesh->SetDrawMode(GL_TRIANGLES);
	return mesh;
}

// Draws a mesh (like a paddle) centered at 'center' with size (w, h).
void Tema1::DrawRectCellMesh(const char* mesh, glm::vec2 center, float w, float h)
{
	glm::mat3 M(1);
	// Position the mesh at its bottom-left corner
	M *= transform2D::Translate(center.x - w * 0.5f, center.y - h * 0.5f);
	// Scale the base 'cellSize' mesh to the new w/h
	M *= transform2D::Scale(w / cellSize, h / cellSize);

	//  Render the mesh at its base position.
	RenderMesh2D(meshes[mesh], shaders["VertexColor"], M);

	// Render the mesh again, but with the camera shake offset applied.
	glm::mat3 S(1);
	S *= transform2D::Translate(shakeOffset.x, shakeOffset.y);
	RenderMesh2D(meshes[mesh], shaders["VertexColor"], S * M);
}

// Draws a single digit (0-9) using a 5x3 grid of 'whiteFill' squares.
void Tema1::DrawDigit(int d, float x, float y, float s)
{
	// Pixel-map for all digits 0-9 (1 = draw, 0 = skip)
	static const int D[10][15] = {
		{1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1}, // 0
		{0,1,0, 1,1,0, 0,1,0, 0,1,0, 1,1,1}, // 1
		{1,1,1, 0,0,1, 1,1,1, 1,0,0, 1,1,1}, // 2
		{1,1,1, 0,0,1, 1,1,1, 0,0,1, 1,1,1}, // 3
		{1,0,1, 1,0,1, 1,1,1, 0,0,1, 0,0,1}, // 4
		{1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1}, // 5
		{1,1,1, 1,0,0, 1,1,1, 1,0,1, 1,1,1}, // 6
		{1,1,1, 0,0,1, 0,1,0, 0,1,0, 0,1,0}, // 7
		{1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1}, // 8
		{1,1,1, 1,0,1, 1,1,1, 0,0,1, 1,1,1}  // 9
	};

	const float px = s; // Size of one 'pixel'
	const float gap = px * 0.25f; // Gap between pixels

	// Iterate over the 5x3 grid
	for (int r = 0; r < 5; ++r)
		for (int c = 0; c < 3; ++c) {
			// If the pixel-map says 1, draw a square
			if (D[d][r * 3 + c]) {
				glm::mat3 M(1);
				float dx = x + c * (px + gap);
				// Calculate the Y position (inverting 'r' so 0 is the top row)
				float dy = y + (4 - r) * (px + gap);
				M *= transform2D::Translate(dx, dy);
				M *= transform2D::Scale(px, px);

				// Draw the pixel (once without shake)
				RenderMesh2D(meshes["whiteFill"], shaders["VertexColor"], M);

				// Draw the pixel (again, with shake)
				glm::mat3 S(1);
				S *= transform2D::Translate(shakeOffset.x, shakeOffset.y);
				RenderMesh2D(meshes["whiteFill"], shaders["VertexColor"], S * M);
			}
		}
}

// Draws a multi-digit number by breaking it into individual digits.
void Tema1::DrawNumber(int n, float x, float y, float s)
{
	// Handle the special case for '0'
	if (n == 0) { DrawDigit(0, x, y, s); return; }

	int digits[4]; int k = 0;
	// Extract each digit from the number (in reverse order)
	while (n > 0 && k < 4) { digits[k++] = n % 10; n /= 10; }

	// Calculate the horizontal space needed for one digit (3 squares + 2 gaps + 1 extra gap)
	float advance = 3 * s + 2 * (s * 0.25f) + s;

	// Draw the digits in the correct (forward) order
	for (int i = k - 1; i >= 0; --i) {
		DrawDigit(digits[i], x, y, s);
		x += advance; // Move to the right for the next digit
	}
}

// Draws a bumper in the editor at a specific (x, y) grid-aligned position.
void Tema1::DrawBumperAt(float x, float y)
{
	const float cs = cellSize;

	// Draw the 1x1 stem
	{
		glm::mat3 M(1);
		M *= transform2D::Translate(x, y);
		M *= transform2D::Scale(cs, cs);
		RenderMesh2D(meshes["bumperStem"], shaders["VertexColor"], M);
	}

	// Draw the semicircle cap, 1.5x cell size, positioned above the stem.
	{
		const float capR = 1.5f * cs;
		const float capCX = x + cs * 0.5f;
		const float capCY = y + cs;

		glm::mat3 M(1);
		M *= transform2D::Translate(capCX, capCY);
		M *= transform2D::Scale(capR, capR);
		RenderMesh2D(meshes["bumperCap"], shaders["VertexColor"], M);

		// Render the cap again with camera shake
		// (This has no effect in the editor, as shakeOffset is 0)
		glm::mat3 S(1);
		S *= transform2D::Translate(shakeOffset.x, shakeOffset.y);
		RenderMesh2D(meshes["bumperCap"], shaders["VertexColor"], S * M);
	}
}

// Draws an in-game bumper, attached to a paddle.
void Tema1::DrawBumperFacing(bool faceRight, glm::vec2 center, glm::vec2 scale)
{
	const float cs = cellSize;
	const float capR = 1.5f * cs; // Radius of the cap
	// Calculate the scale factor for the stem to match the cap's radius
	const float sLoc = cs / capR;

	// Build a base transformation matrix for the whole bumper
	glm::mat3 base(1);
	base *= transform2D::Translate(center.x, center.y);
	// Apply the paddle's 'squash-and-stretch' animation scale
	base *= transform2D::Scale(scale.x, scale.y);
	// Apply the global game scale for bumpers (make them smaller)
	base *= transform2D::Scale(kBumperGameScale, kBumperGameScale);
	// Rotate the bumper to face left or right
	base *= transform2D::Rotate(faceRight ? -(float)M_PI * 0.5f : (float)M_PI * 0.5f);
	// Scale up to the final size (the cap mesh is a 1-unit radius)
	base *= transform2D::Scale(capR, capR);

	// Draw the stem, applying the base transform
	{
		glm::mat3 M = base;
		// Position the stem relative to the cap's center (0,0 in local space)
		M *= transform2D::Translate(-sLoc * 0.5f, -sLoc);
		M *= transform2D::Scale(sLoc, sLoc);
		RenderMesh2D(meshes["bumperStem"], shaders["VertexColor"], M);
	}

	// Draw the cap, applying the base transform
	{
		glm::mat3 M = base;
		RenderMesh2D(meshes["bumperCap"], shaders["VertexColor"], M);
	}
}

// Main rendering function for the GAME mode. Called every frame by Update().
void Tema1::RenderGameBasic()
{
	// First, calculate the camera shake offset for this frame
	shakeOffset = ComputeShake();
	// Create the shake translation matrix (S)
	glm::mat3 S(1.0f);
	S *= transform2D::Translate(shakeOffset.x, shakeOffset.y);

	// Draw the arena's outer border
	{
		glm::mat3 M(1);
		M *= transform2D::Translate(arenaX, arenaY);
		M *= transform2D::Scale(arenaW, arenaH);
		// Apply the shake matrix (S) to *all* game objects
		RenderMesh2D(meshes["whiteStroke"], shaders["VertexColor"], S * M);
	}

	// Draw the dashed center line
	{
		const int dashCount = 12;
		float dashH = arenaH / (dashCount * 1.6f);
		float gap = dashH * 0.6f;
		float xMid = arenaX + arenaW * 0.5f - 2.0f;
		for (int i = 0; i < dashCount; ++i) {
			float y = arenaY + 10.f + i * (dashH + gap);
			glm::mat3 M(1);
			M *= transform2D::Translate(xMid, y);
			M *= transform2D::Scale(4.f, dashH);
			RenderMesh2D(meshes["whiteFill"], shaders["VertexColor"], S * M);
		}
	}

	auto paddleScale = [&](float hitTime) -> glm::vec2 {
		if (hitTime <= 0.0f) return glm::vec2(1.0f, 1.0f); // No animation
		// 't' goes from 0 to 1 as the animation plays
		float t = 1.0f - glm::clamp(hitTime, 0.0f, 1.0f);
		// Use a sin wave (0 -> 1 -> 0) for a smooth pulse
		float pulse = std::sin(t * (float)M_PI);
		float ampY = 0.25f; // Stretch
		float ampX = -0.10f; // Squish
		// Return the (x, y) scale
		return glm::vec2(1.0f + ampX * pulse, 1.0f + ampY * pulse);
		};

	// Get the current scale for both paddles
	glm::vec2 sL = paddleScale(leftHitTime);
	glm::vec2 sR = paddleScale(rightHitTime);

	// Draw the left and right paddles with their animation scale
	DrawRectCellMesh("yellowSquare", leftP.pos, leftP.w * sL.x, leftP.h * sL.y);
	DrawRectCellMesh("redSquare", rightP.pos, rightP.w * sR.x, rightP.h * sR.y);

	// Draw all *living* balls
	for (const auto& b : balls) {
		if (!b.alive) continue;
		glm::mat3 M(1);
		M *= transform2D::Translate(b.pos.x, b.pos.y);
		M *= transform2D::Scale(b.r, b.r);
		RenderMesh2D(meshes["orangeCircle"], shaders["VertexColor"], S * M);
	}

	// Draw the scores for left and right players
	{
		float s = 6.0f; // Size of the score 'pixels'
		DrawNumber(scoreL, arenaX + 8.f, arenaY + arenaH + 18.f, s);
		DrawNumber(scoreR, arenaX + arenaW - 90.f, arenaY + arenaH + 18.f, s);
	}

	// Draw all attached bumpers
	for (const auto& ab : attachedBumpers) {
		glm::vec2 center = BumperCenter(ab);
		// Get the correct paddle's animation scale for the bumper
		glm::vec2 sc = ab.leftSide ? sL : sR;
		// Draw the bumper, passing in its orientation and animation scale
		DrawBumperFacing(ab.leftSide ? true : false, center, sc);
	}

	// Render all active particles
	RenderParticles(this);
}