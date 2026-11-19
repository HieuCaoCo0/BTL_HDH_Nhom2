// EditorWindow.h - Cửa sổ chính của trình soạn thảo Haiku Notepad
//
// Quy ước đặt tên theo coding style của Haiku:
//   fTen    - biến thành viên (field)
//   _Ten()  - hàm private, chỉ dùng trong lớp
#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

#include <Entry.h>
#include <Window.h>

class BFilePanel;
class BMenuItem;
class BScrollView;
class EditorTextView;

class EditorWindow : public BWindow {
public:
							EditorWindow(BRect frame);
	virtual					~EditorWindow();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			MenusBeginning();
	virtual	bool			QuitRequested();

			// EditorTextView gọi hàm này mỗi khi nội dung thay đổi
			void			TextChanged();

private:
			status_t		_LoadFile(const entry_ref& ref);
			status_t		_SaveFile(const entry_ref& ref);
			void			_Save();
			void			_SaveAs();
			bool			_ConfirmDiscardChanges();
			void			_NewDocument();
			void			_UpdateTitle();
			const char*		_DocumentName() const;
			void			_ShowError(const char* action, status_t error);

			EditorTextView*	fTextView;		// vùng soạn thảo
			BScrollView*	fScrollView;	// khung cuộn bọc vùng soạn thảo
			BMenuItem*		fUndoItem;		// Sửa > Hoàn tác
			BMenuItem*		fRedoItem;		// Sửa > Làm lại
			BMenuItem*		fWrapItem;		// Xem > Tự xuống dòng
			BFilePanel*		fOpenPanel;		// hộp thoại Mở
			BFilePanel*		fSavePanel;		// hộp thoại Lưu

			entry_ref		fRef;		// file đang mở
			bool			fHasRef;	// đã có file trên đĩa chưa
			bool			fModified;	// có thay đổi chưa lưu không
};

#endif // EDITOR_WINDOW_H
