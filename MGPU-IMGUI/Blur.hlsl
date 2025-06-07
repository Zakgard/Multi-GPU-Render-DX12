Texture2D<float4> InputTexture : register(t0);
RWTexture2D<float4> OutputTexture : register(u0);

float f : register(v0);

// √ауссово €дро (7x7) Ч можно уменьшить до 5x5 или увеличить точность
static const float weights[7] =
{
    0.006f, 0.061f, 0.242f, 0.383f, 0.242f, 0.061f, 0.006f
};

[numthreads(16, 16, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    float4 color = float4(0, 0, 0, 0);
    for (int j = -3; j <= 3; j++)
    {
        for (int i = -3; i <= 3; i++)
        {
            float2 offset = float2(i, j);
            float weight = weights[i + 3] * weights[j + 3];
            color += InputTexture[DTid.xy + offset] * weight;
        }
    }
    OutputTexture[DTid.xy] = color;
}