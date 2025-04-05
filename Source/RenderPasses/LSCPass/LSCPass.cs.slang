cbuffer PerFrameCB
{
    uint2 gResolution;
}

RWTexture2D<float> gDepth;
RWTexture2D<float4> gNormW;
RWTexture2D<float4> gPosW;
RWTexture2D<float4> gDirect;
RWTexture2D<float4> gIndirect;
RWTexture2D<int> gShadow;

RWTexture2D<float4> gColor;

// TODO: MAKE THESE TO BE CONTROLLABLE VIA FALCOR
int kernelSize = 5;
float maxTChange = 0.5;


[numthreads(16, 16, 1)]
void lsc(uint3 dispatchThreadId: SV_DispatchThreadID)
{
    // TODO: CHECK PERFORMANCE
    if (any(dispatchThreadId.xy >= gResolution))
        return;

    const int2 pixelPos = dispatchThreadId.xy;

    const int2 min = int2(
        clamp(pixelPos.x - kernelSize, 0, gResolution.x),
        clamp(pixelPos.y - kernelSize, 0, gResolution.y)
    );
    const int2 max = int2(
        clamp(pixelPos.x + kernelSize, 0, gResolution.x),
        clamp(pixelPos.y + kernelSize, 0, gResolution.y)
    );

    float summed = 1;
    float4 indirect = gIndirect[pixelPos];
    float shadow = gShadow[pixelPos];

    for (int x = min.x; x < max.x; x++)
    {
        for (int y = min.y; y < max.y; y++)
        {
            uint2 compare = uint2(x, y);

            float score = similarityScore(pixelPos, compare);

            indirect += gIndirect[compare] * score;
            summed += score;
            shadow += gShadow[pixelPos];
        }
    }

    indirect /= summed;
    shadow /= summed;

    gColor[pixelPos] = gDirect[pixelPos] * indirect * shadow;
}

float similarityScore(int2 main, int2 compare)
{
    float angleSimilarity = (dot(gNormW[main], gNormW[compare]) + 1.0) / 2.0;
    int depthSimilarity = abs(gDepth[main] - gDepth[compare]) > maxTChange ? 0 : 1;
    float distanceSimilarity = 1.0 / distance(gPosW[main], gPosW[compare]);
    distanceSimilarity = clamp(distanceSimilarity, 0.0, 1.0);

    return angleSimilarity * depthSimilarity * distanceSimilarity;
}
