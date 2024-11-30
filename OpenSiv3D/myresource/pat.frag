# version 410

//
//	PSInput
//
layout(location = 0) in vec4 Color;
layout(location = 1) in vec2 UV;

//
//	PSOutput
//
layout(location = 0) out vec4 FragColor;

//
//	Constant Buffer
//
layout(std140) uniform PSConstants2D
{
	vec4 g_colorAdd;
	vec4 g_sdfParam;
	vec4 g_sdfOutlineColor;
	vec4 g_sdfShadowColor;
	vec4 g_internal;
};

// PS
layout(std140) uniform Pat
{
	uvec4[512] g_pat;
};
layout(std140) uniform Info
{
	uvec4 g_info;
};
// [C++]
//struct SPat
//{
//	uint32 patterns[2052];
//};
//struct SInfo
//{
//	uint32 info[4];
//};

//
//	Functions
//
void main()
{
	// calc grid position from uv
	uvec2 pos = uvec2(uint(floor(UV.x * g_info[0])), uint(floor(UV.y * g_info[1])));
	if (pos.x == g_info[0]) pos.x--;
	if (pos.y == g_info[1]) pos.y--;

	// get cell value from g_grid
	uint target = g_pat[2 * pos.y + pos.x / 128][(pos.x % 128) / 32];
	target = (target << (pos.x % 32));
	target = (target >> 31);
	float col = float(target);

	//if (UV.x * g_info[0] - float(pos.x) >= 0.8 || UV.y * g_info[1] - float(pos.y) >= 0.8) {
	//	FragColor = vec4(0.05581, 0.16132, 0.24644, 1.0);
	//	return;
	//}

	FragColor = vec4(0.6046, 0.80202, 0.13453, col);
}
