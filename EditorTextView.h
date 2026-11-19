// EditorTextView.h - Vùng soạn thảo của Haiku Notepad, có undo/redo nhiều bước
#ifndef EDITOR_TEXT_VIEW_H
#define EDITOR_TEXT_VIEW_H

#include <TextView.h>

#include <deque>
#include <string>
#include <vector>

class EditorWindow;


// BTextView chỉ có undo một bước (giữ đúng một UndoBuffer, thao tác mới
// sẽ xoá cái cũ). Lớp này tắt undo có sẵn và tự ghi lịch sử: mọi thay đổi
// văn bản của BTextView (gõ phím, xoá, cắt, dán, kéo thả, input method...)
// đều đi qua hai hàm ảo InsertText/DeleteText, nên ta ghi lại tại đó.
class EditorTextView : public BTextView {
public:
								EditorTextView(const char* name,
									EditorWindow* owner);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				Undo(BClipboard* clipboard);

			bool				CanUndo() const;
			bool				CanRedo() const;
			void				UndoEdit();
			void				RedoEdit();
			void				ClearUndoHistory();

protected:
	virtual	void				InsertText(const char* text, int32 length,
									int32 offset, const text_run_array* runs);
	virtual	void				DeleteText(int32 fromOffset, int32 toOffset);

private:
			// Một thao tác cơ bản: chèn hoặc xoá một đoạn byte tại offset
			struct EditStep {
				bool			isInsert;
				int32			offset;
				std::string		text;
			};

			// Một bước undo: gồm một hay nhiều thao tác cơ bản, ví dụ
			// "dán đè lên vùng chọn" = xoá vùng chọn + chèn văn bản.
			struct EditGroup {
				std::vector<EditStep> steps;
				int32			selectionStart;	// vùng chọn trước khi sửa
				int32			selectionEnd;
				bool			typing;			// sinh ra từ gõ phím
			};

			void				_BeginAction(bool typing);
			void				_EndAction();
			void				_Record(bool isInsert, int32 offset,
									const std::string& text);
			bool				_CanContinueTyping(const EditGroup& group,
									bool isInsert, int32 offset,
									const std::string& text) const;
			bool				_Coalesce(EditStep& last, bool isInsert,
									int32 offset,
									const std::string& text) const;
			void				_ApplyStep(const EditStep& step,
									bool reverse);

			EditorWindow*		fOwner;

			std::deque<EditGroup> fUndoStack;
			std::deque<EditGroup> fRedoStack;

			bool				fInAction;		// đang trong KeyDown/MessageReceived
			bool				fActionTyping;	// thao tác hiện tại là gõ phím
			bool				fActionHasGroup;	// thao tác này đã có nhóm
			bool				fBreakTyping;	// không gộp vào nhóm gõ trước
			int32				fActionSelStart;
			int32				fActionSelEnd;
			bool				fReplaying;		// đang undo/redo, không ghi
};

#endif // EDITOR_TEXT_VIEW_H
