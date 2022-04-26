// heif2jpg.cpp : アプリケーションのエントリ ポイントを定義します。
//

#pragma warning(disable:26812) //enumにclassつけてねっていうやつ
#include "framework.h"
#include "heif2jpg.h"

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
UINT dropped_file_count = 0;

class ContextReleaser
{
public:
    ContextReleaser(struct heif_context* ctx) : ctx_(ctx)
    {}

    ~ContextReleaser()
    {
        heif_context_free(ctx_);
    }

private:
    struct heif_context* ctx_;
};

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HEIF2JPG, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    //キーボードアクセラレータ（ショートカット）
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HEIF2JPG));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HEIF2JPG));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HEIF2JPG);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する
   const int nWidth = 400;
   const int nHeight = 320;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, nWidth, nHeight, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   DragAcceptFiles(hWnd, TRUE);

   return TRUE;
}


void WmPaint(HWND hWnd) {
    PAINTSTRUCT ps;
    TCHAR prompt[] = _T("ここにファイルをドロップ");
    HDC hdc = BeginPaint(hWnd, &ps);
    TCHAR message[255 + 1];
    _stprintf_s(message, 255, _T("%d 件のファイル等がドロップされました"), dropped_file_count);

    TextOutW(hdc, 10, 10, prompt, _tcslen(prompt));
    if (dropped_file_count > 0) {
        TextOutW(hdc, 10, 30, message, _tcslen(message));
    }
    EndPaint(hWnd, &ps);
}

void WmDropFiles(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DragAcceptFiles(hWnd, FALSE);
    HDROP hdrop = (HDROP)wParam;

    dropped_file_count = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);

    for (unsigned int i = 0; i < dropped_file_count; i++) {
        LPWSTR lpszFile = new TCHAR[1024 + 1];
        DragQueryFileW(hdrop, i, lpszFile, 1024);

        if (PathIsDirectory(lpszFile))
        {
            OutputDebugStringW(_T("フォルダやで\n"));
        }
        else if (wcscmp(PathFindExtensionW(lpszFile), _T(".heic")) == 0) {
            FILE* fp;
            OutputDebugStringW(_T(".heicやで\n"));

            if (_wfopen_s(&fp, lpszFile, _T("rb"))) {
                continue;
            }
            if (fp == NULL) {
                continue;
            }
            uint8_t buf_magic[12];
            fread_s(buf_magic, 12, 1, 12, fp);
            enum heif_filetype_result filetype_check = heif_check_filetype(buf_magic, 12);
            if (filetype_check == heif_filetype_no) {
                OutputDebugStringW(_T("Input file is not an HEIF/AVIF file\n"));
                continue;
            }

            if (filetype_check == heif_filetype_yes_unsupported) {
                OutputDebugStringW(_T("Input file is an unsupported HEIF/AVIF file type\n"));
                continue;
            }

            //ここから先→
            //https://github.com/strukturag/libheif/blob/master/examples/heif_convert.cc#L220

  // --- read the HEIF file

            struct heif_context* ctx = heif_context_alloc();
            if (!ctx) {
                fprintf(stderr, "Could not create context object\n");
                continue;
            }

            ContextReleaser cr(ctx);
            struct heif_error err;
            /*
            err = heif_context_read_from_file(ctx, lpszFile.c_str(), nullptr);
            if (err.code != 0) {
                std::cerr << "Could not read HEIF/AVIF file: " << err.message << "\n";
               continue;
            }

            int num_images = heif_context_get_number_of_top_level_images(ctx);
            if (num_images == 0) {
                OutputDebugStringW(_T("File doesn't contain any images\n"));
                continue;
            }

            if (TRUE) {
                OutputDebugStringW(_T("File contains "));
                    << num_images << " image" << (num_images > 1 ? "s" : "") << "\n";
            }

            std::vector<heif_item_id> image_IDs(num_images);
            num_images = heif_context_get_list_of_top_level_image_IDs(ctx, image_IDs.data(), num_images);
            */


            fclose(fp);
        }
        else
        {
            OutputDebugStringW(_T("よう知らんなあ\n"));
        }
        OutputDebugStringW(lpszFile);
        OutputDebugStringW(_T("\n"));
    }

    DragFinish(hdrop);

    DragAcceptFiles(hWnd, TRUE);
    InvalidateRect(hWnd, NULL, TRUE); //再描画(WM_PAINT)を促す
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        WmPaint(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_DROPFILES:
        WmDropFiles(hWnd, message, wParam, lParam);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

