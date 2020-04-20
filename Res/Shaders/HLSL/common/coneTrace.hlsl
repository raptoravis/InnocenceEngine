static const float3 CONES[] =
{
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};

inline float4 ConeTrace(in Texture3D<float4> voxelTexture, in SamplerState SamplerTypePoint, in float3 P, in float3 N, in float3 coneDirection, in float coneAperture)
{
	float3 color = 0;

	float dist = 1.0;
	float3 startPos = P + N;

	while (dist < 512)
	{
		float diameter = max(1.0, 2 * coneAperture * dist);
		float mip = log2(diameter);

		float3 tc = startPos + coneDirection * dist;
		tc = tc - voxelizationPassCBuffer.volumeCenter.xyz;
		tc /= (voxelizationPassCBuffer.volumeExtend.xyz * 0.5);
		tc = tc * float3(0.5f, 0.5f, 0.5f) + 0.5f;

		if (mip >= 4)
			break;

		float4 sam = voxelTexture.SampleLevel(SamplerTypePoint, tc, mip);

		color += sam.rgb;

		dist += diameter * 16;
	}

	return float4(color, 1.0f);
}

inline float4 ConeTraceRadiance(in Texture3D<float4> voxelTexture, in SamplerState SamplerTypePoint, in float3 P, in float3 N)
{
	float4 radiance = 0;

	for (uint cone = 0; cone < 16; ++cone)
	{
		float3 coneDirection = normalize(CONES[cone] + N);

		coneDirection *= dot(coneDirection, N) < 0 ? -1 : 1;

		radiance += ConeTrace(voxelTexture, SamplerTypePoint, P, N, coneDirection, tan(PI * 0.5f * 0.33f));
	}

	radiance *= 0.0625;
	radiance.a = saturate(radiance.a);

	return max(0, radiance);
}