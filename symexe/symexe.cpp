// shimexe.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//
//
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

#ifdef _UNICODE
#define SIZEOF(x)		(sizeof(x)/sizeof(x[0]))
#else
#pragma comment("UNIOCDEのみ対応")
#endif

static DWORD GetInitPathW(HMODULE hModule, LPWSTR lpDirName, DWORD nSize);
#define MAX_OPTS		(2048)
#define MAX_ADD_PATH	(10240)
static WCHAR OldPath[MAX_ADD_PATH] = { 0 };
static WCHAR AddPath[MAX_ADD_PATH] = { 0 };
static WCHAR Path[MAX_ADD_PATH] = { 0 };
static WCHAR SelfPath[MAX_PATH + 1] = { 0 };
static WCHAR InitPath[MAX_PATH + 1] = { 0 };
static WCHAR OptName[MAX_OPTS] = { 0 };
static WCHAR ExePath[MAX_PATH + 1] = { 0 };
int wmain(int argCount, wchar_t* argValue[])
{
	/* 自身のパス */
	HMODULE hModule = ::GetModuleHandle(nullptr);
	GetModuleFileName(hModule, SelfPath, SIZEOF(SelfPath));

	/* コマンドラインを取得する */
	WCHAR* pCommandLine;
	pCommandLine = GetCommandLine();
	std::wstring Command = std::wstring(argValue[0]);
	std::wstring CommandLine = std::wstring(pCommandLine);

	if (CommandLine[0] == L'\"' || CommandLine[0] == L'\'')
	{
		CommandLine.erase(0, Command.length() + 2);
	}
	else
	{
		CommandLine.erase(0, Command.length());
	}

	/* 自身のiniファイルのパスを取得する */
	if (GetInitPathW(NULL, InitPath, SIZEOF(InitPath)) == 0)
	{
		_wperror(L"GetInitPathW");
		return __LINE__;
	}
	if (std::filesystem::exists(InitPath) == false)
	{
		_wperror(L"std::filesystem::exists");
		return __LINE__;
	}
#ifdef _DEBUG
	/* デバッグ用のiniファイル作成*/
	WCHAR DebugOpts[MAX_OPTS] = L"TEST_DOTNET_ROOT,TEST_DOTNET_ROOT(x86),TEST_DOTNET_CLI_HOME";
	WritePrivateProfileStringW(L"CONFIG", L"OPTS", DebugOpts, InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_ROOT", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT", InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_ROOT(x86)", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT(x86)", InitPath);
	WritePrivateProfileStringW(L"OPT", L"TEST_DOTNET_CLI_HOME", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_CLI_HOME", InitPath);
	WritePrivateProfileStringW(L"OPT", L"PATH", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200\\DOTNET_ROOT;W:\\anyvm\\dev_dir\\ー ソ 十 表\\anyvm\\envs\\dotnet\\7.0.200", InitPath);
	WritePrivateProfileStringW(L"EXE", L"PATH", L"W:\\anyvm\\dev_dir\\ー ソ 十 表\\test_console.exe", InitPath);
#endif

	/* iniファイルの解析 */
	DWORD OptsLen;

	/* オプションのリストを検索 */
	std::vector<std::wstring> OptNameList;
	OptsLen = GetPrivateProfileStringW(L"CONFIG", L"OPTS", L"", OptName, SIZEOF(OptName), InitPath);
	if (OptsLen > 0)
	{
		WCHAR* Next = NULL;
		OptNameList.push_back(std::wstring(wcstok_s(OptName, L",", &Next)));
		while (Next != NULL && Next[0] != 0)
		{
			OptNameList.push_back(std::wstring(wcstok_s(Next, L",", &Next)));
		}
	}

	/* コードページ変更 */
	UINT CodePage = 0;
	UINT OldCodePage = GetConsoleCP();

	CodePage = GetPrivateProfileIntW(L"CONFIG", L"CODEPAGE", 0, InitPath);

	/* オプションの値を取得 */
	std::vector<std::wstring> OptValueList;
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		WCHAR OptValue[MAX_OPTS] = { 0 };
		OptsLen = GetPrivateProfileStringW(L"OPT", OptNameList[i].c_str(), L"", OptValue, SIZEOF(OptValue), InitPath);
		OptValueList.push_back(std::wstring(OptValue));
	}

	/* 追加パスを取得 */
	OptsLen = GetPrivateProfileStringW(L"OPT", L"PATH", L"", AddPath, SIZEOF(AddPath), InitPath);

	/* 実行ファイルのパスを追加 */

	OptsLen = GetPrivateProfileStringW(L"EXE", L"PATH", L"", ExePath, SIZEOF(ExePath), InitPath);
	if (OptsLen < 1) {
		_wperror(L"GetPrivateProfileStringW");
		return __LINE__;
	}

	/* 変更前の環境変数を保存する */
	DWORD PathLen = GetEnvironmentVariable(L"PATH", OldPath, SIZEOF(OldPath));

	std::vector<std::wstring> OldOptValueList;
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		WCHAR OptValue[MAX_OPTS] = { 0 };
		if (GetEnvironmentVariableW(OptNameList[i].c_str(), OptValue, SIZEOF(OptValue)) == 0) {
			OptValue[0] = '\0';
		}
		OldOptValueList.push_back(std::wstring(OptValue));
	}

	/* 環境変数を設定する */
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		if (SetEnvironmentVariableW(OptNameList[i].c_str(), OptValueList[i].c_str()) != TRUE)
		{
			_wperror(L"SetEnvironmentVariableW");
			return __LINE__;
		}
	}

	/* パスを追加する */
	std::wstring NewPath = std::wstring(AddPath) + L";" + std::wstring(OldPath);
	if (SetEnvironmentVariableW(L"PATH", NewPath.c_str()) != TRUE)
	{
		_wperror(L"SetEnvironmentVariableW");
		return __LINE__;
	}


	if (CodePage > 0) {
		SetConsoleCP(CodePage);
		SetConsoleOutputCP(CodePage);
	}
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	GetPrivateProfileStringW(L"EXE", L"PATH", L"", ExePath, SIZEOF(ExePath), InitPath);

	std::wstring Cmd = L"\"" + std::wstring(ExePath) + L"\" " + CommandLine;
	if (!CreateProcessW(NULL, (LPWSTR)Cmd.c_str(), NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi)) {
		_wperror(L"_wsplitpath_s error");
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	/* 環境変数を元に戻す */
	for (int i = 0; i < OptNameList.size(); ++i)
	{
		if (SetEnvironmentVariableW(OptNameList[i].c_str(), OldOptValueList[i].c_str()) != TRUE)
		{
			_wperror(L"SetEnvironmentVariableW");
			return __LINE__;
		}
	}
	if (SetEnvironmentVariableW(L"PATH", OldPath) != TRUE)
	{
		_wperror(L"SetEnvironmentVariableW");
		return __LINE__;
	}

	DWORD ExitCode = 0;
	GetExitCodeProcess(pi.hProcess, &ExitCode);

	if (CodePage > 0) {
		SetConsoleCP(OldCodePage);
		SetConsoleOutputCP(OldCodePage);
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return ExitCode;
}

static DWORD GetInitPathW(HMODULE hModule, LPWSTR lpDirName, DWORD nSize)
{
	errno_t err = 0;
	WCHAR path[MAX_PATH + 1];
	GetModuleFileName(hModule, path, SIZEOF(path));

	WCHAR drive[_MAX_DRIVE];
	WCHAR dir[_MAX_DIR];
	WCHAR fname[_MAX_FNAME];
	WCHAR ext[_MAX_EXT];
	if ((err = _wsplitpath_s(path, drive, SIZEOF(drive), dir, SIZEOF(dir), fname, SIZEOF(fname), ext, SIZEOF(ext))) != 0)
	{
		_wperror(L"_wsplitpath_s");
		return 0;
	}
	if ((err = _wmakepath_s(lpDirName, nSize, drive, dir, fname, L"ini")) != 0)
	{
		_wperror(L"_wmakepath_s");
		return 0;
	}
	return lstrlenW(lpDirName);
}