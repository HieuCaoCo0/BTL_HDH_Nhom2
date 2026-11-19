// EditorTextView.cpp - Vùng soạn thảo của Haiku Notepad, có undo/redo nhiều bước
//
// Ý tưởng chính:
//   - Mọi thay đổi văn bản của BTextView đều đi qua InsertText()/DeleteText().
//     Ta ghi đè hai hàm này: gọi lớp cha để sửa thật, rồi ghi lại thao tác
//     (chèn/xoá, ở offset nào, đoạn văn bản nào) vào ngăn xếp fUndoStack.
//   - Undo = lấy bước trên cùng của fUndoStack, làm ngược từng thao tác
//     (chèn <-> xoá) theo thứ tự ngược, rồi chuyển bước đó sang fRedoStack.
//   - Redo = lấy bước trên cùng của fRedoStack, làm lại từng thao tác theo
//     thứ tự xuôi, rồi trả bước đó về fUndoStack.
//   - Có thay đổi mới thì xoá fRedoStack (nhánh "tương lai" cũ không còn).
//
// "Thao tác người dùng" được xác định bằng KeyDown() (một lần bấm phím) và
// MessageReceived() (một lệnh: cắt, dán, kéo thả...). Mọi thay đổi xảy ra
// trong cùng một thao tác được gộp vào một bước undo. Gõ phím liên tiếp còn
// được gộp tiếp theo từ, để không phải undo từng ký tự.
//
// Offset tính bằng byte (văn bản lưu dạng UTF-8), không phải số ký tự.
#include "EditorTextView.h"

#include <AppDefs.h>
#include <Message.h>

#include "EditorWindow.h"


// Số bước hoàn tác tối đa được giữ lại. Bước cũ nhất bị bỏ khi vượt quá.
static const size_t kMaxUndoSteps = 1000;


static bool
IsWhitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n';
}


EditorTextView::EditorTextView(const char* name, EditorWindow* owner)
	:
	BTextView(name),
	fOwner(owner),
	fInAction(false),
	fActionTyping(false),
	fActionHasGroup(false),
	fBreakTyping(false),
	fActionSelStart(0),
	fActionSelEnd(0),
	fReplaying(false)
{
	// Tắt undo một bước có sẵn của BTextView, ta tự quản lý lịch sử
	SetDoesUndo(false);
}


void
EditorTextView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_UNDO:
			UndoEdit();
			return;

		case B_REDO:
			RedoEdit();
			return;
	}

	// Mỗi tin nhắn (cắt, dán, kéo thả, input method...) là một thao tác:
	// mọi thay đổi nó gây ra được gộp thành một bước undo.
	if (fInAction) {
		BTextView::MessageReceived(message);
		return;
	}

	_BeginAction(false);
	BTextView::MessageReceived(message);
	_EndAction();
}


void
EditorTextView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fInAction) {
		BTextView::KeyDown(bytes, numBytes);
		return;
	}

	_BeginAction(true);
	BTextView::KeyDown(bytes, numBytes);
	_EndAction();
}


// BTextView gọi hàm ảo này cho B_UNDO; ghi đè để không ai dùng nhầm
// undo một bước cũ.
void
EditorTextView::Undo(BClipboard* /*clipboard*/)
{
	UndoEdit();
}


bool
EditorTextView::CanUndo() const
{
	return !fUndoStack.empty();
}


bool
EditorTextView::CanRedo() const
{
	return !fRedoStack.empty();
}


void
EditorTextView::UndoEdit()
{
	if (fUndoStack.empty())
		return;

	EditGroup group = fUndoStack.back();
	fUndoStack.pop_back();

	// Làm ngược lại các thao tác, theo thứ tự ngược
	fReplaying = true;
	for (size_t i = group.steps.size(); i > 0; i--)
		_ApplyStep(group.steps[i - 1], true);
	fReplaying = false;

	Select(group.selectionStart, group.selectionEnd);
	ScrollToSelection();

	// Sau khi undo, lần gõ tiếp theo phải là một bước mới
	group.typing = false;
	if (!fUndoStack.empty())
		fUndoStack.back().typing = false;
	fRedoStack.push_back(group);
}


void
EditorTextView::RedoEdit()
{
	if (fRedoStack.empty())
		return;

	EditGroup group = fRedoStack.back();
	fRedoStack.pop_back();

	fReplaying = true;
	for (size_t i = 0; i < group.steps.size(); i++)
		_ApplyStep(group.steps[i], false);
	fReplaying = false;

	// Đặt con trỏ ở cuối thao tác cuối cùng
	const EditStep& last = group.steps.back();
	int32 caret = last.offset;
	if (last.isInsert)
		caret += (int32)last.text.size();
	Select(caret, caret);
	ScrollToSelection();

	group.typing = false;
	fUndoStack.push_back(group);
}


void
EditorTextView::ClearUndoHistory()
{
	fUndoStack.clear();
	fRedoStack.clear();
}


void
EditorTextView::InsertText(const char* text, int32 length, int32 offset,
	const text_run_array* runs)
{
	// Chuẩn hoá offset giống BTextView::InsertText để ghi đúng cái sẽ xảy ra.
	// Chép văn bản trước: con trỏ text có thể trỏ vào chính bộ đệm của view
	// (ví dụ khi tự thụt lề) và bị đổi sau khi chèn.
	if (offset < 0)
		offset = 0;
	else if (offset > TextLength())
		offset = TextLength();

	std::string inserted;
	if (length > 0)
		inserted.assign(text, length);

	BTextView::InsertText(text, length, offset, runs);

	if (!inserted.empty())
		_Record(true, offset, inserted);
	fOwner->TextChanged();
}


