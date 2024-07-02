#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#include <d3d11.h>
#include <d3dcompiler.h>

enum D3D11_Shader_Type {
    D3D11_SHADER_UI,
    D3D11_SHADER_COUNT,
};

struct D3D11_Uniform_UI {
    m4 xform;
};

// struct R_Batch {
//     u8 *v;
//     int bytes;
// };

// struct R_Batch_Node {
//     R_Batch *next;
//     R_Batch batch;
// };

// struct R_Batch_List {
//     R_Batch_Node *first;
//     R_Batch_Node *last;
//     int count;
// };

struct R_2D_Vertex {
    v2 p;
    v2 uv;
    v4 color;
};

struct R_D3D11_State {
    Arena *arena;
    IDXGISwapChain *swap_chain;
    ID3D11Device *device;
    ID3D11DeviceContext *device_context;

    //@Note Resources
    ID3D11RenderTargetView *render_target_view;
    ID3D11DepthStencilState *depth_stencil_state;
    ID3D11DepthStencilView *depth_stencil_view;
    ID3D11RasterizerState *rasterizer_state;
    ID3D11BlendState *blend_state;
    ID3D11SamplerState *sampler;
    ID3D11VertexShader *vertex_shaders[D3D11_SHADER_COUNT];
    ID3D11PixelShader *pixel_shaders[D3D11_SHADER_COUNT];
    ID3D11InputLayout *input_layouts[D3D11_SHADER_COUNT];
    ID3D11Buffer *uniform_buffers[D3D11_SHADER_COUNT];

    void *current_texture;
    Auto_Array<R_2D_Vertex> ui_vertices;
    //@Note Immediate context
    Rect draw_region;
    void *fallback_texture;
};

#endif //RENDER_D3D11_H
