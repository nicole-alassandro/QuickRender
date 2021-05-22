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

typedef enum RenderMode {
    RENDER_WIREFRAME,
    RENDER_FLAT,
    RENDER_GOURAUD,
    RENDER_PHONG,
    RENDER_TOON,
} RenderMode;

typedef enum RenderFlags {
    RENDER_TEXTURES = 1 << 0,
    RENDER_LIGHTING = 1 << 1,
} RenderFlags;

typedef struct Camera {
    Vector pos;
    Vector focus;
    Vector up;
    Vector right;
} Camera;

typedef struct RenderContext {
    SDL_Surface * target;
    SDL_Surface * depth;

    Mesh        * mesh;

    RenderMode    mode;
    int           flags;

    Camera        camera;
    Vector        light;

    Quaternion    rotation;
} RenderContext;

static inline bool
TestDepth(
          RenderContext * const context,
    const Vector        * const coord)
{
    SDL_assert(context && context->depth);
    SDL_assert(coord);
    SDL_assert(context->depth->format->BytesPerPixel == (int)sizeof(float));

    const SDL_Point point = {(int)coord->x, (int)coord->y};

    if (point.x < 0 || point.x >= context->depth->w)
        return false;

    if (point.y < 0 || point.y >= context->depth->h)
        return false;

    float * depth = (float*)(
        (uint8_t*)context->depth->pixels
      + (point.y * context->depth->pitch)
      + (point.x * context->depth->format->BytesPerPixel)
    );

    if (*depth > coord->z)
        return false;

    *depth = coord->z;
    return true;
}

static inline bool
TestBackface(
          RenderContext * const context,
    const Vector        * const tri)
{
    SDL_assert(context);
    SDL_assert(tri);

    const Vector side[2] = {
        VectorSub(&tri[2], &tri[0]),
        VectorSub(&tri[1], &tri[0]),
    };

    Vector normal = VectorCross(&side[0], &side[1]);
           normal = VectorNormalize(&normal);

    const Vector cam_to_tri = VectorSub(
        &tri[0], &context->camera.pos
    );

    return (VectorDot(&normal, &cam_to_tri) < 0.0f);
}

static inline void
PutFragment(
          RenderContext * const context,
    const Vector        * const coord,
    const SDL_Color     * const color)
{
    SDL_assert(context && context->target && context->depth);
    SDL_assert(coord);
    SDL_assert(color);

    const SDL_Point point = {(int)coord->x, (int)coord->y};

    if (point.x < 0 || point.x >= context->target->w)
        return;

    if (point.y < 0 || point.y >= context->target->h)
        return;

    uint8_t * pixels = {
        (uint8_t*)context->target->pixels
      + (point.y * context->target->pitch)
      + (point.x * context->target->format->BytesPerPixel)
    };

    *((uint32_t*)pixels) = SDL_MapRGBA(
        context->target->format,
        color->r,
        color->g,
        color->b,
        color->a
    );
}

static void
DrawLine(
          RenderContext * const context,
    const Vector        * const start,
    const Vector        * const end,
    const SDL_Color     * const color)
{
    SDL_assert(context);
    SDL_assert(start);
    SDL_assert(end);
    SDL_assert(color);

    const Vector delta = {
        .x =  fabsf(end->x - start->x),
        .y = -fabsf(end->y - start->y),
    };

    const Vector step = {
        .x = (start->x < end->x) ? 1 : -1,
        .y = (start->y < end->y) ? 1 : -1,
    };

    Vector pos = *start;
    float  err = delta.x + delta.y;

    while (true)
    {
        PutFragment(context, &pos, color);

        if ((err * 2) >= delta.y)
        {
            if (fabsf(pos.x - end->x) < 1.0f)
                break;

            err += delta.y;
            pos.x += step.x;
        }

        if ((err * 2) <= delta.x)
        {
            if (fabsf(pos.y - end->y) < 1.0f)
                break;

            err += delta.x;
            pos.y += step.y;
        }
    }
}

