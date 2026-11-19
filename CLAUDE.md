# Haiku Notepad — trình soạn thảo văn bản cho Haiku OS

## Bối cảnh
- Ứng dụng native cho **Haiku OS** (không phải Linux/Windows), viết bằng **C++ và BeAPI**.
- Chỉ dùng API của Haiku: Application Kit, Interface Kit, Storage Kit (`BApplication`, `BWindow`,
  `BTextView`, `BFile`, `BFilePanel`, `BLayoutBuilder`...). Không dùng Win32, POSIX GUI, Qt, GTK.
- Code được viết trên Windows nhưng **chỉ build và chạy được trên Haiku**: một máy thật riêng cùng mạng LAN,
  truy cập qua SSH bằng tên máy `haiku` (đã khai báo trong `~/.ssh/config` của Windows).

## Cấu trúc
- `App.cpp` — `BApplication`, nhận file từ dòng lệnh/Tracker.
- `EditorWindow.h/.cpp` — cửa sổ chính: menu, mở/lưu file, hỏi khi chưa lưu.
- `EditorTextView.h/.cpp` — vùng soạn thảo (kế thừa `BTextView`): báo thay đổi cho cửa sổ,
  undo/redo nhiều bước tự quản lý (đã tắt undo một bước có sẵn của `BTextView`).
- `Makefile` — dùng makefile-engine của Haiku.
- `README.md` — giới thiệu, chức năng, cách build. `GIAI_THICH_MA_NGUON.md` — giải thích luồng hoạt động
  và cách code vận hành; cập nhật hai file này khi thêm/đổi chức năng.

## Build và test (qua SSH vào máy Haiku)
- Kiểm tra kết nối: `ssh haiku "uname -a"` (phải ra chữ Haiku).
- Đồng bộ code sang Haiku: `ssh haiku "mkdir -p ~/HaikuNotepad"` rồi `scp -r App.cpp EditorWindow.h EditorWindow.cpp EditorTextView.h EditorTextView.cpp Makefile haiku:~/HaikuNotepad/`
- Build: `ssh haiku "cd ~/HaikuNotepad && make"`
- Sau mỗi lần sửa code, luôn build lại và đọc lỗi biên dịch trước khi báo xong.
- File chạy nằm trong thư mục `objects.*` (ví dụ `objects.x86_64-cc13-release/HaikuNotepad`).
- Không chạy được app GUI qua SSH để xem giao diện; người dùng sẽ tự mở trên Haiku để kiểm tra.

## Quy ước code
- Theo coding style của Haiku: tab để thụt lề, thành viên bắt đầu bằng `f`, hàm private bắt đầu bằng `_`,
  kiểu trả về nằm trên dòng riêng trong file .cpp.
- Chuỗi giao diện viết bằng tiếng Việt (UTF-8).
- Khi không chắc một hàm BeAPI có tồn tại hay không, tra Haiku Book (https://www.haiku-os.org/docs/api/)
  hoặc header trong `/boot/system/develop/headers/os/` trên máy Haiku, đừng đoán.
