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

static inline size_t
SizeAdd(
    const size_t a,
    const size_t b)
{
    SDL_assert(b <= SIZE_MAX - a);
    return a + b;
}

static inline size_t
SizeMult(
    const size_t a,
    const size_t b)
{
    if (a > 0)
        SDL_assert(b <= SIZE_MAX / a);

    return a * b;
}

typedef struct File {
          char   * const data;
    const size_t         size;
} File;

static inline File
LoadFile(
    const char * const filepath)
{
    SDL_assert(filepath);

    SDL_RWops * const file = SDL_RWFromFile(filepath, "rb");

    if (!file)
        return (File){.data=NULL};

    const int64_t rwsize = SDL_RWsize(file);

    if (rwsize < 0)
        return (File){.data=NULL};

    const size_t size = (size_t)rwsize;

    char * const data = malloc(SizeAdd(size, 1));

    if (!data)
        return (File){.data=NULL};

    size_t read = 0;
    size_t read_total = 0;

    char * buffer = data;

    do {
        read = SDL_RWread(file, buffer, sizeof(char), size - read_total);
        read_total = SizeAdd(read_total, read);
        buffer += read;
    } while (read_total < size && read != 0);

    SDL_RWclose(file);
    if (read_total != size) {
        free(data);
        return (File){.data=NULL};
    }

    data[read_total] = '\0';
    return (File){data, size};
}
