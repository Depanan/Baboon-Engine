#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct UBO
{
    packed_float3 color;
    float v;
};

struct main0_out
{
    float4 FragColor [[color(0)]];
};

fragment main0_out main0(constant UBO& _15 [[buffer(0)]])
{
    main0_out out = {};
    out.FragColor = float4(_15.color[0], _15.color[1], _15.color[2], float4(1.0).w);
    return out;
}

