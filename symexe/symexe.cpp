/**
 * @file symexe.cpp
 * @brief Windows環境変数ラッパープログラム
 *
 * このプログラムは、INIファイルに基づいて環境変数を一時的に変更し、
 * 指定されたプログラムを実行するWindowsコマンドラインツールです。
 * 実行後は環境変数を元の状態に戻すため、システム環境を汚染しません。
 *
 * @author kznagamori
 * @date 2023
 * @copyright MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>
#include <direct.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <errno.h>

// UNICODEビルドのみサポート
#ifdef _UNICODE
#define SIZEOF(x)		(sizeof(x)/sizeof(x[0]))
#else
#pragma comment("UNIOCDEのみ対応")
#endif

// 関数プロトタイプ宣言
static DWORD GetInitPathW(HMODULE hModule, LPWSTR lpDirName, DWORD nSize);

// 定数定義
#define MAX_OPTS		(2048)		///< オプション文字列の最大長
#define MAX_ADD_PATH	(10240)		///< パス文字列の最大長

// グローバル変数（静的）
static WCHAR OldPath[MAX_ADD_PATH] = { 0 };		///< 元のPATH環境変数
static WCHAR AddPath[MAX_ADD_PATH] = { 0 };		///< 追加するPATH
static WCHAR Path[MAX_ADD_PATH] = { 0 };			///< 作業用PATHバッファ
static WCHAR SelfPath[MAX_PATH + 1] = { 0 };		///< 自身の実行ファイルパス
static WCHAR InitPath[MAX_PATH + 1] = { 0 };		///< INIファイルのパス
static WCHAR OptName[MAX_OPTS] = { 0 };			///< オプション名リスト（カンマ区切り）
static WCHAR ExePath[MAX_PATH + 1] = { 0 };		///< 実行するプログラムのパス

/**
 * @brief メインエントリポイント
 *
 * プログラムの主要な処理フロー:
 * 1. コマンドライン引数の解析
 * 2. INIファイルの読み込み
 * 3. 現在の環境変数の保存
 * 4. 新しい環境変数の設定
 * 5. 指定されたプログラムの実行
 * 6. 環境変数の復元
 *
 * @param argCount コマンドライン引数の数
 * @param argValue コマンドライン引数の配列
 * @return int 実行したプログラムの終了コード
 */
