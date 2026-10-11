struct ShaderData {
    resolution: vec2f,
    time: f32,
}

@group(0) @binding(0) var<uniform> shader_data: ShaderData;

struct InstanceInput {
    @location(0) position: vec2f,
    @location(1) size: vec2f,
}

struct VertexOutput{
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f,
}

@vertex
fn vs_main(@builtin(vertex_index) index : u32, i: InstanceInput) -> VertexOutput {
    let resolution = vec2f(f32(shader_data.resolution.x), f32(shader_data.resolution.y));
    let p = FULLSCREEN_QUAD_POSITIONS[index];
    var position = i.position + p.xy * i.size;
    position = position / resolution;
    position = vec2f(position.x, 1.0f - position.y) * 2.0f - vec2<f32>(1.0f, 1.0f);
    var out: VertexOutput;
    out.position = vec4(position, 0.0, 1.0);
    out.uv = vec2(p.x, p.y) * 0.5f + 0.5f;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    return vec4f(in.uv, 1.0f, 1.0f);
}
