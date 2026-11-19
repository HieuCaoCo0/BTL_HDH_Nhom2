// App.cpp - Điểm khởi động của Haiku Notepad
//
// Mỗi chương trình có giao diện trên Haiku đều có đúng một đối tượng
// BApplication. Nó kết nối với app_server (máy chủ đồ hoạ của Haiku) và
// nhận các tin nhắn ở mức ứng dụng: tham số dòng lệnh, file được mở từ
// Tracker (trình quản lý file của Haiku), yêu cầu thoát...
//
// Thứ tự Haiku gọi các hàm của BApplication khi khởi động:
//   1. ArgvReceived()  - nếu chạy từ Terminal và có tham số
//   2. RefsReceived()  - nếu được mở bằng cách kéo file vào / "Open with…"
//   3. ReadyToRun()    - luôn được gọi, sau hai hàm trên
#include <Application.h>
#include <Entry.h>

#include "EditorWindow.h"


// Chữ ký (signature) của ứng dụng: tên duy nhất dạng kiểu MIME để Haiku
// phân biệt ứng dụng này với ứng dụng khác. Phải giống APP_MIME_SIG trong
// Makefile.
static const char* kAppSignature = "application/x-vnd.ptit-HaikuNotepad";


class App : public BApplication {
public:
	App()
		:
		BApplication(kAppSignature),
		fWindow(NULL)
	{
	}

	// Mọi tin nhắn khởi động đã xử lý xong: hiện cửa sổ lên màn hình.
	// Nếu trước đó đã có file được mở, cửa sổ đã được tạo sẵn và tin nhắn
	// mở file đang chờ trong hàng đợi của nó.
	virtual void ReadyToRun()
	{
		_Window()->Show();
	}

	// Chạy từ Terminal: HaikuNotepad ten-file.txt
	// argv[0] là tên chương trình nên bắt đầu từ argv[1].
	virtual void ArgvReceived(int32 argc, char** argv)
	{
		for (int32 i = 1; i < argc; i++) {
			// BEntry chuyển đường dẫn thành entry_ref (cách Haiku tham chiếu
			// tới một file). true = đi theo symlink tới file thật.
			BEntry entry(argv[i], true);
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				_OpenRef(ref);
				break;	// chỉ một cửa sổ nên chỉ mở file đầu tiên
			}
		}
	}

	// Mở file từ Tracker (kéo thả lên biểu tượng, "Open with…")
	virtual void RefsReceived(BMessage* message)
	{
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref) == B_OK)
			_OpenRef(ref);
	}

private:
	// Tạo cửa sổ ở lần gọi đầu tiên, những lần sau trả về cửa sổ đã có
	EditorWindow* _Window()
	{
		if (fWindow == NULL)
			fWindow = new EditorWindow(BRect(100, 100, 740, 580));
		return fWindow;
	}

	// Không mở file trực tiếp ở đây: cửa sổ chạy trên luồng (thread) riêng
	// của nó. Ta gửi tin nhắn B_REFS_RECEIVED vào hàng đợi của cửa sổ, và
	// cửa sổ tự mở file trong EditorWindow::MessageReceived().
	void _OpenRef(const entry_ref& ref)
	{
		// Tin nhắn nằm chờ trong hàng đợi, được xử lý khi cửa sổ chạy
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &ref);
		_Window()->PostMessage(&message);
	}

	EditorWindow*	fWindow;	// cửa sổ duy nhất của ứng dụng
};


int
main()
{
	App app;
	app.Run();	// vòng lặp nhận tin nhắn, chỉ trả về khi ứng dụng thoát
	return 0;
}
