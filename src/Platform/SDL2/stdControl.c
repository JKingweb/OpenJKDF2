#include "Platform/stdControl.h"

#include "Engine/sithControl.h"
#include "Win95/Window.h"
#include "stdPlatform.h"

#include <SDL.h>

#include "jk.h"

const uint8_t stdControl_aSdlToDik[256] =
{
    0,
    0,
    0,
    0,
    DIK_A,
    DIK_B,
    DIK_C,
    DIK_D,
    DIK_E,
    DIK_F,
    DIK_G,
    DIK_H,
    DIK_I,
    DIK_J,
    DIK_K,
    DIK_L,
    DIK_M,
    DIK_N,
    DIK_O,
    DIK_P,
    DIK_Q,
    DIK_R,
    DIK_S,
    DIK_T,
    DIK_U,
    DIK_V,
    DIK_W,
    DIK_X,
    DIK_Y,
    DIK_Z,

    DIK_1,
    DIK_2,
    DIK_3,
    DIK_4,
    DIK_5,
    DIK_6,
    DIK_7,
    DIK_8,
    DIK_9,
    DIK_0,

    DIK_RETURN,
    DIK_ESCAPE,
    DIK_BACK,
    DIK_TAB,
    DIK_SPACE,

    DIK_MINUS,
    DIK_EQUALS,
    DIK_LBRACKET,
    DIK_RBRACKET,
    DIK_BACKSLASH,
    0,
    DIK_SEMICOLON,
    DIK_APOSTROPHE,
    DIK_GRAVE,
    DIK_COMMA,
    DIK_PERIOD,
    DIK_SLASH,

    DIK_CAPITAL,

    DIK_F1,
    DIK_F2,
    DIK_F3,
    DIK_F4,
    DIK_F5,
    DIK_F6,
    DIK_F7,
    DIK_F8,
    DIK_F9,
    DIK_F10,
    DIK_F11,
    DIK_F12,

    0,//DIK_PRINTSCREEN,
    DIK_SCROLL,
    DIK_PAUSE,
    DIK_INSERT,
    DIK_HOME,
    DIK_PRIOR,
    DIK_DELETE,
    DIK_END,
    DIK_NEXT,
    DIK_RIGHT,
    DIK_LEFT,
    DIK_DOWN,
    DIK_UP,

    0,//DIK_NUMLOCKCLEAR,
    DIK_DIVIDE,
    DIK_MULTIPLY,
    DIK_SUBTRACT,
    DIK_ADD,
    DIK_NUMPADENTER,
    DIK_NUMPAD1,
    DIK_NUMPAD2,
    DIK_NUMPAD3,
    DIK_NUMPAD4,
    DIK_NUMPAD5,
    DIK_NUMPAD6,
    DIK_NUMPAD7,
    DIK_NUMPAD8,
    DIK_NUMPAD9,
    DIK_NUMPAD0,
    DIK_DECIMAL,

    DIK_BACKSLASH,
    0,//DIK_APPLICATION,
    DIK_POWER,
    DIK_NUMPADEQUALS,
    DIK_F13,
    DIK_F14,
    DIK_F15,
    0,//DIK_F16,
    0,//DIK_F17,
    0,//DIK_F18,
    0,//DIK_F19,
    0,//DIK_F20,
    0,//DIK_F21,
    0,//DIK_F22,
    0,//DIK_F23,
    0,//DIK_F24,
    0,//DIK_EXECUTE,
    0,//DIK_HELP,
    0,//DIK_MENU,
    0,//DIK_SELECT,
    0,//DIK_STOP,
    0,//DIK_AGAIN,
    0,//DIK_UNDO,
    0,//DIK_CUT,
    0,//DIK_COPY,
    0,//DIK_PASTE,
    0,//DIK_FIND,
    DIK_MUTE,
    DIK_VOLUMEUP,
    DIK_VOLUMEDOWN,
    0,
    0,
    0,
    DIK_NUMPADCOMMA,
    DIK_NUMPADEQUALS,

    0,//DIK_INTERNATIONAL1,
    0,//DIK_INTERNATIONAL2,
    0,//DIK_INTERNATIONAL3,
    0,//DIK_INTERNATIONAL4,
    0,//DIK_INTERNATIONAL5,
    0,//DIK_INTERNATIONAL6,
    0,//DIK_INTERNATIONAL7,
    0,//DIK_INTERNATIONAL8,
    0,//DIK_INTERNATIONAL9,
    0,//DIK_LANG1,
    0,//DIK_LANG2,
    0,//DIK_LANG3,
    0,//DIK_LANG4,
    0,//DIK_LANG5,
    0,//DIK_LANG6,
    0,//DIK_LANG7,
    0,//DIK_LANG8,
    0,//DIK_LANG9,

    0,//DIK_ALTERASE,
    0,//DIK_SYSREQ,
    0,//DIK_CANCEL,
    0,//DIK_CLEAR,
    DIK_PRIOR,
    0,//DIK_RETURN2,
    0,//DIK_SEPARATOR,
    0,//DIK_OUT,
    0,//DIK_OPER,
    0,//DIK_CLEARAGAIN,
    0,//DIK_CRSEL,
    0,//DIK_EXSEL,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,//DIK_KP_00,
    0,//DIK_KP_000,
    0,//DIK_THOUSANDSSEPARATOR,
    0,//DIK_DECIMALSEPARATOR,
    0,//DIK_CURRENCYUNIT,
    0,//DIK_CURRENCYSUBUNIT,
    0,//DIK_KP_LEFTPAREN,
    0,//DIK_KP_RIGHTPAREN,
    0,//DIK_KP_LEFTBRACE,
    0,//DIK_KP_RIGHTBRACE,
    0,//DIK_KP_TAB,
    0,//DIK_KP_BACKSPACE,
    0,//DIK_KP_A,
    0,//DIK_KP_B,
    0,//DIK_KP_C,
    0,//DIK_KP_D,
    0,//DIK_KP_E,
    0,//DIK_KP_F,
    0,//DIK_KP_XOR,
    0,//DIK_KP_POWER,
    0,//DIK_KP_PERCENT,
    0,//DIK_KP_LESS,
    0,//DIK_KP_GREATER,
    0,//DIK_KP_AMPERSAND,
    0,//DIK_KP_DBLAMPERSAND,
    0,//DIK_KP_VERTICALBAR,
    0,//DIK_KP_DBLVERTICALBAR,
    0,//DIK_KP_COLON,
    0,//DIK_KP_HASH,
    0,//DIK_KP_SPACE,
    0,//DIK_KP_AT,
    0,//DIK_KP_EXCLAM,
    0,//DIK_KP_MEMSTORE,
    0,//DIK_KP_MEMRECALL,
    0,//DIK_KP_MEMCLEAR,
    0,//DIK_KP_MEMADD,
    0,//DIK_KP_MEMSUBTRACT,
    0,//DIK_KP_MEMMULTIPLY,
    0,//DIK_KP_MEMDIVIDE,
    0,//DIK_KP_PLUSMINUS,
    0,//DIK_KP_CLEAR,
    0,//DIK_KP_CLEARENTRY,
    0,//DIK_KP_BINARY,
    0,//DIK_KP_OCTAL,
    0,//DIK_KP_DECIMAL,
    0,//DIK_KP_HEXADECIMAL,
    0,
    0,
    DIK_LCONTROL,
    DIK_LSHIFT,
    DIK_LMENU,
    0,//DIK_LGUI,
    DIK_RCONTROL,
    DIK_RSHIFT,
    DIK_RMENU,
    0,//DIK_RGUI,
};

