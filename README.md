# Haiku Notepad

Trình soạn thảo văn bản thuần (plain text) gọn nhẹ cho **Haiku OS**, viết bằng **C++** và **BeAPI**, giao diện tiếng Việt.

Dự án chỉ dùng API gốc của Haiku (Application Kit, Interface Kit, Storage Kit), không phụ thuộc Qt, GTK hay thư viện ngoài nào.

## Chức năng

- Tạo tài liệu mới, mở và lưu file văn bản (Lưu / Lưu thành…).
- Mở file từ Terminal (`HaikuNotepad ten-file.txt`), bằng cách kéo file thả lên biểu tượng ứng dụng, hoặc bằng "Open with…" trong Tracker.
- **Hoàn tác / Làm lại nhiều bước** (tối đa 1000 bước), gộp thông minh theo từ, theo dòng và theo lệnh.
- Cắt, sao chép, dán, chọn tất cả.
- Bật/tắt tự xuống dòng.
- Tiêu đề cửa sổ có dấu `*` khi còn thay đổi chưa lưu, và hỏi lưu trước khi đóng, mở file khác hay tạo tài liệu mới.
- File mới lưu được gán kiểu MIME `text/plain` để Tracker nhận ra là văn bản.
- Font đều nét (monospace), tiện để viết code.

## Phím tắt

Trên Haiku, phím **Cmd** mặc định là phím **Alt** trên bàn phím PC.

| Phím tắt | Lệnh |
|---|---|
| Cmd + N | Tài liệu mới |
| Cmd + O | Mở… |
| Cmd + S | Lưu |
| Cmd + Shift + S | Lưu thành… |
| Cmd + Q | Thoát |
| Cmd + Z | Hoàn tác |
| Cmd + Shift + Z | Làm lại |
| Cmd + X / C / V | Cắt / Sao chép / Dán |
| Cmd + A | Chọn tất cả |

## Yêu cầu

- Haiku OS (R1/beta trở lên, bản x86_64).
- Trình biên dịch GCC và `makefile-engine` của Haiku. Nếu lệnh `make` báo không tìm thấy `makefile-engine`, cài gói phát triển bằng `pkgman`.

## Build và chạy

Trên máy Haiku, trong thư mục dự án:

```sh
make
./objects.*/HaikuNotepad
```

Mở luôn một file:

```sh
./objects.*/HaikuNotepad ~/ghi-chu.txt
```

File chạy nằm trong thư mục `objects.<kiến trúc>-<trình biên dịch>-release/`, ví dụ `objects.x86_64-cc13-release/HaikuNotepad`.

### Viết code trên Windows, build trên Haiku qua SSH

Nếu bạn soạn code trên máy khác và máy Haiku nằm cùng mạng LAN (đã khai báo tên `haiku` trong `~/.ssh/config`):

```sh
ssh haiku "mkdir -p ~/HaikuNotepad"
scp App.cpp EditorWindow.h EditorWindow.cpp EditorTextView.h EditorTextView.cpp Makefile haiku:~/HaikuNotepad/
ssh haiku "cd ~/HaikuNotepad && make"
```

Ứng dụng có giao diện đồ hoạ nên phải mở trực tiếp trên màn hình máy Haiku để dùng thử.

## Cấu trúc mã nguồn

| File | Nội dung |
|---|---|
| `App.cpp` | Hàm `main()` và lớp `App` (`BApplication`): khởi động, nhận file từ dòng lệnh hoặc Tracker. |
| `EditorWindow.h/.cpp` | Cửa sổ chính: menu, mở/lưu file, tiêu đề, hỏi khi chưa lưu. |
| `EditorTextView.h/.cpp` | Vùng soạn thảo (kế thừa `BTextView`): báo thay đổi cho cửa sổ, hoàn tác/làm lại nhiều bước. |
| `Makefile` | Cấu hình build theo `makefile-engine` của Haiku. |
| `CLAUDE.md` | Ghi chú bối cảnh dự án cho trợ lý AI. |
| `GIAI_THICH_MA_NGUON.md` | Giải thích chi tiết luồng hoạt động, chức năng và cách code vận hành. |

Muốn hiểu code chạy thế nào, đọc [GIAI_THICH_MA_NGUON.md](GIAI_THICH_MA_NGUON.md).

## Giới hạn hiện tại

- Chỉ một cửa sổ, mở một file mỗi lần.
- Chỉ mở file tối đa 64 MB.
- Chỉ hỗ trợ văn bản UTF-8, không có định dạng (in đậm, màu…).
- Undo về đúng trạng thái lúc vừa lưu thì tiêu đề vẫn còn dấu `*`.
- Chưa có tìm kiếm / thay thế, chưa nhớ kích thước cửa sổ giữa các lần mở.
