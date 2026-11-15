struct ShaderData {
    resolution: vec2f,
    time: f32,
}

struct Sprite {
    uv0: vec2f,
    uv1: vec2f,
    size_px: vec2f,
    origin_px: vec2f,
    page: u32,
}

struct InstanceInput {
    @location(0) position: vec2f,
    @location(1) scale: vec2f,
    @location(2) rot: f32,
    @location(3) sprite: u32,
    @location(4) tint: vec4f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
    @location(1) @interpolate(flat) page: u32,
    @location(2) tint: vec3f,
}

@group(0) @binding(0) var<uniform> shader_data: ShaderData;

@group(1) @binding(0) var texture_sampler: sampler;
@group(1) @binding(1) var texture: texture_2d_array<f32>;
@group(1) @binding(2) var<storage, read> sprites: array<Sprite>;

@vertex
fn vs_main(@builtin(vertex_index) index: u32, i: InstanceInput) -> VertexOutput {
    let p = CENTERED_QUAD_POSITIONS[index];
    let sprite = sprites[i.sprite];

    let sprite_pos = (sprite.origin_px + p.xy * sprite.size_px) * i.scale;
    var position = i.position + rotate2d(sprite_pos, i.rot);
    position = (position / shader_data.resolution) * 2.0;

    var out: VertexOutput;
    out.position = vec4f(position, 0.0, 1.0);
    out.uv = mix(sprite.uv0, sprite.uv1, p.xy + 0.5);
    out.page = sprite.page;
    return out;
}

fn sdf_alpha(d: f32, uv: vec2f) -> f32 {
    let unit_range = vec2f(16.0f) / vec2f(textureDimensions(texture));
    let screen_tex_size = vec2f(1.0) / fwidth(uv);
    let screen_px_range = max(0.5 * dot(unit_range, screen_tex_size), 1.0);
    return clamp(screen_px_range * (d - 0.5) + 0.5, 0.0, 1.0);
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let tex = textureSample(texture, texture_sampler, in.uv, in.page);
    let text = vec4f(1.0f, 1.0f, 1.0f, sdf_alpha(tex.a, in.uv));
    return select(tex, text, in.page == 2) * in.tint;
}
