// EditorWindow.cpp - Cửa sổ chính của trình soạn thảo Haiku Notepad
//
// Trên Haiku, mỗi BWindow chạy trên một luồng (thread) riêng và xử lý lần
// lượt các tin nhắn (BMessage) trong hàng đợi của nó. Bấm menu, chọn file
// trong hộp thoại, kéo file vào... đều sinh ra tin nhắn, và cửa sổ phản ứng
// trong MessageReceived(). Vì chỉ một luồng xử lý, không cần khoá dữ liệu.
//
// Cửa sổ này lo phần "tài liệu": menu, mở/lưu file, tiêu đề, hỏi khi chưa
// lưu. Phần soạn thảo và hoàn tác nằm trong EditorTextView.
#include "EditorWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <ScrollView.h>
#include <String.h>

#include <string.h>

#include "EditorTextView.h"


// Mã của các tin nhắn riêng do menu gửi tới cửa sổ. Haiku quy ước mã là
// 4 ký tự, viết thường để không trùng mã hệ thống (mã hệ thống bắt đầu bằng
// '_' hoặc chữ hoa, ví dụ B_QUIT_REQUESTED = '_QRQ').
enum {
	MSG_NEW			= 'new_',
	MSG_OPEN		= 'open',
	MSG_SAVE		= 'save',
	MSG_SAVE_AS		= 'svas',
	MSG_TOGGLE_WRAP	= 'wrap'
};

// Giới hạn kích thước file mở được (64 MB), tránh treo khi lỡ mở file lớn
static const off_t kMaxFileSize = 64 * 1024 * 1024;


// Dựng toàn bộ giao diện: thanh menu, vùng soạn thảo có thanh cuộn,
// hai hộp thoại Mở/Lưu.
EditorWindow::EditorWindow(BRect frame)
	:
	BWindow(frame, "Haiku Notepad", B_DOCUMENT_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fTextView(NULL),
	fScrollView(NULL),
	fUndoItem(NULL),
	fRedoItem(NULL),
	fWrapItem(NULL),
	fOpenPanel(NULL),
	fSavePanel(NULL),
	fHasRef(false),
	fModified(false)
{
	// --- Thanh menu ---
	// Mỗi BMenuItem mang theo một BMessage; khi người dùng chọn mục đó (bằng
	// chuột hoặc phím tắt Cmd+chữ), tin nhắn được gửi tới "target" của mục.
	// Mặc định target là cửa sổ này.
	BMenuBar* menuBar = new BMenuBar("menubar");

	BMenu* fileMenu = new BMenu("Tệp");
	fileMenu->AddItem(new BMenuItem("Mới", new BMessage(MSG_NEW), 'N'));
	fileMenu->AddItem(new BMenuItem("Mở…", new BMessage(MSG_OPEN), 'O'));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Lưu", new BMessage(MSG_SAVE), 'S'));
	fileMenu->AddItem(new BMenuItem("Lưu thành…", new BMessage(MSG_SAVE_AS),
		'S', B_SHIFT_KEY));
	fileMenu->AddSeparatorItem();
	fileMenu->AddItem(new BMenuItem("Thoát", new BMessage(B_QUIT_REQUESTED),
		'Q'));
	menuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Sửa");
	fUndoItem = new BMenuItem("Hoàn tác", new BMessage(B_UNDO), 'Z');
	editMenu->AddItem(fUndoItem);
	fRedoItem = new BMenuItem("Làm lại", new BMessage(B_REDO), 'Z',
		B_SHIFT_KEY);
	editMenu->AddItem(fRedoItem);
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Cắt", new BMessage(B_CUT), 'X'));
	editMenu->AddItem(new BMenuItem("Sao chép", new BMessage(B_COPY), 'C'));
	editMenu->AddItem(new BMenuItem("Dán", new BMessage(B_PASTE), 'V'));
	editMenu->AddSeparatorItem();
	editMenu->AddItem(new BMenuItem("Chọn tất cả", new BMessage(B_SELECT_ALL),
		'A'));
	menuBar->AddItem(editMenu);

	BMenu* viewMenu = new BMenu("Xem");
	fWrapItem = new BMenuItem("Tự xuống dòng", new BMessage(MSG_TOGGLE_WRAP));
	fWrapItem->SetMarked(true);
	viewMenu->AddItem(fWrapItem);
	menuBar->AddItem(viewMenu);

	// --- Vùng soạn thảo ---
	fTextView = new EditorTextView("text", this);
	fTextView->SetStylable(false);		// chỉ văn bản thuần
	// Undo/redo nhiều bước do EditorTextView tự quản lý (đã tắt undo
	// một bước có sẵn của BTextView)
	fTextView->SetWordWrap(true);
	fTextView->SetInsets(4, 4, 4, 4);
	fTextView->SetFontAndColor(be_fixed_font);	// font đều nét, hợp để code

	// Bọc text view trong BScrollView để có thanh cuộn dọc và ngang
	fScrollView = new BScrollView("scroll", fTextView,
		B_WILL_DRAW | B_FRAME_EVENTS, true, true, B_NO_BORDER);

	// --- Bố cục: menu ở trên, vùng soạn thảo chiếm phần còn lại ---
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.Add(fScrollView);

	// Các lệnh Sửa gửi thẳng tới text view (EditorTextView xử lý
	// B_UNDO, B_REDO; BTextView xử lý B_CUT, B_COPY, B_PASTE, B_SELECT_ALL).
	// Phải gọi sau khi text view đã được gắn vào cửa sổ.
	editMenu->SetTargetForItems(fTextView);

	// --- Hộp thoại mở / lưu file ---
	// Hai hộp thoại được tạo một lần và dùng lại. Khi người dùng chọn file,
	// hộp thoại gửi B_REFS_RECEIVED (Mở) hoặc B_SAVE_REQUESTED (Lưu) về
	// cửa sổ này thông qua messenger.
	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE,
		false);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, B_FILE_NODE,
		false);

	fTextView->MakeFocus(true);
	_UpdateTitle();
}


