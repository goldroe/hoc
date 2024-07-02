cbuffer Constants : register(b0) {
    matrix xform;
};

struct Vertex_In {
    float2 p_l    : POSITION;
    float2 uv     : TEXCOORD;
    float4 color  : COLOR;
};

struct Vertex_Out {
    float4 p_h    : SV_POSITION;
    float2 uv     : TEXCOORD;
    float4 color  : COLOR;
};

Texture2D tex : register(t0);
SamplerState tex_sampler : register(s0);

Vertex_Out vs_main(Vertex_In input) {
    Vertex_Out output;
    output.p_h = mul(xform, float4(input.p_l, 0.0, 1.0));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 ps_main(Vertex_Out input) : SV_TARGET {
    float4 tex_color = tex.Sample(tex_sampler, input.uv);
    float4 color = input.color * tex_color;
    return color;
}
