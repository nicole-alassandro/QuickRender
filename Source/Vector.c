// Copyright (C) 2021  Nicole Alassandro

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.

typedef union Vector {
    struct { float x, y, z; };
    struct { float u, v, w; };
    struct { float r, g, b; };
    float xyz[3];
    float uvw[3];
    float rgb[3];
} Vector;

static inline Vector
VectorAdd(
    const Vector * const a,
    const Vector * const b)
{
    SDL_assert(a);
    SDL_assert(b);

    return (Vector){
        .x = a->x + b->x,
        .y = a->y + b->y,
        .z = a->z + b->z,
    };
}

static inline Vector
VectorSub(
    const Vector * const a,
    const Vector * const b)
{
    SDL_assert(a);
    SDL_assert(b);

    return (Vector){
        .x = a->x - b->x,
        .y = a->y - b->y,
        .z = a->z - b->z,
    };
}

static inline Vector
VectorMultf(
    const Vector * const a,
    const float          b)
{
    SDL_assert(a);

    return (Vector){
        .x = a->x * b,
        .y = a->y * b,
        .z = a->z * b,
    };
}

static inline Vector
VectorDivf(
    const Vector * const a,
    const float          b)
{
    SDL_assert(a);

    return (Vector){
        .x = a->x / b,
        .y = a->y / b,
        .z = a->z / b,
    };
}

static inline float
VectorMag(
    const Vector * const a)
{
    SDL_assert(a);

    return sqrtf((a->x * a->x) + (a->y * a->y) + (a->z * a->z));
}

static inline Vector
VectorNormalize(
    const Vector * const a)
{
    SDL_assert(a);

    SDL_assert(VectorMag(a) != 0.0f);

    return VectorDivf(a, VectorMag(a));
}

static inline float
VectorDot(
    const Vector * const a,
    const Vector * const b)
{
    return (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

static inline Vector
VectorCross(
    const Vector * const a,
    const Vector * const b)
{
    SDL_assert(a);
    SDL_assert(b);

    return (Vector){
        .x = (a->y * b->z) - (a->z * b->y),
        .y = (a->z * b->x) - (a->x * b->z),
        .z = (a->x * b->y) - (a->y * b->x),
    };
}

static Vector
Barycenter(
    const Vector * const verts,
    const Vector * const point)
{
    SDL_assert(verts);
    SDL_assert(point);

    const Vector result = VectorCross(
        &(Vector){
            .x = (verts[2].x - verts[0].x),
            .y = (verts[1].x - verts[0].x),
            .z = (verts[0].x - point->x),
        },
        &(Vector){
            .x = (verts[2].y - verts[0].y),
            .y = (verts[1].y - verts[0].y),
            .z = (verts[0].y - point->y),
        }
    );

    // Triangle is degenerate
    if (fabsf(result.y) < 1.0f || result.z == 0.0f)
        return (Vector){.x = -1.0f, .y = -1.0f, .z = -1.0f};

    return (Vector){
        .x = 1.0f - (result.x + result.y) / result.z,
        .y = result.y / result.z,
        .z = result.x / result.z,
    };
}

static inline SDL_FRect
TriBoundingBox(
    const Vector * const verts)
{
    SDL_assert(verts);

    SDL_FRect bounds;
    {
        SDL_FPoint min, max;
        min.x = max.x = verts[0].x;
        min.y = max.y = verts[0].y;

        for (size_t i = 1; i < 3; ++i)
        {
            min.x = (verts[i].x < min.x) ? verts[i].x : min.x;
            min.y = (verts[i].y < min.y) ? verts[i].y : min.y;
            max.x = (verts[i].x > max.x) ? verts[i].x : max.x;
            max.y = (verts[i].y > max.y) ? verts[i].y : max.y;
        }

        bounds.x = min.x;
        bounds.y = min.y;
        bounds.w = fabsf(max.x - min.x);
        bounds.h = fabsf(max.y - min.y);
    }

    return bounds;
}