static inline Matrix
GetViewport(
    const SDL_FRect * const bounds,
    const float             depth)
{
    SDL_assert(bounds);

    Matrix matrix = MatrixIdentity();
    matrix.m[0][0] = bounds->w / 2.0f;
    matrix.m[0][3] = bounds->x + bounds->w / 2.0f;

    matrix.m[1][1] = bounds->h / 2.0f;
    matrix.m[1][3] = bounds->y + bounds->h / 2.0f;

    matrix.m[2][2] = depth / 2.0f;
    matrix.m[2][3] = depth * 2.0f;

    return matrix;
}

static inline Matrix
LookAt(
    const Camera * const camera)
{
    SDL_assert(camera);

    Vector z = VectorSub(&camera->pos, &camera->focus);
           z = VectorNormalize(&z);

    Vector x = VectorCross(&camera->up, &z);
           x = VectorNormalize(&x);

    Vector y = VectorCross(&z, &x);
           y = VectorNormalize(&y);

    Matrix minv      = MatrixIdentity();
    Matrix transform = MatrixIdentity();
    for (size_t i = 0; i < 3; ++i)
    {
        minv.m[0][i] = x.xyz[i];
        minv.m[1][i] = y.xyz[i];
        minv.m[2][i] = z.xyz[i];
        transform.m[i][3] = -camera->focus.xyz[i];
    }

    return MatrixMult(&minv, &transform);
}

static void RenderWireframe(RenderContext * const, const Matrix * const);
static void RenderFlat     (RenderContext * const, const Matrix * const);
static void RenderGouraud  (RenderContext * const, const Matrix * const);
static void RenderPhong    (RenderContext * const, const Matrix * const);
static void RenderToon     (RenderContext * const, const Matrix * const);

static inline int
Render(
    RenderContext * const context)
{
    SDL_assert(context);
    SDL_assert(context->target);
    SDL_assert(context->mesh);
    SDL_assert(context->depth);

    SDL_FillRect(context->target, NULL, 0);
    SDL_FillRect(context->depth,  NULL, 0);

    if (SDL_MUSTLOCK(context->target))
        if (SDL_LockSurface(context->target) != 0)
            goto Error_SurfaceLocking;

    if (SDL_MUSTLOCK(context->depth))
        if (SDL_LockSurface(context->depth) != 0)
            goto Error_SurfaceLocking;

    Matrix projection = MatrixIdentity();
    {
        const Vector camera = VectorSub(
            &context->camera.pos, &context->camera.focus
        );

        projection.m[3][2] = 1.0f / VectorMag(&camera);
    }

    const Matrix viewport = GetViewport(
        &(SDL_FRect){
            .x = (float)WINDOW_WIDTH  / 8.0f,
            .y = (float)WINDOW_HEIGHT / 8.0f,
            .w = (float)WINDOW_WIDTH  * 0.25f,
            .h = (float)WINDOW_HEIGHT * 0.25f,
        },
        255.0f /* depth */
    );

    const Camera camera_old = context->camera;

    context->camera.pos = QuaternionRotatev(
        &context->rotation, &context->camera.pos
    );

    const Matrix model = LookAt(&context->camera);

    Matrix mvpm = MatrixMult(&viewport, &projection);
           mvpm = MatrixMult(&mvpm, &model);

    void (*render_func)(RenderContext * const, const Matrix * const) = NULL;
    switch (context->mode)
    {
        case RENDER_WIREFRAME: render_func = RenderWireframe; break;
        case RENDER_FLAT:      render_func = RenderFlat;      break;
        case RENDER_GOURAUD:   render_func = RenderGouraud;   break;
        case RENDER_PHONG:     render_func = RenderPhong;     break;
        case RENDER_TOON:      render_func = RenderToon;      break;

        default:
            SDL_assert(0);
    }

    if (render_func)
        render_func(context, &mvpm);

    context->camera = camera_old;

    if (SDL_MUSTLOCK(context->target))
        SDL_UnlockSurface(context->target);

    if (SDL_MUSTLOCK(context->depth))
        SDL_UnlockSurface(context->depth);

    return 0;

Error_SurfaceLocking:
    if (SDL_MUSTLOCK(context->target))
        SDL_UnlockSurface(context->target);

    if (SDL_MUSTLOCK(context->depth))
        SDL_UnlockSurface(context->depth);

    return 1;
}
