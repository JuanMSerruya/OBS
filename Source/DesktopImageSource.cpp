/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#include "Main.h"


#define NUM_CAPTURE_TEXTURES 2

class DesktopImageSource : public ImageSource
{
    Texture *renderTextures[NUM_CAPTURE_TEXTURES];
    Texture *lastRendered;

    UINT     captureType;
    String   strWindow, strWindowClass;
    BOOL     bClientCapture, bCaptureMouse;
    HWND     hwndFoundWindow;

    int      width, height;
    RECT     captureRect;
    UINT     frameTime;
    int      curCaptureTexture;
    XElement *data;

public:
    DesktopImageSource(UINT frameTime, XElement *data)
    {
        traceIn(DesktopImageSource::DesktopImageSource);

        this->data = data;
        UpdateSettings();

        curCaptureTexture = 0;
        this->frameTime = frameTime;

        //-------------------------------------------------------

        traceOut;
    }

    ~DesktopImageSource()
    {
        traceIn(DesktopImageSource::~DesktopImageSource);

        for(int i=0; i<NUM_CAPTURE_TEXTURES; i++)
            delete renderTextures[i];

        traceOut;
    }

    void Preprocess()
    {
        traceIn(DesktopImageSource::Preprocess);

        Texture *captureTexture = renderTextures[curCaptureTexture];

        HDC hDC;
        if(captureTexture->GetDC(hDC))
        {
            //----------------------------------------------------------
            // capture screen

            CURSORINFO ci;
            zero(&ci, sizeof(ci));
            ci.cbSize = sizeof(ci);
            BOOL bMouseCaptured;

            bMouseCaptured = bCaptureMouse && GetCursorInfo(&ci);

            bool bWindowMinimized = false, bWindowNotFound = false;
            HWND hwndCapture = NULL;
            if(captureType == 1)
            {
                hwndCapture = FindWindow(strWindowClass, strWindow);
                if(!hwndCapture)
                {
                    if(hwndFoundWindow && IsWindow(hwndFoundWindow))
                        hwndCapture = hwndFoundWindow;
                    else
                        hwndCapture = FindWindow(strWindowClass, NULL);
                }

                hwndFoundWindow = hwndCapture;

                if(!hwndCapture)
                    bWindowNotFound = true;
                if(IsIconic(hwndCapture))
                {
                    captureTexture->ReleaseDC();
                    return;
                    //bWindowMinimized = true;
                }
            }

            HDC hCaptureDC;
            if(hwndCapture && captureType == 1 && !bClientCapture)
                hCaptureDC = GetWindowDC(hwndCapture);
            else
                hCaptureDC = GetDC(hwndCapture);

            if(bWindowMinimized || bWindowNotFound)
            {
                CTSTR lpStr;
                if(bWindowMinimized)
                    lpStr = Str("Sources.SoftwareCaptureSource.WindowMinimized");
                else if(bWindowNotFound)
                    lpStr = Str("Sources.SoftwareCaptureSource.WindowNotFound");

                RECT rc = {0, 0, width, height};
                FillRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

                HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                HFONT hfontOld = (HFONT)SelectObject(hDC, hFont);

                SetBkColor(hDC, 0x000000);
                SetTextColor(hDC, 0xFFFFFF);
                SetTextAlign(hDC, TA_CENTER);
                TextOut(hDC, width/2, height/2, lpStr, slen(lpStr));

                SelectObject(hDC, hfontOld);
            }
            else
            {
                //CAPTUREBLT causes mouse flicker.  I haven't seen anything that doesn't display without it yet, so not going to use it
                if(!BitBlt(hDC, 0, 0, width, height, hCaptureDC, captureRect.left, captureRect.top, SRCCOPY))
                {
                    int chi = GetLastError();
                    AppWarning(TEXT("Capture BitBlt failed..  just so you know"));
                }
            }

            ReleaseDC(hwndCapture, hCaptureDC);

            //----------------------------------------------------------
            // capture mouse

            if(bMouseCaptured)
            {
                if(ci.flags & CURSOR_SHOWING)
                {
                    HICON hIcon = CopyIcon(ci.hCursor);

                    if(hIcon)
                    {
                        ICONINFO ii;
                        if(GetIconInfo(hIcon, &ii))
                        {
                            POINT capturePos = {captureRect.left, captureRect.top};

                            if(captureType == 1)
                            {
                                if(bClientCapture)
                                    ClientToScreen(hwndCapture, &capturePos);
                                else
                                {
                                    RECT windowRect;
                                    GetWindowRect(hwndCapture, &windowRect);
                                    capturePos.x += windowRect.left;
                                    capturePos.y += windowRect.top;
                                }
                            }

                            int x = ci.ptScreenPos.x - int(ii.xHotspot) - capturePos.x;
                            int y = ci.ptScreenPos.y - int(ii.yHotspot) - capturePos.y;

                            DrawIcon(hDC, x, y, hIcon);

                            DeleteObject(ii.hbmColor);
                            DeleteObject(ii.hbmMask);
                        }

                        DestroyIcon(hIcon);
                    }
                }
            }

            captureTexture->ReleaseDC();
        }
        else
            AppWarning(TEXT("Failed to get DC from capture surface"));

        lastRendered = captureTexture;

        if(++curCaptureTexture == NUM_CAPTURE_TEXTURES)
            curCaptureTexture = 0;

        traceOut;
    }

