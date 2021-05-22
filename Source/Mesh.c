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

typedef struct Vertices {
    Vector * const data;
    size_t         size;
} Vertices;

typedef union Index {
    struct { size_t v, t /* unimplemented */ , n; };
    size_t vtn[3];
} Index;

typedef struct Face {
    Index indices[3];
} Face;

typedef struct Faces {
    Face   * const data;
    size_t         size;
} Faces;

typedef struct Mesh {
    Vertices vertices;
    Vertices normals;
    Faces    faces;
} Mesh;

static inline bool
MeshAlloc(
    Mesh * const mesh,
    const size_t verts,
    const size_t norms,
    const size_t faces)
{
    SDL_assert(mesh);
    SDL_assert(!mesh->vertices.data);
    SDL_assert(!mesh->normals.data);
    SDL_assert(!mesh->faces.data);

    size_t pool_size = 0;
    pool_size = SizeAdd(pool_size, SizeMult(sizeof(Vector), verts));
    pool_size = SizeAdd(pool_size, SizeMult(sizeof(Vector), norms));
    pool_size = SizeAdd(pool_size, SizeMult(sizeof(Face  ), faces));

    Vector * mesh_arena = calloc(1, pool_size);

    if (!mesh_arena)
        return SDL_SetError("Unable to allocate mesh memory");

    SDL_memcpy(
        mesh,
        &(Mesh){
            .vertices = (Vertices){
                .data = mesh_arena,
                .size = verts,
            },
            .normals = (Vertices){
                .data = mesh_arena + verts,
                .size = norms,
            },
            .faces = (Faces){
                .data = (Face*)(mesh_arena + verts + norms),
                .size = faces,
            },
        },
        sizeof(Mesh)
    );

    return 0;
}

static inline void
MeshFree(
    Mesh * const mesh)
{
    SDL_assert(mesh);
    SDL_assert(mesh->vertices.data);
    SDL_assert(mesh->normals.data);
    SDL_assert(mesh->faces.data);

    SDL_assert(
        mesh->normals.data
     == mesh->vertices.data + mesh->vertices.size
    );

    SDL_assert(
        mesh->faces.data
     == (Face*)(mesh->normals.data + mesh->normals.size)
    );

    free(mesh->vertices.data);
    memset(mesh, 0, sizeof(Mesh));
}

static inline void
MeshCalcNorms(
    Mesh * const mesh)
{
    SDL_assert(mesh);

    // Prerequisites set by LoadObj()
    SDL_assert(mesh->vertices.size > 0);
    SDL_assert(mesh->faces.size > 0);
    SDL_assert(mesh->normals.size == mesh->vertices.size);

    for (size_t i = 0; i < mesh->faces.size; i++)
    {
        const Vector * verts[3];
        for (size_t j = 0; j < 3; ++j)
        {
            Index * const index = &mesh->faces.data[i].indices[j];
            verts[j] = &mesh->vertices.data[index->v];
            index->n = index->v;
        }

        const Vector side[2] = {
            VectorSub(verts[1], verts[0]),
            VectorSub(verts[2], verts[0]),
        };

        const Vector normal = VectorCross(&side[0], &side[1]);

        for (size_t j = 0; j < 3; ++j)
        {
            Index * const index = &mesh->faces.data[i].indices[j];
            mesh->normals.data[index->n] = VectorAdd(
                &mesh->normals.data[index->n], &normal
            );
        }
    }

    for (size_t i = 0; i < mesh->faces.size; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            Index * const index = &mesh->faces.data[i].indices[j];
            mesh->normals.data[index->n] = VectorNormalize(
                &mesh->normals.data[index->n]
            );
        }
    }
}
