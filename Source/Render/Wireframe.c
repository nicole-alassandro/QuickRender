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

static void
RenderWireframe(
          RenderContext * const context,
    const Matrix        * const model_view_projection)
{
    SDL_assert(context);
    SDL_assert(model_view_projection);

    Mesh * const mesh = context->mesh;
    for (size_t i = 0; i < mesh->faces.size; ++i)
    {
        const Face * const face = &mesh->faces.data[i];

        Vector verts[3];
        for (size_t j = 0; j < 3; ++j)
        {
            verts[j] = mesh->vertices.data[face->indices[j].v];
            verts[j].y *= -1.0f;

            Matrix vert_mat = VectorToMatrix(&verts[j]);
                   vert_mat = MatrixMult(model_view_projection, &vert_mat);
                   verts[j] = MatrixToVector(&vert_mat);
        }

        const SDL_Color color = {255, 255, 255, 255};
        DrawLine(context, &verts[0], &verts[1], &color);
        DrawLine(context, &verts[1], &verts[2], &color);
        DrawLine(context, &verts[2], &verts[0], &color);
    }
}
