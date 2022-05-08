// heif2jpg.cpp : アプリケーションのエントリ ポイントを定義します。
//

#pragma warning(disable:26812) //enumにclassつけてねっていうやつ
#include "framework.h"
#include "heif2jpg.h"

#define MAX_LOADSTRING 100
#define MAX_SEM_COUNT 100
#define THREADCOUNT 102

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
HANDLE ghSemaphore;
HANDLE aThread[THREADCOUNT];
unsigned dropped_file_count = 0;
unsigned converted_file_count = 0;
HANDLE ghMutexConvertedFileCount;

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

    ghSemaphore = CreateSemaphore(
        NULL,           // default security attributes
        MAX_SEM_COUNT,  // initial count
        MAX_SEM_COUNT,  // maximum count
        NULL);          // unnamed semaphore

    if (ghSemaphore == NULL)
    {
        printf("CreateSemaphore error: %d\n", GetLastError());
        return 1;
    }
    ghMutexConvertedFileCount = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex
    if (ghMutexConvertedFileCount == NULL)
    {
        printf("CreateMutex error: %d\n", GetLastError());
        return 1;
    }


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
    
    WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, 30*1000);
    for (int i = 0; i < THREADCOUNT; i++) {
        CloseHandle(aThread[i]);
    }
    CloseHandle(ghMutexConvertedFileCount);
    CloseHandle(ghSemaphore);

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
    TCHAR message[2][255 + 1];
    _stprintf_s(message[0], 255, _T("%d 件のファイル等がドロップされました"), dropped_file_count);
    _stprintf_s(message[1], 255, _T("%d 件のファイルが変換できました"), converted_file_count);

    TextOutW(hdc, 10, 10, prompt, _tcslen(prompt));
    if (dropped_file_count > 0) {
        TextOutW(hdc, 10, 30, message[0], _tcslen(message[0]));
    }
    if(converted_file_count > 0) {
        TextOutW(hdc, 10, 50, message[1], _tcslen(message[1]));
    }
    EndPaint(hWnd, &ps);
}

bool IncrementConvertedFileCount(void){
    int retval = false;
    __try {
        printf("Thread %d add ghMutexConvertedFileCount...\n", GetCurrentThreadId());
        converted_file_count++;
        retval = true;
    }
    __finally {
        // Release ownership of the mutex object
        if (!ReleaseMutex(ghMutexConvertedFileCount))
        {
            // Handle error.
        }
    }
    return retval;
}

