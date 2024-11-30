# pragma once
# include "Executer.hpp"

Executer::Executer()
{
	this->initialize();
}

// 初期化処理
void Executer::initialize()
{
	// solver 用変数
	m_serverHost = TextEditState{ U"localhost" };
	m_serverPort = TextEditState{ U"3000" };
	m_serverResponse = TextEditState{ U"null" };
	m_tempPath = fs::PathAppend(fs::PathAppend(fs::TemporaryDirectoryPath(), U"procon-siv3d"), U"executer");
	fs::CreateDirectories(fs::PathAppend(fs::ParentPath(m_tempPath), U"solvers"));
	if (fs::Exists(m_tempPath))
	{
		fs::RemoveContents(m_tempPath);
		Console << U"Remove Contents with " + m_tempPath;
	}
	else
	{
		fs::CreateDirectories(m_tempPath);
		Console << U"Create Directory to " + m_tempPath;
	}

	m_isValidAutoGet = false;
	m_isProblemGot = false;
	m_isValidAutoSolve = false;
	m_isAnswerPosted = false;
	m_secCnt = 0;

	m_solveProcesses.clear();
	m_turnMax = INT_MAX;
	m_defaultTable = SimpleTable{ {300, 100, 80, 200}, {
		.cellHeight = 30,
		.borderThickness = 2,
		.backgroundColor = none,
		.textColor = ColorF{ 1.0 },
		.borderColor = ColorF{ 0.29, 0.33, 0.41 },
		.hasVerticalBorder = false,
		.hasOuterBorder = false,
		.font = Font{ FontMethod::SDF, 36, Typeface::Bold },
		.fontSize = 15,
		.hoveredRow = [](const Point& index) -> Optional<ColorF>
		{
			if (index.y != 0)
			{
				return ColorF{ 1.0, 0.2 };
			}
			return none;
		},
	} };
	m_defaultTable.push_back_row({ U"solver name", U"status", U"turn", U"revision" });
	m_processTable = m_defaultTable;
	resetState();

	m_infoDisplay = Font{ FontMethod::SDF, 48, Typeface::Bold };

	auto config_path = Dialog::OpenFile({ FileFilter::JSON() }, fs::ParentPath(m_tempPath), U"config.jsonファイルを選択してください(オプション)");
	if (config_path != none)
	{
		JSON config_json = JSON::Load(*config_path);
		m_teamToken = config_json[U"token"].getString();
		m_serverHost.text = config_json[U"URL"].getString();
		m_serverPort.text = config_json[U"PORT"].getString();
	}
}

// 問題取得
String Executer::GetProbrem(const URL address, const String token, const FilePath tempPath) const
{
	const URL url = U"http://" + address + U"/problem";
	const HashTable<String, String> headers = { {U"Procon-Token", token} };
	const FilePath inp = fs::PathAppend(tempPath, U"input.json");

	if (const auto response = SimpleHTTP::Get(url, headers, inp))
	{
		Console << U"[GetProbrem]";
		Console << U"------";
		Console << response.getStatusLine().rtrimmed();
		Console << U"status code: " << FromEnum(response.getStatusCode());
		Console << U"------";
		Console << response.getHeader().rtrimmed();
		Console << U"------\n";

		if (response.isOK())
		{
			const Array<FilePath> dir = fs::DirectoryContents(tempPath, Recursive::No);
			for (FilePath tmp : dir)
			{
				if (fs::IsDirectory(tmp))
				{
					fs::Copy(inp, fs::PathAppend(tmp, U"input.json"));
				}
			}
		}

		return response.getStatusLine().rtrimmed();
	}
	else
	{
		Console << U"GetProbrem Failed.\n";
		return U"GetProbrem Failed.";
	}
}

// 回答提出
String Executer::PostAnswer(URL address, String token, JSON output, FilePath resultPath) const
{
	const URL url = U"http://" + address + U"/answer";
	const HashTable<String, String> headers = { { U"Content-Type", U"application/json" }, {U"Procon-Token", token} };
	const std::string postJson = output.formatUTF8();
	const FilePath res = fs::PathAppend(resultPath, U"result.json");

	if (const auto response = SimpleHTTP::Post(url, headers, postJson.data(), postJson.size(), res))
	{
		Console << U"[PostAnswer]";
		Console << U"------";
		Console << response.getStatusLine().rtrimmed();
		Console << U"status code: " << FromEnum(response.getStatusCode());
		Console << U"------";
		Console << response.getHeader().rtrimmed();
		Console << U"------";

		if (response.isOK())
		{
			Console << TextReader{ res }.readAll() << U"\n";
			JSON tmp = JSON::Load(res);
			return Format(tmp[U"revision"].get<int64>());
		}
		else
		{
			Console << U"\n";
			return response.getStatusLine().rtrimmed();
		}
	}
	else
	{
		Console << U"Failed.\n";
		return U"Failed.";
	}
}

