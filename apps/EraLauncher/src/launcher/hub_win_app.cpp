#include "launcher/hub_win_app.h"
#include "launcher/render/hub_renderer.h"

namespace era_engine::launcher
{
    ref<d3d12_hub_renderer> renderer = nullptr;

    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg)
        {
        case WM_SIZE:
            if (renderer->device != NULL && wParam != SIZE_MINIMIZED)
            {
                renderer->waitForLastSubmittedFrame();
                renderer->cleanupRenderTarget();
                HRESULT result = renderer->swapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
                assert(SUCCEEDED(result) && "Failed to resize swapchain.");
                renderer->createRenderTarget();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        }
        return ::DefWindowProc(hWnd, msg, wParam, lParam);
    }

    hub_win_app::hub_win_app()
    {
    }

    hub_win_app::~hub_win_app()
    {
        release();
    }

    void hub_win_app::init()
    {
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"EraLauncher", NULL };

        ::RegisterClassExW(&wc);
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"EraLauncher", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

        renderer = make_ref<d3d12_hub_renderer>();
        renderer->init(hwnd, wc);

        hub = make_ref<era_hub>(renderer);
        renderer->setHub(hub);

        run(hwnd, wc);
    }

    void hub_win_app::release()
    {
        renderer.reset();
    }

    void hub_win_app::run(HWND hWnd, WNDCLASSEXW wc)
    {
        bool done = false;
        while (!done)
        {
            MSG msg;
            while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            hub->render();
        }

        renderer->waitForLastSubmittedFrame();

        renderer->finalize();

        renderer->cleanupDeviceD3D();
        ::DestroyWindow(hWnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
}