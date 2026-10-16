struct ShaderData {
    resolution: vec2f,
    time: f32,
}

struct Sprite {
    uv0: vec2f,
    uv1: vec2f,
    size_px: vec2f,
    origin_px: vec2f
}

struct InstanceInput {
    @location(0) position: vec2f,
    @location(1) scale: f32,
    @location(2) rot: f32,
    @location(3) sprite: u32,
}

struct VertexOutput{
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@group(0) @binding(0) var<uniform> shader_data: ShaderData;

@group(1) @binding(0) var texture_sampler: sampler;
@group(1) @binding(1) var texture: texture_2d<f32>;
@group(1) @binding(2) var<storage, read> sprites: array<Sprite>;

@vertex
fn vs_main(@builtin(vertex_index) index : u32, i: InstanceInput) -> VertexOutput {
    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));
    let p = FULLSCREEN_QUAD_POSITIONS[index];
    let sprite = sprites[i.sprite];

    let sprite_pos = (sprite.origin_px + p.xy * sprite.size_px * 0.5) * i.scale;

    var position = i.position + rotate2d(sprite_pos, i.rot);
    position = position / resolution;
    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2<f32>(1.0f, 1.0f);
    var out: VertexOutput;
    out.position = vec4(position, 0.0, 1.0);
    out.uv = mix(sprite.uv0, sprite.uv1, vec2(p.x, p.y) * 0.5f + 0.5f);
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return textureSample(texture, texture_sampler, in.uv);
}
