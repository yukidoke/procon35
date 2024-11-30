# pragma once
# include "Visualizer.hpp"
# include "board.hpp"

Visualizer::Visualizer()
{
	if (not m_ps)
	{
		throw Error{ U"Failed to load grid.frag" };
	}
	if (not m_pi)
	{
		throw Error{ U"Failed to load pat.frag" };
	}
	this->initialize();
}

void Visualizer::initialize()
{
	fs::CreateDirectories(fs::ParentPath(m_tempPath));
	fs::CreateDirectories(m_tempPath);
	fs::RemoveContents(m_tempPath);
	fs::CreateDirectories(fs::PathAppend(fs::ParentPath(m_tempPath), U"solvers"));

	// シェーダ用変数
	m_grid = ConstantBuffer<SGrid>{ SGrid{{0}} };
	m_answer = ConstantBuffer<SGrid>{ SGrid{{0}} };
	m_pattern = ConstantBuffer<SPattern>{ SPattern{{0}} };
	m_miniPat = ConstantBuffer<SPattern>{ SPattern{{0}} };
	m_info = ConstantBuffer<SInfo>{ SInfo{{0}} };

	// 盤面表示用変数
	m_isPlaying = false, m_isFileLoaded = false;
	m_opsSize = 0, m_height = 0, m_width = 0;
	m_nowTurn = 0.0, m_speed = 54.0;
	m_sceneSize = Point{ 0, 0 };
	m_rt = RenderTexture{ Size{ 1024, 1024 }, ColorF{ 0.0 } }.resized(m_width, m_height);
	m_save.clear();
	m_allPatterns.clear();
	m_P.clear(), m_X.clear(), m_Y.clear(), m_S.clear();

	// 2Dカメラ用変数
	m_camera = Camera2D{ Scene::CenterF(), 1.0, CameraControl::Keyboard };
	m_cp = m_camera.getParameters();
	m_cp.scaleSmoothTime = 0.001;
	m_cp.positionSmoothTime = 0.001;
	m_cp.zoomOut = [] { return KeySpace.pressed() || KeyLControl.pressed() || KeyLeftCommand.pressed(); };
	m_cp.zoomIn = [] { return KeyLShift.pressed(); };
	m_camera.setParameters(m_cp);

	// 手動操作用の変数
	m_manualState = ManualState::none;
	m_id = { 0, 0, 0, 0 };
	m_mp = m_rt.resized(300, 300);
	m_patList = ListBoxState{};

	resetState();
}

void Visualizer::stringToBuffer(const std::vector<std::string>& str, uint32* buffer)
{
	for (int32 i = 0; i < m_height; i++)
	{
		int32 tmp = 0;
		for (int32 j = 0; j < m_width / 16; j++)
		{
			tmp = 0;
			for (int32 k = 0; k < 16; k++)
			{
				tmp = tmp << 2;
				tmp |= str[i][j * 16 + k] - '0';
			}
			buffer[i * 16 + j] = tmp;
		}
		tmp = 0;
		for (int32 j = 0; j < m_width % 16; j++)
		{
			tmp = tmp << 2;
			tmp |= str[i][(m_width / 16) * 16 + j] - '0';
		}
		buffer[i * 16 + m_width / 16] = (tmp << ((16 - (m_width % 16)) * 2));
	}
}

void Visualizer::patternToBuffer(const std::vector<std::string>& pat, uint32* buffer)
{
	for (int32 i = 0; i < pat.size(); i++)
	{
		for (int32 j = 0; j < pat.front().size(); j++)
		{
			if (pat[i][j] == '1')
			{
				buffer[i * 8 + j / 32] |= (1 << (31 - (j % 32)));
			}
		}
	}
}