unsigned __stdcall ThreadConvert(void* lpParam){
    HWND hWnd = ((PThreadConvertArgs_t)lpParam)->hWnd;
    LPCWSTR lpszInFile = ((PThreadConvertArgs_t)lpParam)->lpszInFile;

    if (PathIsDirectoryW(lpszInFile))
    {
        OutputDebugStringW(_T("フォルダやで\n"));
    }
    else if (wcscmp(PathFindExtensionW(lpszInFile), _T(".heic")) == 0) {
        bool option_quiet = false;
        FILE* fp;
        unsigned quality = 90;
        std::unique_ptr<Encoder> encoder;
        encoder.reset(new JpegEncoder(quality));

        OutputDebugStringW(_T(".heicやで\n"));

        //出力ファイル名を生成
        LPWSTR lpszOutFile = new TCHAR[1024 + 1];
        wcscpy_s(lpszOutFile, 1024, lpszInFile);
        PathRenameExtensionW(lpszOutFile, _T(".jpg"));

        if (_wfopen_s(&fp, lpszInFile, _T("rb"))) {
            return 1;
        }
        if (fp == NULL) {
            return 1;
        }
        uint8_t buf_magic[12];
        fread_s(buf_magic, 12, 1, 12, fp);
        enum heif_filetype_result filetype_check = heif_check_filetype(buf_magic, 12);
        if (filetype_check == heif_filetype_no) {
            OutputDebugStringW(_T("Input file is not an HEIF/AVIF file\n"));
            return 1;
        }

        if (filetype_check == heif_filetype_yes_unsupported) {
            OutputDebugStringW(_T("Input file is an unsupported HEIF/AVIF file type\n"));
            return 1;
        }

        struct heif_context* ctx = heif_context_alloc();
        if (!ctx) {
            OutputDebugStringW(_T("Could not create context object\n"));
            return 1;
        }

        ContextReleaser cr(ctx);
        struct heif_error err;

        HANDLE hFile = CreateFileW(lpszInFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            OutputDebugStringW(_T("CreateFileW (open) Failed\n"));
            return 1;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(hFile, &fileSize)) {
            OutputDebugStringW(_T("GetFileSizeEx Failed\n"));
            return 1;
        }

        LPVOID HEIFBuffer = malloc(fileSize.QuadPart);
        if (HEIFBuffer == NULL) {
            OutputDebugStringW(_T("malloc Failed\n"));
        }

        if (!ReadFile(hFile, HEIFBuffer, fileSize.QuadPart, NULL, NULL)) {
            OutputDebugStringW(_T("ReadFile Failed\n"));
            return 1;
        };
            
        err = heif_context_read_from_memory_without_copy(ctx, HEIFBuffer, fileSize.QuadPart, nullptr);
        if (err.code != 0) {
            OutputDebugF(_T("Could not read HEIF/AVIF file: %s\n"), (LPCWSTR)err.message);
            return 1;
        }

        int num_images = heif_context_get_number_of_top_level_images(ctx);
        if (num_images == 0) {
            OutputDebugStringW(_T("File doesn't contain any images\n"));
            return 1;
        }
        else if (num_images > 1) {
            OutputDebugStringW(_T("簡易実装のため、1ファイルに複数イメージが含まれている場合は未対応です★\n"));
            return 1;
        }

        if (!option_quiet) {
            OutputDebugF(_T("File contains %d image(s)\n"), num_images);
        }

        std::vector<heif_item_id> image_IDs(num_images);
        num_images = heif_context_get_list_of_top_level_image_IDs(ctx, image_IDs.data(), num_images);

        unsigned idx = 0;
        struct heif_image_handle* handle;
        err = heif_context_get_image_handle(ctx, image_IDs[idx], &handle);
        if (err.code) {
            OutputDebugF(_T("Could not read HEIF/AVIF image : %s\n"), err.message);
            return 1;
        }

        int has_alpha = heif_image_handle_has_alpha_channel(handle);
        struct heif_decoding_options* decode_options = heif_decoding_options_alloc();
        encoder->UpdateDecodingOptions(handle, decode_options);

        //decode_options->strict_decoding = strict_decoding;

        int bit_depth = heif_image_handle_get_luma_bits_per_pixel(handle);
        if (bit_depth < 0) {
            heif_decoding_options_free(decode_options);
            heif_image_handle_release(handle);
            OutputDebugF(_T("Input image has undefined bit-depth\n"));
            return 1;
        }

        struct heif_image* image;
        err = heif_decode_image(handle,
            &image,
            encoder->colorspace(has_alpha),
            encoder->chroma(has_alpha, bit_depth),
            decode_options);
        heif_decoding_options_free(decode_options);
        if (err.code) {
            heif_image_handle_release(handle);
            OutputDebugF(_T("Could not decode image: %s\n"), err.message);
            return 1;
        }

        // show decoding warnings
        //vcpkg収録のバージョン2 ?では未実装？
        /*
        for (int i = 0;; i++) {
            int n = heif_image_get_decoding_warnings(image, i, &err, 1);
            if (n == 0) {
                break;
            }

            OutputDebugF(_T("Warning: %s\n"), err.message);
        }
        */

        //TODO: warning（主にデバッグメッセージ表示まわり）を取り除く
        //TODO: jpeg EXIF Orientation(向き情報) 1にセット
        //  どうやら、EXIFの部分はHEIFのものをそのままコピーしているようだ

        if (image) {
            bool written = encoder->Encode(handle, image, lpszOutFile);
            if (!written) {
                fprintf(stderr, "could not write image\n");
            }
            else {
                if (!option_quiet) {
                    OutputDebugF(_T("Written to "));
                    OutputDebugStringW(lpszOutFile);
                    OutputDebugF(_T("\n"));
                }
            }
            heif_image_release(image);
            heif_image_handle_release(handle);
        }

            
        free(HEIFBuffer);
        fclose(fp);

        bool loopFlag = true;
        while (loopFlag)
        {
            DWORD dwWaitResult = WaitForSingleObject(
                ghMutexConvertedFileCount,    // handle to mutex
                INFINITE);  // no time-out interval

            switch (dwWaitResult)
            {
                // The thread got ownership of the mutex
            case WAIT_OBJECT_0:
                if (IncrementConvertedFileCount()) {
                    loopFlag = false;
                }
                break;

                // The thread got ownership of an abandoned mutex
                // The database is in an indeterminate state
            case WAIT_ABANDONED:
                return FALSE;
            }
        }

        InvalidateRect(hWnd, NULL, TRUE); //再描画(WM_PAINT)を促す

    }
    else
    {
        OutputDebugStringW(_T("よう知らんなあ\n"));
    }
    OutputDebugStringW(lpszInFile);
    OutputDebugStringW(_T("\n"));

    HeapFree(GetProcessHeap(), 0, lpParam);
    _endthreadex(0);
    return 0;
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
        {
            HDROP hdrop = (HDROP)wParam;


            dropped_file_count = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);
            //TODO: スレッド数の制限（たぶんセマフォでできる）
            //TODO: 未処理があるにもかかわらずドロップされたときの対応
            for (unsigned int i = 0; i < dropped_file_count; i++) {
                PThreadConvertArgs_t PThreadConvertParam = (PThreadConvertArgs_t)HeapAlloc(
                    GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ThreadConvertArgs_t));
                if (PThreadConvertParam == NULL) {
                    return 1;
                }
                LPWSTR lpszInFile = new TCHAR[1024 + 1];
                DragQueryFileW(hdrop, i, lpszInFile, 1024);
                
                PThreadConvertParam->hWnd = hWnd;
                PThreadConvertParam->lpszInFile = _wcsdup(lpszInFile);
                aThread[i] = (HANDLE)_beginthreadex(NULL, 0, ThreadConvert, (void*)PThreadConvertParam, 0, NULL);
                if (!aThread[i]) {
                    printf("_beginthreadex() error: %d\n", GetLastError());
                    return 1;
                }
            }

            DragFinish(hdrop);

            InvalidateRect(hWnd, NULL, TRUE); //再描画(WM_PAINT)を促す
        }
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