    void Render(const Vect2 &pos, const Vect2 &size)
    {
        traceIn(DesktopImageSource::Render);

        EnableBlending(FALSE);
        DrawSprite(lastRendered, pos.x, pos.y, pos.x+size.x, pos.y+size.y);
        EnableBlending(TRUE);

        traceOut;
    }

    Vect2 GetSize() const
    {
        return Vect2(float(width), float(height));
    }

    void UpdateSettings()
    {
        App->EnterSceneMutex();

        for(int i=0; i<NUM_CAPTURE_TEXTURES; i++)
            delete renderTextures[i];

        captureType     = data->GetInt(TEXT("captureType"));
        strWindow       = data->GetString(TEXT("window"));
        strWindowClass  = data->GetString(TEXT("windowClass"));
        bClientCapture  = data->GetInt(TEXT("innerWindow"), 1);
        bCaptureMouse   = data->GetInt(TEXT("captureMouse"), 1);

        int x  = data->GetInt(TEXT("captureX"));
        int y  = data->GetInt(TEXT("captureY"));
        int cx = data->GetInt(TEXT("captureCX"), 32);
        int cy = data->GetInt(TEXT("captureCY"), 32);

        captureRect.left   = x;
        captureRect.top    = y;
        captureRect.right  = x+cx;
        captureRect.bottom = y+cy;

        width  = cx;
        height = cy;

        for(UINT i=0; i<NUM_CAPTURE_TEXTURES; i++)
            renderTextures[i] = CreateGDITexture(width, height);

        App->LeaveSceneMutex();
    }
};

ImageSource* STDCALL CreateDesktopSource(XElement *data)
{
    if(!data)
        return NULL;

    return new DesktopImageSource(App->GetFrameTime(), data);
}

void RefreshWindowList(HWND hwndCombobox, StringList &classList)
{
    SendMessage(hwndCombobox, CB_RESETCONTENT, 0, 0);
    classList.Clear();

    HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
    do
    {
        if(IsWindowVisible(hwndCurrent) && !IsIconic(hwndCurrent))
        {
            RECT clientRect;
            GetClientRect(hwndCurrent, &clientRect);

            HWND hwndParent = GetParent(hwndCurrent);

            DWORD exStyles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_EXSTYLE);
            DWORD styles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_STYLE);

            if( (exStyles & WS_EX_TOOLWINDOW) == 0 && (styles & WS_CHILD) == 0 &&
                clientRect.bottom != 0 && clientRect.right != 0 && hwndParent == NULL)
            {
                String strWindowName;
                strWindowName.SetLength(GetWindowTextLength(hwndCurrent));
                GetWindowText(hwndCurrent, strWindowName, strWindowName.Length()+1);

                //-------

                DWORD processID;
                GetWindowThreadProcessId(hwndCurrent, &processID);
                if(processID == GetCurrentProcessId())
                    continue;

                TCHAR fileName[MAX_PATH+1];
                scpy(fileName, TEXT("unknown"));

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
                if(hProcess)
                {
                    DWORD dwSize = MAX_PATH;
                    QueryFullProcessImageName(hProcess, 0, fileName, &dwSize);
                    CloseHandle(hProcess);
                }

                //-------

                String strFileName = fileName;
                strFileName.FindReplace(TEXT("\\"), TEXT("/"));

                String strText;
                strText << TEXT("[") << GetPathFileName(strFileName) << TEXT("]: ") << strWindowName;

                int id = (int)SendMessage(hwndCombobox, CB_ADDSTRING, 0, (LPARAM)strWindowName.Array());
                SendMessage(hwndCombobox, CB_SETITEMDATA, id, (LPARAM)hwndCurrent);

                String strClassName;
                strClassName.SetLength(256);
                GetClassName(hwndCurrent, strClassName.Array(), 255);
                strClassName.SetLength(slen(strClassName));

                classList << strClassName;
            }
        }
    } while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));
}

