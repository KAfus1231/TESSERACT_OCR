#include "includes.h"

VOID ShowBalloon(LPCWSTR title, LPCWSTR msg);

std::atomic<int> runTime{ 0 };
void runTimer()
{
	std::thread([]
		{
			while (true)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				runTime++;
			}

		}).detach();
}

std::string ReadTextFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file) return "";

	std::string content((std::istreambuf_iterator<char>(file)), {});
	return content;
}

BOOL SaveBitmapToFile(HBITMAP hBitmap, LPCWSTR filename)
{
	BITMAP bmp;
	if (!GetObject(hBitmap, sizeof(BITMAP), &bmp))
		return false;

	BITMAPFILEHEADER bmfHeader = {};
	BITMAPINFOHEADER bi = {};

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;

	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32 * 4 * bmp.bmHeight);
	char* lpBitmapData = new char[dwBmpSize];

	HDC hDC = GetDC(NULL);

	GetDIBits(hDC, hBitmap, 0, (UINT)bmp.bmHeight, lpBitmapData, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	bmfHeader.bfType = 0x4D42;
	bmfHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	std::ofstream file(filename, std::ios::binary | std::ios::out);

	if (!file)
	{
		MessageBox(NULL, L"Ошибка окрытия файла", L"Ошибка", MB_OK | MB_ICONERROR);
		delete[] lpBitmapData;
		return FALSE;
	}

	file.write((char*)&bmfHeader, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bi, sizeof(BITMAPINFOHEADER));
	file.write(lpBitmapData, dwBmpSize);
	file.close();

	delete[] lpBitmapData;
	return TRUE;
}

VOID SetClipboardText(const std::string& utf8text)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8text.c_str(), -1, nullptr, 0);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));

	if (!hMem) return;

	wchar_t* pText = static_cast<wchar_t*>(GlobalLock(hMem));
	MultiByteToWideChar(CP_UTF8, 0, utf8text.c_str(), -1, pText, len);
	GlobalUnlock(hMem);

	OpenClipboard(nullptr);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();
}

VOID SaveTextToClipboard()
{
	if (!OpenClipboard(NULL))
	{
		MessageBox(NULL, L"Ошибка открытия буфера", L"Ошибка", MB_OK | MB_ICONERROR);
		return;
	}

	if (IsClipboardFormatAvailable(CF_BITMAP))
	{
		HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);

		if (hBitmap)
		{
			if (!SaveBitmapToFile(hBitmap, PATH_TO_SHOT))
				MessageBox(NULL, L"Ошибка сохранения в папке", L"Ошибка", MB_OK | MB_ICONERROR);

			cv::Mat img = cv::imread("assets\\screenshots\\my_shot.bmp");
			cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
			cv::threshold(img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
			cv::imwrite("assets\\screenshots\\my_shot.bmp", img);
		}
	}
	else
	{
		std::cout << "Нет скриншота в буфере" << std::endl;
		return;
	}

	int result = (system("tesseract A:\\CPP_projects\\C++\\OCR\\assets\\screenshots\\my_shot.bmp A:\\CPP_projects\\C++\\OCR\\assets\\screenshots\\output\ -l rus+eng"));

	if (result != 0)
		MessageBox(NULL, L"Ошибка в работе tesseract", L"Ошибка", MB_OK | MB_ICONERROR);
	else
		ShowBalloon(L"Успех!", L"Текст получен в буфер!");

	CloseClipboard();
}

VOID ShowBalloon(LPCWSTR title, LPCWSTR msg)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = GetConsoleWindow();
	nid.uID = 1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_USER + 1;
	nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);
	lstrcpy(nid.szTip, L"Tesseract App");
	lstrcpy(nid.szInfo, msg);
	lstrcpy(nid.szInfoTitle, title);
	nid.dwInfoFlags = NIIF_INFO;

	Shell_NotifyIcon(NIM_ADD, &nid);
	Sleep(3000);
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

int main()
{
	setlocale(LC_ALL, "ru");
	runTimer();
	int lastTimeUpdate = 0;

	while (true)
	{
		SaveTextToClipboard();
		std::string stringToSet = ReadTextFile(PATH_TO_OUT);
		SetClipboardText(stringToSet);

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	return 0;
}
