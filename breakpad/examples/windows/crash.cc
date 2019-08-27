#include <stdio.h>
#include "client/windows/handler/exception_handler.h"

namespace {

	bool callback(const wchar_t* dump_path,
		const wchar_t* minidump_id,
		void* context,
		EXCEPTION_POINTERS* exinfo,
		MDRawAssertionInfo* assertion,
		bool succeeded) {
		if (succeeded) {
			printf("%ws\\%ws.dmp\n", dump_path, minidump_id);
		}
		else {
			printf("ERROR creating minidump\n");
		}

		return succeeded;
	}

	void crash() {
		volatile int *i = reinterpret_cast<int *>(0x45);
		*i = 5;  // crash!
	}

	void start() {
		crash();  // should get inlined with optimizations
	}

}  // namespace

int main(int argc, char **argv) {
	google_breakpad::ExceptionHandler eh(L".", 0, callback, 0, google_breakpad::ExceptionHandler::HANDLER_ALL);
	start();  // should get inlined with optimizations
	return 0;
}