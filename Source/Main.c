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

const int WINDOW_WIDTH  = 400;
const int WINDOW_HEIGHT = 400;

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "Utils.c"
#include "Vector.c"
#include "Matrix.c"
#include "Quaternion.c"
#include "Mesh.c"
#include "Wavefront.c"
#include "Render.c"
#include "Render/Wireframe.c"
#include "Render/Flat.c"
#include "Render/Gouraud.c"
#include "Render/Phong.c"
#include "Render/Toon.c"

int main(int argc, const char** argv)
{
    if (argc != 2)
    {
        printf("Usage: QuickRender <file>\n");
        return EXIT_FAILURE;
    }

    {
        SDL_version version;
        SDL_VERSION(&version);
        printf("SDL %d.%d.%d\n", version.major, version.minor, version.patch);
    }

    RenderContext context;
    context.mode     = RENDER_WIREFRAME;
    context.rotation = (Quaternion){{.w = 1.0f}};
    context.light    = VectorNormalize(&(Vector){.x = 4.0f, .y = 4.0f, .z = 3.0f});
    context.camera   = (Camera){
        .pos   = (Vector){.z = 300.0f},
        .focus = (Vector){.x =   -2.0f, .y = -4.0f},
        .up    = (Vector){.y =   1.0f},
        .right = (Vector){.x =   1.0f},
    };

    context.mesh = LoadObj(argv[1]);

    if (!context.mesh)
        goto Error_Init;

    SDL_assert(context.mesh->vertices.size > 0);
    SDL_assert(context.mesh->faces.size    > 0);
    SDL_assert(context.mesh->normals.size  > 0);

    if (SDL_Init(SDL_INIT_VIDEO))
        goto Error_Init;

    SDL_Window * const window = SDL_CreateWindow(
        argv[1],
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
            | SDL_WINDOW_ALLOW_HIGHDPI
            | SDL_WINDOW_INPUT_FOCUS
            | SDL_WINDOW_MOUSE_FOCUS
    );

    if (!window)
        goto Error_Window;

    context.depth = SDL_CreateRGBSurfaceWithFormat(
        0 /* flags */,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        32, SDL_PIXELFORMAT_RGBA32
    );

    if (!context.depth)
        goto Error_DepthBuffer;

    uint32_t clock = SDL_GetTicks();
    uint32_t delta = 0;

    typedef struct InputState {
        bool w, a, s, d, q, e;
    } InputState;

    InputState input = {false};

    while (!SDL_QuitRequested())
    {
        const uint32_t time = SDL_GetTicks();
        delta = time - clock;
        clock = time;

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                // Trigger keys
                case SDL_KEYDOWN:
                {
                    bool fallthrough = false;

                    switch (event.key.keysym.sym)
                    {
                        case SDLK_1:
                            context.mode = RENDER_WIREFRAME;
                            break;

                        case SDLK_2:
                            context.mode = RENDER_FLAT;
                            break;

                        case SDLK_3:
                            context.mode = RENDER_GOURAUD;
                            break;

                        case SDLK_4:
                            context.mode = RENDER_PHONG;
                            break;

                        case SDLK_5:
                            context.mode = RENDER_TOON;
                            break;

                        default:
                            fallthrough = true;
                    }

                    // Fallthrough to catch WASD/QE key releases
                    if (!fallthrough)
                        break;
                }

                // Stateful keys
                case SDL_KEYUP:
                {
                    if (event.key.repeat)
                        break;

                    switch (event.key.keysym.sym)
                    {
                        case SDLK_w:
                            input.w = event.key.state == SDL_PRESSED;
                            break;

                        case SDLK_a:
                            input.a = event.key.state == SDL_PRESSED;
                            break;

                        case SDLK_s:
                            input.s = event.key.state == SDL_PRESSED;
                            break;

                        case SDLK_d:
                            input.d = event.key.state == SDL_PRESSED;
                            break;

                        case SDLK_q:
                            input.q = event.key.state == SDL_PRESSED;
                            break;

                        case SDLK_e:
                            input.e = event.key.state == SDL_PRESSED;
                            break;
                    }

                    break;
                }
            }
        }

        // Camera transformation
        {
            const float rotation = (float)M_PI / 64.0f;

            float qx  = (input.w) ? -1.0f : 0.0f;
                  qx += (input.s) ?  1.0f : 0.0f;

            float qy  = (input.a) ? -1.0f : 0.0f;
                  qy += (input.d) ?  1.0f : 0.0f;

            float qz  = (input.q) ? -1.0f : 0.0f;
                  qz += (input.e) ?  1.0f : 0.0f;

            qx *= rotation;
            qy *= rotation;
            qz *= rotation;

            Quaternion quaternion = AnglesToQuaternion(qz, qy, qx);
            quaternion = QuaternionNormalize(&quaternion);
            context.rotation = QuaternionMult(&quaternion, &context.rotation);
            context.rotation = QuaternionNormalize(&context.rotation);
        }

        context.target = SDL_GetWindowSurface(window);

        if (!context.target)
            goto Error_Surface;

        if (Render(&context) != 0)
            goto Error_Render;

        if (SDL_UpdateWindowSurface(window) != 0)
            goto Error_Render;

        if (delta < (1000 / 20))
            SDL_Delay((1000 / 20) - delta);
    }

Error_Render:
Error_Surface:
Error_DepthBuffer:
    SDL_FreeSurface(context.depth);

Error_Window:
    SDL_DestroyWindow(window);

Error_Init:
    puts(SDL_GetError());
    SDL_ClearError();

    SDL_Quit();

    if (context.mesh)
    {
        MeshFree(context.mesh);
        free(context.mesh);
    }

    return EXIT_SUCCESS;
}
