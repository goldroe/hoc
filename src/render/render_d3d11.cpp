#include "render_d3d11.h"
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")

global R_D3D11_State *r_d3d11_state;

internal bool d3d11_make_shader(String8 shader_name, String8 source, const char *vs_entry, const char *ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count, ID3D11VertexShader **vshader_out, ID3D11PixelShader **pshader_out, ID3D11InputLayout **input_layout_out) {
    printf("Compiling shader '%s'... ", shader_name.data);
    ID3DBlob *vertex_blob, *pixel_blob, *vertex_error_blob, *pixel_error_blob;
    UINT flags = 0;
    #if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    #endif
    bool compilation_success = true;
    if (D3DCompile(source.data, source.count, (LPCSTR)shader_name.data, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs_entry, "vs_5_0", flags, 0, &vertex_blob, &vertex_error_blob) != S_OK) {
        compilation_success = false;
    }
    if (D3DCompile(source.data, source.count, (LPCSTR)shader_name.data, NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps_entry, "ps_5_0", flags, 0, &pixel_blob, &pixel_error_blob) != S_OK) {
        compilation_success = false;
    }

    if (compilation_success) {
        printf("SUCCESS\n");
    } else {
        printf("FAILED\n");
        if (vertex_error_blob) printf("%s", (char *)vertex_error_blob->GetBufferPointer());
    }
    if (compilation_success) {
        r_d3d11_state->device->CreateVertexShader(vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), NULL, vshader_out);
        r_d3d11_state->device->CreatePixelShader(pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize(), NULL, pshader_out);
        HRESULT hr = r_d3d11_state->device->CreateInputLayout(items, item_count, vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), input_layout_out);
    }
    if (vertex_error_blob) vertex_error_blob->Release();
    if (pixel_error_blob)  pixel_error_blob->Release();
    if (vertex_blob) vertex_blob->Release();
    if (pixel_blob) pixel_blob->Release();

    return compilation_success;
}

internal bool d3d11_make_shader_from_file(String8 file_name, const char *vs_entry, const char *ps_entry, D3D11_INPUT_ELEMENT_DESC *items, int item_count, ID3D11VertexShader **vshader_out, ID3D11PixelShader **pshader_out, ID3D11InputLayout **input_layout_out) {
    OS_Handle file_handle = os_open_file(file_name, OS_ACCESS_READ);
    String8 source = os_read_file_string(file_handle);
    os_close_handle(file_handle);
    return d3d11_make_shader(file_name, source, vs_entry, ps_entry, items, item_count, vshader_out, pshader_out, input_layout_out);
}

internal ID3D11Buffer *d3d11_make_uniform_buffer(u32 size) {
    ID3D11Buffer *constant_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (r_d3d11_state->device->CreateBuffer(&desc, nullptr, &constant_buffer) != S_OK) {
        printf("Failed to create constant buffer\n");
    }
    return constant_buffer;
}

internal void d3d11_upload_uniform(ID3D11Buffer *constant_buffer, void *constants, u32 size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (r_d3d11_state->device_context->Map(constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) == S_OK) {
        memcpy(res.pData, constants, size);
        r_d3d11_state->device_context->Unmap(constant_buffer, 0);
    } else {
        printf("Failed to map constant buffer\n");
    }
}

internal ID3D11Buffer *d3d11_make_vertex_buffer(void *data, u64 buffer_size) {
    ID3D11Buffer *vertex_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = (UINT)buffer_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    HRESULT hr = r_d3d11_state->device->CreateBuffer(&desc, &resource, &vertex_buffer);
    if (hr != S_OK) {
        printf("Failed to create vertex buffer\n");
    }
    return vertex_buffer;
}

internal ID3D11Buffer *d3d11_make_index_buffer(void *data, u32 buffer_size) {
    ID3D11Buffer *index_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = buffer_size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    resource.SysMemPitch = 0;
    resource.SysMemSlicePitch = 0;
    HRESULT hr = r_d3d11_state->device->CreateBuffer(&desc, &resource, &index_buffer);
    if (hr != S_OK) {
        printf("Failed to create index buffer\n");
    }
    return index_buffer;
}

