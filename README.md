# なにこれ

第35回全国高等専門学校プログラミングコンテスト:競技部門において、「CeroRe::Master」チーム(群馬)が使用したビジュアライザ及び自動提出ツールです。

高速なビジュアライズ・手動操作の適用などの特徴を持ちます。

[OpenSiv3D](https://siv3d.github.io/ja-jp/)を用いて開発しました。

# インストール
1. IDEでOpenSiv3Dプロジェクトを作成 ([作成方法](https://siv3d.github.io/ja-jp/download/windows/))
2. Executer.hpp,Visualizer.hpp,board.hppをヘッダーファイルとして追加
3. Executer.cpp,Visualizer.cppを追加
4. Main.cppを置き換える
5. `App/`フォルダ以下に`myresource`をフォルダごとコピーし、中に含まれるファイルを[リソースファイルとして追加](https://zenn.dev/reputeless/books/siv3d-documentation/viewer/tutorial-resource)
6. プロジェクトを`Release`でビルド

# 使い方

起動時・Executerのリセット時に`config.json`ファイルを求められます(任意)。

このファイルは以下の形式で記述します。

```json
{
	"token":"token1",
	"URL":"172.0.0.1",
	"PORT":"3000"
}
```
ファイルの内容によってExecuterの設定が決定されますが、`URL`・`PORT`は
起動後もGUIでの変更が可能です。

また、上に例示した設定内容はデフォルト値と同じものです。
## Visualizer
### 概要
Visualizerでは、盤面操作の可視化・手動操作の適用を行うことができます。

まず、`LoadJson`ボタンを押して使用したい盤面を読み込む必要があります。

１回目のダイアログでは`input.json`を読み込んでください。

２回目のダイアログでは`output.json`を読み込んでください。`output.json`の読み込みは任意です。

### 盤面操作
- 移動: WASDキー
- 拡大: 左シフトキー
- 縮小: 左コントロールキー/スペースキー

### 可視化
画面左側に盤面が表示されます。
- ０の色が最も薄く、３の色が最も濃い青色で表示されます。
- 最終盤面と一致しているセルは緑色でハイライトされます。
- 次の抜き型操作で抜かれるセルが赤色でハイライトされます。

画面右側に操作ボタンが表示されます。
- Turn: 現在までに適用された操作数が表示されます。
- リセット: 初期盤面に戻します。
- start/stop: 操作の再生・停止を行います。
- 左/右: 手数を１つ戻す・進めることができます。
- 速度: 操作適用の速度を変更できます。
- スライダー: 表示する盤面を設定できます。
- p/x/y/s: 次の操作で適用する操作が表示されます。

### 手動操作
盤面を読み込んだ状態では、手動操作を有効にすることができます。

手動操作中は可視化操作用の操作ボタンが使用できません。

手動操作は以下の手順で行います。

1. 抜き型の選択

	抜き型リストをクリックすることで抜き型を選択できます。
	マウスカーソルでの選択のほか、選択状態から上下矢印キーを押すことで抜き型を変更することができます。
2. 抜き型位置の決定

	マウスカーソルを盤面上で動かし、右クリックで位置を仮固定することができます。仮固定後、画面右側の上下左右ボタンで位置セルずつ位置を移動させることができます。

	位置を決定する場合は位置確定ボタンを押します。
3. 寄せ方向の決定

	抜き型位置の決定後、上下左右ボタンで寄せ方向を設定します。

	操作適用ボタンを押すことで寄せ方向を決定し、操作を適用します。

操作適用後抜き型を選択しなおすことで、連続して手動操作を行うことができます。

また、操作適用後solveボタンを押し回答生成を行う実行ファイルを選択することで、
手動操作適用後の盤面を初期盤面として回答生成を行うことができます。
生成された回答は`visualizer/`以下に`output.json`として出力されます。

### ページ切り替え
右下に存在するsolverボタンを押すことでExecuterページに変更することができます。また、resetボタンを押すことでVisualizerページをリセットすることができます。

## Executer
### 概要
問題取得・プログラム実行・回答提出の自動実行システムです。

### 画面左側
画面左側に読み込んだ回答生成プログラムが表示されます。
- solver name: プログラムのファイル名
- status: ステータス

	実行前は`inactive`、実行中は`running`、実行後は`Done`、エラーが発生した場合は`Error`が表示されます。
- turn: プログラムが生成した回答の手数数

	未実行・実行中・エラーの場合は-1が表示されます。
- revision: 競技サーバーから返却された`revision`

	revisionが最も大きい提出が最終提出となります。

現在有効な提出は緑色にハイライトされます。

### 画面右側
各種設定を行うことができます。
- server address: 競技サーバーのIPアドレスと通信用ポート

	デフォルトでは`localhost`:`3000`、`config.json`を読み込んだ場合は
	そこに記述された値となります。
- 自動取得: 有効な間、サーバーから問題を自動で取得します。
- 問題取得: ボタンを押すことで競技サーバーにGetリクエストを送信します。
- solver: ボタンを押すことで開くダイアログを用いて回答生成ファイルを選択します。
- 自動実行: 有効な間、問題取得済みかつ回答生成プログラムの読み込みが完了しているなら自動でプログラムを実行し、最良解を競技サーバーへ送信します。
- solve: ボタンを押すことでプログラムを実行し、最良解を競技サーバーへ送信します。
- 手動提出: ボタンを押すことで開くダイアログを用いて任意の`output.json`ファイルを競技サーバーへ送信します。