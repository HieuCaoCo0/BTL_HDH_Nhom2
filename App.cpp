// App.cpp - Điểm khởi động của SimpleEdit
#include <Application.h>
#include <Entry.h>

#include "EditorWindow.h"


static const char* kAppSignature = "application/x-vnd.ptit-SimpleEdit";


class App : public BApplication {
public:
	App()
		:
		BApplication(kAppSignature),
		fWindow(NULL)
	{
	}

	virtual void ReadyToRun()
	{
		_Window()->Show();
	}

	// Chạy từ Terminal: SimpleEdit ten-file.txt
	virtual void ArgvReceived(int32 argc, char** argv)
	{
		for (int32 i = 1; i < argc; i++) {
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
	EditorWindow* _Window()
	{
		if (fWindow == NULL)
			fWindow = new EditorWindow(BRect(100, 100, 740, 580));
		return fWindow;
	}

	void _OpenRef(const entry_ref& ref)
	{
		// Tin nhắn nằm chờ trong hàng đợi, được xử lý khi cửa sổ chạy
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &ref);
		_Window()->PostMessage(&message);
	}

	EditorWindow*	fWindow;
};


int
main()
{
	App app;
	app.Run();
	return 0;
}
