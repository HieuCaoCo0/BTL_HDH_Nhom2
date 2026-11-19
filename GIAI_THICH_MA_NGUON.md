# Giải thích mã nguồn Haiku Notepad

Tài liệu này giải thích Haiku Notepad hoạt động thế nào: các thành phần chính, luồng xử lý khi người dùng thao tác, và cách từng đoạn code vận hành. Đọc theo thứ tự từ trên xuống là dễ hiểu nhất.

---

## 1. Bức tranh tổng thể

Chương trình gồm 3 lớp, mỗi lớp lo một việc:

```
App  (BApplication)                         App.cpp
 │   khởi động chương trình, nhận file từ dòng lệnh / Tracker
 │
 └── EditorWindow  (BWindow)                EditorWindow.h/.cpp
      │   cửa sổ chính: menu, mở/lưu file, tiêu đề, hỏi khi chưa lưu
      │
      ├── BMenuBar ── Tệp / Sửa / Xem
      ├── BScrollView (thanh cuộn)
      │    └── EditorTextView  (BTextView)  EditorTextView.h/.cpp
      │         vùng gõ chữ + hoàn tác/làm lại nhiều bước
      ├── BFilePanel  (hộp thoại Mở)
      └── BFilePanel  (hộp thoại Lưu)
```

Hai lớp dưới liên lạc với nhau như sau:

- **Cửa sổ → vùng soạn thảo:** gọi hàm trực tiếp (`SetText`, `CanUndo`, `ClearUndoHistory`…).
- **Vùng soạn thảo → cửa sổ:** mỗi khi văn bản đổi, `EditorTextView` gọi `EditorWindow::TextChanged()` để cửa sổ thêm dấu `*` vào tiêu đề.

---

## 2. Kiến thức BeAPI cần nắm trước

Nếu bạn đã quen Qt hay WinForms thì phần này giúp "dịch" khái niệm sang Haiku.

| Khái niệm | Ý nghĩa | Tương đương gần nhất |
|---|---|---|
| `BApplication` | Đối tượng ứng dụng, mỗi chương trình có đúng một. Kết nối với `app_server` (máy chủ đồ hoạ). | `QApplication` |
| `BWindow` | Một cửa sổ. **Mỗi cửa sổ chạy trên một luồng riêng.** | `QMainWindow` |
| `BMessage` | Một "tin nhắn": có mã `what` (4 ký tự, ví dụ `'save'`) và có thể kèm dữ liệu. Mọi sự kiện đều là tin nhắn. | signal / event |
| `MessageReceived()` | Hàm ảo nhận tin nhắn; ta ghi đè để xử lý tin nhắn của mình. | slot / `event()` |
| `BView` | Một vùng vẽ trong cửa sổ (nút, ô chữ…). | `QWidget` |
| `BTextView` | View soạn thảo văn bản có sẵn. | `QPlainTextEdit` |
| `entry_ref` | Cách Haiku "chỉ tới" một file (thư mục + tên). | đường dẫn file |
| `BFile` | Đọc/ghi nội dung file. | `QFile` |
| `BFilePanel` | Hộp thoại chọn file để Mở / Lưu. | `QFileDialog` |
| `BAlert` | Hộp thoại thông báo có tối đa 3 nút. | `QMessageBox` |

**Ý quan trọng nhất:** trên Haiku, hầu như mọi thứ là tin nhắn. Bấm menu, chọn file trong hộp thoại, kéo thả, phím tắt… đều tạo ra một `BMessage` và đưa vào **hàng đợi** của cửa sổ. Luồng của cửa sổ lấy từng tin nhắn ra và gọi `MessageReceived()`. Vì chỉ một luồng xử lý lần lượt, code trong cửa sổ không phải lo hai việc chạy cùng lúc.

Riêng **gõ phím** đi đường tắt: cửa sổ gọi thẳng `KeyDown()` của view đang được focus, không qua `MessageReceived()`.

---

## 3. Chức năng

