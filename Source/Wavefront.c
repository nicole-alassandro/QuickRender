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

static inline int
ObjParseVertex(
    const char   * const str,
          Vector * const result)
{
    SDL_assert(str);
    SDL_assert(result);

    const char * start = str;
          char * end;

    size_t argc = 1;

    errno = 0;

    for (float f = strtof(start, &end); start != end; f = strtof(start, &end))
    {
        result->xyz[argc - 1] = f;

        if (argc >= 3 || errno)
            break;

        argc++;
        start = end;
        errno = 0;
    };

    if (argc > 3)
        return SDL_SetError("Too many vertices given");
    else if (argc < 3)
        return SDL_SetError("Too few vertices given");

    if (errno)
        return SDL_SetError("Invalid vertex value");

    return 0;
}

static inline int
ObjParseFace(
    const char * const str,
          Face * const result)
{
    SDL_assert(str);
    SDL_assert(result);

    const char * start = str;
          char * end;

    size_t argc = 1;
    size_t argf = 1;

    errno = 0;

    for (size_t s = strtoull(start, &end, 10);
         start != end; s = strtoull(start, &end, 10))
    {
        SDL_assert(s > 0);

        if (!s)
            return SDL_SetError("Invalid index value");

        // OBJ face indices begin at 1, we'll convert here to make life easier
        result->indices[argc - 1].vtn[argf - 1] = s - 1;

        if (argc > 3 || argf > 3 || errno)
            break;

        // Assuming carriage returns at line ends...
        if (*end == '\r' || !*end)
            break;

        // Move to next index attribute on '/' delimiters
        if (*end == '/')
        {
            argf++;
            end++;
        }
        else
        {
            argf = 1;
            argc++;
        }

        start = end;
        errno = 0;
    }

    if (argc > 3 || argf > 3)
        return SDL_SetError("Too many indices given");
    else if (argc < 3)
        return SDL_SetError("Too few indices given");

    if (errno)
        return SDL_SetError("Invalid index value");

    return 0;
}