bool Visualizer::loadIOJson(bool isLoadJsonWithDialog = true)
{
	if (isLoadJsonWithDialog)
	{
		Optional<FilePath> input = Dialog::OpenFile({ FileFilter::JSON() }, fs::ParentPath(m_tempPath), U"input.json を選択してください");
		Optional<FilePath> output = Dialog::OpenFile({ FileFilter::JSON() }, fs::ParentPath(m_tempPath), U"output.json を選択してください(オプション)");
		if (not input)
		{
			Print << U"Failed to load json path";
			return false;
		}
		if (not output)
		{
			m_out = JSON::Parse( U"{\"n\":0,\"ops\":[]}" );
		}
		else
		{
			m_out = JSON::Load(*output);
		}
		m_inp = JSON::Load(*input);
	}
	if (not m_inp || not m_out)
	{
		Print << U"Failed to load json";
		return false;
	}

	// init grid
	Board board(m_inp, m_out);
	m_width = board.width();
	m_height = board.height();
	m_grid->grid[4096] = m_width;
	m_grid->grid[4097] = m_height;

	// init operation
	stringToBuffer(board.goal, m_answer->grid);
	m_opsSize = board.ops_size();
	m_save = Array<SGrid>(m_opsSize + 1);
	m_P = Array<int32>(m_opsSize + 1);
	m_X = Array<int32>(m_opsSize + 1);
	m_Y = Array<int32>(m_opsSize + 1);
	m_S = Array<int32>(m_opsSize + 1);
	stringToBuffer(board.start, m_save[0].grid);
	for (int32 i = 0; i < m_opsSize; i++)
	{
		board.update_board();
		stringToBuffer(board.board, m_save[i + 1].grid);
		m_P[i] = board.p(i);
		m_X[i] = board.x(i);
		m_Y[i] = board.y(i);
		m_S[i] = board.s(i);
	}

	// init pattern
	int32 n = board.get_n();
	m_allPatterns = Array<SPattern>(n + 1);
	m_W = Array<int32>(n);
	m_H = Array<int32>(n);
	for (int32 i = 0; i < n; i++)
	{
		m_W.at(i) = board.widths[i];
		m_H.at(i) = board.heights[i];
		m_patList.items.emplace_back(U"{:0>3}: {:0>3}×{:0>3}"_fmt(i, m_H.at(i), m_W.at(i)));
	}
	for (int32 i = 0; i < n; i++)
	{
		patternToBuffer(board.patterns[i], m_allPatterns[i].patterns);
	}
	m_P[m_opsSize] = n;
	m_X[m_opsSize] = 0;
	m_Y[m_opsSize] = 0;
	m_S[m_opsSize] = 0;
	m_allPatterns[n] = SPattern{ {0} };

	return true;
}

void Visualizer::BLoadJson()
{
	if (SimpleGUI::Button(U"LoadJson", Vec2{ m_sceneSize.y + 10, 10 }, 100)) // load json
	{
		m_isFileLoaded = loadIOJson();
		if (m_isFileLoaded)	// ウィンドウサイズ修正・コンスタントバッファ読み込み
		{
			m_rt = m_rt.resized(m_width, m_height);
			m_rt = m_rt.fitted(m_sceneSize.y * 0.99, m_sceneSize.y * 0.99);
			m_camera = Camera2D{ Scene::Center(), 1.0, CameraControl::Default };
			m_camera.setParameters(m_cp);
			m_nowTurn = 0.0;
			m_isPlaying = false;
			Graphics2D::SetPSConstantBuffer(2, m_answer);
		}
		else if (m_width && m_height) m_isFileLoaded = true;
	}
}

