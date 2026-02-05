#ifndef COLOR_CONVERTER_HLSL
#define COLOR_CONVERTER_HLSL

float3 yuvToRgbBt601(float3 yuv)
{
    float y = yuv.x;
    float u = yuv.y - 0.5;
    float v = yuv.z - 0.5;
    
    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    
    return float3(r, g, b);
}

float3 yuvToRgbBt709(float3 yuv)
{
    float y = yuv.x;
    float u = yuv.y - 0.5;
    float v = yuv.z - 0.5;
    
    float r = y + 1.5748 * v;
    float g = y - 0.1873 * u - 0.4681 * v;
    float b = y + 1.8556 * u;
    
    return float3(r, g, b);
}

float3 yuvToRgbBt2020(float3 yuv)
{
    float y = yuv.x;
    float u = yuv.y - 0.5;
    float v = yuv.z - 0.5;
    
    float r = y + 1.4746 * v;
    float g = y - 0.1646 * u - 0.5714 * v;
    float b = y + 1.8814 * u;
    
    return float3(r, g, b);
}

float3 yuvToRgbBt601FullRange(float3 yuv)
{
    float y = yuv.x;
    float u = yuv.y - 0.5;
    float v = yuv.z - 0.5;
    
    float r = y + 1.140 * v;
    float g = y - 0.395 * u - 0.581 * v;
    float b = y + 2.032 * u;
    
    return float3(r, g, b);
}

float3 yuvToRgbBt709FullRange(float3 yuv)
{
    float y = yuv.x;
    float u = yuv.y - 0.5;
    float v = yuv.z - 0.5;
    
    float r = y + 1.280 * v;
    float g = y - 0.215 * u - 0.381 * v;
    float b = y + 2.128 * u;
    
    return float3(r, g, b);
}

float3 yCbCrToRgbBt601(float3 ycbcr)
{
    float y = (ycbcr.x - 0.0625) * 1.164;
    float cb = ycbcr.y - 0.5;
    float cr = ycbcr.z - 0.5;
    
    float r = y + 1.596 * cr;
    float g = y - 0.392 * cb - 0.813 * cr;
    float b = y + 2.017 * cb;
    
    return float3(r, g, b);
}

float3 yCbCrToRgbBt709(float3 ycbcr)
{
    float y = (ycbcr.x - 0.0625) * 1.164;
    float cb = ycbcr.y - 0.5;
    float cr = ycbcr.z - 0.5;
    
    float r = y + 1.793 * cr;
    float g = y - 0.213 * cb - 0.533 * cr;
    float b = y + 2.112 * cb;
    
    return float3(r, g, b);
}


#endif // COLOR_CONVERTER_HLSL