internal ID3D11Buffer *d3d11_make_vertex_buffer_writable(void *data, u32 buffer_size) {
    ID3D11Buffer *vertex_buffer = nullptr;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = buffer_size;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_SUBRESOURCE_DATA resource{};
    resource.pSysMem = data;
    if (r_d3d11_state->device->CreateBuffer(&desc, &resource, &vertex_buffer) != S_OK) {
        printf("Failed to create vertex buffer\n");
    }
    return vertex_buffer;
}

internal void d3d11_write_vertex_buffer(ID3D11Buffer *vertex_buffer, void *data, u32 size) {
    D3D11_MAPPED_SUBRESOURCE res{};
    if (r_d3d11_state->device_context->Map(vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res) == S_OK) {
        memcpy(res.pData, data, size);
        r_d3d11_state->device_context->Unmap(vertex_buffer, 0);
    } else {
        printf("Failed to map vertex buffer\n");
    }
}

internal void *d3d11_create_texture(u8 *image_data, int x, int y, int pitch, DXGI_FORMAT format) {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = x;
    desc.Height = y;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = image_data;
    data.SysMemPitch = pitch;
    data.SysMemSlicePitch = 0;
    ID3D11Texture2D *texture_2d = nullptr;
    ID3D11ShaderResourceView *srv = nullptr;
    HRESULT hr = r_d3d11_state->device->CreateTexture2D(&desc, &data, &texture_2d);
    if (SUCCEEDED(hr)) {
        hr = r_d3d11_state->device->CreateShaderResourceView(texture_2d, NULL, &srv);
        if (FAILED(hr)) {
            printf("Failed to create shader resouce view\n");
            return nullptr;
        }
    } else {
        printf("Failed to create texture 2d\n");
        return nullptr;
    }
    return srv;
}

internal void *d3d11_create_texture_rgba(u8 *image_data, int x, int y, int pitch) {
    void *srv = d3d11_create_texture(image_data, x, y, pitch, DXGI_FORMAT_R8G8B8A8_UNORM);
    return srv;
}

internal void *d3d11_create_texture_r8(u8 *image_data, int x, int y, int pitch) {
    void *srv = d3d11_create_texture(image_data, x, y, pitch, DXGI_FORMAT_R8_UNORM);
    return srv;
}

internal D3D11_RECT rect_to_d3d11_rect(Rect rect) {
    D3D11_RECT result;
    result.left = (int)rect.x0;
    result.right = (int)rect.x1;
    result.top = (int)rect.y0;
    result.bottom = (int)rect.y1;
    return result;
}


internal void d3d11_resize_render_target_view(UINT width, UINT height) {
    HRESULT hr = S_OK;

    // NOTE: Resize render target view
    r_d3d11_state->device_context->OMSetRenderTargets(0, 0, 0);

    // Release all outstanding references to the swap chain's buffers.
    if (r_d3d11_state->render_target_view) r_d3d11_state->render_target_view->Release();

    // Preserve the existing buffer count and format.
    // Automatically choose the width and height to match the client rect for HWNDs.
    hr = r_d3d11_state->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

    // Get buffer and create a render-target-view.
    ID3D11Texture2D *backbuffer;
    hr = r_d3d11_state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);

    hr = r_d3d11_state->device->CreateRenderTargetView(backbuffer, NULL, &r_d3d11_state->render_target_view);

    backbuffer->Release();

    if (r_d3d11_state->depth_stencil_view) r_d3d11_state->depth_stencil_view->Release();

    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        ID3D11Texture2D *depth_stencil_buffer = nullptr;
        hr = r_d3d11_state->device->CreateTexture2D(&desc, NULL, &depth_stencil_buffer);
        hr = r_d3d11_state->device->CreateDepthStencilView(depth_stencil_buffer, NULL, &r_d3d11_state->depth_stencil_view);
    }

    r_d3d11_state->device_context->OMSetRenderTargets(1, &r_d3d11_state->render_target_view, r_d3d11_state->depth_stencil_view);
}