// Added: SDL2
void stdControl_SetSDLKeydown(int keyNum, int bDown, uint32_t readTime)
{
    if (keyNum < 0 || keyNum > 256)
        return;

    stdControl_SetKeydown(stdControl_aSdlToDik[keyNum], bDown, readTime);
}

int stdControl_Startup()
{
    UINT v0; // ebp
    int v2; // ebx
    UINT v4; // eax
    int v5; // ecx
    int v6; // edi
    int v7; // eax
    double v8; // st7
    UINT v14; // [esp+40h] [ebp-1B4h]
    int *v15; // [esp+44h] [ebp-1B0h]
    int v17; // [esp+58h] [ebp-19Ch]
    int v18; // [esp+5Ch] [ebp-198h]
#if 0
    struct joyinfo_tag pji; // [esp+48h] [ebp-1ACh] BYREF
    stdControlJoystickEntry *pJoystickIter; // esi
    IDirectInputDeviceAVtbl *v9; // esi
    struct tagJOYCAPSA pjc; // [esp+60h] [ebp-194h] BYREF
#endif

    v0 = 0;
    _memset(stdControl_aInput1, 0, sizeof(int) * 284);
    _memset(stdControl_aKeyInfo, 0, sizeof(int) * 284);
    _memset(stdControl_aJoysticks, 0, sizeof(stdControlJoystickEntry) * 15);
    _memset(&stdControl_aAxisPos, 0, sizeof(stdControlAxis));

#if 0
    DirectX_DirectInputCreateA(stdGdi_GetHInstance(), 0x500u, &stdControl_ppDI, 0);
    if ( stdControl_ppDI && !stdControl_ppDI->lpVtbl->CreateDevice(stdControl_ppDI, &CLSID_GUID_SysKeyboard, &stdControl_keyboardIDirectInputDevice, 0) )
    {
        stdControl_keyboardIDirectInputDevice->lpVtbl->SetDataFormat(stdControl_keyboardIDirectInputDevice, &stdControl_stru_50E730);
        pji.wXpos = 20;
        pji.wYpos = 16;
        pji.wZpos = 0;
        pji.wButtons = 0;
        v17 = 32;
        stdControl_keyboardIDirectInputDevice->lpVtbl->SetProperty(stdControl_keyboardIDirectInputDevice, (const GUID *const)1, (LPCDIPROPHEADER)&pji);
    }
#endif

    stdControl_arr_865030[0] = 0;
    stdControl_arr_8651A8[0] = 0;
    stdControl_arr_865028[0] = 0;
    stdControl_arr_865030[1] = 0;
    stdControl_arr_8651A8[1] = 0;
    stdControl_arr_865028[1] = 0;
    v2 = 2;

#if 0
    v14 = joyGetNumDevs();
    if ( v14 >= 2 )
        v14 = 2;
    if ( v14 )
    {
        v15 = stdControl_arr_865028;
        pJoystickIter = stdControl_aJoysticks;
        do
        {
            if ( !joyGetPos(v0, &pji) && !joyGetDevCapsA(v0, &pjc, 0x194u) )
            {
                v4 = pjc.wMaxButtons;
                stdControl_arr_865030[v0] = 1;
                if ( v4 >= 0xC )
                    v4 = 12;
                v5 = pjc.wXmax;
                v6 = pjc.wXmin;
                stdControl_arr_8651A8[v0] = v4;
                pJoystickIter->flags |= 1u;
                v7 = v6 + (v5 - v6 + 1) / 2;
                pJoystickIter->uMinVal = v6;
                pJoystickIter->uMaxVal = v5;
                pJoystickIter->dwXoffs = v7;
                v18 = v5 - v7;
                v8 = (double)(v5 - v7);
                pJoystickIter->fRangeConversion = 1.0 / v8;
                if ( 0.1 == 0.0 )
                    pJoystickIter->dwYoffs = 0;
                else
                    pJoystickIter->dwYoffs = (__int64)(v8 * 0.1);
                stdControl_InitAxis(v2 - 1, pjc.wYmin, pjc.wYmax, 0.1);
                if ( (pjc.wCaps & 1) != 0 )
                    stdControl_InitAxis(v2, pjc.wZmin, pjc.wZmax, 0.1);
                if ( (pjc.wCaps & 2) != 0 )
                    stdControl_InitAxis(v2 + 1, pjc.wRmin, pjc.wRmax, 0.1);
                if ( (pjc.wCaps & 4) != 0 )
                    stdControl_InitAxis(v2 + 2, pjc.wUmin, pjc.wUmax, 0.1);
                if ( (pjc.wCaps & 8) != 0 )
                    stdControl_InitAxis(v2 + 3, pjc.wVmin, pjc.wVmax, 0.1);
                if ( (pjc.wCaps & 0x10) != 0 )
                    *v15 = 1;
            }
            ++v0;
            pJoystickIter += 6;
            v2 += 6;
            ++v15;
        }
        while ( v0 < v14 );
    }

    if ( stdControl_ppDI )
    {
        GetSystemMetrics(43);
        if ( !stdControl_ppDI->lpVtbl->CreateDevice(stdControl_ppDI, &CLSID_GUID_SysMouse, &stdControl_mouseDirectInputDevice, 0)
          && !stdControl_mouseDirectInputDevice->lpVtbl->SetDataFormat(stdControl_mouseDirectInputDevice, (LPCDIDATAFORMAT)stdControl_dword_50D710) )
        {
            v9 = stdControl_mouseDirectInputDevice->lpVtbl;
            if ( !v9->SetCooperativeLevel(stdControl_mouseDirectInputDevice, stdGdi_GetHwnd(), 5) )
            {
                pji.wXpos = 20;
                pji.wYpos = 16;
                pji.wZpos = 0;
                pji.wButtons = 0;
                v17 = 32;
                stdControl_mouseDirectInputDevice->lpVtbl->SetProperty(stdControl_mouseDirectInputDevice, (const GUID *const)1, (LPCDIPROPHEADER)&pji);
                stdControl_InitAxis(12, -250, 250, 0.0);
                stdControl_InitAxis(13, -200, 200, 0.0);
                stdControl_InitAxis(14, -20, 20, 0.0);
            }
        }
    }
#endif

    // SDL2 replacements, mouse axis
    stdControl_InitAxis(AXIS_MOUSE_X, -250, 250, 0.0);
    stdControl_InitAxis(AXIS_MOUSE_Y, -200, 200, 0.0);
    stdControl_InitAxis(AXIS_MOUSE_Z, -20, 20, 0.0);

    stdControl_Reset();

    stdControl_bStartup = 1;
    return 1;
}