| Chức năng | Ở đâu trong code |
|---|---|
| Tài liệu mới | `MSG_NEW` → `_ConfirmDiscardChanges()` → `_NewDocument()` |
| Mở file | `MSG_OPEN` → hộp thoại Mở → `B_REFS_RECEIVED` → `_LoadFile()` |
| Mở file từ Terminal / Tracker | `App::ArgvReceived()`, `App::RefsReceived()` → `_OpenRef()` |
| Lưu | `MSG_SAVE` → `_Save()` → `_SaveFile()` |
| Lưu thành… | `MSG_SAVE_AS` → `_SaveAs()` → hộp thoại Lưu → `B_SAVE_REQUESTED` → `_SaveFile()` |
| Hỏi lưu khi còn thay đổi | `_ConfirmDiscardChanges()` |
| Dấu `*` trên tiêu đề | `TextChanged()` → `_UpdateTitle()` |
| Hoàn tác / Làm lại nhiều bước | `EditorTextView::UndoEdit()`, `RedoEdit()` |
| Bật/tắt mục Hoàn tác, Làm lại | `EditorWindow::MenusBeginning()` |
| Cắt / Sao chép / Dán / Chọn tất cả | có sẵn trong `BTextView` |
| Tự xuống dòng | `MSG_TOGGLE_WRAP` |
| Thoát | `QuitRequested()` |

---

## 4. Luồng hoạt động

### 4.1. Khởi động

```
main()
 └─ App app;              tạo BApplication với chữ ký "application/x-vnd.ptit-HaikuNotepad"
 └─ app.Run();            vòng lặp nhận tin nhắn, Haiku lần lượt gọi:
     ├─ ArgvReceived()    (nếu chạy từ Terminal có tham số)
     │    └─ _OpenRef()   tạo cửa sổ + gửi B_REFS_RECEIVED vào hàng đợi của cửa sổ
     ├─ RefsReceived()    (nếu mở bằng cách kéo file / "Open with…")
     │    └─ _OpenRef()
     └─ ReadyToRun()      luôn được gọi
          └─ _Window()->Show()   hiện cửa sổ; luồng cửa sổ bắt đầu chạy,
                                 xử lý B_REFS_RECEIVED đang chờ → mở file
```

Vì sao `_OpenRef()` không mở file ngay mà gửi tin nhắn? Cửa sổ chạy trên luồng riêng. Gửi tin nhắn để chính luồng cửa sổ tự mở file là cách an toàn, và cũng dùng lại được đúng đoạn code mở file của hộp thoại Mở.

### 4.2. Gõ một ký tự

```
Người dùng bấm phím "a"
 └─ EditorTextView::KeyDown()          (ghi đè)
     ├─ _BeginAction(true)             đánh dấu: đang có một thao tác gõ phím
     ├─ BTextView::KeyDown()           lớp cha xử lý phím
     │    └─ InsertText("a", ...)      hàm ảo → vào bản ghi đè của ta:
     │         ├─ BTextView::InsertText()   chèn thật vào bộ đệm
     │         ├─ _Record(...)              ghi lại vào lịch sử undo
     │         └─ fOwner->TextChanged()     báo cửa sổ
     │              └─ _UpdateTitle()       tiêu đề thành "*Chưa đặt tên - Haiku Notepad"
     └─ _EndAction()                   kết thúc thao tác
```

Backspace và Delete đi theo đúng đường này, chỉ khác là gọi `DeleteText()` thay vì `InsertText()`.

### 4.3. Mở file

```
Tệp > Mở…  (Cmd+O)
 └─ tin nhắn MSG_OPEN → MessageReceived()
     └─ fOpenPanel->Show()             hiện hộp thoại, rồi hàm trả về ngay

... người dùng chọn file và bấm Open ...

 └─ hộp thoại gửi B_REFS_RECEIVED (kèm entry_ref của file) → MessageReceived()
     ├─ _ConfirmDiscardChanges()       còn thay đổi chưa lưu? hỏi người dùng
     └─ _LoadFile(ref)
          ├─ mở BFile, lấy kích thước, từ chối nếu > 64 MB
          ├─ fTextView->SetText(&file, ...)   đọc nội dung vào vùng soạn thảo
          ├─ fTextView->ClearUndoHistory()    lịch sử của tài liệu cũ bỏ đi
          └─ ghi nhớ fRef, đặt fModified = false, cập nhật tiêu đề
```

Hộp thoại không "trả về" tên file như `GetOpenFileName` trên Windows. Nó **gửi tin nhắn** về sau, nên việc mở file thật nằm ở nhánh `B_REFS_RECEIVED`.

### 4.4. Lưu file

