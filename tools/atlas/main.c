#define NANOSVG_IMPLEMENTATION
#include "include/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "include/nanosvgrast.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "include/stb_rect_pack.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#include <marrow/marrow.h>
#include <marrow/alloc.h>

#define ORIGIN_ID "origin"
#define PAGE_W    2048
#define PAGE_H    2048
#define PAD       2

STRUCT(Sprite) {
    char* path;
    char  ident[128];
    u8*   px;
    i32   w, h;
    f32   ox, oy;
    i32   page, x, y;
};

Allocator* a = nullptr;

static char* read_file(cstr path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char* buf = mrw_alloc_n(a, char, n + 1);
    fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[n] = 0;
    return buf;
}

static void make_ident(char* path, char* out, u32 cap) {
    str name = str(path);
    char* sep = str_find_filter_r(name, FILTER_PATH_SEPARATOR);
    if (sep) name.start = sep + 1;
    char* dot = str_find_r(name, '.');
    if (dot) name.end = dot;

    u32 n = min((u32)slice_count(name), cap - 1);
    for (u32 i = 0; i < n; i++) {
        char c = name.start[i];
        out[i] = char_in_filter(c, FILTER_CHAR | FILTER_DIGIT) ? char_upper(c) : '_';
    }
    out[n] = 0;
}

static void load_sprite(Sprite* s, NSVGrasterizer* rast) {
    NSVGimage* img = nsvgParse(read_file(s->path), "px", 96.0f);

    f32 b[4] = { INFINITY, INFINITY, -INFINITY, -INFINITY };
    bool found = false;
    for (NSVGshape* sh = img->shapes; sh; sh = sh->next) {
        if (c_str_cmp(sh->id, ORIGIN_ID)) continue;
        b[0] = fminf(b[0], sh->bounds[0]); b[1] = fminf(b[1], sh->bounds[1]);
        b[2] = fmaxf(b[2], sh->bounds[2]); b[3] = fmaxf(b[3], sh->bounds[3]);
        sh->flags &= ~NSVG_FLAGS_VISIBLE;
        found = true;
    }
    if (!found) mrw_abort("{}: missing origin", s->path);

    s->w = (i32)ceilf(img->width);
    s->h = (i32)ceilf(img->height);
    s->ox = 0.5f * (b[0] + b[2]);
    s->oy = 0.5f * (b[1] + b[3]);

    s->px = mrw_alloc_n(a, u8, (usize)s->w * s->h * 4);
    nsvgRasterize(rast, img, 0, 0, 1.0f, s->px, s->w, s->h, s->w * 4);
    nsvgDelete(img);
}

static i32 pack(SpriteSlice sprites) {
    slice_for_each(sprites, s, Sprite) s->page = -1;

    u32 n = slice_count(sprites);
    stbrp_rect* r     = mrw_alloc_n(a, stbrp_rect, n);
    stbrp_node* nodes = mrw_alloc_n(a, stbrp_node, PAGE_W);
    i32 pages = 0;
    u32 left  = n;
    while (left > 0) {
        i32 m = 0;
        slice_for_each_i(sprites, i) {
            Sprite* s = &sprites.start[i];
            if (s->page >= 0) continue;
            r[m].id = (i32)i;
            r[m].w  = s->w + PAD * 2;
            r[m].h  = s->h + PAD * 2;
            m++;
        }

        stbrp_context ctx;
        stbrp_init_target(&ctx, PAGE_W, PAGE_H, nodes, PAGE_W);
        stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
        stbrp_pack_rects(&ctx, r, m);

        u32 placed = 0;
        for (u32 i = 0; i < m; i++) {
            if (!r[i].was_packed) continue;
            Sprite* s = &sprites.start[r[i].id];
            s->page = pages;
            s->x = r[i].x + PAD;
            s->y = r[i].y + PAD;
            placed++;
        }
        left -= placed;
        pages++;
    }
    return pages;
}

static void write_pages(SpriteSlice sprites, i32 pages, cstr dir) {
    usize size = (usize)PAGE_W * PAGE_H * 4;
    u8* buf = mrw_alloc_n(a, u8, size);
    char path[1024] = { 0 };
    for (u32 p = 0; p < pages; p++) {
        buf_set(buf, 0, size);
        slice_for_each(sprites, s, Sprite) {
            if (s->page != p) continue;
            for (u32 y = 0; y < s->h; y++) {
                u8* dst = buf + ((usize)(s->y + y) * PAGE_W + (usize)(s->x - PAD)) * 4;
                buf_copy(dst, s->px + y * s->w * 4, s->w * 4);
            }
        }
        print(path, sizeof(path), "{}/atlas_{d}.png", dir, p);
        stbi_write_png(path, PAGE_W, PAGE_H, 4, buf, PAGE_W * 4);
    }
}

static void write_header(SpriteSlice sprites, i32 pages, cstr dir) {
    char path[1024] = { 0 };
    print(path, sizeof(path), "{}/atlas.h", dir);
    mrw_debug("writing to file {}", path);
    FILE* f = fopen(path, "wb");

    printfb(f, "#pragma once\n\n");
    printfb(f, "#include <marrow/marrow.h>\n\n");
    printfb(f, "#define ATLAS_PAGE_COUNT {}\n#define ATLAS_PAGE_W {}\n#define ATLAS_PAGE_H {}\n\n", pages, PAGE_W, PAGE_H);

    printfb(f, "typedef enum %{\n");
    slice_for_each(sprites, s, Sprite) printfb(f, "    SPRITE_{},\n", s->ident);
    printfb(f, "    SPRITE_COUNT\n} SpriteId;\n\n");

    printfb(f, "static struct %{"
        "i32 page, x, y, w, h; f64 u0, v0, u1, v1, ox, oy;} sprites[SPRITE_COUNT] = %{\n");
    slice_for_each(sprites, s, Sprite)
    {
        printfb(f, "    %{{}, ", s->page);
        printfb(f, "{}, {}, {}, {}, ", s->x, s->y, s->w, s->h);
        printfb(f, "{}, {}, {}, {}, ",
            (f64)s->x / PAGE_W, (f64)s->y / PAGE_H,
            (f64)(s->x + s->w) / PAGE_W, (f64)(s->y + s->h) / PAGE_H);
        printfb(f, "{}, {}},\n", (f64)s->ox, (f64)s->oy);
    }
    printfb(f, "};\n\n");

    fclose(f);
}

// atlasgen <png_dir> <gen_dir> <file.svg>...
i32 main(i32 argc, char** argv) {
    argv++;
    cstr png_dir = *(argv++);
    cstr gen_dir = *(argv++);

    u32 n = (u32)argc - 3;
    Sprite* s = mrw_alloc_n(a, Sprite, n);
    SpriteSlice sprites = slice_to(s, n);

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    slice_for_each(sprites, s, Sprite)
    {
        s->path = *(argv++);
        make_ident(s->path, s->ident, sizeof s->ident);
        load_sprite(s, rast);
    }

    i32 pages = pack(sprites);
    write_pages(sprites, pages, png_dir);
    write_header(sprites, pages, gen_dir);
    return 0;
}
