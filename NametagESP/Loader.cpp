	#include "NametagESP.h"

//Example on How To Use

int main() {
	timeBeginPeriod(1);
	std::thread(NametagSPScanner).detach();
	bool is_enabled = false;

	while (true) {
		if (GetAsyncKeyState('D') & 0x8000) {
			is_enabled = !is_enabled;
			while (GetAsyncKeyState('D') & 0x8000) std::this_thread::sleep_for(std::chrono::milliseconds(1)); //Used to avoid to run multiple safewalk threads
			enableESP(is_enabled);
		}
		else std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}