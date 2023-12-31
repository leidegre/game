#include "renderer-inl.hh"
#include "renderer.hh"

#include <imgui.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace game;

LRESULT WINAPI game::MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
    return true;
  }

  switch (msg) {
  case WM_SIZE:
    // if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
    //   WaitForLastSubmittedFrame();
    //   CleanupRenderTarget();
    //   HRESULT result = g_pSwapChain->ResizeBuffers(
    //       0,
    //       (UINT)LOWORD(lParam),
    //       (UINT)HIWORD(lParam),
    //       DXGI_FORMAT_UNKNOWN,
    //       DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    //   assert(SUCCEEDED(result) && "Failed to resize swapchain.");
    //   CreateRenderTarget();
    // }
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
