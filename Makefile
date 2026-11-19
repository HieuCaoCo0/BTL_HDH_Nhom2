## Makefile cho SimpleEdit, dùng makefile-engine của Haiku
## Build: make        Chạy: ./objects.*/SimpleEdit

NAME = SimpleEdit
TYPE = APP
APP_MIME_SIG = application/x-vnd.ptit-SimpleEdit

SRCS = App.cpp EditorWindow.cpp EditorTextView.cpp
RDEFS =
RSRCS =

# be: Application Kit, Interface Kit...  tracker: BFilePanel
LIBS = be tracker $(STDCPPLIBS)
LIBPATHS =
SYSTEM_INCLUDE_PATHS =
LOCAL_INCLUDE_PATHS =

OPTIMIZE := FULL
LOCALES =
DEFINES =
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS =
LINKER_FLAGS =
APP_VERSION :=
DRIVER_PATH =

## Gọi makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
