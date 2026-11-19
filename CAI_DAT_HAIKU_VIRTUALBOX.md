# Cài Haiku OS vào VirtualBox và chuẩn bị môi trường build Haiku Notepad

Hướng dẫn này đưa bạn từ một máy Windows chưa có gì đến lúc build và chạy được Haiku Notepad trong máy ảo. Toàn bộ mất khoảng 30–45 phút, chủ yếu là thời gian tải và cài.

Thông tin dưới đây đúng với **Haiku R1/beta6** (phát hành ngày 26/8/2026) và VirtualBox 7.x. Bản mới hơn có thể đổi tên file hoặc vị trí menu, nhưng các bước chính vẫn vậy.

**Mục lục**

1. Chuẩn bị máy thật
2. Tải Haiku
3. Tạo máy ảo
4. Cài Haiku vào ổ đĩa ảo
5. Thiết lập sau khi cài
6. Cài các gói cần thiết (dependency)
7. Build và chạy Haiku Notepad
8. (Tuỳ chọn) Viết code trên Windows, build qua SSH
9. Xử lý sự cố

---

## 1. Chuẩn bị máy thật

### Cấu hình nên có

| Thành phần | Tối thiểu | Nên có |
|---|---|---|
| CPU | 64-bit, hỗ trợ ảo hoá (Intel VT-x / AMD-V) | 4 nhân trở lên |
| RAM máy thật | 4 GB | 8 GB trở lên (để chia 2–4 GB cho máy ảo) |
| Ổ đĩa trống | 20 GB | 30 GB |

### Bật ảo hoá trong BIOS/UEFI

VirtualBox cần tính năng ảo hoá của CPU. Kiểm tra trên Windows: mở **Task Manager → Performance → CPU**, xem dòng **Virtualization**.

- Nếu là **Enabled**: bỏ qua bước này.
- Nếu là **Disabled**: khởi động lại máy, vào BIOS/UEFI (thường bấm F2, Del hoặc F10 khi vừa bật máy), tìm mục **Intel Virtualization Technology**, **VT-x**, **SVM Mode** (máy AMD) hoặc tên tương tự, chuyển sang **Enabled**, rồi lưu và thoát.

### Cài VirtualBox

1. Tải bản cho Windows tại https://www.virtualbox.org/wiki/Downloads ("Windows hosts").
2. Chạy file cài, giữ nguyên các lựa chọn mặc định. Trong lúc cài, mạng có thể chập chờn vài giây; đó là VirtualBox đang tạo card mạng ảo.

---

## 2. Tải Haiku

1. Vào https://www.haiku-os.org/get-haiku/
2. Chọn bản **64-bit**, file có tên `haiku-r1beta6-x86_64-anyboot.iso` (khoảng 1,4 GB).
   - Đừng chọn bản `x86_gcc2h` (32-bit). Bản đó dành cho máy cũ và để chạy ứng dụng BeOS cổ; build theo hướng dẫn này cần bản 64-bit.
   - Tải từ máy chủ nào cũng được, hoặc dùng file `.torrent`.
3. (Nên làm) Kiểm tra file không bị lỗi khi tải. Mở PowerShell trong thư mục chứa file và chạy:

   ```powershell
   Get-FileHash .\haiku-r1beta6-x86_64-anyboot.iso -Algorithm SHA256
   ```

   Kết quả phải trùng với mã SHA256 công bố trên trang tải. Với bản R1/beta6 64-bit, mã đó là:

   ```
   bb7743474fd175aea511be1799764ba054465285bbdf54c4722ca61d1fb4f5c9
   ```

   Nếu khác, tải lại file.

---

## 3. Tạo máy ảo

### 3.1. Tạo máy ảo mới

Mở VirtualBox, bấm **New** (Mới), rồi điền:

