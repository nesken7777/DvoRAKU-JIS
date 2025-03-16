use std::{
    ffi::c_void,
    io::Write,
    ptr::NonNull,
    sync::{
        OnceLock,
        atomic::{AtomicPtr, Ordering},
    },
};

use fxhash::FxHashMap;
use windows::{
    Win32::{
        Foundation::{HWND, LPARAM, LRESULT, WPARAM},
        System::LibraryLoader::GetModuleHandleW,
        UI::{
            Input::KeyboardAndMouse::{
                INPUT, INPUT_0, INPUT_KEYBOARD, KEYBDINPUT, KEYEVENTF_KEYUP, SendInput,
                VIRTUAL_KEY, VK_0, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9, VK_A,
                VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N, VK_O,
                VK_OEM_1, VK_OEM_2, VK_OEM_3, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_7, VK_OEM_102,
                VK_OEM_COMMA, VK_OEM_MINUS, VK_OEM_PERIOD, VK_OEM_PLUS, VK_P, VK_Q, VK_R, VK_S,
                VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,
            },
            WindowsAndMessaging::{
                CallNextHookEx, CreateWindowExW, DefWindowProcW, DispatchMessageW, GetMessageW,
                HC_ACTION, HHOOK, HWND_MESSAGE, KBDLLHOOKSTRUCT, LLKHF_INJECTED, LLKHF_UP, MSG,
                PostQuitMessage, RegisterClassW, SetWindowsHookExW, TranslateMessage,
                UnhookWindowsHookEx, WH_KEYBOARD_LL, WM_CREATE, WM_DESTROY, WNDCLASSW, WS_BORDER,
                WS_EX_NOACTIVATE, WS_EX_TOOLWINDOW, WS_EX_TOPMOST, WS_POPUP,
            },
        },
    },
    core::{PCWSTR, w},
};

const CLASS_NAME: PCWSTR = w!("DvoRAKU");
static HHOOK_PTR: OnceLock<AtomicPtr<c_void>> = OnceLock::new();

const KEYMAP: [(u16, u16); 48] = [
    (VK_1.0, VK_1.0),
    (VK_2.0, VK_2.0),
    (VK_3.0, VK_3.0),
    (VK_4.0, VK_4.0),
    (VK_5.0, VK_5.0),
    (VK_6.0, VK_6.0),
    (VK_7.0, VK_7.0),
    (VK_8.0, VK_8.0),
    (VK_9.0, VK_9.0),
    (VK_0.0, VK_0.0),
    (VK_OEM_MINUS.0, VK_X.0),
    (VK_OEM_7.0, VK_C.0),
    (VK_OEM_5.0, VK_OEM_5.0), // (円マーク)
    (VK_Q.0, VK_OEM_3.0),     // `@`
    (VK_W.0, VK_OEM_4.0),     // `[`
    (VK_E.0, VK_OEM_6.0),     // `]`
    (VK_R.0, VK_OEM_PLUS.0),  // `;`
    (VK_T.0, VK_OEM_1.0),     // `:`
    (VK_Y.0, VK_Y.0),
    (VK_U.0, VK_D.0),
    (VK_I.0, VK_G.0),
    (VK_O.0, VK_F.0),
    (VK_P.0, VK_P.0),
    (VK_OEM_3.0, VK_B.0),
    (VK_OEM_4.0, VK_V.0),
    (VK_A.0, VK_A.0),
    (VK_S.0, VK_O.0),
    (VK_D.0, VK_E.0),
    (VK_F.0, VK_U.0),
    (VK_G.0, VK_I.0),
    (VK_H.0, VK_H.0),
    (VK_J.0, VK_T.0),
    (VK_K.0, VK_K.0),
    (VK_L.0, VK_R.0),
    (VK_OEM_PLUS.0, VK_S.0),
    (VK_OEM_1.0, VK_Q.0),
    (VK_OEM_6.0, VK_L.0),
    (VK_Z.0, VK_OEM_MINUS.0),  // `-`
    (VK_X.0, VK_OEM_COMMA.0),  // `,`
    (VK_C.0, VK_OEM_PERIOD.0), // `.`
    (VK_V.0, VK_OEM_2.0),      // `/`
    (VK_B.0, VK_OEM_102.0),    // `\`
    (VK_N.0, VK_N.0),
    (VK_M.0, VK_M.0),
    (VK_OEM_COMMA.0, VK_W.0),
    (VK_OEM_PERIOD.0, VK_J.0),
    (VK_OEM_2.0, VK_Z.0),
    (VK_OEM_102.0, VK_OEM_7.0), // `^`
];

