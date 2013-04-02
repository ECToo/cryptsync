// CryptSync - A folder sync tool with encryption

// Copyright (C) 2012 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "CmdLineParser.h"
#include "TrayWindow.h"
#include "Ignores.h"
#include "resource.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
CPairs g_pairs;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

std::wstring GetMutexID()
{
    std::wstring t;
    CAutoGeneralHandle token;
    BOOL result = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, token.GetPointer());
    if(result)
    {
        DWORD len = 0;
        GetTokenInformation(token, TokenStatistics, NULL, 0, &len);
        if (len >= sizeof (TOKEN_STATISTICS))
        {
            std::unique_ptr<BYTE[]> data (new BYTE[len]);
            GetTokenInformation(token, TokenStatistics, data.get(), len, &len);
            LUID uid = ((PTOKEN_STATISTICS)data.get())->AuthenticationId;
            wchar_t buf[100] = {0};
            swprintf_s(buf, L"{81C34844-03AC-4DAA-865B-BC51F07F7F9E}-%08x%08x", uid.HighPart, uid.LowPart);
            t = buf;
        }
    }
    return t;
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    SetDllDirectory(L"");
    OleInitialize(NULL);
    CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    LoadLibrary(L"riched32.dll");


    CCmdLineParser parser(lpCmdLine);
    if (parser.HasVal(L"src") && parser.HasVal(L"dst"))
    {
        std::wstring src =   parser.GetVal(L"src");
        std::wstring dst =   parser.GetVal(L"dst");
        std::wstring pw  =   parser.HasVal(L"pw") ? parser.GetVal(L"pw") : L"";
        bool encnames    = !!parser.HasKey(L"encnames");
        bool mirror      = !!parser.HasKey(L"mirror");
        std::wstring ign =   parser.HasVal(L"ignore") ? parser.GetVal(L"ignore") : L"";

        CIgnores::Instance().Reload();
        if (!ign.empty())
            CIgnores::Instance().Reload(ign);

        CPairs pair;
        pair.clear();
        pair.AddPair(src, dst, pw, encnames, mirror);
        CFolderSync foldersync;
        foldersync.SyncFoldersWait(pair, parser.HasKey(L"progress") ? GetDesktopWindow() : NULL);
        return 1;
    }

    MSG msg;

    HANDLE hReloadProtection = ::CreateMutex(NULL, FALSE, GetMutexID().c_str());
    if ((!hReloadProtection) || (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        // An instance of CryptSync is already running
        CoUninitialize();
        OleUninitialize();
        return 0;
    }


    CTrayWindow trayWindow(hInstance);
    trayWindow.ShowDialogImmediately(!parser.HasKey(L"tray"));

    if (trayWindow.RegisterAndCreateWindow())
    {
        HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CryptSync));
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        return (int) msg.wParam;
    }
    CoUninitialize();
    OleUninitialize();
    return 1;
}
