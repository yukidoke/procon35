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
layout(std140) uniform Grid
{
	uvec4[1025] g_grid;
};
layout(std140) uniform Ans
{
	uvec4[1025] g_ans;
};
layout(std140) uniform Pat
{
	uvec4[512] g_pat;
};
// [C++]
//struct SGrid
//{
//	uint32 grid[4100];
//};
//struct SPattern
//{
//	uint32 patterns[2048];
//};

//
//	Functions
//
void main()
{
	// calc grid position from uv
	uvec2 pos =  uvec2(uint(floor(UV.x * g_grid[1024][0])), uint(floor(UV.y * g_grid[1024][1])));
	if (pos.x == g_grid[1024][0]) pos.x--;
	if (pos.y == g_grid[1024][1]) pos.y--;

	// set alpha
	float alp = float(1 - min((max(0, int(pos.x + 1) - int(g_grid[1024][0])) + max(0, int(pos.y + 1) - int(g_grid[1024][1]))), 1));

	// get cell value from g_grid
	uint target = g_grid[4 * pos.y + pos.x / 64][(pos.x % 64) / 16];
	target = (target << ((pos.x % 16) * 2));
	target = (target >> 30);

	// get cell value from ans
	uint ans_t = g_ans[4 * pos.y + pos.x / 64][(pos.x % 64) / 16];
	ans_t = (ans_t << ((pos.x % 16) * 2));
	ans_t = (ans_t >> 30);


	// cell color
	vec4[4] ans = vec4[](vec4(0.784, 0.863, 0.941, alp), vec4(0.588, 0.784, 0.863, alp), vec4(0.588, 0.667, 0.863, alp), vec4(0.392, 0.431, 0.863, alp));

	if (ans_t == target) {
		ans[target].r *= 0.4;
		ans[target].b *= 0.4;
	}
	
	FragColor = ans[target];
	
	// get cell value from pattern
	int x = int(pos.x) - int(g_grid[1024][2]);
	int y = int(pos.y) - int(g_grid[1024][3]);
	if (x < 0 || y < 0 || 256 <= x || 256 <= y) return;
	float x_s = UV.x * g_grid[1024][0] - pos.x;
	float y_s = UV.y * g_grid[1024][1] - pos.y;
	if (x_s > 0.2 && x_s < 0.8 && y_s > 0.2 && y_s < 0.8) return;

	uint ux = uint(x);
	uint uy = uint(y);
	uint pat_t = g_pat[2 * uy + ux / 128][(ux % 128) / 32];
	if ((pat_t & (1u << (31u - (ux % 32u)))) != 0) FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