| Mục | Giá trị |
|---|---|
| Name | `Haiku` |
| ISO Image | **để trống**; ta gắn ISO ở bước sau, để VirtualBox không tự chạy "Unattended Install" (Haiku không hỗ trợ) |
| Type | `Other` |
| Version | `Other/Unknown (64-bit)` |

> **Quan trọng:** phải chọn bản **(64-bit)**. Chọn nhầm bản 32-bit thì Haiku 64-bit sẽ không khởi động được (thường báo lỗi *panic* ngay lúc boot).

Bấm **Next**, rồi cấu hình phần cứng:

| Mục | Giá trị |
|---|---|
| Base Memory (RAM) | `4096 MB` (tối thiểu `2048 MB`) |
| Processors (CPU) | `2`–`4` (không quá nửa số nhân máy thật) |
| Enable EFI | **không** đánh dấu (dùng BIOS cho đơn giản) |

Bấm **Next**, tạo ổ đĩa ảo:

| Mục | Giá trị |
|---|---|
| Create a Virtual Hard Disk Now | chọn mục này |
| Kích thước | `20 GB` |
| Loại file | `VDI` |
| Pre-allocate Full Size | **không** đánh dấu (file ổ đĩa lớn dần theo dung lượng dùng thật) |

Bấm **Next → Finish**.

### 3.2. Chỉnh thiết lập

Chọn máy ảo **Haiku**, bấm **Settings** (Cài đặt). Nếu không thấy đủ mục bên dưới, chuyển góc trên cùng của cửa sổ Settings sang chế độ **Expert**.

**System → Motherboard**

- **Pointing Device:** `USB Tablet`. Chuột di chuyển mượt giữa Windows và máy ảo, không bị "nhốt" trong cửa sổ.

**System → Processor / Acceleration**

- Đánh dấu **Enable PAE/NX** nếu có.
- **Hardware Virtualization** (VT-x/AMD-V) phải được bật.

**Display → Screen**

- **Video Memory:** kéo lên mức cao nhất (128 MB).
- **Graphics Controller:** để mặc định. Nếu gặp màn hình đen hoặc lỗi hiển thị, xem mục 9.

**Storage** (gắn file ISO)

1. Trong **Storage Devices**, bấm vào ổ đĩa CD có chữ **Empty**.
2. Bên phải, bấm biểu tượng đĩa CD cạnh **Optical Drive**, chọn **Choose a disk file…**.
3. Chọn file `haiku-r1beta6-x86_64-anyboot.iso` vừa tải.

**Network → Adapter 1**

- **Attached to:** `NAT` (mặc định; máy ảo dùng chung mạng của máy thật).
- Mở **Advanced**, chọn **Adapter Type:** `Intel PRO/1000 MT Desktop (82540EM)`. Đây là card mạng Haiku có driver ổn định nhất.
- Bấm **Port Forwarding** và thêm một dòng nếu bạn định làm mục 8 (SSH). Có thể bỏ qua nếu chỉ code trực tiếp trong máy ảo.

  | Name | Protocol | Host IP | Host Port | Guest IP | Guest Port |
  |---|---|---|---|---|---|
  | ssh | TCP | 127.0.0.1 | 2222 | (để trống) | 22 |

**Audio:** không cần cho dự án này, có thể bỏ đánh dấu **Enable Audio** để máy ảo nhẹ hơn.

Bấm **OK** để lưu.

---

## 4. Cài Haiku vào ổ đĩa ảo

### 4.1. Khởi động từ ISO

Chọn máy ảo **Haiku**, bấm **Start**. Sau vài giây sẽ hiện màn hình khởi động có logo Haiku, rồi đến một bảng chọn ngôn ngữ và bàn phím.

- Chọn ngôn ngữ (English cho dễ tra lỗi trên mạng, hoặc Tiếng Việt nếu có) và bố cục bàn phím (US).
- Bấm **Install Haiku**. Nếu muốn dùng thử trước mà không cài, chọn **Try Haiku**.
- Đọc thông báo, bấm **Continue**.
- Nếu hiện thông báo *"No BFS partitions were found"*, bấm **OK**. Điều này bình thường vì ổ đĩa ảo đang trống.