static Mesh*
LoadObj(
    const char * const filepath)
{
    SDL_assert(filepath);

    const File source = LoadFile(filepath);

    if (!source.data || !source.size)
        return NULL;

    Mesh * result = NULL;

    size_t verts = 0;
    size_t norms = 0;
    size_t faces = 0;

    // Cursor row/column
    size_t liner = 0;
    size_t linec = 0;

          char * ptr = strtok(source.data, "\n");
    const char * err = NULL;

    // Preprocess
    // - Find counts to use for memory allocation
    // - Use strtok() to place null delimiters at newlines
    // - Detect unsupported OBJ features
    while (ptr)
    {
        if (liner == SIZE_MAX)
            goto Error_OversizedFile;

        liner++;
        linec = 0;

        // Ignore potential leading whitespace
        ptr += strspn(ptr, " \r\n");

        // Grab length of initial identifier
        const size_t toklen = strcspn(ptr, " ");

        if (!toklen); // pass

        // Supported Features
        else if (!strncmp(ptr,  "v", toklen)) verts++;
        else if (!strncmp(ptr, "vn", toklen)) norms++;
        else if (!strncmp(ptr,  "f", toklen)) faces++;

        // Ignored Features
        else if (!strncmp(ptr,          "o", toklen));
        else if (!strncmp(ptr,          "#", toklen));
        else if (!strncmp(ptr,          "l", toklen));
        else if (!strncmp(ptr,          "p", toklen));
        else if (!strncmp(ptr,          "g", toklen));
        else if (!strncmp(ptr,          "s", toklen));
        else if (!strncmp(ptr,         "sp", toklen));
        else if (!strncmp(ptr,         "mg", toklen));
        else if (!strncmp(ptr,         "fo", toklen));
        else if (!strncmp(ptr,         "vp", toklen));
        else if (!strncmp(ptr,         "vt", toklen));
        else if (!strncmp(ptr,        "lod", toklen));
        else if (!strncmp(ptr,        "con", toklen));
        else if (!strncmp(ptr,        "deg", toklen));
        else if (!strncmp(ptr,       "bmat", toklen));
        else if (!strncmp(ptr,       "step", toklen));
        else if (!strncmp(ptr,       "trim", toklen));
        else if (!strncmp(ptr,       "surf", toklen));
        else if (!strncmp(ptr,       "hole", toklen));
        else if (!strncmp(ptr,       "scrv", toklen));
        else if (!strncmp(ptr,       "curv", toklen));
        else if (!strncmp(ptr,      "curv2", toklen));
        else if (!strncmp(ptr,      "ctech", toklen));
        else if (!strncmp(ptr,      "stech", toklen));
        else if (!strncmp(ptr,      "bevel", toklen));
        else if (!strncmp(ptr,     "mtllib", toklen));
        else if (!strncmp(ptr,     "usemtl", toklen));
        else if (!strncmp(ptr,     "cstype", toklen));
        else if (!strncmp(ptr,   "c_interp", toklen));
        else if (!strncmp(ptr,   "d_interp", toklen));
        else if (!strncmp(ptr,  "trace_obj", toklen));
        else if (!strncmp(ptr, "shadow_obj", toklen));
        else goto Error_Unknown;

        ptr = strtok(NULL, "\n");
    }

    // Vert and face data required for rendering, but baked normals are optional
    if (!(verts | faces))
        goto Error_NoGeometry;

    printf(
        "verts: %zu\n"
        "norms: %zu\n"
        "faces: %zu\n",
        verts,
        norms,
        faces
    );

    // Force calculation of normals if the .obj did not have any
    const bool calculate_normals = (norms == 0);
    norms = (calculate_normals) ? verts : norms;

    if (calculate_normals)
        printf("Calculating normals...\n");

    result = calloc(1, sizeof(Mesh));
    if (!result || MeshAlloc(result, verts, norms, faces))
        goto Error_Allocation;

    // Re-use variables as current indices for each data structure
    verts = norms = faces = 0;

    liner = 0;
    linec = 0;
    ptr   = source.data;

    while (ptr < source.data + source.size)
    {
        liner++;
        linec = 0;

        // Ignore potential leading whitespace
        ptr += strspn(ptr, " \r\n");

        // Grab length of initial identifier
        const size_t toklen = strcspn(ptr, " ");

        if (!toklen)
        {
            // pass
        }
        else if (!strncmp(ptr, "v", toklen))
        {
            SDL_assert(verts < result->vertices.size);
            if (ObjParseVertex(ptr + toklen, &result->vertices.data[verts++]))
                goto Error;
        }
        else if (!strncmp(ptr, "vn", toklen))
        {
            SDL_assert(norms < result->normals.size);
            if (ObjParseVertex(ptr + toklen, &result->normals.data[norms++]))
                goto Error;
        }
        else if (!strncmp(ptr, "f", toklen))
        {
            SDL_assert(faces < result->faces.size);
            if (ObjParseFace(ptr + toklen, &result->faces.data[faces++]))
                goto Error;
        }

        // Skip over null delimiters
        while (*ptr++);
    }

    // {
    //     float max_vert = 0.0f;
    //     for (size_t i = 0; i < result->vertices.size; ++i)
    //     {
    //         const Vector * const vert = &result->vertices.data[i];
    //         max_vert = fmaxf(max_vert, fabsf(vert->x));
    //         max_vert = fmaxf(max_vert, fabsf(vert->y));
    //         max_vert = fmaxf(max_vert, fabsf(vert->z));
    //     }

    //     for (size_t i = 0; i < result->vertices.size; ++i)
    //     {
    //         Vector * const vert = &result->vertices.data[i];
    //         vert->x /= max_vert;
    //         vert->y /= max_vert;
    //         vert->z /= max_vert;
    //     }
    // }

    // Calculate normals if none were provided
    if (calculate_normals)
        MeshCalcNorms(result);

    // Validate face index values
    for (size_t i = 0; i < result->faces.size; ++i)
    {
        const Face * const face = &result->faces.data[i];
        for (size_t j = 0; j < 3; ++j)
        {
            if (face->indices[j].v >= result->vertices.size)
                goto Error_Value;

            if (result->normals.size > 0)
                if (face->indices[j].n >= result->normals.size)
                    goto Error_Value;
        }
    }

    ptr = NULL;

    free(source.data);

    return result;

Error_OversizedFile:
    err = "File is too large";
    goto Set_Error;

Error_NoGeometry:
    err = "No geometry data found";
    goto Set_Error;

Error_Allocation:
    err = "Failed to allocate mesh data";
    goto Set_Error;

Error_Unsupported:
    err = "Unsupported attribute";
    goto Set_Error;

Error_Unknown:
    err = "Unknown identifier";
    goto Set_Error;

Error_Syntax:
    err = "Syntax error";
    goto Set_Error;

Error_Value:
    err = "Invalid index value";
    goto Set_Error;

Error:
    goto Cleanup;

Set_Error:
    SDL_SetError("%s:%zu:%zu: %s", filepath, liner, linec, err);

Cleanup:
    free(source.data);

    if (result) {
        MeshFree(result);
        free(result);
    }

    return NULL;
}
