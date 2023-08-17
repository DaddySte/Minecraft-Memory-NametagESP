#include "NametagESP.h"

	//Set to Multibyte
	HANDLE checkMinecraftHandle() {
		HANDLE pHandle = NULL;

		while (!pHandle) {

			if (HWND hwnd = FindWindow("LWJGL", NULL)) {

				DWORD pid;
				GetWindowThreadProcessId(hwnd, &pid);
				pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return pHandle;
	}

	void floatScanner(float target_value, std::vector<uint64_t>& memory_addresses) {
		MEMORY_BASIC_INFORMATION memInfo = {};
		SYSTEM_INFO sysInfo = {};
		GetSystemInfo(&sysInfo);

		LPVOID currentAddress = sysInfo.lpMinimumApplicationAddress;
		LPVOID maxAddress = sysInfo.lpMaximumApplicationAddress;

		while (currentAddress < maxAddress) {

			if (VirtualQueryEx(MINECRAFT_HANDLE, currentAddress, &memInfo, sizeof(memInfo))) {
				if ((memInfo.State == MEM_COMMIT) && ((memInfo.Protect & PAGE_GUARD) == 0) && ((memInfo.Protect & PAGE_EXECUTE) ||
					(memInfo.Protect & PAGE_EXECUTE_READ) || (memInfo.Protect & PAGE_EXECUTE_READWRITE) ||
					(memInfo.Protect & PAGE_EXECUTE_WRITECOPY))) {
					SIZE_T bytesRead;
					std::vector<float> buffer(memInfo.RegionSize / sizeof(float));

					if (ReadProcessMemory(MINECRAFT_HANDLE, currentAddress, buffer.data(), memInfo.RegionSize, &bytesRead)) {
						uint64_t addressOffset = 0;
						for (size_t i = 0; i < bytesRead / sizeof(float); i++) {
							if (buffer[i] == target_value) {
								LPBYTE address = (LPBYTE)currentAddress + addressOffset;
								if (std::find(memory_addresses.begin(), memory_addresses.end(), (uint64_t)address) == memory_addresses.end()) {
									memory_addresses.push_back((uint64_t)address);
								}
							}
							addressOffset += sizeof(float);
						}
					}
				}
				currentAddress = (LPBYTE)memInfo.BaseAddress + memInfo.RegionSize;
			}
			else {
				currentAddress = (LPBYTE)currentAddress + sysInfo.dwPageSize;
			}
		}
	}

	struct {
		float esp_no_sneak = 0.02666667;
		float esp_sneak = -0.02666667;
		float changed_height = 3.35;
		float unchanged_height = 2.5;
		std::vector<uint64_t> default_addresses;
		std::vector<uint64_t> memory_no_sneak_addresses;
		std::vector<uint64_t> memory_sneak_addresses;

	}NametagMemory;

	//this functions has been modified to find the height variable that keep changing addresses
	//we return them only when we are using them to enable esp without pushing back box address, so we only modify the size and height addresses
	uint64_t NametagSneakHelper(std::vector<uint64_t> base_address, bool return_height, float height_value) {
		for (int i = 0; i < base_address.size(); i++) {
			float buffer = 0;
			for (int j = 0; j < 40; j += 4) {
				ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address[i] + j), &buffer, sizeof(buffer), NULL);
				if (buffer == height_value) {
					if (return_height) return (base_address[i] + j);
					NametagMemory.memory_sneak_addresses.push_back(base_address[i]);
					break;
				}
			}
		}
	}

	void NametagNoSneakHelper(std::vector<uint64_t> base_address) {
		for (int i = 0; i < base_address.size(); i++) {

			float buffer = 0, buffertwo = 0;
			ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address[i] + 16), &buffer, sizeof(buffer), NULL);
			if (buffer == 2.5) {
				ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address[i] - 8), &buffertwo, sizeof(buffertwo), NULL);
				if (buffertwo == 1.875) {
					NametagMemory.memory_no_sneak_addresses.push_back(base_address[i]);
					buffer = 0;
					buffertwo = 0;
				}
			}
		}
	}

	//removes invalid addresses
	void CorrectorHelper(std::vector<uint64_t>& base_address, float unchanged) {
		std::sort(base_address.begin(), base_address.end());
		base_address.erase(std::unique(base_address.begin(), base_address.end()), base_address.end());

		for (int i = base_address.size() - 1; i >= 0; i--) {
			float buffer = 0;
			ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address[i]), &buffer, sizeof(buffer), NULL);

			if (buffer != unchanged) base_address.erase(base_address.begin() + i);
		}

	}

	void NametagSPScanner() {

		while (true) {
			//Search For Floats
			floatScanner(NametagMemory.esp_no_sneak, NametagMemory.default_addresses);

			//Search for nametag height and background floats 
			NametagNoSneakHelper(NametagMemory.default_addresses);

			//Clear buffer vector
			NametagMemory.default_addresses.clear();

			//Same for Sneak
			floatScanner(NametagMemory.esp_sneak, NametagMemory.default_addresses);

			NametagSneakHelper(NametagMemory.default_addresses, false, NametagMemory.unchanged_height);

			NametagMemory.default_addresses.clear();

			//Delete repetitions and check if values are correct
			CorrectorHelper(NametagMemory.memory_no_sneak_addresses, NametagMemory.esp_no_sneak);
			CorrectorHelper(NametagMemory.memory_sneak_addresses, NametagMemory.esp_sneak);

			std::this_thread::sleep_for(std::chrono::milliseconds(MINECRAFT_TICK));
		}
	}

	//find addresses and rewrite values

	void SneakChangerHelper(uint64_t addresses, bool is_esp_enabled) {
		std::vector<uint64_t> buffer;
		buffer.push_back(addresses);

		if (is_esp_enabled) {
			uint64_t height_address = NametagSneakHelper(buffer, true, NametagMemory.unchanged_height);
			WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)height_address, &NametagMemory.changed_height, sizeof(NametagMemory.changed_height), NULL);;
		}
		else {
			uint64_t height_address = NametagSneakHelper(buffer, true, NametagMemory.changed_height);
			WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)height_address, &NametagMemory.unchanged_height, sizeof(NametagMemory.unchanged_height), NULL);;
		}

		buffer.clear();
	}

	void enableESP(bool condition) {
		CorrectorHelper(NametagMemory.memory_no_sneak_addresses, NametagMemory.esp_no_sneak);
		CorrectorHelper(NametagMemory.memory_sneak_addresses, NametagMemory.esp_sneak);

		if (condition) {
			for (int i = 0; i < NametagMemory.memory_no_sneak_addresses.size(); i++)
				WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)(NametagMemory.memory_no_sneak_addresses[i] + 16), &NametagMemory.changed_height, sizeof(NametagMemory.changed_height), NULL);

			for (int i = 0; i < NametagMemory.memory_sneak_addresses.size(); i++)
				SneakChangerHelper(NametagMemory.memory_sneak_addresses[i], condition);
		}
		else {
			for (int i = 0; i < NametagMemory.memory_no_sneak_addresses.size(); i++)
				WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)(NametagMemory.memory_no_sneak_addresses[i] + 16), &NametagMemory.unchanged_height, sizeof(NametagMemory.unchanged_height), NULL);

			for (int i = 0; i < NametagMemory.memory_sneak_addresses.size(); i++)
				SneakChangerHelper(NametagMemory.memory_sneak_addresses[i], condition);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(MINECRAFT_TICK));
	}

/* How to use
* Run Esp scanner in a different thread. Jvm keeps changing addresses so the loop must keep running as long as the program is opened (Infinite loop)
* To enable or disable esp use the function "enableESP(bool condition)".
*/