```
Tệp > Lưu  (Cmd+S)
 └─ MSG_SAVE → _Save()
     ├─ đã có file (fHasRef = true)  → _SaveFile(fRef)  ghi đè, xong
     └─ chưa có file                 → _SaveAs()
                                        └─ hiện hộp thoại Lưu

... người dùng nhập tên và bấm Save ...

 └─ hộp thoại gửi B_SAVE_REQUESTED (kèm "directory" + "name") → MessageReceived()
     ├─ ghép thư mục + tên thành entry_ref
     └─ _SaveFile(ref)
          ├─ mở BFile (tạo mới nếu chưa có, xoá nội dung cũ)
          ├─ ghi toàn bộ văn bản; kiểm tra ghi đủ số byte
          ├─ gán kiểu MIME text/plain nếu file chưa có kiểu
          └─ fModified = false, cập nhật tiêu đề (mất dấu *)
```

Lỗi bất kỳ (không có quyền ghi, đĩa đầy…) được trả về dạng `status_t`, rồi `_ShowError()` hiện hộp thoại báo lỗi.

### 4.5. Hỏi khi chưa lưu

`_ConfirmDiscardChanges()` được gọi trước khi **tạo mới**, **mở file khác** và **thoát**. Hàm trả về `true` nếu được phép bỏ nội dung hiện tại:

| Tình huống | Kết quả |
|---|---|
| Không có thay đổi | `true`, không hỏi gì |
| Chọn "Huỷ" (hoặc Esc) | `false`, giữ nguyên |
| Chọn "Không lưu" | `true`, bỏ thay đổi |
| Chọn "Lưu", tài liệu đã có file | lưu; thành công thì `true` |
| Chọn "Lưu", tài liệu chưa có tên | mở hộp thoại Lưu và trả `false`. Lưu xong người dùng làm lại thao tác. |

### 4.6. Đóng cửa sổ / thoát

```
Bấm nút đóng, Tệp > Thoát, hoặc Cmd+Q
 └─ EditorWindow::QuitRequested()
     ├─ _ConfirmDiscardChanges() == false → return false (cửa sổ ở lại)
     └─ be_app->PostMessage(B_QUIT_REQUESTED)   báo ứng dụng thoát
        return true                              cửa sổ đóng
```

Ứng dụng chỉ có một cửa sổ, nên đóng cửa sổ cũng là thoát chương trình. `app.Run()` trong `main()` trả về và chương trình kết thúc.

### 4.7. Hoàn tác (Cmd+Z)

```
Người dùng bấm Cmd+Z
 └─ BWindow thấy phím có Cmd → gọi MenusBeginning() trước
     └─ EditorWindow::MenusBeginning()
          bật/tắt "Hoàn tác" theo CanUndo(), "Làm lại" theo CanRedo()
 └─ tìm mục menu có phím tắt Z; nếu đang bật thì gửi B_UNDO tới text view
     └─ EditorTextView::MessageReceived() → UndoEdit()
```

Menu Sửa được đặt "target" là text view (`editMenu->SetTargetForItems(fTextView)`), nên B_UNDO, B_REDO, B_CUT… đi thẳng tới vùng soạn thảo chứ không qua cửa sổ.

---

## 5. Cơ chế hoàn tác nhiều bước

Đây là phần phức tạp nhất của chương trình, nằm trọn trong `EditorTextView`.

### 5.1. Vì sao phải tự làm

`BTextView` có sẵn undo (`SetDoesUndo(true)`), nhưng nó chỉ giữ **một** bộ đệm undo. Mỗi thao tác mới xoá bộ đệm cũ, nên chỉ hoàn tác được thao tác gần nhất. Vì thế `EditorTextView` tắt cơ chế đó (`SetDoesUndo(false)` trong hàm khởi tạo) và tự quản lý lịch sử.

### 5.2. Điểm then chốt: mọi thay đổi đều qua hai hàm

Trong mã nguồn Haiku, mọi cách sửa văn bản của `BTextView` (gõ phím, Backspace, Delete, cắt, dán, kéo thả, bộ gõ, `SetText`) cuối cùng đều gọi một trong hai **hàm ảo** (trừ một ngoại lệ nêu bên dưới):

- `InsertText(text, length, offset, runs)`: chèn văn bản tại `offset`.
- `DeleteText(fromOffset, toOffset)`: xoá đoạn `[fromOffset, toOffset)`.

Ghi đè hai hàm này thì **không bỏ sót thay đổi nào** do người dùng gây ra. Đây là lý do cách làm này đáng tin cậy.

Ngoại lệ duy nhất là `SetText(BFile*, …)` (dùng khi mở file): nó xoá nội dung cũ qua `DeleteText()`, nhưng chèn nội dung file thẳng vào bộ đệm. Điều này không gây hại, vì ngay sau đó `_LoadFile()` gọi `ClearUndoHistory()`.

### 5.3. Dữ liệu lưu trữ