void Executer::update()
{
	ClearPrint();

	// server からの問題取得
	{
		m_secCnt++;
		m_infoDisplay(U"server address:").draw(23, Vec2{ 710, 10 });
		SimpleGUI::TextBox(m_serverHost, Vec2{ 900, 10 }, 150);
		m_infoDisplay(U":").draw(23, Vec2{ 1055, 10 });
		SimpleGUI::TextBox(m_serverPort, Vec2{ 1070, 10 }, 70);
		if (SimpleGUI::Button(U"自動取得", Vec2{ 710, 50 }, 100))
		{
			m_isValidAutoGet = !m_isValidAutoGet;
		}
		if (SimpleGUI::Button(U"問題取得", Vec2{ 710, 90 }, 100, !m_isValidAutoGet) || (m_secCnt > m_secParFrame && m_isValidAutoGet && not m_isProblemGot))
		{
			m_serverResponse.text = GetProbrem(m_serverHost.text + U":" + m_serverPort.text, m_teamToken, m_tempPath);
			m_secCnt = 0;
		}
		m_infoDisplay(U"{}"_fmt(m_isValidAutoGet ? U"有効" : U"無効")).draw(23, Vec2{ 820, 50 }, m_isValidAutoGet ? ColorF{ 0.0, 1.0, 0.0 } : ColorF{ 1.0, 0.0, 0.0 });
		m_infoDisplay(U"{}"_fmt(m_serverResponse.text)).draw(23, Vec2{ 820, 90 }, m_serverResponse.text == U"HTTP/1.1 200 OK" ? ColorF{ 0.0, 1.0, 0.0 } : ColorF{ 1.0, 0.0, 0.0 });
		if (m_serverResponse.text == U"HTTP/1.1 200 OK")
		{
			m_isProblemGot = true;
		}
	}

	// solverを取得
	{
		if (SimpleGUI::Button(U"solver", Vec2{ 710, 135 }, 100))
		{
			m_processTable = m_defaultTable;
			m_solveProcesses.clear();
			for (const auto& fp : fs::DirectoryContents(m_tempPath))
			{
				if (fs::IsFile(fp)) continue;
				fs::Remove(fp, AllowUndo::Yes);
			}
			auto solvers = Dialog::OpenFiles({ FileFilter::AllFiles() }, fs::PathAppend(fs::ParentPath(m_tempPath), U"solvers"), U"回答生成プログラムの実行ファイルをすべて選択してください");
			Console << U"[solver load]";
			Console << U"------";
			for (const auto&& [i, s] : Indexed(solvers))
			{
				Console << i << U":\t" << s;
				FilePath fp = fs::PathAppend(m_tempPath, fs::BaseName(s));
				if (m_isProblemGot)
				{
					FilePath inp = fs::PathAppend(m_tempPath, U"input.json");
					fs::CreateDirectories(fp);
					if (fs::IsFile(inp))
					{
						fs::Copy(inp, fs::PathAppend(fp, U"input.json"));
					}
				}
				fs::Copy(s, fs::PathAppend(fp, fs::FileName(s)));
				m_processTable.push_back_row({ fs::FileName(s), U"inactive", Format(-1), Format(-1) });
				m_solveProcesses.push_back(Process{ fs::PathAppend(fp, fs::FileName(s)), false, false });
			}
			Console << U"------\n";
		}
		m_infoDisplay(U"{} files loaded."_fmt(m_solveProcesses.size())).draw(23, Vec2{ 820, 135 }, m_solveProcesses.size() != 0 ? ColorF{ 0.0, 1.0, 0.0 } : ColorF{ 1.0, 0.0, 0.0 });
	}

	// solve
	{
		if (SimpleGUI::Button(U"自動実行", Vec2{ 710, 180 }, 100))
		{
			m_isValidAutoSolve = !m_isValidAutoSolve;
		}
		m_infoDisplay(U"{}"_fmt(m_isValidAutoSolve ? U"有効" : U"無効")).draw(23, Vec2{ 820, 180 }, m_isValidAutoSolve ? ColorF{ 0.0, 1.0, 0.0 } : ColorF{ 1.0, 0.0, 0.0 });
		if (SimpleGUI::Button(U"solve", Vec2{ 710, 220 }, 100, !m_isValidAutoSolve) || (m_isValidAutoSolve && m_isProblemGot && not m_isAnswerPosted))
		{
			m_isAnswerPosted = true;
			m_turnMax = INT_MAX;
			for (auto&& sp : m_solveProcesses)
			{
				sp.childProcess = Async([](FilePath fp) -> int32 {
					ChildProcess child{ fp };
					child.wait();
					auto ret = child.getExitCode();
					if (not ret)
					{
						return ret.value();
					}
					else
					{
						return -1;
					}
				}, sp.solverPath);
				sp.isTaskLoaded = true;
			}
		}
		for (int32 i = 0; auto&& sp : m_solveProcesses)
		{
			if (not sp.isTaskLoaded or sp.isAlreadyRan)
			{
				i++;
				continue;
			}
			if (not sp.childProcess.isReady())
			{
				m_processTable.setText(i + 1, { fs::FileName(sp.solverPath), U"Running", U"-1", U"-1" });
			}
			else
			{
				FilePath fp = fs::PathAppend(fs::ParentPath(sp.solverPath), U"output.json");
				JSON outjson = JSON::Load(fp);
				if (outjson)
				{
					int32 turn = outjson[U"n"].get<int32>();
					String tmp = U"";
					if (m_turnMax > turn)
					{
						m_turnMax = turn;
						tmp = PostAnswer(m_serverHost.text + U":" + m_serverPort.text, m_teamToken, outjson, fs::ParentPath(fp));
					}
					m_processTable.setText(i + 1, { fs::FileName(sp.solverPath), U"Done", Format(turn), tmp });
				}
				else
				{
					m_processTable.setText(i + 1, { fs::FileName(sp.solverPath), U"Error", U"-1", U"-1" });
				}
				sp.isAlreadyRan = true;
			}
			i++;
		}
		for (int32 i = 0; i < m_processTable.rows(); i++)
		{
			if (i == 0)
			{
				m_processTable.setRowTextColor(i, ColorF{ 1.0, 0.7, 0.25 });
				continue;
			}
			String status = m_processTable.getItem(i, 1).text;
			if (status == U"Running")
			{
				m_processTable.setTextColor(i, 1, ColorF{ 1.0, 1.0, 0.0 });
			}
			else if (status == U"Done")
			{
				m_processTable.setTextColor(i, 1, ColorF{ 0.0, 1.0, 0.0 });
			}
			else if (status == U"Error")
			{
				m_processTable.setTextColor(i, 1, ColorF{ 1.0, 0.5, 0.5 });
			}
			else
			{
				m_processTable.setTextColor(i, 1, ColorF{ 1.0, 0.0, 0.0 });
			}
			if (m_processTable.getItem(i, 2).text == Format(m_turnMax) && m_processTable.getItem(i, 3).text != U"")
			{
				m_processTable.setRowBackgroundColor(i, ColorF{ 0.0, 1.0, 0.0, 0.1 });
			}
			else
			{
				m_processTable.setRowBackgroundColor(i, ColorF{ 0.0, 0.0 });
			}
		}
	}

	// 手動提出
	{
		String ret = U"none";
		if (SimpleGUI::Button(U"手動提出", Vec2{ 710, 265 }, 100))
		{
			auto output = Dialog::OpenFile({ FileFilter::JSON() }, fs::ParentPath(m_tempPath), U"提出するJsonファイルを選択してください");
			if (output != none)
			{
				ret = PostAnswer(m_serverHost.text + U":" + m_serverPort.text, m_teamToken, JSON::Load(*output), fs::ParentPath(*output));
			}
		}
		m_infoDisplay(ret).draw(23, Vec2{ 820, 265 }, Palette::White);
	}

	if (SimpleGUI::Button(U"vis", Vec2{ Max(0, Scene::Size().x - 80), Max(0, Scene::Size().y - 110)}, 75)) m_currentState = State::Visualizer; // change state
	if (SimpleGUI::Button(U"reset", Vec2{ Max(0, Scene::Size().x - 80), Max(0, Scene::Size().y - 40)}, 75)) this->initialize();	// reset state
}

void Executer::draw() const
{
	m_processTable.draw(Vec2{ 10, 10 });
	Line{ 710, 130, Max(0, Scene::Size().x - 10), 130 }.draw(Palette::White);
	Line{ 710, 175, Max(0, Scene::Size().x - 10), 175 }.draw(Palette::White);
	Line{ 705, 10 , 705, Max(0, Scene::Size().y - 10) }.draw(Palette::White);
	Line{ 710, 260, Max(0, Scene::Size().x - 10), 260 }.draw(Palette::White);
}

State Executer::getNextState() const
{
	return m_currentState;
}

void Executer::resetState()
{
	m_currentState = State::Executer;
}