// Menu, text view... là con của cửa sổ nên BWindow tự xoá. Hai hộp thoại
// không thuộc cửa sổ nên phải tự xoá.
EditorWindow::~EditorWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
}


// Trung tâm xử lý: mọi tin nhắn gửi tới cửa sổ đều đi qua đây.
// message->what cho biết loại tin nhắn.
void
EditorWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW:
			if (_ConfirmDiscardChanges())
				_NewDocument();
			break;

		case MSG_OPEN:
			fOpenPanel->Show();
			break;

		// Đến từ hộp thoại Mở, từ Tracker, hoặc từ dòng lệnh
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;
			if (!_ConfirmDiscardChanges())
				break;

			status_t status = _LoadFile(ref);
			if (status != B_OK)
				_ShowError("mở tệp", status);
			break;
		}

		case MSG_SAVE:
			_Save();
			break;

		case MSG_SAVE_AS:
			_SaveAs();
			break;

		// Đến từ hộp thoại Lưu: gồm thư mục + tên file người dùng nhập
		case B_SAVE_REQUESTED:
		{
			entry_ref directoryRef;
			const char* name;
			if (message->FindRef("directory", &directoryRef) != B_OK
				|| message->FindString("name", &name) != B_OK)
				break;

			BDirectory directory(&directoryRef);
			BEntry entry(&directory, name);
			entry_ref ref;
			status_t status = entry.GetRef(&ref);
			if (status == B_OK)
				status = _SaveFile(ref);
			if (status != B_OK)
				_ShowError("lưu tệp", status);
			break;
		}

		case MSG_TOGGLE_WRAP:
		{
			bool wrap = !fTextView->DoesWordWrap();
			fTextView->SetWordWrap(wrap);
			fWrapItem->SetMarked(wrap);
			break;
		}

		// Tin nhắn không phải của ta: chuyển cho lớp cha xử lý
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


// Được gọi khi mở menu, và cả khi bấm phím tắt có Cmd (trước khi tìm phím
// tắt), nên mục Hoàn tác/Làm lại luôn đúng trạng thái.
void
EditorWindow::MenusBeginning()
{
	fUndoItem->SetEnabled(fTextView->CanUndo());
	fRedoItem->SetEnabled(fTextView->CanRedo());
	BWindow::MenusBeginning();
}