### 4.2. Chia ổ đĩa bằng DriveSetup

Trong cửa sổ Installer, bấm **Set up partitions…** để mở **DriveSetup**. Danh sách có hai thiết bị: ổ CD (chứa ISO) và ổ cứng ảo 20 GB. **Thao tác trên ổ cứng ảo 20 GB, không phải ổ CD.**

1. **Khởi tạo ổ đĩa:** bấm chọn ổ 20 GB (có viền xanh). Vào menu **Disk → Initialize → Intel Partition Map…**, bấm **Write changes**, rồi **OK**.
2. **Tạo phân vùng:** bấm chọn vùng trống (*empty space*). Vào **Partition → Create…**:
   - Kéo thanh trượt lấy toàn bộ dung lượng.
   - **Partition type:** `Be File System`.
   - Đánh dấu **Active partition**.
   - Bấm **Create**, rồi **Write changes**.
3. **Định dạng:** chọn phân vùng vừa tạo. Vào **Partition → Format → Be File System…**:
   - Đặt tên, ví dụ `Haiku`.
   - Giữ nguyên **Blocksize** mặc định và để đánh dấu **Enable query support**.
   - Bấm **Format**, rồi **Write changes**, rồi **OK**.
4. Đóng DriveSetup bằng ô vuông nhỏ ở góc trên bên trái cửa sổ.

> BFS (Be File System) là hệ thống file riêng của Haiku. Haiku chỉ cài được lên phân vùng BFS.

### 4.3. Cài đặt

1. Quay lại Installer. Ở mục **Onto**, chọn phân vùng `Haiku` vừa tạo.
2. Nếu có mục **Show optional packages**, mở ra và đảm bảo các gói phát triển (Development) được đánh dấu. Bản cài chuẩn thường đã chọn sẵn.
3. Bấm **Begin**. Việc cài mất vài phút.
4. Khi nút **Stop** đổi thành **Quit**, bấm **Quit**.

### 4.4. Gỡ ISO và khởi động lại

Nếu không gỡ ISO, lần sau máy ảo sẽ lại khởi động vào bộ cài.

1. Tắt máy ảo: **Machine → ACPI Shutdown**, hoặc đóng cửa sổ và chọn **Power off the machine**.
2. Vào **Settings → Storage**, chọn ổ CD đang chứa ISO, bấm biểu tượng đĩa CD, chọn **Remove Disk from Virtual Drive**.
3. Bấm **Start** lại. Lần khởi động đầu, Haiku chạy nền một lúc để tạo chỉ mục; máy hơi chậm là bình thường.

> **Mẹo:** khi đã cài xong mọi thứ ở mục 5–6, tạo một **Snapshot** trong VirtualBox (**Machine → Take Snapshot**). Nếu sau này làm hỏng hệ thống, bạn khôi phục lại trong vài giây.

---

## 5. Thiết lập sau khi cài

### Một vài điều cần biết về Haiku

- **Deskbar** là thanh nằm góc trên bên phải, có logo chiếc lá. Bấm vào logo để mở menu ứng dụng (giống nút Start).
- **Terminal:** Deskbar → **Applications → Terminal**.
- **Phím Cmd** của Haiku mặc định là phím **Alt** trên bàn phím PC: `Alt + C` là sao chép, `Alt + V` là dán, `Alt + Q` là đóng ứng dụng.
- Haiku là hệ điều hành **một người dùng**. Tài khoản mặc định tên là `user`, không có mật khẩu và có toàn quyền (không cần `sudo`).
- Thư mục nhà là `/boot/home` (viết tắt `~`).

### Chỉnh độ phân giải màn hình

Deskbar → **Preferences → Screen**. Chọn độ phân giải vừa với cửa sổ VirtualBox, bấm **Apply**.

### Kiểm tra mạng

Mở Terminal và chạy:

```sh
ping -c 3 haiku-os.org
```

Thấy các dòng `64 bytes from …` là mạng đã thông. Nếu không, xem mục 9.

### Cập nhật hệ thống

```sh
pkgman update
```

Trả lời `y` khi được hỏi. Cập nhật xong thì khởi động lại máy ảo bằng lệnh `shutdown -r` (hoặc từ menu Deskbar). Bạn cũng có thể dùng ứng dụng **SoftwareUpdater** (Deskbar → Applications) nếu thích giao diện đồ hoạ.

---

## 6. Cài các gói cần thiết (dependency)

Haiku Notepad chỉ dùng thư viện có sẵn của Haiku (`libbe.so`, `libtracker.so`, thư viện C++ chuẩn), không phụ thuộc thư viện ngoài nào. Thứ cần có là **bộ công cụ build**:

| Gói | Dùng để làm gì |
|---|---|
| `gcc` | Trình biên dịch C/C++ |
| `binutils` | Trình liên kết `ld` và các công cụ đi kèm |
| `make` | Chạy `Makefile` |
| `haiku_devel` | Header (`.h`) và thư viện để build ứng dụng Haiku (BeAPI) |
| `makefile_engine` | Bộ Makefile dựng sẵn mà `Makefile` của dự án gọi tới |

Bản cài chuẩn của Haiku thường **đã có sẵn** các gói này. Kiểm tra bằng Terminal:

```sh
gcc --version
make --version
ls /boot/system/develop/etc/makefile-engine
ls /boot/system/develop/headers/os/interface/TextView.h
```

Nếu cả bốn lệnh đều in ra thông tin (không báo `not found` hay `No such file`), bạn có thể sang mục 7.

Nếu thiếu, cài bằng `pkgman` (trình quản lý gói của Haiku, tương tự `apt` trên Ubuntu):

```sh
pkgman refresh
pkgman install gcc binutils make haiku_devel makefile_engine
```

Trả lời `y` để đồng ý. Gói nào đã có thì `pkgman` tự bỏ qua.

### Gói không bắt buộc nhưng hữu ích

```sh
pkgman install git
```

- `git`: quản lý phiên bản mã nguồn (có thể đã có sẵn).
- Trình soạn thảo code: Haiku có sẵn **Pe** và **StyledEdit**. Muốn có trình soạn thảo hiện đại hơn thì cài **Koder** trong **HaikuDepot** (kho ứng dụng có giao diện, ở Deskbar → Applications).

> Mẹo: muốn tìm gói, dùng `pkgman search <từ-khoá>`. Muốn cài theo tên lệnh mà không cần biết tên gói, dùng `pkgman install cmd:<tên-lệnh>`.

---

## 7. Build và chạy Haiku Notepad

### Đưa mã nguồn vào máy ảo

Chọn một trong các cách:

- **Dùng git** (nếu mã nguồn nằm trên GitHub):
  ```sh
  cd ~
  git clone <địa-chỉ-repo> HaikuNotepad
  ```
- **Tải file zip trong máy ảo:** mở trình duyệt **WebPositive** trong Haiku, tải file zip về, rồi giải nén:
  ```sh
  cd ~
  unzip ~/Desktop/HaikuNotepad.zip
  ```
  (Đổi đường dẫn theo nơi file được lưu, thường là `~/Desktop` hoặc `~/Downloads`.)
- **Chép qua SSH từ Windows:** xem mục 8.

### Build

```sh
cd ~/HaikuNotepad
make
```

Build thành công thì dòng cuối không có chữ `error`, và có file chạy trong thư mục `objects.x86_64-cc13-release/` (phần `cc13` thay đổi theo phiên bản gcc).

### Chạy

```sh
./objects.*/HaikuNotepad
```

Hoặc mở một file có sẵn:

```sh
./objects.*/HaikuNotepad ~/ghi-chu.txt
```