```cpp
struct EditStep {          // một thao tác cơ bản
    bool        isInsert;  // true = chèn, false = xoá
    int32       offset;    // vị trí (tính bằng byte)
    std::string text;      // đoạn văn bản được chèn / bị xoá
};

struct EditGroup {                // một bước undo
    std::vector<EditStep> steps;  // một hay nhiều thao tác cơ bản
    int32 selectionStart;         // vùng chọn TRƯỚC khi sửa,
    int32 selectionEnd;           //   để undo xong chọn lại đúng chỗ
    bool  typing;                 // bước này sinh ra từ gõ phím?
};

std::deque<EditGroup> fUndoStack;  // các bước đã làm (mới nhất ở cuối)
std::deque<EditGroup> fRedoStack;  // các bước đã hoàn tác (mới nhất ở cuối)
```

Vì sao một bước lại có nhiều thao tác? Ví dụ **dán đè lên vùng chọn** gồm 2 thao tác: xoá vùng chọn rồi chèn chữ dán vào. Người dùng mong một lần Cmd+Z hoàn tác cả hai.

Vì sao lưu cả đoạn văn bản bị xoá? Muốn hoàn tác một lần xoá thì phải chèn lại đúng đoạn đã mất, nên phải giữ nó.

Dùng `std::deque` vì khi vượt 1000 bước, ta bỏ bước **cũ nhất** ở đầu hàng (`pop_front`), thao tác này nhanh trên deque.

### 5.4. Ghi lại một thay đổi

Ví dụ với `DeleteText()`:

```cpp
void EditorTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
    // 1. Chuẩn hoá offset y như lớp cha, để ghi đúng cái sẽ thực sự bị xoá
    // 2. Lấy đoạn sắp bị xoá bằng GetText() — PHẢI làm trước khi xoá
    // 3. BTextView::DeleteText(...)   — xoá thật
    // 4. _Record(false, fromOffset, deleted)  — ghi vào lịch sử
    // 5. fOwner->TextChanged()        — báo cửa sổ
}
```

Hai chi tiết dễ sai:

- **Lấy đoạn bị xoá trước khi xoá.** Xoá rồi thì không còn gì để lấy.
- **Dùng `GetText()` chứ không dùng `Text()`.** `BTextView` lưu văn bản trong một *gap buffer* (bộ đệm có "khoảng trống" ở vị trí con trỏ để chèn nhanh). `Text()` phải dồn cả bộ đệm thành một chuỗi liền, file lớn thì mỗi lần Backspace rất tốn. `GetText()` chỉ chép đúng đoạn cần.

`InsertText()` làm tương tự, nhưng **chép văn bản trước khi gọi lớp cha**. Lý do: khi tự thụt lề (auto-indent), con trỏ `text` trỏ vào chính bộ đệm của view, và chèn xong thì bộ đệm có thể bị cấp phát lại.

### 5.5. Gộp thao tác thành bước undo

Nếu mỗi ký tự là một bước, gõ một câu xong phải bấm Cmd+Z mấy chục lần. Vì vậy chương trình gộp theo các quy tắc sau.

**Khái niệm "thao tác người dùng":** mỗi lần `KeyDown()` (một lần bấm phím) hoặc `MessageReceived()` (một lệnh: cắt, dán, kéo thả…) là một thao tác. `_BeginAction()` đánh dấu bắt đầu, `_EndAction()` đánh dấu kết thúc. **Mọi thay đổi trong cùng một thao tác luôn vào cùng một bước.**

**Gõ phím liên tiếp còn được gộp tiếp** sang bước gõ trước đó (`_CanContinueTyping()`), khi thoả hết các điều kiện:

1. Bước trước cũng là gõ phím (`typing = true`).
2. Cùng loại: chèn tiếp chèn, hoặc xoá tiếp xoá.
3. Liền vị trí: chữ mới nằm ngay sau chữ vừa gõ; Backspace xoá ngay trước đoạn vừa xoá; Delete xoá ngay tại chỗ vừa xoá.
4. Không phải xuống dòng: Enter luôn mở bước mới.
5. Không bắt đầu từ mới: vừa gõ khoảng trắng mà giờ gõ chữ thì mở bước mới.
6. Không gõ đè lên vùng chọn: gõ khi đang bôi đen thì luôn là bước riêng.

**Ví dụ:** gõ `xin chao ban`, rồi Backspace 3 lần, rồi Enter.

