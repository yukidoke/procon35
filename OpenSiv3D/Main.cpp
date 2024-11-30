# pragma once
# include <Siv3D.hpp> // Siv3D v0.6.15
# include "SceneCommon.hpp"
# include "Visualizer.hpp"
# include "Executer.hpp"

SIV3D_SET(EngineOption::Renderer::OpenGL);	// シェーダ言語をGLSLへ変更

void Main()
{
	// init window
	Window::SetTitle(U"GUI v2.0.0");
	Window::SetStyle(WindowStyle::Sizable);
	Scene::SetResizeMode(ResizeMode::Virtual);

	// scene management
	StateManager manager;
	manager.states.push_back(std::make_unique<Visualizer>());
	manager.states.push_back(std::make_unique<Executer>());

	// FPS制限
	int FPS = 60;
	Stopwatch sw;
	sw.start();
	while (System::Update())
	{
		manager.update();

		while (sw.msF() < 1000.0 / FPS);
		sw.restart();
	}
}

//
// - Debug ビルド: プログラムの最適化を減らす代わりに、エラーやクラッシュ時に詳細な情報を得られます。
//
// - Release ビルド: 最大限の最適化でビルドします。
//
// - [デバッグ] メニュー → [デバッグの開始] でプログラムを実行すると、[出力] ウィンドウに詳細なログが表示され、エラーの原因を探せます。
//
// - Visual Studio を更新した直後は、プログラムのリビルド（[ビルド]メニュー → [ソリューションのリビルド]）が必要な場合があります。
//
