/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * DvoRAKU
 * 
 * 自作配列「どぼ楽」
 *
 * Copyright 2024 TK Lab. All rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <imm.h>
#include <stdio.h>

#pragma comment( lib, "User32.lib" )
#pragma comment( lib, "Imm32.lib" )


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK HookProc(int nCode, WPARAM wp, LPARAM lp);
void HookStart();
void HookEnd();

char SendKey( char keyCode );
char SendKey( char modifierCode, char keyCode );

HINSTANCE hInst;
HHOOK hHook;

HWND hWnd;

int lastKeyCode;
bool isLastKeyConsonant;
bool enableDvoRAKU = TRUE;
bool enableExtendedRAKU = TRUE;

int main( int argc, char *argv[] ){

   const TCHAR CLASS_NAME[] = "DvoRAKU";
   
   WNDCLASS wc = { };

   wc.lpfnWndProc = WindowProc;
   HINSTANCE hInstance;
   hInstance = GetModuleHandle( NULL );
   wc.hInstance = hInstance;
   wc.lpszClassName = CLASS_NAME;

   RegisterClass(&wc);


   hWnd = CreateWindowEx(
      WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
      CLASS_NAME,
      "どぼ楽",
      WS_POPUP | WS_BORDER,
      0, 0, 0, 0,
      NULL,
      NULL,
      hInstance,
      NULL
      );

   if (hWnd == NULL){
      return -1;
   }

   MSG msg;
   while ( GetMessage(&msg, NULL, 0, 0) ) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return 0;
}

LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch (uMsg) {

      case WM_CREATE:
         HookStart();
         break;

      case WM_DESTROY:
         HookEnd();
         PostQuitMessage(0);
         return 0;
   }

   return DefWindowProc( hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK HookProc(int nCode, WPARAM wp, LPARAM lp)
{
   if ( nCode != HC_ACTION ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   KBDLLHOOKSTRUCT *key;
   unsigned int keyCode, scanCode, keyFlags, keyChar;

   key = (KBDLLHOOKSTRUCT*)lp;
   
   keyCode = key->vkCode;
   scanCode = key->scanCode;
   keyFlags = key->flags;
   keyChar = MapVirtualKey( keyCode, MAPVK_VK_TO_CHAR );

   printf( "(%c), %u, %u, %u, ", keyChar, keyCode, scanCode, keyFlags );

   bool isKeyUp;
   isKeyUp = keyFlags & LLKHF_UP;
   if ( isKeyUp ){
      printf( "KeyUp " );
   }
   else{
      printf( "KeyDown " );
   }

   bool isInjected;
   isInjected = keyFlags & LLKHF_INJECTED;
   if( isInjected ){
      printf( "injected\n" );
   }
   else{
      printf( "detected\n" );
   }

  // Ctrlキー状態 
  // Ctrl + △ を Ctrl + □　ではなく □ 単打に入れ替えられるようにCtrl状態を内部管理する
   static bool isControl = FALSE;
   if ( keyCode == VK_CONTROL || keyCode == VK_LCONTROL || keyCode == VK_RCONTROL ){
      if ( keyFlags & LLKHF_INJECTED ){
         return CallNextHookEx(hHook, nCode, wp, lp);
      }
      else {
         if( ( keyFlags & LLKHF_UP ) == 0  ){
            isControl = TRUE;
         }
         else {
            isControl = FALSE;
         }
         return TRUE;
      }
   }

   // Shiftキー状態
   bool isShift;
   isShift = GetAsyncKeyState( VK_SHIFT) & 0x8000;

   // Altキー状態
   bool isAlt;
   isAlt = GetAsyncKeyState( VK_MENU ) & 0x8000;

   // キーアップイベントは入れ替え処理しない
   if( ( keyFlags & LLKHF_UP ) != 0  ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   // Ctrlキーと同時押しのキーバインド設定
   // 設定のないキーはQWERTYで処理する
   if( isControl ) {
      if ( ( keyFlags & LLKHF_INJECTED ) == FALSE ){
         if ( keyCode == 'M' ){
            SendKey( VK_RETURN );
         }
         else if ( keyCode == 'H' ){
            SendKey( VK_BACK );
         }
         else {
            SendKey( VK_CONTROL, keyCode );
         }

         return TRUE;
      }
   }

   // Alt キーとの同時押しのキーバインド設定
   if( isAlt  ){
      // Alt + Q で QWERTY ⇔ どぼ楽　切り替え
      if( keyCode == 'Q' ){
         enableDvoRAKU = !enableDvoRAKU;
         return TRUE;
      }
      // Alt + X で 拡張レイヤーの有効無効をトグル
      else if( keyCode == 'X' ){
         enableExtendedRAKU= !enableExtendedRAKU;
         return TRUE;
      }
   }

   // 無効状態のとき入れ替え処理しない
   if ( enableDvoRAKU == FALSE ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }
   

   // IMEがオフの時はQWERTY
   HWND hForeground;
   hForeground = GetForegroundWindow();

   HWND hIME;
   hIME = ImmGetDefaultIMEWnd( hForeground );

   LRESULT imeStatus;
   imeStatus = SendMessage( hIME, WM_IME_CONTROL, 5, 0 );

   if( imeStatus == 0 ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }


   // 挿入されたキーイベント
   if ( keyFlags & LLKHF_INJECTED ){
      //子音キーが挿入されたら拡張レイヤーに入る
      if( keyCode == 'B' ||
          keyCode == 'C' || 
          keyCode == 'D' || 
          keyCode == 'F' || 
          keyCode == 'G' || 
          keyCode == 'H' || 
          keyCode == 'J' || 
          keyCode == 'K' || 
          keyCode == 'L' || 
          keyCode == 'M' || 
          keyCode == 'N' || 
          keyCode == 'P' || 
          keyCode == 'Q' || 
          keyCode == 'R' || 
          keyCode == 'S' || 
          keyCode == 'T' || 
          keyCode == 'V' || 
          keyCode == 'W' || 
          keyCode == 'X' || 
          keyCode == 'Y' || 
          keyCode == 'Z' ) {


         // 同じ子音2連続は拡張レイヤー処理しない
         if ( lastKeyCode == keyCode ){
            // NN は拡張レイヤー処理せず拡張レイヤーを抜ける
            if ( lastKeyCode == 'N' && keyCode == 'N' ){
               isLastKeyConsonant = FALSE;
            }
            // 同じ子音2連続は拡張レイヤー処理しないが、拡張レイヤーを継続する
            else {
               isLastKeyConsonant = TRUE;
            }
         }
         // XN は拡張レイヤー処理せず拡張レイヤーを抜ける
         else if ( lastKeyCode == 'X' && keyCode == 'N' ){
            isLastKeyConsonant = FALSE;
         }
         // 拡張レイヤー処理
         else if( isLastKeyConsonant == TRUE ){
            // Q<子音> → Qを削除して<子音>に置き換える
            // e.q. QS → SS
            if( lastKeyCode == 'Q' ){
               SendKey( VK_BACK );
               SendKey( keyCode );
               isLastKeyConsonant = TRUE;
            }
            // <子音1><子音2> → <子音1>A<子音2>A 
            // e.q. KS → KASA
            else{
               SendKey( 'A' );
               SendKey( keyCode );
               SendKey( 'A' );
               return TRUE;
            }
         }
         // 子音が挿入されたら拡張レイヤーに入る
         else {
            if( enableExtendedRAKU ){
               if ( isShift == FALSE ) { 
                  isLastKeyConsonant = TRUE;
               }
            }
         }
      }
      else {
         // 子音以外のキーが挿入されたら拡張レイヤーから抜ける
         isLastKeyConsonant = FALSE;
      }

      //最後に挿入されたキーを記録
      lastKeyCode = keyCode;

      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   // キー入れ替え処理
   //最上段
   if ( keyCode == '1' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'A' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( '1' );
      }
   }
   else if ( keyCode == '2' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'O' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( '2' );
      }
   }
   else if ( keyCode == '3' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'E' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( '3' );
      }
   }
   else if ( keyCode == '4' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'U' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( '4' );
      }
   }
   else if ( keyCode == '5' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'H' );
         SendKey( 'I' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( '5' );
      }
   }
   else if ( keyCode == VK_OEM_MINUS ){ // -_ 
      SendKey( VK_OEM_4 ); // [{
   }
   else if ( keyCode == VK_OEM_PLUS ){ // =+
      SendKey( VK_OEM_6 ); // ]}
   }
   //上段左
   else if ( keyCode == 'Q' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'A' );
      }
      else {
         SendKey( VK_OEM_7 ); // '"
      }
   }
   else if ( keyCode == 'W' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'O' );
      }
      else {
         SendKey( VK_OEM_COMMA ); // ,<
      }
   }
   else if ( keyCode == 'E' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'E' );
      }
      else {
         SendKey( VK_OEM_PERIOD ); // .>
      }
   }
   else if ( keyCode == 'R' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'Y' );
         SendKey( 'U' );
      }
      else {
         SendKey( VK_OEM_MINUS ); // -_
      }
   }
   else if ( keyCode == 'T' ){
      if( isLastKeyConsonant ){
         isLastKeyConsonant = FALSE;
         SendKey( 'H' );
         SendKey( 'I' );
      }
      else {
         SendKey( VK_OEM_PLUS ); // =+
      }
   }
   //上段右
   else if ( keyCode == 'Y' ){
      SendKey( 'Y' );
   }
   else if ( keyCode == 'U' ){
      SendKey( 'D' );
   }
   else if ( keyCode == 'I' ){
      SendKey( 'G' );
   }
   else if ( keyCode == 'O' ){
      SendKey( 'F' );
   }
   else if ( keyCode == 'P' ){
      SendKey( 'P' );
   }
   else if ( keyCode == VK_OEM_4 ){ // [{
      SendKey( 'B' );
   }
   else if ( keyCode == VK_OEM_6 ){ // ]}
      SendKey( 'V' );
   }
   //中段左
   else if ( keyCode == 'A' ){
      SendKey( 'A' );
   }
   else if ( keyCode == 'S' ){
      SendKey( 'O' );
   }
   else if ( keyCode == 'D' ){
      SendKey( 'E' );
   }
   else if ( keyCode == 'F' ){
      SendKey( 'U' );
   }
   else if ( keyCode == 'G' ){
      SendKey( 'I' );
   }
   //中段右
   else if ( keyCode == 'H' ){
      SendKey( 'H' );
   }
   else if ( keyCode == 'J' ){
      SendKey( 'T' );
   }
   else if ( keyCode == 'K' ){
      SendKey( 'K' );
   }
   else if ( keyCode == 'L' ){
      SendKey( 'R' );
   }
   else if ( keyCode == VK_OEM_1 ){ // ;:
      SendKey( 'S' );
   }
   else if ( keyCode == VK_OEM_7 ){ // '"
      SendKey( 'Q');
   }
   //下段左
   else if ( keyCode == 'Z' ){
      if( isLastKeyConsonant ){
         SendKey( 'A' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( VK_OEM_1 ); // ;:
      }
   }
   else if ( keyCode == 'X' ){
      if( isLastKeyConsonant ){
         SendKey( 'O' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( 'X' );
      }
   }
   else if ( keyCode == 'C' ){
      if( isLastKeyConsonant ){
         SendKey( 'E' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( 'C' );
      }
   }
   else if ( keyCode == 'V' ){
      if( isLastKeyConsonant ){
         SendKey( 'U' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( 'L' );
      }
   }
   else if ( keyCode == 'B' ){
      if( isLastKeyConsonant ){
         SendKey( 'I' );
         SendKey( 'N' );
         SendKey( 'N' );
      }
      else {
         SendKey( VK_OEM_2 ); // /?
      }
   }
   //下段右
   else if ( keyCode == 'N' ){
      SendKey( 'N' );
   }
   else if ( keyCode == 'M' ){
      SendKey( 'M' );
   }
   else if ( keyCode == VK_OEM_COMMA ){ // ,<
      SendKey( 'W' );
   }
   else if ( keyCode == VK_OEM_PERIOD ){ // .>
      SendKey( 'J' );
   }
   else if ( keyCode == VK_OEM_2 ){ // /?
      SendKey( 'Z' );
   }
   //入れ替え対象外
   else {
      lastKeyCode = keyCode;
      isLastKeyConsonant = FALSE;
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   return TRUE; 
}

void HookStart()
{
   printf( "keyChar, keyCode, scanCode, keyFlags, eventType\n" );
   hHook = SetWindowsHookEx( WH_KEYBOARD_LL, HookProc, hInst, 0);
}

void HookEnd()
{
   UnhookWindowsHookEx(hHook);
}


char SendKey( char keyCode ){

   keybd_event( keyCode, NULL, NULL, NULL );
   keybd_event( keyCode, NULL, KEYEVENTF_KEYUP, NULL );

   return keyCode;
}

char SendKey( char modifierCode, char keyCode ){

   keybd_event( modifierCode, NULL, NULL, NULL );

   keybd_event( keyCode, NULL, NULL, NULL );
   keybd_event( keyCode, NULL, KEYEVENTF_KEYUP, NULL );

   keybd_event( modifierCode, NULL, KEYEVENTF_KEYUP, NULL );

   return keyCode;
}