void stdControl_Shutdown()
{
    stdControl_bStartup = 0;
#if 0
    if ( stdControl_mouseDirectInputDevice )
        stdControl_mouseDirectInputDevice->lpVtbl->Release(stdControl_mouseDirectInputDevice);
    if ( stdControl_keyboardIDirectInputDevice )
        stdControl_keyboardIDirectInputDevice->lpVtbl->Release(stdControl_keyboardIDirectInputDevice);
    if ( stdControl_ppDI )
        stdControl_ppDI->lpVtbl->Release(stdControl_ppDI);
#endif
}

int stdControl_Open()
{
    stdControl_bOpen = 1;
#if 0
    if ( stdControl_mouseDirectInputDevice && stdControl_bReadMouse )
    {
        stdControl_mouseDirectInputDevice->lpVtbl->Acquire(stdControl_mouseDirectInputDevice);
        ShowCursor(0);
    }
    if ( stdControl_keyboardIDirectInputDevice )
        stdControl_keyboardIDirectInputDevice->lpVtbl->Acquire(stdControl_keyboardIDirectInputDevice);
#endif
    stdControl_bControlsActive = 1;
    return 1;
}

int stdControl_Close()
{
    if ( !stdControl_bOpen )
        return 0;

#if 0
    if ( stdControl_mouseDirectInputDevice )
    {
        stdControl_mouseDirectInputDevice->lpVtbl->Unacquire(stdControl_mouseDirectInputDevice);
        ShowCursor(1);
    }
    if ( stdControl_keyboardIDirectInputDevice )
        stdControl_keyboardIDirectInputDevice->lpVtbl->Unacquire(stdControl_keyboardIDirectInputDevice);
#endif

    stdControl_bControlsActive = 0;
    stdControl_bOpen = 0;
    return 1;
}