Bạn cũng có thể mở thư mục `objects.*` bằng Tracker (trình quản lý file) và nhấp đúp vào `HaikuNotepad`.

### Build lại sau khi sửa code

```sh
make            # build lại những file đã đổi
make clean      # xoá kết quả build cũ khi cần build lại từ đầu
```

---

## 8. (Tuỳ chọn) Viết code trên Windows, build qua SSH

Phần này dành cho ai muốn viết code bằng VS Code (hoặc dùng trợ lý AI) trên Windows, rồi đẩy sang máy ảo Haiku để build. Cách làm khớp với `CLAUDE.md` của dự án.

Bạn phải đã thêm luật **Port Forwarding** `2222 → 22` ở mục 3.2.

### 8.1. Bật SSH server trong Haiku

Mở Terminal trong máy ảo:

**Bước 1: đặt mật khẩu cho tài khoản `user`** (mặc định không có mật khẩu, nên SSH không đăng nhập được):

```sh
passwd
```

Nhập mật khẩu mới hai lần. Khi gõ sẽ không hiện ký tự nào; đó là bình thường.

**Bước 2: cho phép đăng nhập bằng mật khẩu.** Mở file cấu hình SSH:

```sh
nano /system/settings/ssh/sshd_config
```

Tìm các dòng sau (bỏ dấu `#` ở đầu nếu có) hoặc thêm vào cuối file:

```
PasswordAuthentication yes
PermitRootLogin yes
```

`PermitRootLogin yes` là cần thiết vì tài khoản `user` của Haiku có quyền như root (uid 0). Lưu bằng `Ctrl + O`, `Enter`, rồi thoát bằng `Ctrl + X`.

**Bước 3: chạy (hoặc chạy lại) SSH server:**

```sh
ps | grep sshd          # xem sshd có đang chạy không, và số PID của nó
kill <PID>              # nếu đang chạy, dừng nó lại (thay <PID> bằng số vừa thấy)
/bin/sshd               # chạy sshd với cấu hình mới
```

Nếu lệnh `/bin/sshd` báo không tìm thấy, cài OpenSSH trước bằng `pkgman install openssh`.

> Nếu sau khi khởi động lại máy ảo mà `ps | grep sshd` không thấy sshd, cho nó tự chạy mỗi lần bật máy:
> ```sh
> mkdir -p ~/config/settings/boot
> echo "/bin/sshd" >> ~/config/settings/boot/UserBootscript
> ```

### 8.2. Kết nối từ Windows

Windows 10/11 có sẵn lệnh `ssh`. Mở PowerShell và thử:

```powershell
ssh -p 2222 user@127.0.0.1
```

Lần đầu sẽ hỏi có tin máy này không: gõ `yes`. Sau đó nhập mật khẩu đã đặt ở bước 1. Thấy dấu nhắc `~>` của Haiku là đã vào được.

Để gõ ngắn `ssh haiku` như trong `CLAUDE.md`, mở (hoặc tạo) file `C:\Users\<tên-bạn>\.ssh\config` và thêm:

```
Host haiku
    HostName 127.0.0.1
    Port 2222
    User user
```

Kiểm tra:

```powershell
ssh haiku "uname -a"
```

Kết quả có chữ `Haiku` là thành công.

**Không muốn nhập mật khẩu mỗi lần?** Dùng SSH key:

```powershell
ssh-keygen -t ed25519                     # bấm Enter để nhận mặc định
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh haiku "mkdir -p ~/config/settings/ssh && cat >> ~/config/settings/ssh/authorized_keys"
```

Trên Haiku, thư mục SSH của người dùng nằm ở `~/config/settings/ssh` (thay cho `~/.ssh` trên Linux). Nếu sau bước này vẫn bị hỏi mật khẩu, bạn có thể tiếp tục dùng mật khẩu; mọi thứ khác vẫn hoạt động.

### 8.3. Đồng bộ và build từ Windows

Trong thư mục dự án trên Windows:

```powershell
ssh haiku "mkdir -p ~/HaikuNotepad"
scp App.cpp EditorWindow.h EditorWindow.cpp EditorTextView.h EditorTextView.cpp Makefile haiku:~/HaikuNotepad/
ssh haiku "cd ~/HaikuNotepad && make"
```

Đọc lỗi biên dịch ngay trong PowerShell. Build xong, chuyển sang cửa sổ máy ảo để mở app và thử, vì ứng dụng giao diện không hiển thị qua SSH được.

---

## 9. Xử lý sự cố

| Triệu chứng | Cách xử lý |
|---|---|
| Lỗi *panic* hoặc treo ngay khi khởi động ISO | Kiểm tra **Version** là `Other/Unknown (64-bit)`, và đã bật VT-x/AMD-V (mục 1 và 3.2). |
| VirtualBox báo *VT-x is not available* | Bật ảo hoá trong BIOS (mục 1). Trên Windows, tắt các tính năng chiếm VT-x: vào **Turn Windows features on or off**, bỏ **Hyper-V** và **Virtual Machine Platform**, rồi khởi động lại. Lưu ý: WSL2 và Docker Desktop cần các tính năng này. |
| Biểu tượng con rùa xanh ở góc VirtualBox, máy ảo rất chậm | VirtualBox đang chạy trên nền Hyper-V. Xử lý như dòng trên, hoặc chấp nhận chạy chậm hơn. |
| Máy ảo giật, lag nhiều | Tăng RAM lên 4 GB và CPU lên 2–4 nhân. Một số người dùng báo rằng đổi **Version** sang `Other Linux (64-bit)` giúp mượt hơn (bật x2APIC). |
| Màn hình đen hoặc lỗi hiển thị sau khi boot | **Settings → Display**: đổi **Graphics Controller** (thử `VBoxVGA` hoặc `VMSVGA`), tăng **Video Memory** lên tối đa. |
| Không vào được mạng | Tắt máy ảo, vào **Network → Advanced → Adapter Type**, chọn `Intel PRO/1000 MT Desktop`. |
| Chuột bị "nhốt" trong máy ảo | Bấm phím **Ctrl bên phải** (phím Host của VirtualBox) để nhả chuột. Đặt **Pointing Device** là `USB Tablet` để không bị lại. |
| Máy ảo lại vào bộ cài sau khi cài xong | Chưa gỡ ISO. Làm lại mục 4.4. |
| `make: … makefile-engine: No such file` | Thiếu gói: `pkgman install makefile_engine haiku_devel`. |
| `gcc: command not found` | `pkgman install gcc binutils make`. |
| Lỗi `fatal error: wchar.h: No such file or directory` hay thiếu header hệ thống | Thiếu `haiku_devel`: `pkgman install haiku_devel`. |
| `ssh: connect to host 127.0.0.1 port 2222: Connection refused` | Chưa thêm Port Forwarding (mục 3.2), hoặc sshd chưa chạy trong máy ảo (`ps \| grep sshd`, mục 8.1). |
| SSH báo *Permission denied* | Chưa đặt mật khẩu (`passwd`) hoặc chưa sửa `sshd_config` rồi chạy lại sshd (mục 8.1). |
| `pkgman` báo lỗi kết nối kho gói | Kiểm tra mạng (`ping`). Nếu mạng ổn, chạy `pkgman refresh` rồi thử lại. |

---

## Tài liệu tham khảo

- Trang tải Haiku: https://www.haiku-os.org/get-haiku/
- Hướng dẫn cài Haiku chính thức: https://www.haiku-os.org/get-haiku/installation-guide
- Hướng dẫn chạy Haiku trong VirtualBox (tiếng Anh): https://www.haiku-os.org/guides/virtualizing/virtualbox
- Hướng dẫn dùng makefile-engine: tài liệu đi kèm Haiku tại `/boot/system/develop/etc/`
- Tài liệu BeAPI (Haiku Book): https://www.haiku-os.org/docs/api/