int wmain(int argCount, wchar_t* argValue[])
{
	/* ========================================
	 * 1. 自身のパスとコマンドラインの取得
	 * ======================================== */

	// 自身の実行ファイルパスを取得
	HMODULE hModule = ::GetModuleHandle(nullptr);
	GetModuleFileName(hModule, SelfPath, SIZEOF(SelfPath));

	// コマンドライン全体を取得
	WCHAR* pCommandLine;
	pCommandLine = GetCommandLine();
	std::wstring Command = std::wstring(argValue[0]);
	std::wstring CommandLine = std::wstring(pCommandLine);

	// コマンドラインから自身のプログラム名を除去
	// （引用符で囲まれている場合と囲まれていない場合の両方に対応）
	if (CommandLine[0] == L'\"' || CommandLine[0] == L'\'')
	{
		// 引用符付きの場合: "program.exe" args... → args...
		CommandLine.erase(0, Command.length() + 2);
	}
	else
	{
		// 引用符なしの場合: program.exe args... → args...
		CommandLine.erase(0, Command.length());
	}

	/* ========================================
	 * 2. INIファイルのパス取得と存在確認
	 * ======================================== */

	// 自身の実行ファイルと同じディレクトリにある.iniファイルのパスを取得
	if (GetInitPathW(NULL, InitPath, SIZEOF(InitPath)) == 0)
	{
		_wperror(L"GetInitPathW");
		return __LINE__;
	}

	// INIファイルが存在するか確認
	if (std::filesystem::exists(InitPath) == false)
	{
		_wperror(L"std::filesystem::exists");
		return __LINE__;
	}

#ifdef _DEBUG
	/* ========================================
	 * デバッグ用: サンプルINIファイルの作成
	 * ======================================== */
	WCHAR DebugOpts[MAX_OPTS] = L"TEST_DOTNET_ROOT,TEST_DOTNET_ROOT(x86),TEST_DOTNET_CLI_HOME";
	WritePrivateProfileStringW(L"CONFIG", L"OPTS", DebugOpts, InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_ROOT", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT", InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_ROOT(x86)", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT(x86)", InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_CLI_HOME", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_CLI_HOME", InitPath);
	WritePrivateProfileStringW(L"OPT", L"PATH", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT;W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200", InitPath);
	WritePrivateProfileStringW(L"EXE", L"PATH", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\test_console.exe", InitPath);
#endif

	/* ========================================
	 * 3. INIファイルからの設定読み込み
	 * ======================================== */

	DWORD OptsLen;

	// [CONFIG]セクションから環境変数名のリストを取得
	// 形式: "VAR1,VAR2,VAR3"
	std::vector<std::wstring> OptNameList;
	OptsLen = GetPrivateProfileStringW(L"CONFIG", L"OPTS", L"", OptName, SIZEOF(OptName), InitPath);
	if (OptsLen > 0)
	{
		// カンマ区切りの文字列を分割してベクターに格納
		WCHAR* Next = NULL;
		OptNameList.push_back(std::wstring(wcstok_s(OptName, L",", &Next)));
		while (Next != NULL && Next[0] != 0)
		{
			OptNameList.push_back(std::wstring(wcstok_s(Next, L",", &Next)));
		}
	}

	// [CONFIG]セクションからコードページ設定を取得
	// 0の場合は変更しない、65001=UTF-8、932=Shift_JIS等
	UINT CodePage = 0;
	UINT OldCodePage = GetConsoleCP();
	CodePage = GetPrivateProfileIntW(L"CONFIG", L"CODEPAGE", 0, InitPath);

	// [OPT]セクションから各環境変数の値を取得
	std::vector<std::wstring> OptValueList;
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		WCHAR OptValue[MAX_OPTS] = { 0 };
		OptsLen = GetPrivateProfileStringW(L"OPT", OptNameList[i].c_str(), L"", OptValue, SIZEOF(OptValue), InitPath);
		OptValueList.push_back(std::wstring(OptValue));
	}

	// [OPT]セクションから追加PATHを取得
	OptsLen = GetPrivateProfileStringW(L"OPT", L"PATH", L"", AddPath, SIZEOF(AddPath), InitPath);

	// [EXE]セクションから実行するプログラムのパスを取得（必須）
	OptsLen = GetPrivateProfileStringW(L"EXE", L"PATH", L"", ExePath, SIZEOF(ExePath), InitPath);
	if (OptsLen < 1) {
		_wperror(L"GetPrivateProfileStringW");
		return __LINE__;
	}

	/* ========================================
	 * 4. 現在の環境変数を保存
	 * ======================================== */

	// 現在のPATH環境変数を保存
	DWORD PathLen = GetEnvironmentVariable(L"PATH", OldPath, SIZEOF(OldPath));

	// 各環境変数の現在の値を保存
	std::vector<std::wstring> OldOptValueList;
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		WCHAR OptValue[MAX_OPTS] = { 0 };
		if (GetEnvironmentVariableW(OptNameList[i].c_str(), OptValue, SIZEOF(OptValue)) == 0) {
			// 環境変数が存在しない場合は空文字列
			OptValue[0] = '\0';
		}
		OldOptValueList.push_back(std::wstring(OptValue));
	}

	/* ========================================
	 * 5. 新しい環境変数を設定
	 * ======================================== */

	// INIファイルで指定された環境変数を設定
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		if (SetEnvironmentVariableW(OptNameList[i].c_str(), OptValueList[i].c_str()) != TRUE)
		{
			_wperror(L"SetEnvironmentVariableW");
			return __LINE__;
		}
	}

	// PATHに新しいパスを追加（既存のPATHの先頭に追加）
	std::wstring NewPath = std::wstring(AddPath) + L";" + std::wstring(OldPath);
	if (SetEnvironmentVariableW(L"PATH", NewPath.c_str()) != TRUE)
	{
		_wperror(L"SetEnvironmentVariableW");
		return __LINE__;
	}

	/* ========================================
	 * 6. コンソールのコードページ変更
	 * ======================================== */

	// コードページが指定されている場合のみ変更
	if (CodePage > 0) {
		SetConsoleCP(CodePage);
		SetConsoleOutputCP(CodePage);
	}

	/* ========================================
	 * 7. 指定されたプログラムを実行
	 * ======================================== */

	// プロセス起動用の構造体を初期化
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);

	// 実行するプログラムのパスを再度取得（念のため）
	GetPrivateProfileStringW(L"EXE", L"PATH", L"", ExePath, SIZEOF(ExePath), InitPath);

	// コマンドライン文字列を構築: "実行ファイル" 引数...
	std::wstring Cmd = L"\"" + std::wstring(ExePath) + L"\" " + CommandLine;

	// プロセスを作成して実行
	// TRUE: ハンドルを継承、CREATE_UNICODE_ENVIRONMENT: Unicode環境変数を使用
	if (!CreateProcessW(NULL, (LPWSTR)Cmd.c_str(), NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi)) {
		_wperror(L"_wsplitpath_s error");
	}

	// プロセスの終了を待機
	WaitForSingleObject(pi.hProcess, INFINITE);

	/* ========================================
	 * 8. 環境変数を元に戻す
	 * ======================================== */

	// 各環境変数を元の値に戻す
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		if (SetEnvironmentVariableW(OptNameList[i].c_str(), OldOptValueList[i].c_str()) != TRUE)
		{
			_wperror(L"SetEnvironmentVariableW");
			return __LINE__;
		}
	}

	// PATH環境変数を元に戻す
	if (SetEnvironmentVariableW(L"PATH", OldPath) != TRUE)
	{
		_wperror(L"SetEnvironmentVariableW");
		return __LINE__;
	}

	/* ========================================
	 * 9. 終了処理
	 * ======================================== */

	// 実行したプロセスの終了コードを取得
	DWORD ExitCode = 0;
	GetExitCodeProcess(pi.hProcess, &ExitCode);

	// コードページを元に戻す
	if (CodePage > 0) {
		SetConsoleCP(OldCodePage);
		SetConsoleOutputCP(OldCodePage);
	}

	// プロセスハンドルをクローズ
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// 実行したプログラムの終了コードを返す
	return ExitCode;
}

