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
 * Main game logic update function. Called every frame.
 */
void Tema1::UpdateGame(float dt)
{
	// Decrease timers for visual feedback (hit animations, camera shake)
	leftHitTime = std::max(0.0f, leftHitTime - dt);
	rightHitTime = std::max(0.0f, rightHitTime - dt);
	if (shakeTime > 0.0f) shakeTime = std::max(0.0f, shakeTime - dt);

	//Process paddle input
	// Get direction from keyboard (W/S for left, Up/Down for right)
	int dirL = (keyW ? 1 : 0) + (keyS ? -1 : 0);
	int dirR = (keyUp ? 1 : 0) + (keyDown ? -1 : 0);

	// Update paddle positions based on direction, speed, and delta time
	leftP.pos.y += dirL * leftP.speed * dt;
	rightP.pos.y += dirR * rightP.speed * dt;

	// Define the base movement limits (top/bottom of the arena)
	float baseMinY = arenaY + leftP.h * 0.5f + 8.f;
	float baseMaxY = arenaY + arenaH - leftP.h * 0.5f - 8.f;

	// Start with the base limits
	float minY_L = baseMinY, maxY_L = baseMaxY;
	float minY_R = baseMinY, maxY_R = baseMaxY;

	// Adjust limits if bumpers are attached
	// Bumpers must also stay within the arena, which restricts paddle movement
	const float bumpR = 1.5f * cellSize * kBumperGameScale;
	for (const auto& ab : attachedBumpers) {
		if (ab.leftSide) {
			// Calculate the paddle's Y position where the bumper would hit the wall
			float bMin = arenaY + bumpR - ab.relY;
			float bMax = arenaY + arenaH - bumpR - ab.relY;
			// Tighten the bounds: min must be the highest min, max must be the lowest max
			minY_L = std::max(minY_L, bMin);
			maxY_L = std::min(maxY_L, bMax);
		}
		else {
			float bMin = arenaY + bumpR - ab.relY;
			float bMax = arenaY + arenaH - bumpR - ab.relY;
			minY_R = std::max(minY_R, bMin);
			maxY_R = std::min(maxY_R, bMax);
		}
	}

	// Apply the final, calculated clamps to the paddle positions
	leftP.pos.y = glm::clamp(leftP.pos.y, minY_L, maxY_L);
	rightP.pos.y = glm::clamp(rightP.pos.y, minY_R, maxY_R);

	// Create AABBs (Axis-Aligned Bounding Boxes) for the paddles for collision detection
	auto leftAABB = glm::vec4(leftP.pos.x - leftP.w * 0.5f, leftP.pos.y - leftP.h * 0.5f, leftP.w, leftP.h);
	auto rightAABB = glm::vec4(rightP.pos.x - rightP.w * 0.5f, rightP.pos.y - rightP.h * 0.5f, rightP.w, rightP.h);

	// Process physics for every ball currently in play
	for (auto& b : balls) {
		if (!b.alive) continue; // Skip balls that have scored

		if (b.pos.y - b.r < arenaY) {
			b.pos.y = arenaY + b.r;     // Correct position to prevent sticking
			b.vel.y = std::abs(b.vel.y); // Reflect Y velocity
		}
		if (b.pos.y + b.r > arenaY + arenaH) {
			b.pos.y = arenaY + arenaH - b.r;
			b.vel.y = -std::abs(b.vel.y); // Reflect Y velocity
		}

		if (b.pos.x + b.r < arenaX) { // Left goal
			glm::vec2 hitPos(arenaX, glm::clamp(b.pos.y, arenaY, arenaY + arenaH));
			SpawnBallExplosion(hitPos, /*rightSide=*/false); // Trigger explosion
			b.alive = false;    // Mark ball as dead
			roundRight++;       // Right player scored
			roundAlive--;       // One less ball in play
			continue;           // Skip rest of physics for this ball
		}
		if (b.pos.x - b.r > arenaX + arenaW) { // Right goal
			glm::vec2 hitPos(arenaX + arenaW, glm::clamp(b.pos.y, arenaY, arenaY + arenaH));
			SpawnBallExplosion(hitPos, /*rightSide=*/true);
			b.alive = false;
			roundLeft++;        // Left player scored
			roundAlive--;
			continue;
		}

		if (b.vel.x < 0.f && CircleAABB(b.pos, b.r, leftAABB)) {
			// Ball is moving left and hit the left paddle
			b.pos.x = leftP.pos.x + leftP.w * 0.5f + b.r + 1.f; // Correct position

			// Calculate hit position on paddle (-1.0 at bottom, 0.0 at center, 1.0 at top)
			float t = (b.pos.y - leftP.pos.y) / (leftP.h * 0.5f);
			t = glm::clamp(t, -1.0f, 1.0f);

			// Use 't' as the reflection angle (in radians)
			float angle = t;
			b.vel.x = b.speed * std::cos(angle);
			b.vel.y = b.speed * std::sin(angle);

			leftHitTime = 1.0f; // Trigger hit animation
		}
		if (b.vel.x > 0.f && CircleAABB(b.pos, b.r, rightAABB)) {
			// Ball is moving right and hit the right paddle
			b.pos.x = rightP.pos.x - rightP.w * 0.5f - b.r - 1.f; // Correct position

			float t = (b.pos.y - rightP.pos.y) / (rightP.h * 0.5f);
			t = glm::clamp(t, -1.0f, 1.0f);

			// Angle is inverted for the right paddle (still -1 to 1)
			float angle = t;
			b.vel.x = -b.speed * std::cos(angle); // Reflect X velocity
			b.vel.y = b.speed * std::sin(angle);

			rightHitTime = 1.0f; // Trigger hit animation
		}

		{
			const float speedFactor = 1.05f; // How much to increase speed on hit
			const float maxSpeed = 800.0f;   // Max ball speed

			// Cooldown to prevent multiple hits in one frame or "zittering"
			if (bumperHitCooldownFrames > 0) bumperHitCooldownFrames--;

			glm::vec2 p0 = b.pos;          // Ball start position this frame
			glm::vec2 delta = b.vel * dt;  // Ball's intended movement this frame

			float bestT = 1.1f;    // Time of first collision (initially > 1)
			glm::vec2 bestN(0.f);  // Normal of first collision
			bool hit = false;      // Did we hit anything?

			const float stem = cellSize * kBumperGameScale; // Bumper stem size

			// Check against all attached bumpers
			for (const auto& ab : attachedBumpers) {
				glm::vec2 C = BumperCenter(ab); // Get bumper's world position
				float sign = ab.leftSide ? +1.0f : -1.0f;
				// Calculate the AABB of the bumper's stem
				glm::vec2 stemCenter = C + glm::vec2(sign * 0.5f * stem, 0.0f);
				glm::vec4 aabb(
					stemCenter.x - 0.5f * stem,
					stemCenter.y - 0.5f * stem,
					stem, stem
				);

				// Perform a sweep test (continuous collision detection)
				float tHit; glm::vec2 nHit;
				if (SweepCircleAABB(p0, delta, b.r, aabb, tHit, nHit)) {
					// If this hit is earlier than the previous best, store it
					if (tHit < bestT) { bestT = tHit; bestN = nHit; hit = true; }
				}
			}

			// Process the collision *if* one happened and cooldown is over
			if (hit && bumperHitCooldownFrames == 0) {
				const float EPS = 0.0005f; // Epsilon to prevent floating point errors
				// Move ball to the exact point of collision
				b.pos = p0 + delta * std::max(0.0f, bestT - EPS);

				// Reflect the velocity
				glm::vec2 v = (glm::length2(b.vel) < 1e-8f) ? glm::vec2(1.f, 0.f) : b.vel;
				v = glm::reflect(v, glm::normalize(bestN));

				// Increase speed and update velocity
				b.speed = std::min(maxSpeed, b.speed * speedFactor);
				b.vel = glm::normalize(v) * b.speed;

				// Give a small push-out to prevent sticking
				b.pos += glm::normalize(bestN) * 0.5f;

				// Apply remaining movement for this frame with the new velocity
				float dtLeft = dt * (1.0f - glm::clamp(bestT, 0.0f, 1.0f));
				b.pos += b.vel * dtLeft;

				// Trigger camera shake
				shakeDuration = 0.25f;
				shakeIntensity = 14.0f;
				shakeTime = shakeDuration;

				bumperHitCooldownFrames = 2; // Set cooldown
			}
			else {
				// No hit, just move the ball normally
				b.pos = p0 + delta;
			}
		}
	} // end for (auto& b : balls)

	if (roundAlive == 0) {
		// All balls are dead, round is over

		// Award points
		if (roundLeft > roundRight) scoreL++;
		else if (roundRight > roundLeft) scoreR++;
		else { scoreL++; scoreR++; } // Tie: both get a point

		// Check for match end
		if (scoreL >= maxScore || scoreR >= maxScore) {
			ResetMatch(); // Someone won, reset the entire match
			return;
		}

		// Match is not over, start the next round
		StartRound();
		return;
	}
}