void stdControl_Flush()
{
    uint32_t v0; // ecx
    unsigned int i; // eax
    int v2; // edx
    uint32_t v3; // ebx
    int v4; // esi
    int *v5; // ecx
    unsigned int v6; // eax
    int v7; // edx
    int v8; // edx
    int v9; // [esp+24h] [ebp-14h] BYREF
    char v10[16]; // [esp+28h] [ebp-10h] BYREF

    stdControl_curReadTime = stdPlatform_GetTimeMsec();

#if 0
    if ( stdControl_keyboardIDirectInputDevice )
    {
        v9 = -1;
        stdControl_keyboardIDirectInputDevice->lpVtbl->GetDeviceData(stdControl_keyboardIDirectInputDevice, 16, 0, (LPDWORD)&v9, 0);
        v0 = stdControl_msDelta;
        for ( i = 0; i < 256; ++i )
        {
            if ( stdControl_aKeyInfo[i] )
            {
                v2 = stdControl_aInput1[i];
                stdControl_aKeyInfo[i] = 0;
                if ( !v2 )
                    stdControl_aInput1[i] = v0;
                stdControl_aInput1[i] = stdControl_aInput1[i];
            }
        }
    }
    if ( stdControl_mouseDirectInputDevice )
    {
        v9 = -1;
        if ( stdControl_bReadMouse )
        {
            stdControl_mouseDirectInputDevice->lpVtbl->GetDeviceState(stdControl_mouseDirectInputDevice, 16, v10);
            stdControl_mouseDirectInputDevice->lpVtbl->GetDeviceData(stdControl_mouseDirectInputDevice, 16, 0, (LPDWORD)&v9, 0);
            v3 = stdControl_msDelta;
            v4 = 0;
            v5 = &stdControl_aInput1[280];
            v6 = 0;
            while ( v10[v4 + 12] )
            {
                if ( stdControl_aKeyInfo[v6 + 280] )
                {
                    if ( !v10[v4 + 12] )
                        break;
                }
                else
                {
                    v7 = stdControl_aInput2[v6 + 280];
                    stdControl_aKeyInfo[v6 + 280] = 1;
                    *v5 = 0;
                    stdControl_aInput2[v6 + 280] = v7 + 1;
                }
LABEL_19:
                ++v6;
                ++v4;
                ++v5;
                if ( v6 >= 4 )
                    return;
            }
            if ( stdControl_aKeyInfo[v6 + 280] )
            {
                v8 = *v5;
                stdControl_aKeyInfo[v6 + 280] = 0;
                if ( !v8 )
                    *v5 = v3;
                *v5 = *v5;
            }
            goto LABEL_19;
        }
    }
#endif
}

