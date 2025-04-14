cbuffer PerFrameCB
{
    uint2 gResolution;
}

RWTexture2D<float> gDepth;
RWTexture2D<float4> gNormW;
RWTexture2D<float4> gPosW;
RWTexture2D<float4> gDirect;
RWTexture2D<float4> gIndirect;
//RWTexture2D<int> gShadow;

RWTexture2D<float4> gColor;

// TODO: MAKE THESE TO BE CONTROLLABLE VIA FALCOR
static int kernelSize = 5;
static float maxTChange = 0.5;


[numthreads(16, 16, 1)]
void lowSampleCompensation(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    // TODO: CHECK PERFORMANCE
    if (any(dispatchThreadId.xy >= gResolution))
        return;
    
    const uint2 pixelPos = dispatchThreadId.xy;

    const uint2 min = uint2(
        clamp(pixelPos.x - kernelSize, 0, gResolution.x),
        clamp(pixelPos.y - kernelSize, 0, gResolution.y)
    );
    const uint2 max = uint2(
        clamp(pixelPos.x + kernelSize, 0, gResolution.x),
        clamp(pixelPos.y + kernelSize, 0, gResolution.y)
    );

    float summed = 1.0;
    float4 otherIndirects = float4(0);
    //float shadow = gShadow[pixelPos];

    float depth = gDepth[pixelPos];
    float4 normal = gNormW[pixelPos];
    float4 position = gPosW[pixelPos];
    float4 indirect = gIndirect[pixelPos];
    
    for (int x = min.x; x < max.x; x++)
    {
        for (int y = min.y; y < max.y; y++)
        {
            uint2 compare = uint2(x, y);
            if (compare.x != pixelPos.x && compare.y != pixelPos.y)
            {
                float score = similarityScore(pixelPos, compare, depth, normal, position);

                otherIndirects += lerp(indirect, getIndirect(compare), score);
                summed += 1;
                // shadow += gShadow[pixelPos];
            }
        }
    }

    otherIndirects /= summed;
    //shadow /= summed;

    gColor[pixelPos] = gDirect[pixelPos] + lerp(getIndirect(pixelPos), otherIndirects, 0.6); //* shadow;*/
}

float similarityScore(uint2 main, uint2 compare, float depth, float4 normal, float4 position)
{
    float angleSimilarity = (dot(normal, gNormW[compare]) + 1.0) / 2.0;
    int depthSimilarity = abs(depth - gDepth[compare]) > maxTChange ? 0 : 1;
    float distanceSimilarity = 1.0 / distance(position, gPosW[compare]);
    distanceSimilarity = clamp(distanceSimilarity, 0.0, 1.0);

    return angleSimilarity * depthSimilarity * distanceSimilarity;
}

float4 getIndirect(uint2 pos)
{
    return gIndirect[pos];
}

float4 getDirect(uint2 pos)
{
    return gDirect[pos];
}