void
EditorTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	int32 textLength = TextLength();
	if (fromOffset < 0)
		fromOffset = 0;
	else if (fromOffset > textLength)
		fromOffset = textLength;
	if (toOffset < 0)
		toOffset = 0;
	else if (toOffset > textLength)
		toOffset = textLength;

	// Phải lấy đoạn sắp bị xoá trước khi gọi lớp cha.
	// Dùng GetText thay vì Text(): Text() phải dồn cả bộ đệm lại mỗi lần gọi.
	std::string deleted;
	if (fromOffset < toOffset) {
		int32 length = toOffset - fromOffset;
		deleted.resize(length + 1);		// GetText ghi thêm '\0' ở cuối
		GetText(fromOffset, length, &deleted[0]);
		deleted.resize(length);
	}

	BTextView::DeleteText(fromOffset, toOffset);

	if (!deleted.empty())
		_Record(false, fromOffset, deleted);
	fOwner->TextChanged();
}


void
EditorTextView::_BeginAction(bool typing)
{
	fInAction = true;
	fActionTyping = typing;
	fActionHasGroup = false;
	GetSelection(&fActionSelStart, &fActionSelEnd);

	// Gõ đè lên vùng chọn thì luôn là một bước riêng
	fBreakTyping = fActionSelStart != fActionSelEnd;
}


void
EditorTextView::_EndAction()
{
	fInAction = false;
	fActionTyping = false;
	fActionHasGroup = false;
}


void
EditorTextView::_Record(bool isInsert, int32 offset, const std::string& text)
{
	if (fReplaying)
		return;

	// Có thay đổi mới thì không redo được nữa
	fRedoStack.clear();

	if (!fInAction) {
		// Thay đổi từ code (ví dụ SetText): mỗi lần gọi là một bước
		GetSelection(&fActionSelStart, &fActionSelEnd);
		fActionTyping = false;
		fActionHasGroup = false;
	}

	if (!fActionHasGroup) {
		bool continueTyping = fActionTyping && !fBreakTyping
			&& !fUndoStack.empty()
			&& _CanContinueTyping(fUndoStack.back(), isInsert, offset, text);

		if (!continueTyping) {
			EditGroup group;
			group.selectionStart = fActionSelStart;
			group.selectionEnd = fActionSelEnd;
			group.typing = fActionTyping;
			fUndoStack.push_back(group);

			if (fUndoStack.size() > kMaxUndoSteps)
				fUndoStack.pop_front();
		}
		fActionHasGroup = fInAction;
	}

	EditGroup& group = fUndoStack.back();
	if (group.steps.empty()
		|| !_Coalesce(group.steps.back(), isInsert, offset, text)) {
		EditStep step;
		step.isInsert = isInsert;
		step.offset = offset;
		step.text = text;
		group.steps.push_back(step);
	}
}


// Quyết định một lần gõ phím mới có được gộp vào bước gõ trước không.
// Gộp theo từ: "xin chào" -> hai bước "xin " và "chào". Xuống dòng luôn
// mở bước mới. Gõ chữ rồi xoá (hoặc ngược lại) là hai bước khác nhau.
bool
EditorTextView::_CanContinueTyping(const EditGroup& group, bool isInsert,
	int32 offset, const std::string& text) const
{
	if (!group.typing || group.steps.empty())
		return false;

	const EditStep& last = group.steps.back();
	if (last.isInsert != isInsert)
		return false;

	if (isInsert) {
		if (offset != last.offset + (int32)last.text.size())
			return false;
		if (text[0] == '\n')
			return false;
		// Đã gõ xong khoảng trắng sau một từ, giờ bắt đầu từ mới
		if (IsWhitespace(last.text[last.text.size() - 1])
			&& !IsWhitespace(text[0]))
			return false;
		return true;
	}

	// Backspace liên tiếp hoặc Delete liên tiếp
	return offset + (int32)text.size() == last.offset
		|| offset == last.offset;
}


// Nối thao tác mới vào thao tác cuối nếu liền nhau, để lịch sử gọn hơn
bool
EditorTextView::_Coalesce(EditStep& last, bool isInsert, int32 offset,
	const std::string& text) const
{
	if (last.isInsert != isInsert)
		return false;

	if (isInsert) {
		if (offset != last.offset + (int32)last.text.size())
			return false;
		last.text += text;
		return true;
	}

	if (offset + (int32)text.size() == last.offset) {
		// Backspace: đoạn mới nằm ngay trước đoạn đã xoá
		last.text.insert(0, text);
		last.offset = offset;
		return true;
	}
	if (offset == last.offset) {
		// Delete: đoạn mới nằm ngay sau đoạn đã xoá
		last.text += text;
		return true;
	}
	return false;
}


void
EditorTextView::_ApplyStep(const EditStep& step, bool reverse)
{
	int32 length = (int32)step.text.size();
	bool insert = step.isInsert != reverse;

	if (insert)
		Insert(step.offset, step.text.c_str(), length);
	else
		Delete(step.offset, step.offset + length);
}
