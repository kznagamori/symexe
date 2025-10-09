# symexe

Windowsでシンボリックリンクのように環境変数を一時的に変更してプログラムを実行するためのラッパーツール

## 概要

`symexe`は、Windows環境でINI設定ファイルに基づいて環境変数を一時的に変更し、指定したプログラムを実行するためのC++製のコマンドラインツールです。実行後は元の環境変数に戻すため、システム全体の環境を汚染することなく、特定のアプリケーション用の環境を構築できます。

**重要な特徴**: `symexe.exe`を実行したいプログラム名にリネームすることで、あたかもそのプログラム自体が実行されているかのように動作します。例えば、`symexe.exe`を`dotnet.exe`にリネームすると、`dotnet.exe`を実行するだけで自動的に環境変数が設定され、実際の`dotnet.exe`（INIファイルで指定）が実行されます。

## 特徴

- **透過的な実行**: 実行ファイル名を変更するだけで、元のプログラムと同じように使える
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

`symexe`は、実行ファイル名に基づいてINIファイルを読み込む仕組みです。

#### ステップ1: ファイルのリネーム
`symexe.exe`を実行したいプログラム名にリネームします。

例: `.NET`の`dotnet.exe`をラップする場合
```cmd
copy symexe.exe dotnet.exe
```

#### ステップ2: INIファイルの作成
リネームした実行ファイルと同じ名前のINIファイルを作成します。

例: `dotnet.exe`にリネームした場合は`dotnet.ini`を作成

#### ステップ3: INIファイルの設定
INIファイルに環境変数と実際に実行するプログラムのパスを設定します。

#### ステップ4: 実行
リネームしたファイルを普通のプログラムとして実行します。

```cmd
dotnet.exe --version
```

このとき、`dotnet.ini`に基づいて環境変数が設定され、INIファイルで指定された実際の`dotnet.exe`が実行されます。

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

#### ディレクトリ構成
```
C:\tools\dotnet-7.0\
  ├── dotnet.exe        ← symexe.exeをリネームしたもの
  └── dotnet.ini        ← 設定ファイル
```

#### dotnet.ini の内容
```ini
[CONFIG]
OPTS=DOTNET_ROOT,DOTNET_CLI_HOME
CODEPAGE=0

[OPT]
DOTNET_ROOT=C:\Program Files\dotnet\7.0.200
DOTNET_CLI_HOME=%USERPROFILE%\.dotnet
PATH=C:\Program Files\dotnet\7.0.200

[EXE]
PATH=C:\Program Files\dotnet\7.0.200\dotnet.exe
```

#### 実行方法
```cmd
C:\tools\dotnet-7.0\dotnet.exe --version
```

または、`C:\tools\dotnet-7.0`をPATHに追加すれば:
```cmd
dotnet --version
```

これにより、`dotnet.ini`で指定した.NET SDK 7.0.200の環境変数が自動的に設定され、実際の`dotnet.exe`が実行されます。ユーザーは通常の`dotnet`コマンドと同じように使用できます。

### 複数バージョンの管理

異なるバージョンを切り替える場合は、複数のディレクトリを作成:

```
C:\tools\
  ├── dotnet-6.0\
  │   ├── dotnet.exe    ← symexe.exeのコピー
  │   └── dotnet.ini    ← .NET 6.0向けの設定
  └── dotnet-7.0\
      ├── dotnet.exe    ← symexe.exeのコピー
      └── dotnet.ini    ← .NET 7.0向けの設定
```

必要に応じてPATHを切り替えるか、フルパスで実行することで、異なるバージョンを使い分けられます。

## 技術的な詳細

### 動作フロー

1. 自身の実行ファイル名から対応する`.ini`ファイルのパスを取得（例: `dotnet.exe` → `dotnet.ini`）
2. INIファイルから設定を読み込み
3. 現在の環境変数を保存
4. INIファイルで指定された環境変数とPATHを設定
5. INIファイルの`[EXE]`セクションで指定されたプログラムを`CreateProcessW`で起動し、完了を待機
6. プログラム終了後、環境変数とコードページを元に戻す
7. 実行したプログラムの終了コードを返す

### 仕組みの詳細

`symexe`は実行ファイル名（拡張子を除く）と同じ名前のINIファイルを探します。

- 実行ファイル: `dotnet.exe` → 読み込むINI: `dotnet.ini`
- 実行ファイル: `python.exe` → 読み込むINI: `python.ini`
- 実行ファイル: `node.exe` → 読み込むINI: `node.ini`

この仕組みにより、1つの`symexe.exe`を複数のプログラム名にコピー・リネームして、それぞれ異なる設定で異なるプログラムをラップできます。

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

## ユースケース

- **複数バージョンの開発環境の管理**: .NET、Python、Node.js等の異なるバージョンを切り替え
- **ポータブル開発環境**: USBメモリやネットワークドライブ上の開発ツールを環境変数なしで実行
- **プロジェクト固有の環境**: プロジェクトごとに異なる環境変数でツールを実行
- **シンボリックリンクの代替**: 管理者権限なしで実行可能な軽量なラッパー
- **環境分離**: システム環境を汚染せずに一時的な環境でプログラムを実行

## 類似ツールとの比較

- **シンボリックリンク**: 管理者権限が必要、環境変数の設定はできない
- **バッチファイル**: 環境変数は設定できるが、プログラム名が変わる、終了コードの処理が複雑
- **環境変数の永続的な変更**: システム全体に影響、複数バージョンの共存が困難

`symexe`は、これらの問題を解決し、透過的かつ柔軟にプログラムをラップできます。
