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

typedef union Quaternion {
    struct { float w, x, y, z; };
    float wxyz[4];
} Quaternion;

static inline Quaternion
AnglesToQuaternion(
    const float yaw,
    const float pitch,
    const float roll)
{
    const float cos_yaw   = cosf(yaw / 2.0f);
    const float sin_yaw   = sinf(yaw / 2.0f);

    const float cos_pitch = cosf(pitch / 2.0f);
    const float sin_pitch = sinf(pitch / 2.0f);

    const float cos_roll  = cosf(roll / 2.0f);
    const float sin_roll  = sinf(roll / 2.0f);

    return (Quaternion){
        .w = cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll,
        .x = cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll,
        .y = cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll,
        .z = sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll,
    };
}

static inline Quaternion
QuaternionDivf(
    const Quaternion * const a,
    const float              b)
{
    SDL_assert(a);
    SDL_assert(b != 0.0f);

    return (Quaternion){
        .w = a->w / b,
        .x = a->x / b,
        .y = a->y / b,
        .z = a->z / b,
    };
}

static inline float
QuaternionMag(
    const Quaternion * const a)
{
    SDL_assert(a);

    return sqrtf(a->w * a->w + a->x * a->x + a->y * a->y + a->z * a->z);
}

static inline Quaternion
QuaternionMult(
    const Quaternion * const a,
    const Quaternion * const b)
{
    Quaternion result = {
        .w = a->w * b->w - a->x * b->x - a->y * b->y - a->z * b->z,
        .x = a->w * b->x + a->x * b->w + a->y * b->z - a->z * b->y,
        .y = a->w * b->y - a->x * b->z + a->y * b->w + a->z * b->x,
        .z = a->w * b->z + a->x * b->y - a->y * b->x + a->z * b->w
    };

    return QuaternionDivf(&result, QuaternionMag(&result));
}

static inline float
QuaternionDot(
    const Quaternion * const a,
    const Quaternion * const b)
{
    return (a->w * b->w) + (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
}

static inline Quaternion
QuaternionNormalize(
    const Quaternion * const a)
{
    SDL_assert(a);

    return QuaternionDivf(a, QuaternionMag(a));
}

static inline Vector
QuaternionRotatev(
    const Quaternion * const q,
    const Vector     * const v)
{
    SDL_assert(q);
    SDL_assert(v);

    const Vector u = {.x = q->x, .y = q->y, .z = q->z};
    const float  s = q->w;

    const float  dot_uv = VectorDot(&u, v);
    const float  dot_uu = VectorDot(&u, &u);
    const Vector crs_uv = VectorCross(&u, v);

    Vector result = VectorMultf(&u, 2.0f * dot_uv);
    Vector temp = VectorMultf(v, s * s - dot_uu);
    result = VectorAdd(&result, &temp);
    temp = VectorMultf(&crs_uv, 2.0f * s);
    result = VectorAdd(&result, &temp);
    return result;
}