void stdControl_ToggleCursor(int a)
{
    SDL_SetRelativeMouseMode(!!a);
}

static int _cursorState = 0;

int stdControl_ShowCursor(int a)
{
    if (a)
    {
        _cursorState++;
    }
    else
    {
        _cursorState--;
    }
    return _cursorState;
}

void stdControl_ToggleMouse()
{
    if ( stdControl_bReadMouse )
    {
        stdControl_bReadMouse = 0;
        stdControl_ShowCursor(1);
    }
    else
    {
        stdControl_bReadMouse = 1;
        stdControl_ShowCursor(0);
    }
}

void stdControl_ReadControls()
{
    double khz;


    if (!stdControl_bControlsActive)
        return;

    _memset(stdControl_aInput1, 0, sizeof(int) * 284);
    stdControl_bControlsIdle = 1;
    _memset(stdControl_aInput2, 0, sizeof(int) * 284);
    stdControl_curReadTime = stdPlatform_GetTimeMsec();
    stdControl_msDelta = stdControl_curReadTime - stdControl_msLast;
    if (stdControl_msDelta != 0)
        khz = 1.0 / (double)(__int64)(stdControl_msDelta);
    else
        khz = 1.0;
    _memset(&stdControl_aAxisPos, 0, sizeof(stdControlAxis));
    stdControl_updateKHz = khz;
    stdControl_updateHz = khz * 1000.0;
    if ( !stdControl_bDisableKeyboard )
    {
        const Uint8 *state = SDL_GetKeyboardState(NULL);
        for (int i = 0; i < 256; i++)
        {
            stdControl_SetSDLKeydown(i, !!state[i], stdControl_curReadTime);
        }
        // stdControl_SetKeydown(keyNum, keyVal, timestamp)
    }
    if ( stdControl_joy_related )
    {
        for (int i = 0; i < 2; i++)
        {
            if ( !(stdControl_unk_55C828[i] && stdControl_arr_865030[i]) )
                continue;

            stdControl_unk_55C828[i] = 0;

            // TODO
            stdControl_aAxisPos.aEntries[i].dwXpos = 0;
            stdControl_aAxisPos.aEntries[i].dwYpos = 0;
            stdControl_aAxisPos.aEntries[i].dwZpos = 0;
            stdControl_aAxisPos.aEntries[i].dwUpos = 0;
            stdControl_aAxisPos.aEntries[i].dwVpos = 0;

            
            for (int j = 0; j <= 7; ++j )
            {
                stdControl_SetKeydown(j + (256 + 12*i), 0 /* button val */, stdControl_curReadTime);
            }

            // Hat?
#if 0
            if ( (pji.dwFlags & 0x200) != 0 )
            {
                v27 = pji.dwPOV;
                stdControl_SetKeydown((256 + 12*i) + 8, v27 >= 22500 && v27 <= 31500, stdControl_curReadTime);
                stdControl_SetKeydown((256 + 12*i) + 9, (v27 >= 31500 || v27 <= 4500) && v27 >= 0 && v27 != 0xFFFF, stdControl_curReadTime);
                stdControl_SetKeydown((256 + 12*i) + 10, v27 >= 4500 && v27 <= 13500, stdControl_curReadTime);
                stdControl_SetKeydown((256 + 12*i) + 11, v27 >= 13500 && v27 <= 22500, stdControl_curReadTime);
            }
#endif
        }
    }
    stdControl_ReadMouse();
    stdControl_msLast = stdControl_curReadTime;
}

