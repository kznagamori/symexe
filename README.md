# symexe

Windowsでシンボリックリンクのように環境変数を一時的に変更してプログラムを実行するためのラッパーツール

## 概要

`symexe`は、Windows環境でINI設定ファイルに基づいて環境変数を一時的に変更し、指定したプログラムを実行するためのC++製のコマンドラインツールです。実行後は元の環境変数に戻すため、システム全体の環境を汚染することなく、特定のアプリケーション用の環境を構築できます。

## 特徴

- **環境変数の一時的な設定**: INIファイルで定義した環境変数を実行時のみ適用
- **PATHの追加**: 既存のPATHに新しいパスを追加して実行
- **コードページ変更**: コンソールのコードページを一時的に変更可能
- **コマンドライン引数の転送**: ラッパーに渡された引数をそのまま実行プログラムに転送
- **自動復元**: プログラム終了後に環境変数とコードページを自動的に元に戻す
- **Unicode対応**: UNICODEビルドのみサポート

## ビルド要件

- Visual Studio 2019以降
- Windows SDK
- C++17以降のサポート

## ビルド方法

1. Visual Studioでソリューションファイル `symexe.sln` を開く
2. ビルド構成を選択（Debug/Release, x64）
3. ビルド実行

```
ビルド > ソリューションのビルド
```

## 使い方

### 基本的な使い方

1. `symexe.exe`と同じディレクトリに`symexe.ini`ファイルを配置
2. INIファイルに環境変数と実行ファイルのパスを設定
3. `symexe.exe`を実行（コマンドライン引数は実行プログラムに転送される）

```cmd
symexe.exe [実行プログラムへの引数...]
```

### INIファイルの設定例

```ini
[CONFIG]
; 設定する環境変数名をカンマ区切りで列挙
OPTS=DOTNET_ROOT,DOTNET_ROOT(x86),DOTNET_CLI_HOME
; コンソールのコードページ（省略可能、0で変更なし）
CODEPAGE=65001

[OPT]
; 各環境変数の値を設定
DOTNET_ROOT=C:\Program Files\dotnet
DOTNET_ROOT(x86)=C:\Program Files (x86)\dotnet
DOTNET_CLI_HOME=C:\Users\YourName\.dotnet
; PATHに追加するパス（セミコロン区切り）
PATH=C:\Program Files\dotnet;C:\custom\path

[EXE]
; 実行するプログラムのフルパス
PATH=C:\Program Files\dotnet\dotnet.exe
```

### 設定項目の説明

#### [CONFIG] セクション
- **OPTS**: 設定する環境変数名のリスト（カンマ区切り）
- **CODEPAGE**: コンソールのコードページ（例: 65001=UTF-8, 932=Shift_JIS）

#### [OPT] セクション
- **[変数名]**: 各環境変数の値を設定
- **PATH**: 既存のPATHの先頭に追加するパス（セミコロン区切り）

#### [EXE] セクション
- **PATH**: 実行する実際のプログラムのフルパス（必須）

## 使用例

### .NET SDKバージョンの切り替え

```ini
[CONFIG]
OPTS=DOTNET_ROOT
CODEPAGE=0

[OPT]
DOTNET_ROOT=C:\dotnet\7.0.200
PATH=C:\dotnet\7.0.200

[EXE]
PATH=C:\dotnet\7.0.200\dotnet.exe
```

実行:
```cmd
symexe.exe --version
```

これにより、指定した.NET SDKバージョンで`dotnet --version`が実行されます。

## 技術的な詳細

### 動作フロー

1. 自身の実行ファイルパスから`.ini`ファイルのパスを取得
2. INIファイルから設定を読み込み
3. 現在の環境変数を保存
4. INIファイルで指定された環境変数とPATHを設定
5. 指定されたプログラムを`CreateProcessW`で起動し、完了を待機
6. プログラム終了後、環境変数とコードページを元に戻す
7. 実行したプログラムの終了コードを返す

### 制限事項

- UNICODEビルドのみサポート
- Windows専用（Win32 API使用）
- 環境変数名やパスの最大長に制限あり（設定により変更可能）
  - `MAX_OPTS`: 2048文字
  - `MAX_ADD_PATH`: 10240文字
  - `MAX_PATH`: 260文字（Windows標準）

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) ファイルを参照してください。

Copyright (c) 2023 kznagamori

## 貢献

プルリクエストや問題報告を歓迎します。

## 参考

- Windows環境でのアプリケーションランチャー
- 環境変数の一時的な変更を必要とするツールチェーンのラッパー
- 複数バージョンの開発環境の切り替え