// Haiku gọi hàm này khi người dùng bấm nút đóng, chọn Thoát hoặc Cmd+Q.
// Trả về false để giữ cửa sổ lại (ví dụ người dùng chọn "Huỷ").
bool
EditorWindow::QuitRequested()
{
	if (!_ConfirmDiscardChanges())
		return false;

	// Ứng dụng chỉ có một cửa sổ: đóng cửa sổ là thoát luôn
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// EditorTextView gọi hàm này sau mỗi lần chèn/xoá văn bản. Chỉ cập nhật
// tiêu đề ở lần thay đổi đầu tiên (thêm dấu * trước tên tài liệu).
void
EditorWindow::TextChanged()
{
	if (!fModified) {
		fModified = true;
		_UpdateTitle();
	}
}


// Đọc toàn bộ file vào vùng soạn thảo. Trả về B_OK hoặc mã lỗi để nơi gọi
// hiện thông báo.
status_t
EditorWindow::_LoadFile(const entry_ref& ref)
{
	BFile file(&ref, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	off_t size;
	status = file.GetSize(&size);
	if (status != B_OK)
		return status;
	if (size > kMaxFileSize)
		return B_NO_MEMORY;

	fTextView->SetText(&file, 0, (int32)size);
	fTextView->ClearUndoHistory();	// lịch sử của tài liệu cũ không còn đúng
	fTextView->Select(0, 0);
	fTextView->ScrollToSelection();

	// SetText đã gọi DeleteText -> TextChanged (khi xoá nội dung cũ), nên
	// đánh dấu "có thay đổi"; tài liệu vừa mở thì chưa đổi gì, đặt lại.
	fRef = ref;
	fHasRef = true;
	fModified = false;
	_UpdateTitle();
	return B_OK;
}


// Ghi toàn bộ nội dung ra file. B_ERASE_FILE xoá nội dung cũ của file
// trước khi ghi; B_CREATE_FILE tạo file nếu chưa có.
status_t
EditorWindow::_SaveFile(const entry_ref& ref)
{
	BFile file(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	const char* text = fTextView->Text();
	int32 length = fTextView->TextLength();
	ssize_t written = file.Write(text, length);
	if (written < 0)
		return (status_t)written;
	if (written != length)
		return B_IO_ERROR;

	// Gán kiểu MIME cho file mới để Tracker nhận ra là văn bản
	BNodeInfo nodeInfo(&file);
	char type[B_MIME_TYPE_LENGTH];
	if (nodeInfo.GetType(type) != B_OK)
		nodeInfo.SetType("text/plain");

	fRef = ref;
	fHasRef = true;
	fModified = false;
	_UpdateTitle();
	return B_OK;
}


// Lệnh "Lưu": tài liệu đã có file thì ghi đè, chưa có thì hỏi tên.
void
EditorWindow::_Save()
{
	if (!fHasRef) {
		_SaveAs();
		return;
	}

	status_t status = _SaveFile(fRef);
	if (status != B_OK)
		_ShowError("lưu tệp", status);
}


// Lệnh "Lưu thành…": chỉ mở hộp thoại. Việc lưu thật diễn ra sau, khi
// hộp thoại gửi B_SAVE_REQUESTED về MessageReceived().
void
EditorWindow::_SaveAs()
{
	fSavePanel->SetSaveText(fHasRef ? fRef.name : "chua-dat-ten.txt");
	fSavePanel->Show();
}


// Trả về true nếu được phép bỏ nội dung hiện tại (đã lưu, hoặc người dùng
// chọn "Không lưu"). Với file chưa có tên, chọn "Lưu" sẽ mở hộp thoại Lưu
// và huỷ thao tác đang làm; lưu xong người dùng thực hiện lại thao tác đó.
bool
EditorWindow::_ConfirmDiscardChanges()
{
	if (!fModified)
		return true;

	BString text;
	text.SetToFormat("Tài liệu \"%s\" có thay đổi chưa lưu.\n"
		"Bạn có muốn lưu trước không?", _DocumentName());

	BAlert* alert = new BAlert("Chưa lưu", text.String(),
		"Huỷ", "Không lưu", "Lưu",
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	// Go() hiện hộp thoại và chờ người dùng bấm nút, trả về số thứ tự nút
	// (0, 1, 2 theo thứ tự truyền vào ở trên). BAlert tự xoá sau khi đóng.
	switch (alert->Go()) {
		case 0:		// Huỷ
			return false;

		case 1:		// Không lưu
			return true;

		default:	// Lưu
			if (!fHasRef) {
				_SaveAs();
				return false;
			}
			status_t status = _SaveFile(fRef);
			if (status != B_OK) {
				_ShowError("lưu tệp", status);
				return false;
			}
			return true;
	}
}


// Lệnh "Mới": xoá trắng và quay về tài liệu chưa có tên.
void
EditorWindow::_NewDocument()
{
	fTextView->SetText("");
	fTextView->ClearUndoHistory();
	fHasRef = false;
	fModified = false;
	_UpdateTitle();
}


// Tiêu đề dạng "*ten-file.txt - Haiku Notepad"; dấu * = có thay đổi chưa lưu
void
EditorWindow::_UpdateTitle()
{
	BString title;
	if (fModified)
		title << "*";
	title << _DocumentName() << " - Haiku Notepad";
	SetTitle(title.String());
}


const char*
EditorWindow::_DocumentName() const
{
	return fHasRef ? fRef.name : "Chưa đặt tên";
}


// Hiện hộp thoại báo lỗi. strerror() đổi mã lỗi của Haiku thành câu mô tả.
void
EditorWindow::_ShowError(const char* action, status_t error)
{
	BString text;
	text.SetToFormat("Không thể %s:\n%s", action, strerror(error));

	BAlert* alert = new BAlert("Lỗi", text.String(), "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}
