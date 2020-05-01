// shadertype=hlsl
#include "common/common.hlsl"
//#define AUTO_EXPOSURE

Texture2D basePassRT0 : register(t0);
Texture2D billboardPassRT0 : register(t1);
Texture2D debugPassRT0 : register(t2);
StructuredBuffer<float> in_luminanceAverage : register(t3);

SamplerState SampleTypePoint : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

static const float3x3 RGB2XYZ = float3x3(
	0.4124564, 0.3575761, 0.1804375,
	0.2126729, 0.7151522, 0.0721750,
	0.0193339, 0.1191920, 0.9503041
);

static const float3x3 XYZ2RGB = float3x3(
	3.2404542, -1.5371385, -0.4985314,
	-0.9692660, 1.8760108, 0.0415560,
	0.0556434, -0.2040259, 1.0572252
);

float3 rgb2xyz(float3 rgb)
{
	return mul(rgb, RGB2XYZ);
}

float3 xyz2rgb(float3 xyz)
{
	return mul(xyz, XYZ2RGB);
}

float3 xyz2xyY(float3 xyz)
{
	float Y = xyz.y;
	float x = xyz.x / (xyz.x + xyz.y + xyz.z);
	float y = xyz.y / (xyz.x + xyz.y + xyz.z);
	return float3(x, y, Y);
}

float3 xyY2xyz(float3 xyY)
{
	float Y = xyY.z;
	float x = Y * xyY.x / xyY.y;
	float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
	return float3(x, Y, z);
}

float3 rgb2xyY(float3 rgb)
{
	float3 xyz = rgb2xyz(rgb);
	return xyz2xyY(xyz);
}

float3 xyY2rgb(float3 xyY)
{
	float3 xyz = xyY2xyz(xyY);
	return xyz2rgb(xyz);
}

float computeEV100(float aperture, float shutterTime, float ISO)
{
	return log2(aperture * aperture / shutterTime * 100 / ISO);
}

float computeEV100FromAvgLuminance(float avgLuminance)
{
	return log2(avgLuminance * 100.0f / 12.5f);
}
float convertEV100ToExposure(float EV100)
{
	float maxLuminance = 1.2f * pow(2.0f, EV100);
	return 1.0f / maxLuminance;
}

//Academy Color Encoding System
//[http://www.oscars.org/science-technology/sci-tech-projects/aces]
float3 acesFilm(const float3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//gamma correction with respect to human eyes non-linearity
//[https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
float3 accurateLinearToSRGB(float3 linearCol)
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;

	return sRGB;
}

float4 main(PixelInputType input) : SV_TARGET
{
	float3 basePass = basePassRT0.Sample(SampleTypePoint, input.texcoord).xyz;
	float4 billboardPass = billboardPassRT0.Sample(SampleTypePoint, input.texcoord);
	float4 debugPass = debugPassRT0.Sample(SampleTypePoint, input.texcoord);

	// HDR to LDR
#ifdef AUTO_EXPOSURE
	float EV100 = computeEV100FromAvgLuminance(in_luminanceAverage[0]);
#else
	float EV100 = computeEV100(perFrameCBuffer.aperture, perFrameCBuffer.shutterTime, perFrameCBuffer.ISO);
#endif
	float exposure = convertEV100ToExposure(EV100);

	float3 bassPassxyY = rgb2xyY(basePass);
	bassPassxyY.z *= exposure;

	basePass = xyY2rgb(bassPassxyY);

	// Tone Mapping
	float3 finalColor = acesFilm(basePass);

	// Gamma Correction
	finalColor = accurateLinearToSRGB(finalColor);

	// billboard overlay
	finalColor += billboardPass.rgb;

	// debug overlay
	if (debugPass.a == 1.0)
	{
		finalColor = debugPass.rgb;
	}

	return float4(finalColor, 1.0f);
}