void Visualizer::BVisualizeOP()
{
	auto tmpx = m_sceneSize.y + 10, tmpy = m_visOP;
	m_infoDisplay(U"Turn: {:0>5}"_fmt(int32(m_nowTurn))).draw(23, tmpx, tmpy);
	tmpy += 40;
	if (SimpleGUI::Button(U"\U000F099B", Vec2{ tmpx, tmpy }, 50, m_manualState == ManualState::none))
	{
		m_nowTurn = 0.0;	// reset
	}
	tmpx += 50;
	if (SimpleGUI::Button(m_isPlaying ? U"\U000F03E4 Stop" : U"\U000F040A Start", Vec2{ tmpx, tmpy }, 100, m_manualState == ManualState::none))	// start/stop
	{
		m_isPlaying = not m_isPlaying;
	}
	tmpx += 100;
	if (SimpleGUI::Button(U"\U000F0A02", Vec2{ tmpx, tmpy }, 50, not m_isPlaying && m_manualState == ManualState::none))
	{
		m_nowTurn = Max(m_nowTurn - 1.0, 0.0);	// left
	}
	tmpx += 50;
	if (SimpleGUI::Button(U"\U000F0A03", Vec2{ tmpx, tmpy }, 50, not m_isPlaying && m_manualState == ManualState::none))
	{
		m_nowTurn = Min(m_nowTurn + 1.0, (double)m_opsSize); // right
	}
	tmpx += 50;
	SimpleGUI::Slider(U"\U000F0152", m_speed, 1.0, m_maxSpeed, Vec2{ tmpx, tmpy }, 40, m_sceneSize.x - m_sceneSize.y - 305, m_manualState == ManualState::none);	// speed
	tmpx = m_sceneSize.y + 10, tmpy += 40;
	SimpleGUI::Slider(U"", m_nowTurn, 0.0, m_opsSize, Vec2{ tmpx, tmpy }, 0, Max(0, m_sceneSize.x - m_sceneSize.y - 15), m_manualState == ManualState::none);	// turn
}

void Visualizer::updateBoardBuffer()
{
	const auto t = int32(m_nowTurn);
	if (m_isFileLoaded)	// 描画用バッファ更新
	{
		for (int32 i = 0; i < 4096; i++)
		{
			m_grid->grid[i] = m_save[t].grid[i];
		}
		for (int32 i = 0; i < 2048; i++)
		{
			m_pattern->patterns[i] = m_allPatterns[m_P[t]].patterns[i];
		}
		m_grid->grid[4098] = m_X[t];
		m_grid->grid[4099] = m_Y[t];
		Graphics2D::SetPSConstantBuffer(1, m_grid);
		Graphics2D::SetPSConstantBuffer(3, m_pattern);
	}
}

void Visualizer::applyManualOperation()
{
	const int32 t = int32(m_nowTurn);

	// update output.json
	while (m_out[U"ops"].size() > t + 1)
	{
		m_out[U"ops"].erase(m_out[U"ops"].size() - 1);
	}
	m_out[U"ops"][t][U"p"] = m_id.p;
	m_out[U"ops"][t][U"x"] = m_id.x;
	m_out[U"ops"][t][U"y"] = m_id.y;
	m_out[U"ops"][t][U"s"] = m_id.s;
	m_out[U"n"] = t + 1;
	loadIOJson(false);
}

void Visualizer::bufferToString(const uint32* buffer, std::vector<std::string>& str)
{
	str = std::vector<std::string>(m_height, std::string(m_width, ' '));
	for (int32 i = 0; i < m_height; i++)
	{
		for (int32 j = 0; j < m_width / 16; j++)
		{
			uint32 target = buffer[i * 16 + j];
			for (int32 k = 15; k >= 0; k--)
			{
				str.at(i).at(j * 16 + k) = '0' + (target & 3U);
				target = target >> 2;
			}
		}
		if (m_width % 16 == 0)
		{
			continue;
		}
		uint32 target = buffer[i * 16 + m_width / 16];
		target = target >> (2 * (16 - m_width % 16));
		for (int32 j = m_width % 16 - 1; j >= 0; j--)
		{
			str.at(i).at((m_width / 16) * 16 + j) = '0' + (target & 3U);
			target = target >> 2;
		}
	}
}

void Visualizer::makeInputFromCurrentBoard(FilePath inputFilePath)
{
	std::vector<std::string> tmp;
	bufferToString(m_save.back().grid, tmp);
	JSON insertedJson = m_inp;
	for (int32 i = 0; i < m_height; i++)
	{
		insertedJson[U"board"][U"start"][i] = Unicode::Widen(tmp.at(i));
	}
	insertedJson.save(fs::PathAppend(inputFilePath, U"input.json"));
}