void stdControl_ReadMouse()
{
    if (!stdControl_bReadMouse)
        return;

    stdControl_aAxisPos.mouseZ = 0; // TODO
    stdControl_aAxisPos.mouseX = Window_lastXRel; // TODO
    stdControl_aAxisPos.mouseY = Window_lastYRel; // TODO

    if ( stdControl_msDelta < 25 )
    {
        //stdControl_aAxisPos.mouseX = (stdControl_aAxisPos.mouseX + stdControl_dwLastMouseX) >> 1;
        //stdControl_aAxisPos.mouseY = (stdControl_aAxisPos.mouseY + stdControl_dwLastMouseY) >> 1;
    }
    stdControl_dwLastMouseX = Window_lastXRel; // TODO
    stdControl_dwLastMouseY = Window_lastYRel; // TODO

    Window_lastXRel = 0;
    Window_lastYRel = 0;

    for (int i = 0; i < 32; i++)
    {
        //stdControl_SetKeydown(268 + i, 0 /* buttonval */, stdControl_curReadTime);
    }

    for (int i = 0; i < 4; i++)
    {
        //stdControl_SetKeydown(280 + i, 0 /* buttonval */, stdControl_curReadTime);
    }

    int x,y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);

    stdControl_SetKeydown(KEY_MOUSE_B1, Window_bMouseLeft, stdControl_curReadTime);
    stdControl_SetKeydown(KEY_MOUSE_B2, Window_bMouseRight, stdControl_curReadTime);
    stdControl_SetKeydown(KEY_MOUSE_B3, !!(buttons & SDL_BUTTON_MMASK), stdControl_curReadTime);
    stdControl_SetKeydown(KEY_MOUSE_B4, !!(buttons & SDL_BUTTON_X1MASK), stdControl_curReadTime);
    //stdControl_SetKeydown(KEY_MOUSE_B5, !!(buttons & SDL_BUTTON_X2MASK), stdControl_curReadTime);
}