struct ConfigDesktopSourceInfo
{
    XElement *data;
    StringList strClasses;
};

void SetDesktopCaptureType(HWND hwnd, UINT type)
{
    SendMessage(GetDlgItem(hwnd, IDC_MONITORCAPTURE), BM_SETCHECK, type == 0 ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE),  BM_SETCHECK, type == 1 ? BST_CHECKED : BST_UNCHECKED, 0);

    EnableWindow(GetDlgItem(hwnd, IDC_MONITOR),     type == 0);

    EnableWindow(GetDlgItem(hwnd, IDC_WINDOW),      type == 1);
    EnableWindow(GetDlgItem(hwnd, IDC_REFRESH),     type == 1);
    EnableWindow(GetDlgItem(hwnd, IDC_OUTERWINDOW), type == 1);
    EnableWindow(GetDlgItem(hwnd, IDC_INNERWINDOW), type == 1);
}

void SelectTargetWindow(HWND hwnd)
{
    HWND hwndWindowList = GetDlgItem(hwnd, IDC_WINDOW);
    UINT windowID = (UINT)SendMessage(hwndWindowList, CB_GETCURSEL, 0, 0);
    if(windowID == CB_ERR)
        return;

    String strWindow = GetCBText(hwndWindowList, windowID);

    ConfigDesktopSourceInfo *info = (ConfigDesktopSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

    HWND hwndTarget = FindWindow(info->strClasses[windowID], strWindow);
    if(!hwndTarget)
    {
        HWND hwndLastKnownHWND = (HWND)SendMessage(hwndWindowList, CB_GETITEMDATA, windowID, 0);
        if(IsWindow(hwndLastKnownHWND))
            hwndTarget = hwndLastKnownHWND;
        else
            hwndTarget = FindWindow(info->strClasses[windowID], NULL);
    }

    if(!hwndTarget)
        return;

    //------------------------------------------

    BOOL bInnerWindow = SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_GETCHECK, 0, 0) == BST_CHECKED;

    RECT rc;
    if(bInnerWindow)
        GetClientRect(hwndTarget, &rc);
    else
    {
        GetWindowRect(hwndTarget, &rc);

        rc.bottom -= rc.top;
        rc.right -= rc.left;
        rc.left = rc.top = 0;
    }

    //------------------------------------------

    rc.bottom -= rc.top;
    rc.right -= rc.left;

    if(rc.right < 32)
        rc.right = 32;
    if(rc.bottom < 32)
        rc.bottom = 32;

    SetWindowText(GetDlgItem(hwnd, IDC_POSX),  IntString(rc.left).Array());
    SetWindowText(GetDlgItem(hwnd, IDC_POSY),  IntString(rc.top).Array());
    SetWindowText(GetDlgItem(hwnd, IDC_SIZEX), IntString(rc.right).Array());
    SetWindowText(GetDlgItem(hwnd, IDC_SIZEY), IntString(rc.bottom).Array());
}

struct RegionWindowData
{
    HWND hwndConfigDialog, hwndCaptureWindow;
    POINT startPos;
    RECT captureRect;
    bool bMovingWindow;
    bool bInnerWindowRegion;
};

RegionWindowData regionWindowData;

#define CAPTUREREGIONCLASS TEXT("CaptureRegionThingy")