void Visualizer::update()
{
	ClearPrint();
	// 盤面表示
	{
		m_camera.update();
		const Transformer2D transformer = m_camera.createTransformer();
		const ScopedCustomShader2D shader{ m_ps };

		if (m_sceneSize != Scene::Size())	// ウィンドウサイズ修正
		{
			m_sceneSize = Scene::Size();
			m_rt = m_rt.fitted(m_sceneSize.y * 0.99, m_sceneSize.y * 0.99);
			m_camera = Camera2D{ Scene::CenterF(), 1.0, CameraControl::Keyboard };
			m_camera.setParameters(m_cp);
		}
		m_rt.draw(5, 5);	// 盤面テクスチャ描画

		updateBoardBuffer();
		if (m_manualState != ManualState::none)
		{
			m_grid->grid[4098] = m_id.x;
			m_grid->grid[4099] = m_id.y;
		}

		if (m_manualState == ManualState::setPositionWithMouse)
		{
			auto gridParPos = double(m_grid->grid[4096]) / double(m_rt.size.x);
			m_id.x = Cursor::PosF().x * gridParPos - m_W[m_id.p] / 2;
			m_id.y = Cursor::PosF().y * gridParPos - m_H[m_id.p] / 2;
			m_grid->grid[4098] = m_id.x;
			m_grid->grid[4099] = m_id.y;
			if (MouseR.down())
			{
				m_manualState = ManualState::setPositionWithCursor;
				MouseR.clearInput();
			}
		}
	}

	// GUI
	{
		BLoadJson();

		BVisualizeOP();

		// manual edit mode
		{
			const auto t = int32(m_nowTurn);

			// display operation
			auto tmpx = m_sceneSize.y + 10, tmpy = m_disp;
			{
				if (m_isFileLoaded)
				{
					m_infoDisplay(U"p:{:0>3}"_fmt(m_manualState != ManualState::none ? m_id.p : m_P[t])).draw(TextStyle::Outline(0.2, ColorF{ 0.0, 1.0 }), 23, Vec2{ tmpx, tmpy });
					tmpx += 90;
					m_infoDisplay(U"x:{:0>3}"_fmt(m_manualState != ManualState::none ? m_id.x : m_X[t])).draw(TextStyle::Outline(0.2, ColorF{ 0.0, 1.0 }), 23, Vec2{ tmpx, tmpy });
					tmpx += 90;
					m_infoDisplay(U"y:{:0>3}"_fmt(m_manualState != ManualState::none ? m_id.y : m_Y[t])).draw(TextStyle::Outline(0.2, ColorF{ 0.0, 1.0 }), 23, Vec2{ tmpx, tmpy });
					tmpx += 90;
					m_infoDisplay(U"s:{:0>3}"_fmt(m_manualState != ManualState::none ? m_id.s : m_S[t])).draw(TextStyle::Outline(0.2, ColorF{ 0.0, 1.0 }), 23, Vec2{ tmpx, tmpy });
					tmpx += 90;
					m_Emojies[m_manualState != ManualState::none ? m_id.s : m_S[t]].scaled(0.25).draw(tmpx, tmpy);
				}
			}

			// switch to manual mode
			{
				tmpx = m_sceneSize.y + 10, tmpy += 40;
				if (SimpleGUI::Button(U"手動操作", Vec2{ tmpx, tmpy }, 100, m_isFileLoaded))
				{
					if (m_manualState != ManualState::none)
					{
						m_manualState = ManualState::none;
					}
					else
					{
						m_manualState = ManualState::selectPattern;
						m_isPlaying = false;
					}
				}
				tmpx += 110;
				m_infoDisplay(m_manualState != ManualState::none ? U"有効" : U"無効").draw(23, Vec2{ tmpx, tmpy }, m_manualState != ManualState::none ? Palette::Yellow : Palette::White);
			}

			// pattern list
			{
				tmpx = m_sceneSize.y + 10, tmpy += 40;
				if (m_isFileLoaded)
				{
					if (SimpleGUI::ListBox(m_patList, Vec2{ tmpx, tmpy }, 180, 200, m_manualState != ManualState::none))
					{
						if (m_patList.selectedItemIndex != none)
						{
							m_id.p = *m_patList.selectedItemIndex;
							for (int32 i = 0; i < 2048; i++)
							{
								m_miniPat->patterns[i] = m_allPatterns[m_id.p].patterns[i];
							}
							m_info->info[0] = m_W[m_id.p];
							m_info->info[1] = m_H[m_id.p];
							m_mp = m_mp.resized(m_W[m_id.p], m_H[m_id.p]);
							m_mp = m_mp.fitted(300, 300);
							m_manualState = ManualState::setPositionWithMouse;
						}
					}
				}
			}

			// cursor button
			if (m_isFileLoaded)
			{
				tmpx = m_sceneSize.y + 58, tmpy += 210;
				if (m_manualState == ManualState::setPositionWithCursor && SimpleGUI::Button(U"\U000F06B7", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.y -= 1;
				}
				if (m_manualState == ManualState::setDirection && SimpleGUI::Button(U"\U000F06B7", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.s = 0;
				}
				tmpx = m_sceneSize.y + 10, tmpy += 40;
				if (m_manualState == ManualState::setPositionWithCursor && SimpleGUI::Button(U"\U000F0A02", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.x -= 1;
				}
				if (m_manualState == ManualState::setDirection && SimpleGUI::Button(U"\U000F0A02", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.s = 2;
				}
				tmpx += 95;
				if (m_manualState == ManualState::setPositionWithCursor && SimpleGUI::Button(U"\U000F0A03", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.x += 1;
				}
				if (m_manualState == ManualState::setDirection && SimpleGUI::Button(U"\U000F0A03", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.s = 3;
				}
				tmpx = m_sceneSize.y + 58, tmpy += 40;
				if (m_manualState == ManualState::setPositionWithCursor && SimpleGUI::Button(U"\U000F06B6", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.y += 1;
				}
				if (m_manualState == ManualState::setDirection && SimpleGUI::Button(U"\U000F06B6", Vec2{ tmpx, tmpy }, 85))
				{
					m_id.s = 1;
				}
				if (m_manualState == ManualState::setPositionWithCursor && MouseR.down())
				{
					m_manualState = ManualState::setPositionWithMouse;
					MouseR.clearInput();
				}
			}

			// submit, solve
			{
				tmpx = m_sceneSize.y + 10, tmpy += 40;
				switch (m_manualState)
				{
				case Visualizer::ManualState::setPositionWithCursor:
					if (SimpleGUI::Button(U"位置確定", Vec2{ tmpx, tmpy }, 100))
					{
						m_manualState = ManualState::setDirection;
					}
					break;
				case Visualizer::ManualState::setDirection:
					if (SimpleGUI::Button(U"操作適用", Vec2{ tmpx, tmpy }, 100))
					{
						m_manualState = ManualState::apply;
						applyManualOperation();
						makeInputFromCurrentBoard(m_tempPath);
						m_nowTurn += 1.0;
					}
					break;
				case Visualizer::ManualState::apply:
					if (SimpleGUI::Button(U"solve", Vec2{ tmpx, tmpy }, 100))
					{
						auto executer = Dialog::OpenFile({ FileFilter::AllFiles() }, fs::PathAppend(fs::ParentPath(m_tempPath), U"solvers"), U"回答生成プログラムの実行ファイルを選択してください");
						if (executer)
						{
							FilePath newExe = fs::PathAppend(m_tempPath, fs::FileName(*executer));
							Console << U"Load program: " + newExe;
							fs::Copy(*executer, newExe);
							AsyncTask<int32> p = Async([](FilePath fp) -> int32
								{
									ChildProcess child{ fp };
									child.wait();
									auto ret = child.getExitCode();
									if (ret != none)
									{
										return *ret;
									}
									else
									{
										return -1;
									}
								}, newExe);
							while (not p.isReady())
							{
								p.wait();
							}
							const JSON tmpOut = JSON::Load( fs::PathAppend(m_tempPath, U"output.json") );
							JSON ans;
							auto diff = m_out[U"n"].get<int32>();
							ans[U"n"] = diff + tmpOut[U"n"].get<int32>();
							Array<JSON> arrayJson(ans[U"n"].get<int32>());
							for (int32 i = 0; i < diff; i++)
							{
								arrayJson[i][U"p"] = m_out[U"ops"][i][U"p"].get<int32>();
								arrayJson[i][U"x"] = m_out[U"ops"][i][U"x"].get<int32>();
								arrayJson[i][U"y"] = m_out[U"ops"][i][U"y"].get<int32>();
								arrayJson[i][U"s"] = m_out[U"ops"][i][U"s"].get<int32>();
							}
							for (int32 i = 0; i < tmpOut[U"n"].get<int32>(); i++)
							{
								arrayJson[i + diff][U"p"] = tmpOut[U"ops"][i][U"p"].get<int32>();
								arrayJson[i + diff][U"x"] = tmpOut[U"ops"][i][U"x"].get<int32>();
								arrayJson[i + diff][U"y"] = tmpOut[U"ops"][i][U"y"].get<int32>();
								arrayJson[i + diff][U"s"] = tmpOut[U"ops"][i][U"s"].get<int32>();
							}
							ans[U"ops"] = arrayJson;

							ans.save(fs::PathAppend(m_tempPath, U"output.json"));
							Console << U"Output manual operated result: " + fs::PathAppend(m_tempPath, U"output.json");
							m_manualState = ManualState::none;
						}
					}
					break;
				default:
					break;
				}
			}

			// mini pattern
			tmpx = m_sceneSize.y + 10, tmpy += 40;
			if (m_isFileLoaded && m_manualState != ManualState::none)
			{
				const ScopedCustomShader2D shader{ m_pi };
				Graphics2D::SetPSConstantBuffer(4, m_miniPat);
				Graphics2D::SetPSConstantBuffer(5, m_info);
				m_mp.draw(Vec2{ tmpx, tmpy });
			}

			// 
			if (m_isFileLoaded && m_manualState != ManualState::none)
			{
				for (int32 i = 0; i < 2048; i++)
				{
					m_pattern->patterns[i] = m_allPatterns[m_id.p].patterns[i];
				}
				Graphics2D::SetPSConstantBuffer(1, m_grid);
				Graphics2D::SetPSConstantBuffer(3, m_pattern);
			}
		}

		if (SimpleGUI::Button(U"solver", Vec2{ Max(0, m_sceneSize.x - 80), Max(0, m_sceneSize.y - 75) }, 75)) m_currentState = State::Executer;	// change state
		if (SimpleGUI::Button(U"reset", Vec2{ Max(0, m_sceneSize.x - 80), Max(0, m_sceneSize.y - 40) }, 75)) this->initialize();	// reset state
	}

	// ターン更新
	{
		const auto t = int32(m_nowTurn);
		if (m_isPlaying)
		{
			if (t < m_opsSize)
			{
				m_nowTurn += Scene::DeltaTime() * m_speed * m_speed;
				m_nowTurn = Min(m_nowTurn, (double)m_opsSize);
			}
			else m_isPlaying = false;
		}
	}
}

void Visualizer::draw() const
{
	Line{ m_sceneSize.y + 5, 10, m_sceneSize.y + 5, Max(0, m_sceneSize.y - 10) }.draw(Palette::White);
	Line{ m_sceneSize.y + 10, m_visOP - 5, Max(0, m_sceneSize.x - 5), m_visOP - 5 }.draw(Palette::White);
	Line{ m_sceneSize.y + 10, m_disp - 5, Max(0, m_sceneSize.x - 5), m_disp - 5 }.draw(Palette::White);
}

State Visualizer::getNextState() const
{
	return m_currentState;
}

void Visualizer::resetState()
{
	m_currentState = State::Visualizer;
}
