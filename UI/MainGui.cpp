#include "App.h"

// Entry point for the GUI build (Dark Glass Edition).
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
_In_opt_ HINSTANCE hPrevInstance,
_In_ LPWSTR    lpCmdLine,
_In_ int       nCmdShow)
{
(void)hPrevInstance; (void)lpCmdLine; (void)nCmdShow;
return reise::App::Run(hInstance);
}
