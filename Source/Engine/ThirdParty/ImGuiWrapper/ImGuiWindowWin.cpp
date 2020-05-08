#include "ImGuiWindowWin.h"
#include "../../Engine/Platform/WinWindow/WinWindowSystem.h"

#include "../ImGui/imgui_impl_win32.cpp"

#include "../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace ImGuiWindowWinNS
{
	WindowEventCallbackFunctor m_windowEventCallbackFunctor;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool ImGuiWindowWin::setup()
{
	ImGuiWindowWinNS::m_windowEventCallbackFunctor = [](void* hWnd, uint32_t msg, uint64_t wParam, int64_t lParam) {
		ImGui_ImplWin32_WndProcHandler((HWND)hWnd, msg, wParam, lParam);
	};
	g_pModuleManager->getWindowSystem()->addEventCallback(&ImGuiWindowWinNS::m_windowEventCallbackFunctor);

	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin setup finished.");

	return true;
}

bool ImGuiWindowWin::initialize()
{
	ImGui_ImplWin32_Init(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd());
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin has been initialized.");

	return true;
}

bool ImGuiWindowWin::newFrame()
{
	ImGui_ImplWin32_NewFrame();
	return true;
}

bool ImGuiWindowWin::terminate()
{
	ImGui_ImplWin32_Shutdown();
	g_pModuleManager->getLogSystem()->Log(LogLevel::Success, "ImGuiWindowWin has been terminated.");

	return true;
}