LRESULT WINAPI RegionWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_MOUSEMOVE)
    {
        RECT client;
        GetClientRect(hwnd, &client);

        POINT pos;
        pos.x = (long)(short)LOWORD(lParam);
        pos.y = (long)(short)HIWORD(lParam);

        if(regionWindowData.bMovingWindow)
        {
            RECT rc;
            GetWindowRect(hwnd, &rc);

            POINT posOffset = {pos.x-regionWindowData.startPos.x, pos.y-regionWindowData.startPos.y};
            POINT newPos = {rc.left+posOffset.x, rc.top+posOffset.y};
            SIZE windowSize = {rc.right-rc.left, rc.bottom-rc.top};

            if(regionWindowData.hwndCaptureWindow)
            {
                if(newPos.x < regionWindowData.captureRect.left)
                    newPos.x = regionWindowData.captureRect.left;
                if(newPos.y < regionWindowData.captureRect.top)
                    newPos.y = regionWindowData.captureRect.top;

                POINT newBottomRight = {rc.right+posOffset.x, rc.bottom+posOffset.y};

                if(newBottomRight.x > regionWindowData.captureRect.right)
                    newPos.x -= (newBottomRight.x-regionWindowData.captureRect.right);
                if(newBottomRight.y > regionWindowData.captureRect.bottom)
                    newPos.y -= (newBottomRight.y-regionWindowData.captureRect.bottom);
            }

            SetWindowPos(hwnd, NULL, newPos.x, newPos.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
        }
        else
        {
            BOOL bLeft   = (pos.x < 6);
            BOOL bTop    = (pos.y < 6);
            BOOL bRight  = (!bLeft && (pos.x > client.right-6));
            BOOL bBottom = (!bTop  && (pos.y > client.bottom-6));

            if(bLeft)
            {
                if(bTop)
                    SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
                else if(bBottom)
                    SetCursor(LoadCursor(NULL, IDC_SIZENESW));
                else
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            }
            else if(bRight)
            {
                if(bTop)
                    SetCursor(LoadCursor(NULL, IDC_SIZENESW));
                else if(bBottom)
                    SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
                else
                    SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            }
            else if(bTop || bBottom)
                SetCursor(LoadCursor(NULL, IDC_SIZENS));
        }

        return 0;
    }
    else if(message == WM_LBUTTONDOWN)
    {
        RECT client;
        GetClientRect(hwnd, &client);

        POINT pos;
        pos.x = (long)LOWORD(lParam);
        pos.y = (long)HIWORD(lParam);

        BOOL bLeft   = (pos.x < 6);
        BOOL bTop    = (pos.y < 6);
        BOOL bRight  = (!bLeft && (pos.x > client.right-6));
        BOOL bBottom = (!bTop  && (pos.y > client.bottom-6));

        ClientToScreen(hwnd, &pos);

        POINTS newPos;
        newPos.x = (SHORT)pos.x;
        newPos.y = (SHORT)pos.y;

        SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);

        if(bLeft)
        {
            if(bTop)
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOPLEFT, (LPARAM)&newPos);
            else if(bBottom)
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOMLEFT, (LPARAM)&newPos);
            else
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTLEFT, (LPARAM)&newPos);
        }
        else if(bRight)
        {
            if(bTop)
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOPRIGHT, (LPARAM)&newPos);
            else if(bBottom)
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOMRIGHT, (LPARAM)&newPos);
            else
                SendMessage(hwnd, WM_NCLBUTTONDOWN, HTRIGHT, (LPARAM)&newPos);
        }
        else if(bTop)
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTTOP, (LPARAM)&newPos);
        else if(bBottom)
            SendMessage(hwnd, WM_NCLBUTTONDOWN, HTBOTTOM, (LPARAM)&newPos);
        else
        {
            regionWindowData.bMovingWindow = true; //can't use HTCAPTION -- if holding down long enough, other windows minimize

            pos.x = (long)(short)LOWORD(lParam);
            pos.y = (long)(short)HIWORD(lParam);
            mcpy(&regionWindowData.startPos, &pos, sizeof(pos));
            SetCapture(hwnd);
        }

        return 0;
    }
    else if(message == WM_LBUTTONUP)
    {
        if(regionWindowData.bMovingWindow)
        {
            regionWindowData.bMovingWindow = false;
            ReleaseCapture();
        }
    }
    else if(message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        if(hDC)
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            //-----------------------------------------

            HPEN oldPen = (HPEN)SelectObject(hDC, GetStockObject(BLACK_PEN));

            MoveToEx(hDC, 0, 0, NULL);
            LineTo(hDC, clientRect.right-1, 0);
            LineTo(hDC, clientRect.right-1, clientRect.bottom-1);
            LineTo(hDC, 0, clientRect.bottom-1);
            LineTo(hDC, 0, 0);

            MoveToEx(hDC, 5, 5, NULL);
            LineTo(hDC, clientRect.right-6, 5);
            LineTo(hDC, clientRect.right-6, clientRect.bottom-6);
            LineTo(hDC, 5, clientRect.bottom-6);
            LineTo(hDC, 5, 5);

            SelectObject(hDC, oldPen);

            //-----------------------------------------

            CTSTR lpStr = Str("Sources.SoftwareCaptureSource.RegionWindowText");

            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HFONT hfontOld = (HFONT)SelectObject(hDC, hFont);

            SetTextAlign(hDC, TA_CENTER);
            TextOut(hDC, clientRect.right/2, clientRect.bottom/2, lpStr, slen(lpStr));

            //-----------------------------------------

            SelectObject(hDC, hfontOld);

            EndPaint(hwnd, &ps);
        }
    }
    else if(message == WM_KEYDOWN)
    {
        if(wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == 'Q')
            DestroyWindow(hwnd);
    }
    else if(message == WM_SIZING)
    {
        RECT &rc = *(RECT*)lParam;

        bool bTop  = wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_TOP;
        bool bLeft = wParam == WMSZ_TOPLEFT || wParam == WMSZ_LEFT     || wParam == WMSZ_BOTTOMLEFT;

        if(bLeft)
        {
            if(rc.right-rc.left < 32)
                rc.left = rc.right-32;
        }
        else
        {
            if(rc.right-rc.left < 32)
                rc.right = rc.left+32;
        }

        if(bTop)
        {
            if(rc.bottom-rc.top < 32)
                rc.top = rc.bottom-32;
        }
        else
        {
            if(rc.bottom-rc.top < 32)
                rc.bottom = rc.top+32;
        }

        if(regionWindowData.hwndCaptureWindow)
        {
            if(rc.left < regionWindowData.captureRect.left)
                rc.left = regionWindowData.captureRect.left;
            if(rc.top < regionWindowData.captureRect.top)
                rc.top = regionWindowData.captureRect.top;

            if(rc.right > regionWindowData.captureRect.right)
                rc.right = regionWindowData.captureRect.right;
            if(rc.bottom > regionWindowData.captureRect.bottom)
                rc.bottom = regionWindowData.captureRect.bottom;
        }

        return TRUE;
    }
    else if(message == WM_SIZE || message == WM_MOVE)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);

        if(rc.right-rc.left < 32)
            rc.right = rc.left+32;
        if(rc.bottom-rc.top < 32)
            rc.bottom = rc.top+32;

        SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_SIZEX), IntString(rc.right-rc.left).Array());
        SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_SIZEY), IntString(rc.bottom-rc.top).Array());

        if(regionWindowData.hwndCaptureWindow)
        {
            rc.left -= regionWindowData.captureRect.left;
            rc.top  -= regionWindowData.captureRect.top;
        }

        SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_POSX),  IntString(rc.left).Array());
        SetWindowText(GetDlgItem(regionWindowData.hwndConfigDialog, IDC_POSY),  IntString(rc.top).Array());

        if(message == WM_SIZE)
            RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT|RDW_ERASE|RDW_INVALIDATE);
    }
    else if(message == WM_ACTIVATE)
    {
        if(wParam == WA_INACTIVE)
            DestroyWindow(hwnd);
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigDesktopSourceProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_INITDIALOG:
            {
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
                LocalizeWindow(hwnd);

                //--------------------------------------------

                HWND hwndToolTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    hwnd, NULL, hinstMain, NULL);

                TOOLINFO ti;
                zero(&ti, sizeof(ti));
                ti.cbSize = sizeof(ti);
                ti.uFlags = TTF_SUBCLASS|TTF_IDISHWND;
                ti.hwnd = hwnd;

                SendMessage(hwndToolTip, TTM_SETMAXTIPWIDTH, 0, 500);
                SendMessage(hwndToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 8000);

                //-----------------------------------------------------

                ConfigDesktopSourceInfo *info = (ConfigDesktopSourceInfo*)lParam;
                XElement *data = info->data;

                HWND hwndTemp = GetDlgItem(hwnd, IDC_MONITOR);
                for(UINT i=0; i<App->NumMonitors(); i++)
                    SendMessage(hwndTemp, CB_ADDSTRING, 0, (LPARAM)UIntString(i+1).Array());
                SendMessage(hwndTemp, CB_SETCURSEL, 0, 0);

                UINT captureType = data->GetInt(TEXT("captureType"), 0);
                SetDesktopCaptureType(hwnd, captureType);

                //-----------------------------------------------------

                hwndTemp = GetDlgItem(hwnd, IDC_MONITOR);
                UINT monitor = data->GetInt(TEXT("monitor"));
                SendMessage(hwndTemp, CB_SETCURSEL, monitor, 0);

                ti.lpszText = (LPWSTR)Str("Sources.SoftwareCaptureSource.MonitorCaptureTooltip");
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_MONITORCAPTURE);
                SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

                //-----------------------------------------------------

                HWND hwndWindowList = GetDlgItem(hwnd, IDC_WINDOW);

                ti.lpszText = (LPWSTR)Str("Sources.SoftwareCaptureSource.WindowCaptureTooltip");
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_WINDOWCAPTURE);
                SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

                CTSTR lpWindowName = data->GetString(TEXT("window"));
                CTSTR lpWindowClass = data->GetString(TEXT("windowClass"));
                BOOL bInnerWindow = (BOOL)data->GetInt(TEXT("innerWindow"), 1);

                RefreshWindowList(hwndWindowList, info->strClasses);

                UINT windowID = 0;
                if(lpWindowName)
                    windowID = (UINT)SendMessage(hwndWindowList, CB_FINDSTRINGEXACT, -1, (LPARAM)lpWindowName);

                bool bFoundWindow = (windowID != CB_ERR);
                if(!bFoundWindow)
                    windowID = 0;

                SendMessage(hwndWindowList, CB_SETCURSEL, windowID, 0);

                if(bInnerWindow)
                    SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_SETCHECK, BST_CHECKED, 0);
                else
                    SendMessage(GetDlgItem(hwnd, IDC_OUTERWINDOW), BM_SETCHECK, BST_CHECKED, 0);

                //-----------------------------------------------------

                bool bMouseCapture = data->GetInt(TEXT("captureMouse"), 1) != FALSE;
                SendMessage(GetDlgItem(hwnd, IDC_CAPTUREMOUSE), BM_SETCHECK, (bMouseCapture) ? BST_CHECKED : BST_UNCHECKED, 0);

                //-----------------------------------------------------

                bool bRegion = data->GetInt(TEXT("regionCapture")) != FALSE;
                if(!bFoundWindow)
                    bRegion = false;

                SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_SETCHECK, (bRegion) ? BST_CHECKED : BST_UNCHECKED, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_POSX),            bRegion);
                EnableWindow(GetDlgItem(hwnd, IDC_POSY),            bRegion);
                EnableWindow(GetDlgItem(hwnd, IDC_SIZEX),           bRegion);
                EnableWindow(GetDlgItem(hwnd, IDC_SIZEY),           bRegion);
                EnableWindow(GetDlgItem(hwnd, IDC_SELECTREGION),    bRegion);

                int posX = 0, posY = 0, sizeX = 32, sizeY = 32;
                if(!data->GetBaseItem(TEXT("captureX")) || !bFoundWindow)
                {
                    if(captureType == 0)
                        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MONITOR, 0), 0);
                    else if(captureType == 1)
                        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);
                }
                else
                {
                    posX  = data->GetInt(TEXT("captureX"));
                    posY  = data->GetInt(TEXT("captureY"));
                    sizeX = data->GetInt(TEXT("captureCX"), 32);
                    sizeY = data->GetInt(TEXT("captureCY"), 32);

                    if(sizeX < 32)
                        sizeX = 32;
                    if(sizeY < 32)
                        sizeY = 32;

                    SetWindowText(GetDlgItem(hwnd, IDC_POSX),  IntString(posX).Array());
                    SetWindowText(GetDlgItem(hwnd, IDC_POSY),  IntString(posY).Array());
                    SetWindowText(GetDlgItem(hwnd, IDC_SIZEX), IntString(sizeX).Array());
                    SetWindowText(GetDlgItem(hwnd, IDC_SIZEY), IntString(sizeY).Array());
                }

                ti.lpszText = (LPWSTR)Str("Sources.SoftwareCaptureSource.SelectRegionTooltip");
                ti.uId = (UINT_PTR)GetDlgItem(hwnd, IDC_SELECTREGION);
                SendMessage(hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);

                return TRUE;
            }

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_MONITORCAPTURE:
                    SetDesktopCaptureType(hwnd, 0);
                    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MONITOR, 0), 0);
                    break;

                case IDC_WINDOWCAPTURE:
                    SetDesktopCaptureType(hwnd, 1);
                    PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);
                    break;

                case IDC_REGIONCAPTURE:
                    {
                        UINT captureType = (SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                        BOOL bRegion = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        EnableWindow(GetDlgItem(hwnd, IDC_POSX),            bRegion);
                        EnableWindow(GetDlgItem(hwnd, IDC_POSY),            bRegion);
                        EnableWindow(GetDlgItem(hwnd, IDC_SIZEX),           bRegion);
                        EnableWindow(GetDlgItem(hwnd, IDC_SIZEY),           bRegion);
                        EnableWindow(GetDlgItem(hwnd, IDC_SELECTREGION),    bRegion);

                        if(!bRegion)
                        {
                            if(captureType == 0)
                                PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MONITOR, 0), 0);
                            else
                                PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW, 0), 0);
                        }
                        break;
                    }

                case IDC_SELECTREGION:
                    {
                        UINT posX, posY, sizeX, sizeY;
                        posX  = GetEditText(GetDlgItem(hwnd, IDC_POSX)).ToInt();
                        posY  = GetEditText(GetDlgItem(hwnd, IDC_POSY)).ToInt();
                        sizeX = GetEditText(GetDlgItem(hwnd, IDC_SIZEX)).ToInt();
                        sizeY = GetEditText(GetDlgItem(hwnd, IDC_SIZEY)).ToInt();

                        //--------------------------------------------

                        UINT captureType = (SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                        BOOL bRegion = (SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED);

                        if(bRegion && captureType == 1)
                        {
                            ConfigDesktopSourceInfo *info = (ConfigDesktopSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

                            UINT windowID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_WINDOW), CB_GETCURSEL, 0, 0);
                            if(windowID == CB_ERR) windowID = 0;

                            String strWindow = GetCBText(GetDlgItem(hwnd, IDC_WINDOW), windowID);

                            regionWindowData.hwndCaptureWindow = FindWindow(info->strClasses[windowID], strWindow);
                            if(!regionWindowData.hwndCaptureWindow)
                            {
                                HWND hwndLastKnownHWND = (HWND)SendMessage(GetDlgItem(hwnd, IDC_WINDOW), CB_GETITEMDATA, windowID, 0);
                                if(IsWindow(hwndLastKnownHWND))
                                    regionWindowData.hwndCaptureWindow = hwndLastKnownHWND;
                                else
                                    regionWindowData.hwndCaptureWindow = FindWindow(info->strClasses[windowID], NULL);
                            }

                            if(!regionWindowData.hwndCaptureWindow)
                            {
                                MessageBox(hwnd, Str("Sources.SoftwareCaptureSource.WindowNotFound"), NULL, 0);
                                break;
                            }
                            else if(IsIconic(regionWindowData.hwndCaptureWindow))
                            {
                                MessageBox(hwnd, Str("Sources.SoftwareCaptureSource.WindowMinimized"), NULL, 0);
                                break;
                            }

                            regionWindowData.bInnerWindowRegion = SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_GETCHECK, 0, 0) == BST_CHECKED;

                            if(regionWindowData.bInnerWindowRegion)
                            {
                                GetClientRect(regionWindowData.hwndCaptureWindow, &regionWindowData.captureRect);
                                ClientToScreen(regionWindowData.hwndCaptureWindow, (POINT*)&regionWindowData.captureRect);
                                ClientToScreen(regionWindowData.hwndCaptureWindow, (POINT*)&regionWindowData.captureRect.right);
                            }
                            else
                                GetWindowRect(regionWindowData.hwndCaptureWindow, &regionWindowData.captureRect);

                            posX += regionWindowData.captureRect.left;
                            posY += regionWindowData.captureRect.top;
                        }
                        else
                            regionWindowData.hwndCaptureWindow = NULL;

                        //--------------------------------------------

                        regionWindowData.hwndConfigDialog = hwnd;
                        CreateWindow(CAPTUREREGIONCLASS, NULL, WS_POPUP|WS_VISIBLE, posX, posY, sizeX, sizeY, hwnd, NULL, hinstMain, NULL);
                        break;
                    }

                case IDC_MONITOR:
                    {
                        UINT id = (UINT)SendMessage(GetDlgItem(hwnd, IDC_MONITOR), CB_GETCURSEL, 0, 0);
                        const MonitorInfo &monitor = App->GetMonitor(id);

                        UINT x, y, cx, cy;
                        x  = monitor.rect.left;
                        y  = monitor.rect.top;
                        cx = monitor.rect.right-monitor.rect.left;
                        cy = monitor.rect.bottom-monitor.rect.top;

                        if(cx < 32)
                            cx = 32;
                        if(cy < 32)
                            cy = 32;

                        SetWindowText(GetDlgItem(hwnd, IDC_POSX),  IntString(x).Array());
                        SetWindowText(GetDlgItem(hwnd, IDC_POSY),  IntString(y).Array());
                        SetWindowText(GetDlgItem(hwnd, IDC_SIZEX), IntString(cx).Array());
                        SetWindowText(GetDlgItem(hwnd, IDC_SIZEY), IntString(cy).Array());
                        break;
                    }

                case IDC_WINDOW:
                case IDC_OUTERWINDOW:
                case IDC_INNERWINDOW:
                    SelectTargetWindow(hwnd);
                    break;

                case IDC_REFRESH:
                    {
                        ConfigDesktopSourceInfo *info = (ConfigDesktopSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        XElement *data = info->data;

                        CTSTR lpWindowName = data->GetString(TEXT("window"));

                        HWND hwndWindowList = GetDlgItem(hwnd, IDC_WINDOW);
                        RefreshWindowList(hwndWindowList, info->strClasses);

                        UINT windowID = 0;
                        if(lpWindowName)
                            windowID = (UINT)SendMessage(hwndWindowList, CB_FINDSTRINGEXACT, -1, (LPARAM)lpWindowName);

                        if(windowID != CB_ERR)
                            SendMessage(hwndWindowList, CB_SETCURSEL, windowID, 0);
                        else
                        {
                            SendMessage(hwndWindowList, CB_INSERTSTRING, 0, (LPARAM)lpWindowName);
                            SendMessage(hwndWindowList, CB_SETCURSEL, 0, 0);
                        }
                        break;
                    }

                case IDOK:
                    {
                        UINT captureType = 0;
                        if(SendMessage(GetDlgItem(hwnd, IDC_MONITORCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                            captureType = 0;
                        else if(SendMessage(GetDlgItem(hwnd, IDC_WINDOWCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                            captureType = 1;

                        BOOL bRegion = (SendMessage(GetDlgItem(hwnd, IDC_REGIONCAPTURE), BM_GETCHECK, 0, 0) == BST_CHECKED);

                        UINT monitorID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_MONITOR), CB_GETCURSEL, 0, 0);
                        if(monitorID == CB_ERR) monitorID = 0;

                        UINT windowID = (UINT)SendMessage(GetDlgItem(hwnd, IDC_WINDOW), CB_GETCURSEL, 0, 0);
                        if(windowID == CB_ERR) windowID = 0;

                        String strWindow = GetCBText(GetDlgItem(hwnd, IDC_WINDOW), windowID);

                        BOOL bInnerWindow = SendMessage(GetDlgItem(hwnd, IDC_INNERWINDOW), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        UINT posX, posY, sizeX, sizeY;
                        posX  = GetEditText(GetDlgItem(hwnd, IDC_POSX)).ToInt();
                        posY  = GetEditText(GetDlgItem(hwnd, IDC_POSY)).ToInt();
                        sizeX = GetEditText(GetDlgItem(hwnd, IDC_SIZEX)).ToInt();
                        sizeY = GetEditText(GetDlgItem(hwnd, IDC_SIZEY)).ToInt();

                        if(sizeX < 32)
                            sizeX = 32;
                        if(sizeY < 32)
                            sizeY = 32;

                        BOOL bCaptureMouse = SendMessage(GetDlgItem(hwnd, IDC_CAPTUREMOUSE), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        //---------------------------------

                        ConfigDesktopSourceInfo *info = (ConfigDesktopSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        XElement *data = info->data;

                        data->SetInt(TEXT("captureType"),   captureType);

                        data->SetInt(TEXT("monitor"),       monitorID);

                        data->SetString(TEXT("window"),     strWindow);
                        data->SetString(TEXT("windowClass"),info->strClasses[windowID]);
                        data->SetInt(TEXT("innerWindow"),   bInnerWindow);

                        data->SetInt(TEXT("regionCapture"), bRegion);

                        data->SetInt(TEXT("captureMouse"),  bCaptureMouse);

                        data->SetInt(TEXT("captureX"),      posX);
                        data->SetInt(TEXT("captureY"),      posY);
                        data->SetInt(TEXT("captureCX"),     sizeX);
                        data->SetInt(TEXT("captureCY"),     sizeY);
                    }

                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
    }

    return FALSE;
}

bool bMadeCaptureRegionClass = false;

bool STDCALL ConfigureDesktopSource(XElement *element, bool bInitialize)
{
    if(!bMadeCaptureRegionClass)
    {
        WNDCLASS wc;
        zero(&wc, sizeof(wc));
        wc.hInstance = hinstMain;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = CAPTUREREGIONCLASS;
        wc.lpfnWndProc = (WNDPROC)RegionWindowProc;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        RegisterClass(&wc);

        bMadeCaptureRegionClass = true;
    }

    if(!element)
    {
        AppWarning(TEXT("ConfigureDesktopSource: NULL element"));
        return false;
    }

    XElement *data = element->GetElement(TEXT("data"));
    if(!data)
        data = element->CreateElement(TEXT("data"));

    ConfigDesktopSourceInfo info;
    info.data = data;

    if(DialogBoxParam(hinstMain, MAKEINTRESOURCE(IDD_CONFIGUREDESKTOPSOURCE), hwndMain, ConfigDesktopSourceProc, (LPARAM)&info) == IDOK)
    {
        if(bInitialize)
        {
            element->SetInt(TEXT("cx"), data->GetInt(TEXT("captureCX")));
            element->SetInt(TEXT("cy"), data->GetInt(TEXT("captureCY")));
        }
        return true;
    }

    return false;
}