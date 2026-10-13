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
        if (strcmp(sh->id, ORIGIN_ID)) continue;
        b[0] = fminf(b[0], sh->bounds[0]); b[1] = fminf(b[1], sh->bounds[1]);
        b[2] = fmaxf(b[2], sh->bounds[2]); b[3] = fmaxf(b[3], sh->bounds[3]);
        sh->flags &= ~NSVG_FLAGS_VISIBLE;
        found = true;
    }
    if (!found) mrw_abort("{}: missing origin", s->path);
    f32 cx = 0.5f * (b[0] + b[2]), cy = 0.5f * (b[1] + b[3]);

    i32 rw = (i32)ceilf(img->width), rh = (i32)ceilf(img->height);
    u8* full = mrw_alloc_n(a, u8, (usize)rw * rh * 4);
    nsvgRasterize(rast, img, 0, 0, 1.0f, full, rw, rh, rw * 4);
    nsvgDelete(img);

    i32 x0 = rw, y0 = rh, x1 = -1, y1 = -1;
    for (i32 y = 0; y < rh; y++) {
        for (i32 x = 0; x < rw; x++) {
            if (!full[((usize)y * rw + x) * 4 + 3]) continue;
            x0 = min(x0, x); x1 = max(x1, x);
            y0 = min(y0, y); y1 = max(y1, y);
        }
    }

    s->w = x1 - x0 + 1;
    s->h = y1 - y0 + 1;
    s->px = mrw_alloc_n(a, u8, (usize)s->w * s->h * 4);
    for (i32 y = 0; y < s->h; y++)
        buf_copy(s->px + (usize)y * s->w * 4, full + ((usize)(y0 + y) * rw + x0) * 4, (usize)s->w * 4);

    s->ox = cx - (f32)x0;
    s->oy = cy - (f32)y0;
}

static i32 pack(Sprite* s, u32 n) {
    for (u32 i = 0; i < n; i++) s[i].page = -1;

    stbrp_rect* r     = mrw_alloc_n(a, stbrp_rect, n);
    stbrp_node* nodes = mrw_alloc_n(a, stbrp_node, PAGE_W);
    i32 pages = 0;
    u32 left  = n;
    while (left > 0) {
        u32 m = 0;
        for (u32 i = 0; i < n; i++) {
            if (s[i].page >= 0) continue;
            r[m].id = (i32)i;
            r[m].w  = s[i].w + 2 * PAD;
            r[m].h  = s[i].h + 2 * PAD;
            m++;
        }
        stbrp_context ctx;
        stbrp_init_target(&ctx, PAGE_W, PAGE_H, nodes, PAGE_W);
        stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
        stbrp_pack_rects(&ctx, r, (i32)m);

        u32 placed = 0;
        for (u32 j = 0; j < m; j++) {
            if (!r[j].was_packed) continue;
            Sprite* sp = &s[r[j].id];
            sp->page = pages;
            sp->x = r[j].x + PAD;
            sp->y = r[j].y + PAD;
            placed++;
        }
        if (!placed) mrw_abort("a sprite is larger than the page");
        left -= placed;
        pages++;
    }
    return pages;
}

static void write_pages(const Sprite* sprites, u32 n, i32 pages, cstr dir) {
    usize size = (usize)PAGE_W * PAGE_H * 4;
    u8* buf = mrw_alloc_n(a, u8, size);
    char path[1024];
    for (i32 p = 0; p < pages; p++) {
        buf_set(buf, 0, size);
        for (u32 i = 0; i < n; i++) {
            const Sprite* s = &sprites[i];
            if (s->page != p) continue;
            for (i32 y = -PAD; y < s->h + PAD; y++) {
                const u8* row = s->px + (usize)clamp(y, 0, s->h - 1) * s->w * 4;
                u8* dst = buf + ((usize)(s->y + y) * PAGE_W + (usize)(s->x - PAD)) * 4;
                for (i32 x = -PAD; x < s->w + PAD; x++, dst += 4)
                    buf_copy(dst, row + (usize)clamp(x, 0, s->w - 1) * 4, 4);
            }
        }
        print(path, sizeof(path), "{}/atlas_{}.png", dir, p);
        stbi_write_png(path, PAGE_W, PAGE_H, 4, buf, PAGE_W * 4);
    }
}

static void write_header(const Sprite* s, u32 n, i32 pages, cstr dir) {
    char path[1024] = { 0 };
    print(path, sizeof(path), "{}/atlas.h", dir);
    FILE* f = fopen(path, "wb");

    printfb(f, "#pragma once\n\n");
    printfb(f, "#include <marrow/marrow.h>\n\n");
    printfb(f, "#define ATLAS_PAGE_COUNT {}\n#define ATLAS_PAGE_W {}\n#define ATLAS_PAGE_H {}\n\n", pages, PAGE_W, PAGE_H);

    printfb(f, "typedef enum %{\n");
    for (u32 i = 0; i < n; i++) printfb(f, "    SPRITE_{},\n", s[i].ident);
    printfb(f, "    SPRITE_COUNT\n} SpriteId;\n\n");

    printfb(f, "static struct %{"
        "i32 page, x, y, w, h; f64 u0, v0, u1, v1, ox, oy; } sprites[SPRITE_COUNT] = %{\n");
    for (u32 i = 0; i < n; i++)
    {
        printfb(f, "%{{},", s[i].page);
        printfb(f, "{}, {}, {}, {},", s[i].x, s[i].y, s[i].w, s[i].h);
        printfb(f, "{}, {}, {}, {},",
            (f64)s[i].x / PAGE_W, (f64)s[i].y / PAGE_H,
            (f64)(s[i].x + s[i].w) / PAGE_W, (f64)(s[i].y + s[i].h) / PAGE_H);
        printfb(f, "{}, {}},\n", (f64)s[i].ox, (f64)s[i].oy);
    }
    printfb(f, "};\n\n");

    fclose(f);
}

// atlasgen <png_dir> <gen_dir> <file.svg>...
i32 main(i32 argc, char** argv) {
    cstr png_dir = argv[1];
    cstr gen_dir = argv[2];
    u32  n       = (u32)argc - 3;

    Sprite* sprites = mrw_alloc_n(a, Sprite, n);
    for (u32 i = 0; i < n; i++) {
        sprites[i].path = argv[3 + i];
        make_ident(sprites[i].path, sprites[i].ident, sizeof sprites[i].ident);
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    for (u32 i = 0; i < n; i++) load_sprite(&sprites[i], rast);

    i32 pages = pack(sprites, n);
    write_pages(sprites, n, pages, png_dir);
    write_header(sprites, n, pages, gen_dir);
    return 0;
}
