#version 450

struct PerFrame_CB
{
	mat4 p_original; // 0 - 3
	mat4 p_jittered; // 4 - 7
	mat4 v; // 8 - 11
	mat4 v_prev; // 12 - 15
	mat4 p_inv; // 16 - 19
	mat4 v_inv; // 20 - 23
	float zNear; // Tight packing 24
	float zFar; // Tight packing 24
	float minLogLuminance; // Tight packing 24
	float maxLogLuminance; // Tight packing 24
	vec4 sun_direction; // 25
	vec4 sun_illuminance; // 26
	vec4 viewportSize; // 27
	vec4 posWSNormalizer; // 28
	vec4 camera_posWS; // 29
	vec4 padding[2]; // 30 - 31
};

layout(std140, row_major, set = 0, binding = 8) uniform GICBufferBlock
{
	mat4 p;
	mat4 r[6];
	mat4 t;
	mat4 p_inv;
	mat4 v_inv[6];
	vec4 probeCount;
	vec4 probeRange;
	vec4 workload;
	vec4 irradianceVolumeOffset;
} _53;

layout(std140, row_major, set = 0, binding = 0) uniform perFrameCBufferBlock
{
	PerFrame_CB data;
} _63;

layout(set = 1, binding = 1) uniform texture3D probeVolume;
layout(set = 2, binding = 0) uniform sampler SamplerTypeLinear;

layout(location = 0) in vec4 input_posWS;
layout(location = 1) in vec4 input_probeIndex;
layout(location = 2) in vec4 input_normal;
layout(location = 0) out vec4 _entryPointOutput_probeTestPassRT0;

void main()
{
	vec3 _333 = normalize(input_normal.xyz);
	vec3 _336 = _333 * _333;
	ivec3 _339 = mix(ivec3(0), ivec3(1), lessThan(_333, vec3(0.0)));
	vec3 _350 = (input_posWS.xyz - _53.irradianceVolumeOffset.xyz) / _63.data.posWSNormalizer.xyz;
	vec3 _498 = _350;
	_498.z = _350.z * 0.16666667163372039794921875;
	vec3 _508;
	if (_339.x != int(0u))
	{
		_508 = vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498 + vec3(0.0, 0.0, 0.16666667163372039794921875)) * _336.x).xyz);
	}
	else
	{
		_508 = vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498) * _336.x).xyz);
	}
	vec3 _509;
	if (_339.y != int(0u))
	{
		_509 = _508 + vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498 + vec3(0.0, 0.0, 0.5)) * _336.y).xyz);
	}
	else
	{
		_509 = _508 + vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498 + vec3(0.0, 0.0, 0.3333333432674407958984375)) * _336.y).xyz);
	}
	vec3 _510;
	if (_339.z != int(0u))
	{
		_510 = _509 + vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498 + vec3(0.0, 0.0, 0.833333313465118408203125)) * _336.z).xyz);
	}
	else
	{
		_510 = _509 + vec3((texture(sampler3D(probeVolume, SamplerTypeLinear), _498 + vec3(0.0, 0.0, 0.666666686534881591796875)) * _336.z).xyz);
	}
	_entryPointOutput_probeTestPassRT0 = vec4(_510, 1.0);
}