internal void d3d11_render(OS_Handle window_handle, Draw_Bucket *draw_bucket) {
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = r_d3d11_state->draw_region.x0;
    viewport.TopLeftY = r_d3d11_state->draw_region.y0;
    viewport.Width = rect_width(r_d3d11_state->draw_region);
    viewport.Height = rect_height(r_d3d11_state->draw_region);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    r_d3d11_state->device_context->RSSetState(r_d3d11_state->rasterizer_state);
    r_d3d11_state->device_context->RSSetViewports(1, &viewport);

    float clear_color[4] = {0, 0, 0, 1};
    r_d3d11_state->device_context->ClearRenderTargetView(r_d3d11_state->render_target_view, clear_color);
    r_d3d11_state->device_context->ClearDepthStencilView(r_d3d11_state->depth_stencil_view, D3D11_CLEAR_DEPTH, 0.0f, 0);

    r_d3d11_state->device_context->OMSetDepthStencilState(r_d3d11_state->depth_stencil_state, 0);
    r_d3d11_state->device_context->OMSetRenderTargets(1, &r_d3d11_state->render_target_view, r_d3d11_state->depth_stencil_view);

    r_d3d11_state->device_context->OMSetBlendState(r_d3d11_state->blend_state, NULL, 0xffffffff);
    r_d3d11_state->device_context->PSSetSamplers(0, 1, &r_d3d11_state->sampler);

    for (R_Batch_Node *batch_node = draw_bucket->batches.first; batch_node; batch_node = batch_node->next) {
        R_Batch batch = batch_node->batch;
        R_Params params = batch.params;
        switch (params.type) {
        default:
            Assert(0);
            break;
            
        case R_PARAMS_UI:
        {
            int vertices_count = batch.bytes / sizeof(R_2D_Vertex);
            R_2D_Vertex *vertices = (R_2D_Vertex *)batch.v;
            if (vertices_count == 0) return;

            void *tex = params.ui.tex;
            if (tex == nullptr) {
                tex = r_d3d11_state->fallback_tex;
            }
            
            // Rect clip = params.ui.clip;
            // D3D11_RECT d3d_clip = rect_to_d3d11_rect(clip);
            // r_d3d11_state->device_context->RSSetScissorRects(1, &d3d_clip);

            r_d3d11_state->device_context->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView **)&tex);

            ID3D11VertexShader *vertex_shader = r_d3d11_state->vertex_shaders[D3D11_SHADER_UI];
            ID3D11PixelShader *pixel_shader = r_d3d11_state->pixel_shaders[D3D11_SHADER_UI];
            r_d3d11_state->device_context->VSSetShader(vertex_shader, nullptr, 0);
            r_d3d11_state->device_context->PSSetShader(pixel_shader, nullptr, 0);

            Rect draw_region = r_d3d11_state->draw_region;
            D3D11_Uniform_UI ui_uniform = {};
            ui_uniform.xform = ortho_rh_zo(draw_region.x0, draw_region.x1, draw_region.y1, draw_region.y0, -1.f, 1.f);
            ID3D11Buffer *uniform_buffer = r_d3d11_state->uniform_buffers[D3D11_SHADER_UI];
            d3d11_upload_uniform(uniform_buffer, (void *)&ui_uniform, sizeof(ui_uniform));
            r_d3d11_state->device_context->VSSetConstantBuffers(0, 1, &uniform_buffer);

            Auto_Array<u32> indices;
            indices.reserve(vertices_count * 6 / 4);
            for (int index = 0; index < vertices_count; index += 4) {
                indices.push(index);
                indices.push(index + 1);
                indices.push(index + 2);
                indices.push(index);
                indices.push(index + 2);
                indices.push(index + 3);
            }
            ID3D11Buffer *vertex_buffer = d3d11_make_vertex_buffer(vertices, sizeof(R_2D_Vertex) * vertices_count);
            ID3D11Buffer *index_buffer = d3d11_make_index_buffer(indices.data, sizeof(u32) * (UINT)indices.count);

            // IA
            u32 stride = sizeof(R_2D_Vertex), offset = 0;
            r_d3d11_state->device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            r_d3d11_state->device_context->IASetInputLayout(r_d3d11_state->input_layouts[D3D11_SHADER_UI]);
            r_d3d11_state->device_context->IASetVertexBuffers(0, 1, &vertex_buffer, &stride, &offset);
            r_d3d11_state->device_context->IASetIndexBuffer(index_buffer, DXGI_FORMAT_R32_UINT, 0);
            
            r_d3d11_state->device_context->DrawIndexed((UINT)indices.count, 0, 0);

            indices.clear();
            if (vertex_buffer) vertex_buffer->Release();
            if (index_buffer) index_buffer->Release();
            break;
        }
        }
    }

    r_d3d11_state->swap_chain->Present(1, 0);
}

