#define NANOSVG_IMPLEMENTATION
#include "include/nanosvg.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "include/stb_rect_pack.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

#include <resvg.h>

#include <marrow/marrow.h>
#include <marrow/alloc.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 2048
#define PAD       2
#define MAX_TAGS  128

STRUCT(Tag) {
    char name[64];
    f32  x, y;
};

STRUCT(Sprite) {
    char* path;
    char  ident[128];
    u8*   px;
    i32   w, h;
    i32   ox, oy;
    Tag   tags[MAX_TAGS];
    u32   n_tags;
    i32   page, x, y;
};

Allocator* a = nullptr;

static str read_file(cstr path) {
    FILE* f = fopen(path, "rb");
    if (!f) mrw_abort("cannot open {}", path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    if (n < 0) mrw_abort("cannot read {}", path);
    rewind(f);
    char* buf = mrw_alloc_n(a, char, (usize)n + 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) mrw_abort("cannot read {}", path);
    fclose(f);
    buf[n] = 0;
    return (str)slice_to(buf, n);
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

static void make_tag_name(cstr id, char* out, u32 cap) {
    u32 i = 0;
    while (id[i] && i < cap - 1)
    {
        out[i] = char_in_filter(id[i], FILTER_CHAR | FILTER_DIGIT) ? id[i] : '_';
        i++;
    }
    out[i] = 0;
}

static void load_sprite(Sprite* s, resvg_options* opt) {
    str file = read_file(s->path);

    resvg_render_tree* tree = nullptr;
    i32 err = resvg_parse_tree_from_data(file.start, slice_size(file), opt, &tree);
    if (err != RESVG_OK) mrw_abort("{}: resvg error {}", s->path, err);

    resvg_size size = resvg_get_image_size(tree);
    u32 w = (u32)ceilf(size.width);
    u32 h = (u32)ceilf(size.height);

    usize n_bytes = (usize)w * h * 4;
    u8* bytes = mrw_alloc_n(a, u8, n_bytes);
    buf_set(bytes, 0, n_bytes);
    resvg_render(tree, resvg_transform_identity(), w, h, (char*)bytes);
    resvg_tree_destroy(tree);

    i32 x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (i32 y = 0; y < h; y++) {
        for (i32 x = 0; x < w; x++) {
            if (bytes[((usize)y * w + x) * 4 + 3] == 0) continue;
            x0 = min(x0, x); x1 = max(x1, x);
            y0 = min(y0, y); y1 = max(y1, y);
        }
    }

    s->w  = x1 - x0 + 1;
    s->h  = y1 - y0 + 1;
    s->ox = size.width  * 0.5f;
    s->oy = size.height * 0.5f;
    s->px = mrw_alloc_n(a, u8, (usize)s->w * s->h * 4);
    for (i32 y = 0; y < s->h; y++)
    {
        buf_copy(s->px + (usize)y * s->w * 4, bytes + ((usize)(y0 + y) * w + x0) * 4, (usize)s->w * 4);
    }

    NSVGimage* img = nsvgParse(file.start, "px", 96.0f);
    if (!img) mrw_abort("{}: nanosvg parse failed", s->path);

    bool has_origin = false;
    for (NSVGshape* sh = img->shapes; sh; sh = sh->next) {
        if (sh->id[0] != '_') continue;
        Tag* t = &s->tags[s->n_tags];
        t->x = (sh->bounds[0] + sh->bounds[2]) * 0.5f - x0;
        t->y = (sh->bounds[1] + sh->bounds[3]) * 0.5f - y0;
        make_tag_name(sh->id, t->name, sizeof(t->name));
        if (c_str_cmp(t->name, "_origin") == 0)
        {
            s->ox = t->x;
            s->oy = t->y;
            continue;
        }
        s->n_tags++;
    }

    nsvgDelete(img);
}

static bool pack_page(stbrp_rect* r, i32 m, i32 w, i32 h, stbrp_node* nodes) {
    stbrp_context ctx;
    stbrp_init_target(&ctx, w, h, nodes, w);
    stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
    return stbrp_pack_rects(&ctx, r, m) == 1;
}

static i32 pack(SpriteSlice sprites) {
    u32 n = (u32)slice_count(sprites);
    stbrp_rect* r     = mrw_alloc_n(a, stbrp_rect, n);
    stbrp_node* nodes = mrw_alloc_n(a, stbrp_node, PAGE_SIZE);
    slice_for_each(sprites, s, Sprite) s->page = -1;

    i32 pages = 0;
    u32 left  = n;
    while (left > 0) {
        i32 m = 0;
        slice_for_each_i(sprites, i) {
            Sprite* s = &sprites.start[i];
            if (s->page >= 0) continue;
            r[m++] = (stbrp_rect){ .id = (i32)i, .w = s->w + PAD * 2, .h = s->h + PAD * 2 };
        }

        stbrp_context ctx;
        stbrp_init_target(&ctx, PAGE_SIZE, PAGE_SIZE, nodes, PAGE_SIZE);
        stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
        stbrp_pack_rects(&ctx, r, m);

        for (i32 i = 0; i < m; i++) {
            if (!r[i].was_packed) continue;
            Sprite* s = &sprites.start[r[i].id];
            s->page = pages;
            s->x = r[i].x + PAD;
            s->y = r[i].y + PAD;
            left--;
        }
        pages++;
    }
    return pages;
}

static void write_pages(SpriteSlice sprites, i32 pages, cstr dir) {
    usize size = (usize)PAGE_SIZE * PAGE_SIZE * 4;
    u8* buf = mrw_alloc_n(a, u8, size);
    char path[1024] = { 0 };
    for (i32 p = 0; p < pages; p++) {
        buf_set(buf, 0, size);
        slice_for_each(sprites, s, Sprite) {
            if (s->page != p) continue;
            for (i32 y = 0; y < s->h; y++)
                buf_copy(buf + ((usize)(s->y + y) * PAGE_SIZE + s->x) * 4,
                         s->px + (usize)y * s->w * 4,
                         (usize)s->w * 4);
        }
        print(path, sizeof path, "{}/atlas_{}.png", dir, p);
        if (!stbi_write_png(path, PAGE_SIZE, PAGE_SIZE, 4, buf, PAGE_SIZE * 4))
            mrw_abort("cannot write {}", path);
    }
}

static i32 cmp_name(const void* l, const void* r) {
    return strcmp(*(cstr*)l, *(cstr*)r);
}

static void write_header(SpriteSlice sprites, i32 pages, cstr dir) {
    u32 total = 0;
    slice_for_each(sprites, s, Sprite) total += s->n_tags;
    cstr* names = mrw_alloc_n(a, cstr, total);
    u32 n_names = 0;
    slice_for_each(sprites, s, Sprite) {
        for (u32 i = 0; i < s->n_tags; i++) {
            cstr name = s->tags[i].name;
            bool seen = false;
            for (u32 j = 0; j < n_names && !seen; j++) seen = strcmp(names[j], name) == 0;
            if (!seen) names[n_names++] = name;
        }
    }
    qsort(names, n_names, sizeof(*names), cmp_name);

    char path[1024] = { 0 };
    print(path, sizeof path, "{}/atlas.h", dir);
    mrw_debug("writing to file {}", path);
    FILE* f = fopen(path, "wb");
    if (!f) mrw_abort("cannot open {}", path);

    printfb(f, "#pragma once\n\n#include <marrow/marrow.h>\n\n");
    printfb(f, "#define ATLAS_PAGE_COUNT {}\n", pages);
    printfb(f, "#define ATLAS_SIZE {}\n", PAGE_SIZE);

    printfb(f, "typedef enum %{\n    SpriteTag_None,\n");
    for (u32 i = 0; i < n_names; i++) printfb(f, "    SpriteTag{},\n", names[i]);
    printfb(f, "    SpriteTag_Count\n} SpriteTag;\n\n");

    printfb(f, "typedef enum %{\n");
    slice_for_each(sprites, s, Sprite) printfb(f, "    SPRITE_{},\n", s->ident);
    printfb(f, "    SPRITE_COUNT\n} SpriteId;\n\n");

    printfb(f,
        "typedef struct %{ SpriteTag tag; f32 x, y; } SpriteTagInfo;\n"
        "typedef struct %{\n"
        "    i32 page;\n"
        "    i32 x, y, w, h;\n"
        "    i32 ox, oy;\n"
        "    f64 u0, v0, u1, v1;\n"
        "    u32 tags_i, n_tags;\n"
        "} SpriteInfo;\n\n");

    printfb(f, "static const SpriteTagInfo sprite_tags[] = %{\n");
    slice_for_each(sprites, s, Sprite)
        for (u32 i = 0; i < s->n_tags; i++)
            printfb(f, "    %{ SpriteTag{}, {}, {} },\n", s->tags[i].name, s->tags[i].x, s->tags[i].y);
    printfb(f, "};\n\n");

    printfb(f, "static const SpriteInfo sprites[SPRITE_COUNT] = %{\n");
    u32 tag_i = 0;
    slice_for_each(sprites, s, Sprite) {
        printfb(f, "    %{ {}, {}, {}, ", s->page, s->x, s->y);
        printfb(f, "{}, {}, {}, {}, ", s->w, s->h, s->ox, s->oy);
        printfb(f, "{}, {}, ", s->x / (f64)PAGE_SIZE, s->y / (f64)PAGE_SIZE);
        printfb(f, "{}, {}, ", (s->x + s->w) / (f64)PAGE_SIZE, (s->y + s->h) / (f64)PAGE_SIZE);
        printfb(f, "{}, {} },\n", tag_i, s->n_tags);
        tag_i += s->n_tags;
    }
    printfb(f, "};\n");

    fclose(f);
}

// atlasgen <png_dir> <gen_dir> <file.svg>...
i32 main(i32 argc, char** argv) {
    cstr png_dir = argv[1];
    cstr gen_dir = argv[2];
    u32  n       = (u32)(argc - 3);

    Sprite* mem = mrw_alloc_n(a, Sprite, n);
    buf_set(mem, 0, sizeof(Sprite) * n);
    SpriteSlice sprites = slice_to(mem, n);

    resvg_init_log();
    resvg_options* opt = resvg_options_create();
    // resvg_options_set_shape_rendering_mode(opt, RESVG_SHAPE_RENDERING_CRISP_EDGES);
    resvg_options_load_system_fonts(opt);

    for (u32 i = 0; i < n; i++) {
        Sprite* s = &mem[i];
        s->path = argv[3 + i];
        make_ident(s->path, s->ident, sizeof s->ident);
        load_sprite(s, opt);
        if (s->w + PAD * 2 > PAGE_SIZE || s->h + PAD * 2 > PAGE_SIZE)
            mrw_abort("{}: {}x{} does not fit a {} page", s->path, s->w, s->h, PAGE_SIZE);
    }

    resvg_options_destroy(opt);

    i32 pages = pack(sprites);
    write_pages(sprites, pages, png_dir);
    write_header(sprites, pages, gen_dir);
    return 0;
}
