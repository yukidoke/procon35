# pragma once
# include "SceneCommon.hpp"

namespace fs = FileSystem;

// ビジュアライザ
class Visualizer : public ISState
{
public:
	Visualizer();
	void initialize() override;
	void update() override;
	void draw() const override;
	State getNextState() const override;
	void resetState() override;

private:
	// シェーダ用変数
	struct SGrid	// シェーダに盤面を渡すための構造体
	{
		uint32 grid[4100];
	};
	struct SPattern	// シェーダに抜き型を渡すための構造体
	{
		uint32 patterns[2048];
	};
	struct SInfo	// シェーダに盤面縦横サイズを渡すための構造体
	{
		uint32 info[4];
	};
	enum class ManualState
	{
		none,
		selectPattern,
		setPositionWithMouse,
		setPositionWithCursor,
		setDirection,
		apply,
	};
	ConstantBuffer<SGrid> m_grid;
	ConstantBuffer<SGrid> m_answer;
	ConstantBuffer<SPattern> m_pattern;
	ConstantBuffer<SPattern> m_miniPat;
	ConstantBuffer<SInfo> m_info;
	const PixelShader m_ps = GLSL{ Resource(U"myresource/grid.frag"), {{U"PSConstants2D", 0}, {U"Grid", 1}, {U"Ans", 2}, {U"Pat", 3}} };
	const PixelShader m_pi = GLSL{ Resource(U"myresource/pat.frag"), {{U"PSConstants2D", 0}, {U"Pat", 4}, {U"Info", 5}} };

	// 盤面表示用変数
	bool m_isPlaying, m_isFileLoaded;
	int32 m_opsSize, m_height, m_width;
	double m_nowTurn, m_speed;
	constexpr static double m_maxSpeed = 200.0;
	Point m_sceneSize;
	TextureRegion m_rt;
	const Array<Texture> m_Emojies{ Texture(U"⬆️"_emoji), Texture(U"⬇️"_emoji), Texture(U"⬅️"_emoji), Texture(U"➡️"_emoji) };
	Array<SGrid> m_save;
	Array<SPattern> m_allPatterns;
	Array<int32> m_P, m_X, m_Y, m_S, m_W, m_H;

	// 2Dカメラ用変数
	Camera2D m_camera;
	Camera2DParameters m_cp;

	State m_currentState;
	Font m_infoDisplay{ FontMethod::SDF, 48, Typeface::Bold };
	constexpr static int32 m_visOP = 55;
	constexpr static int32 m_disp = 180;
	

	TextureRegion m_mp;
	ListBoxState m_patList;
	ManualState m_manualState;
	struct InsertData
	{
		int32 p;
		int32 x;
		int32 y;
		int32 s;
	};
	InsertData m_id;
	JSON m_out, m_inp;

	const FilePath m_tempPath = fs::PathAppend(fs::PathAppend(fs::TemporaryDirectoryPath(), U"procon-siv3d"), U"visualizer");

	bool loadIOJson(bool);
	void stringToBuffer(const std::vector<std::string>& str, uint32* buffer);
	void patternToBuffer(const std::vector<std::string>& pat, uint32* buffer);
	void BLoadJson();
	void BVisualizeOP();
	void updateBoardBuffer();
	void applyManualOperation();
	void bufferToString(const uint32*, std::vector<std::string>&);
	void makeInputFromCurrentBoard(FilePath);
};