fn send_key_down(key_code: VIRTUAL_KEY) -> VIRTUAL_KEY {
    unsafe {
        let input = INPUT {
            r#type: INPUT_KEYBOARD,
            Anonymous: INPUT_0 {
                ki: KEYBDINPUT {
                    wVk: key_code,
                    ..Default::default()
                },
            },
        };
        SendInput(&[input], size_of::<INPUT>().try_into().unwrap());
        key_code
    }
}

fn send_key_up(key_code: VIRTUAL_KEY) -> VIRTUAL_KEY {
    unsafe {
        let input = INPUT {
            r#type: INPUT_KEYBOARD,
            Anonymous: INPUT_0 {
                ki: KEYBDINPUT {
                    wVk: key_code,
                    dwFlags: KEYEVENTF_KEYUP,
                    ..Default::default()
                },
            },
        };
        SendInput(&[input], size_of::<INPUT>().try_into().unwrap());
        key_code
    }
}

extern "system" fn hook_proc(ncode: i32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    unsafe {
        let keymap = FxHashMap::from_iter(KEYMAP);
        if ncode != HC_ACTION.try_into().unwrap() {
            return CallNextHookEx(None, ncode, wparam, lparam);
        }
        let key_ = *(&lparam as *const LPARAM).cast::<*mut KBDLLHOOKSTRUCT>();
        let key = NonNull::new(key_).unwrap().as_ref();
        let key_code = VIRTUAL_KEY(key.vkCode.try_into().unwrap());
        let key_flags = key.flags;
        let is_key_up = key_flags.contains(LLKHF_UP);
        if is_key_up {
            print!("KeyUp");
            let _ = std::io::stdout().flush();
        } else {
            print!("KeyDown");
            let _ = std::io::stdout().flush();
        }
        let is_injected = key_flags.contains(LLKHF_INJECTED);
        if is_injected {
            println!("injected");
        } else {
            println!("detected");
        }
        if is_injected {
            return CallNextHookEx(None, ncode, wparam, lparam);
        }
        match keymap.get(&key_code.0) {
            Some(mapped_code) => {
                if is_key_up {
                    send_key_up(VIRTUAL_KEY(*mapped_code));
                    LRESULT(1)
                } else {
                    send_key_down(VIRTUAL_KEY(*mapped_code));
                    LRESULT(1)
                }
            }
            None => CallNextHookEx(None, ncode, wparam, lparam),
        }
    }
}

fn hook_start() {
    unsafe {
        let h_hook = SetWindowsHookExW(WH_KEYBOARD_LL, Some(hook_proc), None, 0).unwrap();
        HHOOK_PTR.get_or_init(|| AtomicPtr::new(h_hook.0));
    }
}

fn hook_end() {
    unsafe {
        let _ = UnhookWindowsHookEx(HHOOK(HHOOK_PTR.get().unwrap().load(Ordering::Relaxed)));
    }
}

extern "system" fn window_proc(hwnd: HWND, msg: u32, wparam: WPARAM, lparam: LPARAM) -> LRESULT {
    unsafe {
        match msg {
            WM_CREATE => {
                hook_start();
                DefWindowProcW(hwnd, msg, wparam, lparam)
            }
            WM_DESTROY => {
                hook_end();
                PostQuitMessage(0);
                LRESULT::default()
            }
            _ => DefWindowProcW(hwnd, msg, wparam, lparam),
        }
    }
}

fn main() {
    unsafe {
        let instance = GetModuleHandleW(None).unwrap();
        let wc = WNDCLASSW {
            lpfnWndProc: Some(window_proc),
            hInstance: instance.into(),
            lpszClassName: CLASS_NAME,
            ..Default::default()
        };
        RegisterClassW(&wc);
        CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            CLASS_NAME,
            w!("どぼ楽"),
            WS_POPUP | WS_BORDER,
            0,
            0,
            0,
            0,
            Some(HWND_MESSAGE),
            None,
            Some(instance.into()),
            None,
        )
        .unwrap();

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0).into() {
            let _ = TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}
