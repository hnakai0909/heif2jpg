# heif2jpg

heif2jpgは、libheifおよびlibjpeg-turboを利用した、HEIF形式のファイル（現時点で拡張子 `.heic` のみ対応）をドラッグアンドドロップすると`.jpg`に変換するWindowsアプリケーションです。

- 開発言語：C++
  - ただし、Windows API はC言語インターフェースを中心に利用している
- フレームワーク：なし
  - MFC も .NET(Windows Form)も未使用、Win 32 APIを直叩き
- 使用ライブラリ
  - [libheif](https://github.com/strukturag/libheif)
    - 本アプリのコードは、`examples` の `encoder` アプリのコードをWindows向けに改変したものが中心
  - [libjpeg-turbo](https://libjpeg-turbo.org/)

- 著作権・ライセンス表示
  - libheif関連のコードについては、ファイル `COPYING` を参照
  - 本アプリのライセンスは2022年5月1日時点で未定（近日決定します、MITライセンスになると思われます）
  - 2022 hnakai
