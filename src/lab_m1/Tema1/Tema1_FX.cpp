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
 * Calculates a 2D offset vector for the camera shake effect.
 * The shake intensity decreases (falls off) over its duration.
 */
glm::vec2 Tema1::ComputeShake() {
	// If the shake is not active, return no offset.
	if (shakeTime <= 0.0f || shakeDuration <= 0.0f || shakeIntensity <= 0.0f)
		return glm::vec2(0.0f);

	// Calculate how far along the shake is (0.0 to 1.0)
	float t = glm::clamp(1.0f - (shakeTime / shakeDuration), 0.0f, 1.0f);
	// The falloff factor makes the shake weaker as it nears its end
	float falloff = 1.0f - t;

	// Use sin and cos with large, prime-like numbers to create a pseudo-random jitter
	float x = std::sin(133.0f * shakeTime) * shakeIntensity * falloff;
	float y = std::cos(97.0f * shakeTime) * shakeIntensity * falloff;

	return glm::vec2(x, y);
}


/**
 * Spawns a burst of particles at a specific position.
 * This is used when the ball goes out of bounds.
 * If false, it's biased towards the right (coming from the left goal).
 */
void Tema1::SpawnBallExplosion(glm::vec2 p, bool rightSide)
{
	//Explosion parameters
	const int   N = 60;     // Number of particles
	const float vMin = 250.f; // Minimum speed
	const float vMax = 600.f; // Maximum speed
	const float sMin = 6.f;   // Minimum size
	const float sMax = 12.f;  // Maximum size
	const float life = 0.7f;  // Lifetime in seconds
	// Pi (180 deg) for left-side goal, 0 for right-side goal
	const float dirBias = rightSide ? (float)M_PI : 0.f;

	for (int i = 0; i < N; ++i) {
		// Calculate a random angle within a 180-degree arc (PI radians)
		// The 'dirBias' directs this arc left or right.
		float a = dirBias + ((std::rand() % 1000) / 1000.f - 0.5f) * (float)M_PI; // ±90°

		// Get a random speed and size within the defined range
		float sp = vMin + (std::rand() % 1000) / 1000.f * (vMax - vMin);
		float si = sMin + (std::rand() % 1000) / 1000.f * (sMax - sMin);

		// Create the new particle
		Particle pr;
		pr.pos = p;
		pr.vel = glm::vec2(std::cos(a), std::sin(a)) * sp; // Velocity from angle and speed
		pr.size = si;
		pr.life = pr.life0 = life; // Set both current and initial lifetime
		pr.orange = (std::rand() & 1); // Randomly pick one of two colors

		particles.push_back(pr);
	}
}

/**
 * Updates the physics for all active particles.
 * Called once per frame from the main Update function.
 */
void Tema1::UpdateParticles(float dt)
{
	// A simple gravity force pointing down
	const glm::vec2 grav(0.f, -480.f);

	for (auto& pr : particles) {
		if (pr.life <= 0.f) continue; // Skip dead particles

		// Apply simple Euler integration for physics
		pr.vel += grav * dt; // Apply gravity to velocity
		pr.pos += pr.vel * dt; // Update position with velocity
		pr.life -= dt;         // Decrease particle's remaining life
	}

	// Clean up dead particles
	// This is the "erase-remove idiom", an efficient way to remove elements
	// from a vector that match a condition (life <= 0).
	particles.erase(
		std::remove_if(particles.begin(), particles.end(),
			[](const Particle& p) { return p.life <= 0.f; }),
		particles.end()
	);
}

/**
 * Renders all active particles.
 * Called once per frame from RenderGameBasic.
 */
void Tema1::RenderParticles(m1::Tema1* self)
{
	// Create a translation matrix based on the current camera shake offset
	// This makes the particles shake along with the camera.
	glm::mat3 S(1);
	S *= transform2D::Translate(self->shakeOffset.x, self->shakeOffset.y);

	for (const auto& pr : particles) {
		if (pr.life <= 0.f) continue; // Skip dead ones

		// Calculate normalized life (a value from 1.0 down to 0.0)
		float k = glm::clamp(pr.life / pr.life0, 0.f, 1.f);
		// Make the particle shrink as it dies
		float s = pr.size * (0.3f + 0.7f * k);

		// Create the model matrix for this particle
		glm::mat3 M(1);
		M *= transform2D::Translate(pr.pos.x, pr.pos.y);
		M *= transform2D::Scale(s, s);

		// Select the correct mesh (color) for the particle
		const char* mesh = pr.orange ? "partOrange" : "partYellow";

		// Render the particle, applying the camera shake (S) first
		self->RenderMesh2D(self->meshes[mesh], self->shaders["VertexColor"], S * M);
	}
}