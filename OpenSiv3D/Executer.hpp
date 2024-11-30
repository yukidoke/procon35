# pragma once
# include "SceneCommon.hpp"

namespace fs = FileSystem;

class Executer : public ISState
{
public:
	Executer();
	void initialize();
	void update() override;
	void draw() const override;
	State getNextState() const override;
	void resetState() override;

private:
	struct Process	// solver 一つを管理する構造体
	{
		FilePath solverPath;
		bool isAlreadyRan;
		bool isTaskLoaded;
		AsyncTask<int32> childProcess;
	};

	// solver 用変数
	String m_teamToken = U"token1";	// TODO: 読み込むか実行時に書き直すか決める
	TextEditState m_serverHost;
	TextEditState m_serverPort;
	TextEditState m_serverResponse;
	FilePath m_tempPath;

	bool m_isValidAutoGet;
	bool m_isProblemGot;
	bool m_isValidAutoSolve;
	bool m_isAnswerPosted;
	constexpr static int32 m_secParFrame = 6;
	int32 m_secCnt;

	Array<Process> m_solveProcesses;
	int32 m_turnMax;
	SimpleTable m_defaultTable;
	SimpleTable m_processTable;

	State m_currentState;
	Font m_infoDisplay;

	String GetProbrem(const URL address, const String token, const FilePath tempPath) const;
	String PostAnswer(URL address, String token, JSON output, FilePath resultPath) const;
};
