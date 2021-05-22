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

typedef struct Matrix {
    float m[4][4];
} Matrix;

static inline Matrix
VectorToMatrix(
    const Vector * const v)
{
    SDL_assert(v);

    return (Matrix){{
        {v->x, 0.0f, 0.0f, 0.0f},
        {v->y, 0.0f, 0.0f, 0.0f},
        {v->z, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
    }};
}

static inline Vector
MatrixToVector(
    const Matrix * const matrix)
{
    SDL_assert(matrix);

    return (Vector){
        .x = matrix->m[0][0] / matrix->m[3][0],
        .y = matrix->m[1][0] / matrix->m[3][0],
        .z = matrix->m[2][0] / matrix->m[3][0],
    };
}

static inline Matrix
MatrixIdentity(void)
{
    return (Matrix){{
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    }};
}

static inline Matrix
MatrixMult(
    const Matrix * const a,
    const Matrix * const b)
{
    SDL_assert(a);
    SDL_assert(b);

    Matrix result = {{[0] = {[0] = 0}}};

    for (size_t i = 0; i < 4; ++i)
        for (size_t j = 0; j < 4; ++j)
            for (size_t k = 0; k < 4; ++k)
                result.m[i][j] += a->m[i][k] * b->m[k][j];

    return result;
}

static inline Vector
MatrixMultv(
    const Matrix * const m,
    const Vector * const v)
{
    SDL_assert(m);
    SDL_assert(v);

    const Matrix vmat = VectorToMatrix(v);
    const Matrix tmat = MatrixMult(m, &vmat);
    return MatrixToVector(&tmat);
}