internal void d3d11_render_initialize(HWND window_handle) {
    r_d3d11_state = new R_D3D11_State();

    HRESULT hr = S_OK;
    DXGI_MODE_DESC buffer_desc{};
    buffer_desc.Width = 0;
    buffer_desc.Height = 0;
    buffer_desc.RefreshRate.Numerator = 0;
    buffer_desc.RefreshRate.Denominator = 1;
    buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    buffer_desc.Scaling = DXGI_MODE_SCALING_STRETCHED;
    DXGI_SWAP_CHAIN_DESC swapchain_desc{};
    swapchain_desc.BufferDesc = buffer_desc;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.OutputWindow = window_handle;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_DEBUG, NULL, NULL, D3D11_SDK_VERSION, &swapchain_desc, &r_d3d11_state->swap_chain, &r_d3d11_state->device, NULL, &r_d3d11_state->device_context);
    Assert(SUCCEEDED(hr));

    ID3D11Texture2D *backbuffer;
    hr = r_d3d11_state->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backbuffer);
    hr = r_d3d11_state->device->CreateRenderTargetView(backbuffer, NULL, &r_d3d11_state->render_target_view);
    backbuffer->Release();

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace = desc.FrontFace;
        r_d3d11_state->device->CreateDepthStencilState(&desc, &r_d3d11_state->depth_stencil_state);
    }

    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = false;
        desc.DepthClipEnable = false;
        desc.FrontCounterClockwise = true;
        r_d3d11_state->device->CreateRasterizerState(&desc, &r_d3d11_state->rasterizer_state);
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        r_d3d11_state->device->CreateSamplerState(&desc, &r_d3d11_state->sampler);
    }

    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        // desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        // desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRESULT hr = r_d3d11_state->device->CreateBlendState(&desc, &r_d3d11_state->blend_state);
        if (FAILED(hr)) {
            printf("Error CreateBlendState\n");
        }
    }

    D3D11_INPUT_ELEMENT_DESC ui_input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(R_2D_Vertex, dst),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(R_2D_Vertex, src),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(R_2D_Vertex, color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    d3d11_make_shader_from_file(str8_lit("data/shaders/ui.hlsl"), "vs_main", "ps_main", ui_input_layout, ARRAYSIZE(ui_input_layout), &r_d3d11_state->vertex_shaders[D3D11_SHADER_UI], &r_d3d11_state->pixel_shaders[D3D11_SHADER_UI], &r_d3d11_state->input_layouts[D3D11_SHADER_UI]);

    r_d3d11_state->uniform_buffers[D3D11_SHADER_UI] = d3d11_make_uniform_buffer(sizeof(D3D11_Uniform_UI));

    {
        u8 bitmap[16] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };
        r_d3d11_state->fallback_tex = d3d11_create_texture_rgba(bitmap, 2, 2, 8);
    }
}
