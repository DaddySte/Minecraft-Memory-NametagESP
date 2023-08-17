#pragma once
#include <Windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <iostream>
#define MINECRAFT_TICK 600
#define MINECRAFT_HANDLE checkMinecraftHandle()

#pragma comment(lib,"winmm.lib")

void NametagSPScanner();

void enableESP(bool condition);