| Bước | Nội dung | Vì sao tách bước |
|---|---|---|
| 1 | chèn `"xin "` | bắt đầu |
| 2 | chèn `"chao "` | sau khoảng trắng gõ chữ mới (quy tắc 5) |
| 3 | chèn `"ban"` | như trên |
| 4 | xoá `"ban"` | đổi từ chèn sang xoá (quy tắc 2) |
| 5 | chèn `"\n"` | xuống dòng (quy tắc 4) |

Ngoài ra, `_Coalesce()` nối các thao tác cơ bản liền nhau trong cùng một bước. Gõ `c`, `h`, `a`, `o` không tạo 4 `EditStep` mà chỉ một `EditStep` với `text = "chao"`, giúp lịch sử gọn và undo nhanh hơn.

### 5.6. Hoàn tác và làm lại

**Hoàn tác** (`UndoEdit()`):

1. Lấy bước cuối của `fUndoStack`.
2. Duyệt các thao tác **từ cuối về đầu**, làm ngược từng cái: thao tác *chèn* thì xoá đoạn đó đi, thao tác *xoá* thì chèn lại đoạn đã xoá.
3. Chọn lại vùng chọn cũ (`selectionStart`, `selectionEnd`).
4. Đẩy bước đó sang `fRedoStack`.

**Làm lại** (`RedoEdit()`) làm điều ngược lại: lấy bước cuối của `fRedoStack`, làm **xuôi** từng thao tác, đặt con trỏ ở cuối thao tác cuối cùng, rồi trả bước về `fUndoStack`.

Vì sao phải duyệt ngược khi hoàn tác? Lấy ví dụ dán `"hello"` đè lên vùng chọn `"chao"` ở offset 4. Bước này gồm (1) xoá `"chao"` tại 4, (2) chèn `"hello"` tại 4. Muốn quay lại phải gỡ `"hello"` trước rồi mới chèn `"chao"` vào. Làm sai thứ tự sẽ ra văn bản lộn xộn.

**Ví dụ chạy tay:**

```
Văn bản: "xin chao ban"    chọn "chao" (offset 4–8), dán "hello"
  → bước = [ xoá "chao" @4 , chèn "hello" @4 ]
Văn bản: "xin hello ban"

Cmd+Z:  làm ngược, từ cuối về đầu
  - ngược của (chèn "hello" @4) = xoá 5 byte tại 4   → "xin  ban"
  - ngược của (xoá "chao" @4)   = chèn "chao" tại 4  → "xin chao ban"
  - chọn lại 4–8 ("chao" được bôi đen như trước khi dán)

Cmd+Shift+Z:  làm xuôi
  - xoá "chao" @4   → "xin  ban"
  - chèn "hello" @4 → "xin hello ban"
```

### 5.7. Những chi tiết nhỏ nhưng quan trọng

- **`fReplaying`.** Khi hoàn tác, ta gọi `Insert()`/`Delete()` của `BTextView`, và chúng lại gọi `InsertText()`/`DeleteText()` của ta. Nếu không chặn, chính thao tác hoàn tác sẽ bị ghi vào lịch sử. Cờ `fReplaying = true` bảo `_Record()` bỏ qua. `TextChanged()` vẫn được gọi, nên hoàn tác cũng làm hiện dấu `*`, đúng vì nội dung đã đổi.
- **Thay đổi mới xoá `fRedoStack`.** Đang hoàn tác dở mà gõ chữ mới thì nhánh "tương lai" cũ không còn ý nghĩa, giống mọi trình soạn thảo khác.
- **Sau khi hoàn tác/làm lại**, bước trên cùng được đánh dấu `typing = false` để chữ gõ tiếp theo không bị gộp nhầm vào bước cũ.
- **`ClearUndoHistory()`** được gọi khi mở file hoặc tạo mới. Lịch sử của tài liệu trước không áp dụng được cho tài liệu khác (offset sẽ sai hoàn toàn).
- **Thay đổi ngoài thao tác người dùng** (ví dụ code gọi `SetText()`) vẫn được ghi, mỗi lần gọi là một bước riêng, để lịch sử luôn khớp với văn bản.
- **Offset tính bằng byte, không phải ký tự.** Văn bản lưu dạng UTF-8 nên chữ `à` chiếm 2 byte. Vì ta lưu và phát lại đúng từng byte, chữ tiếng Việt vẫn hoàn tác đúng.
- **Ghi đè `Undo(BClipboard*)`**: đây là hàm ảo cũ của `BTextView`. Ghi đè để nếu có chỗ nào gọi nó thì cũng dùng undo mới.

---