/**
 * Calculates how many balls to spawn based on the current score.
 * More balls are added as the score gets higher.
 */
static inline int BallsForScore(int scoreL, int scoreR) {
	int mx = std::max(scoreL, scoreR);
	return 1 + (mx / 3); // 0-2 pts -> 1 ball, 3-5 pts -> 2 balls, etc.
}

/**
 * Starts a new round.
 * Resets ball-related counters and spawns the correct number of balls.
 */
void Tema1::StartRound()
{
	// Determine how many balls to spawn
	roundBalls = BallsForScore(scoreL, scoreR);
	roundLeft = roundRight = 0; // Reset round score counters

	balls.clear();
	balls.reserve(roundBalls);

	// Spawn each ball
	for (int i = 0; i < roundBalls; ++i) {
		Ball b;
		b.r = 10.f;
		b.speed = 400.f; // Base speed

		// Start all balls at the center
		b.pos = { arenaX + arenaW * 0.5f, arenaY + arenaH * 0.5f };

		// Launch in alternating directions with a slight random tilt
		int dir = (i % 2 == 0) ? +1 : -1;
		float tilt = 0.10f + 0.08f * (float)((std::rand() % 1000) / 1000.0f);
		glm::vec2 d = glm::normalize(glm::vec2((float)dir, (i % 3 == 0 ? +tilt : -tilt)));
		b.vel = d * b.speed;

		b.alive = true;
		balls.push_back(b);
	}

	roundAlive = (int)balls.size();
}

/**
 * Resets the entire match to its initial state (scores 0-0).
 */
void Tema1::ResetMatch()
{
	// Reset scores
	scoreL = 0;
	scoreR = 0;
	particles.clear();

	// Clear all game state
	balls.clear();
	roundBalls = 1;
	roundLeft = roundRight = 0;
	roundAlive = 0;

	StartRound(); // Start a new round (which will be 1-ball at 0-0)
}