/**
 * @brief 実行ファイルと同じディレクトリにあるINIファイルのパスを取得
 *
 * 実行ファイルのパスから拡張子を.iniに変更したパスを生成します。
 * 例: C:\path\to\program.exe → C:\path\to\program.ini
 *
 * @param hModule モジュールハンドル（NULLの場合は現在のプロセス）
 * @param lpDirName INIファイルのパスを格納するバッファ
 * @param nSize バッファのサイズ（文字数）
 * @return DWORD 生成されたパスの文字列長（失敗時は0）
 *
 * @note この関数は_wsplitpath_sと_wmakepath_sを使用してパスを操作します
 */
static DWORD GetInitPathW(HMODULE hModule, LPWSTR lpDirName, DWORD nSize)
{
	errno_t err = 0;
	WCHAR path[MAX_PATH + 1];

	// モジュール（実行ファイル）のフルパスを取得
	GetModuleFileName(hModule, path, SIZEOF(path));

	// パスを構成要素に分解するためのバッファ
	WCHAR drive[_MAX_DRIVE];	// ドライブ文字（例: "C:"）
	WCHAR dir[_MAX_DIR];		// ディレクトリパス（例: "\path\to\"）
	WCHAR fname[_MAX_FNAME];	// ファイル名（拡張子なし、例: "program"）
	WCHAR ext[_MAX_EXT];		// 拡張子（例: ".exe"）

	// パスを分解
	if ((err = _wsplitpath_s(path, drive, SIZEOF(drive), dir, SIZEOF(dir), fname, SIZEOF(fname), ext, SIZEOF(ext))) != 0)
	{
		_wperror(L"_wsplitpath_s");
		return 0;
	}

	// 拡張子を.iniに変更してパスを再構築
	if ((err = _wmakepath_s(lpDirName, nSize, drive, dir, fname, L"ini")) != 0)
	{
		_wperror(L"_wmakepath_s");
		return 0;
	}

	// 生成されたパスの長さを返す
	return lstrlenW(lpDirName);
}