## 6. Tra cứu nhanh các hàm

### `App.cpp`

| Hàm | Việc làm |
|---|---|
| `main()` | Tạo `App`, chạy vòng lặp tin nhắn. |
| `ArgvReceived()` | Lấy tên file từ dòng lệnh, mở file đầu tiên. |
| `RefsReceived()` | Nhận file được mở từ Tracker. |
| `ReadyToRun()` | Hiện cửa sổ. |
| `_Window()` | Tạo cửa sổ ở lần gọi đầu, sau đó dùng lại. |
| `_OpenRef()` | Gửi tin nhắn mở file vào hàng đợi của cửa sổ. |

### `EditorWindow.cpp`

| Hàm | Việc làm |
|---|---|
| Hàm khởi tạo | Dựng menu, vùng soạn thảo, bố cục, hai hộp thoại. |
| `MessageReceived()` | Phân loại và xử lý tin nhắn từ menu, hộp thoại. |
| `MenusBeginning()` | Bật/tắt mục Hoàn tác, Làm lại. |
| `QuitRequested()` | Hỏi lưu rồi cho phép / không cho phép đóng. |
| `TextChanged()` | Đánh dấu có thay đổi, thêm `*` vào tiêu đề. |
| `_LoadFile()` | Đọc file vào vùng soạn thảo. |
| `_SaveFile()` | Ghi nội dung ra file, gán kiểu MIME. |
| `_Save()` / `_SaveAs()` | Lệnh Lưu / Lưu thành…. |
| `_ConfirmDiscardChanges()` | Hỏi "Huỷ / Không lưu / Lưu". |
| `_NewDocument()` | Xoá trắng, về tài liệu chưa có tên. |
| `_UpdateTitle()` / `_DocumentName()` | Dựng tiêu đề cửa sổ. |
| `_ShowError()` | Hiện hộp thoại báo lỗi. |

### `EditorTextView.cpp`

| Hàm | Việc làm |
|---|---|
| `InsertText()` / `DeleteText()` | Sửa văn bản thật, ghi lịch sử, báo cửa sổ. |
| `KeyDown()` / `MessageReceived()` | Bao quanh mỗi thao tác người dùng bằng `_BeginAction()` / `_EndAction()`; nhận `B_UNDO`, `B_REDO`. |
| `UndoEdit()` / `RedoEdit()` | Hoàn tác / làm lại một bước. |
| `CanUndo()` / `CanRedo()` | Còn bước để hoàn tác / làm lại không. |
| `ClearUndoHistory()` | Xoá toàn bộ lịch sử. |
| `_Record()` | Ghi một thao tác cơ bản vào bước hiện tại hoặc mở bước mới. |
| `_CanContinueTyping()` | Quyết định có gộp lần gõ này vào bước gõ trước không. |
| `_Coalesce()` | Nối thao tác mới vào thao tác cuối nếu liền nhau. |
| `_ApplyStep()` | Thực hiện (hoặc làm ngược) một thao tác cơ bản. |

---

## 7. Muốn thêm một lệnh menu mới thì làm sao?

Ví dụ thêm lệnh "Xoá trắng":

1. Thêm mã tin nhắn vào `enum` đầu `EditorWindow.cpp`, ví dụ `MSG_CLEAR = 'clr_'`.
2. Thêm mục menu trong hàm khởi tạo:
   `editMenu->AddItem(new BMenuItem("Xoá trắng", new BMessage(MSG_CLEAR)));`
   Lưu ý: menu Sửa đang gửi mọi tin nhắn tới text view (`SetTargetForItems`). Lệnh muốn cửa sổ xử lý thì phải gọi `item->SetTarget(this)` sau dòng đó, hoặc đặt ở menu khác.
3. Thêm nhánh `case MSG_CLEAR:` trong `MessageReceived()` để xử lý.
4. Nếu lệnh sửa văn bản qua `Insert()`/`Delete()` của text view thì **undo tự hoạt động**, không cần làm gì thêm.
5. Build lại (`make`) và mở app trên Haiku để thử.

---

## 8. Quy ước code

Dự án theo coding style của Haiku:

- Thụt lề bằng tab.
- Biến thành viên bắt đầu bằng `f` (`fTextView`, `fModified`).
- Hàm private bắt đầu bằng `_` (`_LoadFile`, `_Record`).
- Trong file `.cpp`, kiểu trả về nằm trên một dòng riêng, tên hàm ở dòng dưới.
- Chuỗi giao diện viết bằng tiếng Việt, file lưu dạng UTF-8.
