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

// Converts mouse coordinates from screen pixel space to virtual world space.
glm::vec2 Tema1::ToVirtual(int mouseX, int mouseY) const
{
	auto res = window->GetResolution();
	// Calculate scaling factors based on virtual vs. actual resolution
	float sx = vW / (float)res.x;
	float sy = vH / (float)res.y;

	// Apply scaling
	float vx = mouseX * sx;
	// Apply scaling and invert the Y-axis
	// (screen space 0 is at the top, world space 0 is at the bottom)
	float vy = ((float)res.y - mouseY) * sy;
	return glm::vec2(vx, vy);
}

// Checks for collision between a circle (center c, radius r) and
// an Axis-Aligned Bounding Box (AABB).
bool Tema1::CircleAABB(glm::vec2 c, float r, glm::vec4 aabb)
{
	// aabb = (x, y, w, h)
	float x = aabb.x, y = aabb.y, w = aabb.z, h = aabb.w;

	// Find the point on the AABB that is closest to the circle's center
	float cx = std::max(x, std::min(c.x, x + w));
	float cy = std::max(y, std::min(c.y, y + h));

	// Calculate the distance between the circle's center and this closest point
	float dx = c.x - cx, dy = c.y - cy;

	// If the distance squared is less than the radius squared, they are colliding
	return (dx * dx + dy * dy) <= (r * r);
}

// Calculates the absolute world position of an attached bumper.
// The bumper's position is relative to its parent paddle.
glm::vec2 Tema1::BumperCenter(const AttachedBumper& ab)
{
	// Get a reference to the correct paddle (left or right)
	const Paddle& P = ab.leftSide ? leftP : rightP;
	const float cs = cellSize;
	const float bumpR = 1.5f * cs * kBumperGameScale; // Bumper's radius

	glm::vec2 c; // The bumper's center

	// Position the bumper horizontally, offset from the paddle's edge
	if (ab.leftSide) {
		c.x = P.pos.x + P.w * 0.5f + cs * kBumperGameScale;
	}
	else {
		c.x = P.pos.x - P.w * 0.5f - cs * kBumperGameScale;
	}

	// Position the bumper vertically, based on the paddle's center and the relative Y offset
	c.y = P.pos.y + ab.relY;

	// Clamp the bumper's X position to ensure it stays within the arena walls
	c.x = glm::clamp(c.x, arenaX + bumpR, arenaX + arenaW - bumpR);
	return c;
}

// Performs a "swept" or continuous collision detection for a moving circle against a static AABB.
// This prevents fast-moving objects (like the ball) from "tunneling" through obstacles.
bool Tema1::SweepCircleAABB(const glm::vec2& p0, const glm::vec2& delta, float r,
	const glm::vec4& aabb, float& tHit, glm::vec2& nHit) {

	// Expand the AABB by the circle's radius 'r' in all directions.
	// This turns the problem from (moving circle vs AABB)
	// into (moving point vs expanded AABB).
	glm::vec2 bmin(aabb.x - r, aabb.y - r);
	glm::vec2 bmax(aabb.x + aabb.z + r, aabb.y + aabb.w + r);

	// Calculate the inverse of the velocity (delta).
	// This is for the "slabs" collision method.
	// Handle division by zero by using a very large number (infinity).
	glm::vec2 inv(
		std::abs(delta.x) > 1e-8f ? 1.0f / delta.x : (delta.x >= 0 ? 1e30f : -1e30f),
		std::abs(delta.y) > 1e-8f ? 1.0f / delta.y : (delta.y >= 0 ? 1e30f : -1e30f)
	);

	// Find the time 't' when the point (p0) enters and exits the X and Y "slabs"
	glm::vec2 t0 = (bmin - p0) * inv;
	glm::vec2 t1 = (bmax - p0) * inv;

	// Sort the times so tmin is always the entry time and tmax is the exit time
	glm::vec2 tmin = glm::min(t0, t1);
	glm::vec2 tmax = glm::max(t0, t1);

	// The actual collision can only happen when we are inside *both* slabs.
	// So, we find the *last* entry time and the *first* exit time.
	float tEnter = std::max(tmin.x, tmin.y);
	float tExit = std::min(tmax.x, tmax.y);

	//Check for a "miss"
	// tExit < 0.0f: The AABB is entirely behind the movement (moving away)
	// tEnter > tExit: The X and Y slabs do not overlap in time (missed)
	//tEnter > 1.0f: The collision would happen *after* this frame
	if (tExit < 0.0f || tEnter > tExit || tEnter > 1.0f) return false;

	//We have a "hit"
	// The collision happens at tEnter (clamped just in case)
	tHit = glm::clamp(tEnter, 0.0f, 1.0f);

	// Calculate the normal of the surface we hit.
	// This is determined by which slab we entered *last*.
	if (tmin.x > tmin.y) {
		// Entered the X-slab last, so it's a vertical wall hit.
		// Normal points left (-1) if delta.x > 0, or right (1) if delta.x < 0.
		nHit = glm::vec2((delta.x > 0 ? -1.f : 1.f), 0.f);
	}
	else {
		// Entered the Y-slab last, so it's a horizontal wall hit.
		nHit = glm::vec2(0.f, (delta.y > 0 ? -1.f : 1.f));
	}

	return true;
}