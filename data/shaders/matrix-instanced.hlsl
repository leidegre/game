cbuffer MyBuffer : register(b0)
{
    float4x4 viewProj;
    float4x4 model[511];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR, uint instanceID : SV_InstanceID)
{
    PSInput result;

    // float4 tmp = mul(model[instanceID], position);
    float4 pos = mul(viewProj, position);

    result.position = pos;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}