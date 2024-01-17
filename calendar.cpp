#pragma region HEADER
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMINMAX
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
//#define NOCTLMGR
#define NOMETAFILE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ctime>

#ifdef _DEBUG
#include <iostream>
#include <chrono>
#endif // _DEBUG
#pragma endregion

#pragma region SETTING
#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 500
#define TITLEBAR_HEIGHT 32

#define CALENDAR_POS_X 10
#define CALENDAR_POS_Y (TITLEBAR_HEIGHT + WINDOW_HEIGHT / 16 + 4)
#define CALENDAR_WIDTH (WINDOW_WIDTH - CALENDAR_POS_X)
#define CALENDAR_HEIGHT (WINDOW_HEIGHT - CALENDAR_POS_Y)
#define CALENDAR_ROWS 7
#define CALENDAR_COLS 7
#define CALENDAR_PADDING_X ((CALENDAR_WIDTH - (CALENDAR_ROWS - 1) * (CELL_WIDTH + CELL_PADDING_X) - CELL_WIDTH) / 2)
#define CALENDAR_PADDING_Y ((CALENDAR_HEIGHT - (CALENDAR_COLS - 1) * (CELL_HEIGHT + CELL_PADDING_Y) - CELL_HEIGHT) / 2)

#define CELL_WIDTH ((CALENDAR_WIDTH - (CALENDAR_ROWS + 1) * CELL_PADDING_X) / CALENDAR_ROWS)
#define CELL_HEIGHT ((CALENDAR_HEIGHT - (CALENDAR_COLS + 1) * CELL_PADDING_Y) / CALENDAR_COLS)
#define CELL_PADDING_X 2
#define CELL_PADDING_Y 2

#define PREVMONTH_BUTTON_ID 2000
#define NEXTMONTH_BUTTON_ID 2001
#define CLOSE_BUTTON_ID		2002
#define MONTH_LABEL_ID		2003
#define YEAR_LABEL_ID		2004
#define CALENDAR_ID			2005
#pragma endregion

static bool isMouseLeave = true;
static bool calendarRedraw = true;
static POINT clickedCell = { -1, -1 };
#ifdef _DEBUG
static float loopCount = 0;
static std::chrono::high_resolution_clock timer;
static auto start = timer.now();
#endif // _DEBUG
struct Date {
	size_t date;

	Date() : date{ 0 } {}

	Date(size_t d)
	{
		date = d;
	}

	Date(int year, int month, int day)
	{
		date = (size_t)(year * 100000 + month * 1000 + day * 10);
	}

	int GetYear() const
	{
		return (int)(date / 100000);
	}

	int GetMonth() const
	{
		return (date / 1000) % 100;
	}

	int GetMonthDay() const
	{
		return (date / 10) % 100;
	}

	int GetWeekDay() const
	{
		//	Gauss's algorithm
		int year = GetYear(), month = GetMonth(), mday = GetMonthDay();
		int offsets[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
		int offset = offsets[month - 1];
		if (month > 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
			offset = (offset + 1) % 7;
		return (mday + offset + 5 * ((year - 1) % 4) + 4 * ((year - 1) % 100) + 6 * ((year - 1) % 400)) % 7;
	}
};
static Date today;
static Date currDate;

const char listMonthName[12][12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
const char mday[31][3] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31" };
const char dayOfWeek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static bool RegisterWindowClass(const char* className, WNDPROC wndProc) {
	WNDCLASSA wc{};
	wc.lpfnWndProc = wndProc;
	wc.lpszClassName = className;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hInstance = GetModuleHandle(nullptr);
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.style = CS_PARENTDC;

	return RegisterClassA(&wc);
}
inline static unsigned long RGB2BGR(unsigned long color)
{
	return RGB((BYTE)(color >> 16), (BYTE)(color >> 8), (BYTE)(color >> 0));
}
int DaysInMonth(int year, int month)
{
	if ((month <= 7 && month & 1) || (month >= 8 && month % 2 == 0)) return 31;
	else if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) return 29;

	return 28;
}
static int UintToCstr(int num, char res[])
{
	if (num < 0)
		return -1;

	if (num == 0) {
		res[0] = '0';
		res[1] = '\0';
		return 1;
	}

	int len = 0;
	while (num > 0) {
		res[len++] = (num % 10) + '0';
		num /= 10;
	}

	int temp = 0;
	for (int i = 0; i < len / 2; i++) {
		temp = res[i];
		res[i] = res[len - i - 1];
		res[len - i - 1] = temp;
	}

	res[len + 1] = '\0';
	return len;
}

void UpdateCurrentDate(int month)
{
	int newMonth = currDate.GetMonth() + month, newYear = currDate.GetYear();
	if (newMonth > 12) {
		newYear += newMonth / 12;
		newMonth %= 12;
		if (newMonth == 0) {
			newMonth = 12;
			newYear--;
		}
	}
	else if (newMonth < 1) {
		newYear += newMonth / 12 - 1;
		newMonth = 12 + newMonth % 12;
	}

	if (newYear >= 1 && newYear <= 9999) {
		calendarRedraw = true;
		HWND base = FindWindowA("BaseWindow", nullptr);
		if (base != nullptr) {
			if (newMonth != currDate.GetMonth()) {
				if (newMonth >= 1 && newMonth <= 12) {
					SetWindowTextA(GetDlgItem(base, MONTH_LABEL_ID), listMonthName[newMonth - 1]);
				}
			}
			if (newYear != currDate.GetYear()) {
				char year[6] = { 0 };
				UintToCstr(newYear, year);
				SetWindowTextA(GetDlgItem(base, YEAR_LABEL_ID), year);
			}
		}
		int maximumDayInMonth = DaysInMonth(newYear, newMonth);
		currDate = Date(newYear, newMonth, currDate.GetMonthDay() <= maximumDayInMonth ? currDate.GetMonthDay() : maximumDayInMonth);
	}
}
LRESULT BaseWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HBRUSH hbrBackground = CreateSolidBrush(0x1e1e1e);
	static int fontSize = WINDOW_HEIGHT / 32 + 10;
	static HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));

	switch (msg)
	{
	case WM_CREATE:
	{
		HINSTANCE hInstance = GetModuleHandle(nullptr);
		int buttonSize = CALENDAR_POS_Y - TITLEBAR_HEIGHT - 4;
		SIZE septemberSz{}, year9999Sz{};
		char year[6] = { 0 };
		UintToCstr(currDate.GetYear(), year);
		{
			HDC hdc = GetDC(hwnd);
			HFONT olfhfont = (HFONT)SelectObject(hdc, globalFont);
			GetTextExtentPoint32A(hdc, "September", 9, &septemberSz);
			GetTextExtentPoint32A(hdc, "9999", 4, &year9999Sz);
			SelectObject(hdc, olfhfont);
			ReleaseDC(hwnd, hdc);
		}

		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)PREVMONTH_BUTTON_ID, hInstance, nullptr);
		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X + buttonSize + 4, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)NEXTMONTH_BUTTON_ID, hInstance, nullptr);
		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, WINDOW_WIDTH - TITLEBAR_HEIGHT * 3 / 2, 0, TITLEBAR_HEIGHT * 3 / 2, TITLEBAR_HEIGHT, hwnd, (HMENU)CLOSE_BUTTON_ID, hInstance, nullptr);
		HWND monthLabel = CreateWindowA("Static", listMonthName[today.GetMonth() - 1], WS_CHILD | WS_VISIBLE | SS_RIGHT, CALENDAR_POS_X + CALENDAR_WIDTH / 2 - (septemberSz.cx + 10) * 5 / 6, TITLEBAR_HEIGHT, septemberSz.cx + 10, septemberSz.cy, hwnd, (HMENU)MONTH_LABEL_ID, hInstance, nullptr);
		SendMessageA(monthLabel, WM_SETFONT, (WPARAM)globalFont, 0);
		HWND yearLabel = CreateWindowA("Static", year, WS_CHILD | WS_VISIBLE, CALENDAR_POS_X + CALENDAR_WIDTH / 2 - (septemberSz.cx + 10) * 5 / 6 + septemberSz.cx + 20, TITLEBAR_HEIGHT, year9999Sz.cx, septemberSz.cy, hwnd, (HMENU)YEAR_LABEL_ID, hInstance, nullptr);
		SendMessageA(yearLabel, WM_SETFONT, (WPARAM)globalFont, 0);
	} break;
	case WM_PAINT:
	{
#ifdef _DEBUG
			std::cout << "Redraw Window!!!\n";
#endif // _DEBUG

		PAINTSTRUCT ps{};
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT rc;
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);

		RECT wndRect{ 0, TITLEBAR_HEIGHT, rc.right, rc.bottom };
		SetBkColor(hdc, 0x1e1e1e);
		ExtTextOutA(hdc, wndRect.left, wndRect.top, ETO_OPAQUE, &wndRect, nullptr, 0, nullptr);

		RECT titleBarRect{ 0, 0, rc.right, TITLEBAR_HEIGHT };
		SetTextColor(hdc, 0xffffff);
		SetBkMode(hdc, TRANSPARENT);
		SetBkColor(hdc, 0x2f2f2f);
		HFONT hfont = CreateFont(-20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
		SelectObject(hdc, hfont);
		SIZE textSz{};
		GetTextExtentPoint32A(hdc, "Calendar", 8, &textSz);
		ExtTextOutA(hdc, rc.right / 2 - textSz.cx / 2, TITLEBAR_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &titleBarRect, "Calendar", 8, nullptr);
		DeleteObject(hfont);

		EndPaint(hwnd, &ps);
	} break;
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wparam;
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, 0x1e1e1e);

		return (INT_PTR)hbrBackground;
	} break;
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		RECT rc;
		GetWindowRect(item->hwndItem, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);

		switch (wparam)
		{
		case PREVMONTH_BUTTON_ID:
		{
			SetBkColor(item->hDC, 0x2f2f2f);
			ExtTextOutA(item->hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

			HPEN hpen = CreatePen(PS_SOLID, 2, 0xffffff);
			HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);
			int offsetX = -4, offsetY = 0;
			MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
			LineTo(item->hDC, rc.right / 2 + rc.right / 5 + offsetX, rc.bottom / 2 - rc.right / 5 + offsetY);						// 4 sqrt(2) ~ 5.6 ~ 6
			MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
			LineTo(item->hDC, rc.right / 2 + rc.right / 5 + offsetX, rc.bottom / 2 + rc.right / 5 + offsetY);
			SelectObject(item->hDC, oldhpen);
			DeleteObject(oldhpen);
		} break;
		case NEXTMONTH_BUTTON_ID:
		{
			SetBkColor(item->hDC, 0x2f2f2f);
			ExtTextOutA(item->hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

			HPEN hpen = CreatePen(PS_SOLID, 2, 0xffffff);
			HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);

			int offsetX = 4, offsetY = 0;
			MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
			LineTo(item->hDC, rc.right / 2 - rc.right / 5 + offsetX, rc.bottom / 2 - rc.right / 5 + offsetY);						// 4 sqrt(2) ~ 5.6 ~ 6
			MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
			LineTo(item->hDC, rc.right / 2 - rc.right / 5 + offsetX, rc.bottom / 2 + rc.right / 5 + offsetY);
			SelectObject(item->hDC, oldhpen);
			DeleteObject(oldhpen);
		} break;
		case CLOSE_BUTTON_ID:
		{
			SetBkColor(item->hDC, isMouseLeave ? 0x2f2f2f : 0x0000ff);
			ExtTextOutA(item->hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

			HPEN hpen = CreatePen(PS_SOLID, 2, 0xffffff);
			HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, nullptr);
			LineTo(item->hDC, rc.right / 2 - 6, rc.bottom / 2 - 6);						// 4 sqrt(2) ~ 5.6 ~ 6
			LineTo(item->hDC, rc.right / 2 + 6, rc.bottom / 2 + 6);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, nullptr);
			LineTo(item->hDC, rc.right / 2 - 6, rc.bottom / 2 + 6);
			LineTo(item->hDC, rc.right / 2 + 6, rc.bottom / 2 - 6);
			SelectObject(item->hDC, oldhpen);
			DeleteObject(oldhpen);
		} break;
		}
	} break;
	case WM_COMMAND:
	{
		switch (wparam)
		{
		case PREVMONTH_BUTTON_ID:
			UpdateCurrentDate(-1);
			break;
		case NEXTMONTH_BUTTON_ID:
			UpdateCurrentDate(1);
			break;
		case CLOSE_BUTTON_ID:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
	} break;
	case WM_LBUTTONDOWN:
	{
		POINT e;
		if (GetCursorPos(&e)) {
			if (ScreenToClient(hwnd, &e)) {
				if (e.y <= TITLEBAR_HEIGHT) {
					PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
				}
			}
		}
	} break;
	case WM_DESTROY:
	{
		if (hbrBackground) {
			DeleteObject(hbrBackground);
		}
		if (globalFont) {
			DeleteObject(globalFont);
		}
		PostQuitMessage(0);
	} break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
};
LRESULT CalendarWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HFONT oldFont;
	switch (msg)
	{
	case WM_CREATE:
	{
		SetTimer(hwnd, 1000, 1, nullptr);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (calendarRedraw) {
			calendarRedraw = false;
#ifdef _DEBUG
			std::cout << "Redraw Calendar!!!\n";
#endif // _DEBUG

			RECT fillRect{ 0, 0, CALENDAR_WIDTH, CALENDAR_HEIGHT };

			SetBkColor(hdc, 0x1e1e1e);
			ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &fillRect, nullptr, 0, nullptr);

			SetTextColor(hdc, 0xffffff);

			HFONT hfont = CreateFont(-fillRect.bottom / 24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			SIZE textSz{};
			RECT cellRect{ CALENDAR_PADDING_X, CALENDAR_PADDING_Y, CELL_WIDTH, CELL_HEIGHT };
			oldFont = (HFONT)SelectObject(hdc, hfont);
			for (int i = 0; i < CALENDAR_ROWS; i++) {
				GetTextExtentPoint32A(hdc, dayOfWeek[i], 3, &textSz);
				ExtTextOutA(hdc, cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, dayOfWeek[i], 3, nullptr);
				OffsetRect(&cellRect, CELL_WIDTH + CELL_PADDING_X, 0);
			}
			SelectObject(hdc, oldFont);
			DeleteObject(hfont);

			int firstDayOfMonth = Date{ currDate.GetYear(), currDate.GetMonth(), 1 }.GetWeekDay();
			int numberOfDays = DaysInMonth(currDate.GetYear(), currDate.GetMonth());

			hfont = CreateFont(-fillRect.bottom / 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			oldFont = (HFONT)SelectObject(hdc, hfont);
			for (int i = 0; i < CALENDAR_ROWS; i++) {
				for (int j = 0; j < 6; j++) {
					int row = i + firstDayOfMonth, col = j + 1;
					if (row >= 7) {
						col++;
						row %= 7;
					}

					if (i + 7 * j >= numberOfDays)
						break;

					cellRect = { 0, 0, CELL_WIDTH, CELL_HEIGHT };
					OffsetRect(&cellRect, row * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, col * (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
					GetTextExtentPoint32A(hdc, mday[i + 7 * j], (int)strlen(mday[i + 7 * j]), &textSz);
					ExtTextOutA(hdc, cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, mday[i + 7 * j], (int)strlen(mday[i + 7 * j]), nullptr);
				}
			}

			if (currDate.date == today.date) {
				int todayRow = currDate.GetWeekDay();
				int todayCol = (currDate.GetMonthDay() + firstDayOfMonth - 1) / 7 + 1;
				SetTextColor(hdc, 0);
				SetBkColor(hdc, 0xffffff);

				cellRect = { 0, 0, CELL_WIDTH, CELL_HEIGHT };
				OffsetRect(&cellRect, todayRow * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, todayCol * (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
				GetTextExtentPoint32A(hdc, mday[today.GetMonthDay() - 1], (int)strlen(mday[today.GetMonthDay() - 1]), &textSz);
				ExtTextOutA(hdc,cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, mday[today.GetMonthDay() - 1], (int)strlen(mday[today.GetMonthDay() - 1]), nullptr);
			}

			if (clickedCell.x != -1) {
				int cellBorderWidth = 1;

				cellRect = { 0, 0, CELL_WIDTH, CELL_HEIGHT };
				OffsetRect(&cellRect, clickedCell.x * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, clickedCell.y* (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
				HPEN p = CreatePen(PS_SOLID, cellBorderWidth, 0xffffff);
				HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_BRUSH));
				SelectObject(hdc, p);
				Rectangle(hdc, cellRect.left - cellBorderWidth, cellRect.top - cellBorderWidth, cellRect.right + cellBorderWidth, cellRect.bottom + cellBorderWidth);
				SelectObject(hdc, oldPen);
				DeleteObject(p);
				clickedCell.x = clickedCell.y = -1;
			}

			SelectObject(hdc, oldFont);
			DeleteObject(hfont);
		}
		EndPaint(hwnd, &ps);
	} break;
	case WM_TIMER:
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		break;
	case WM_DESTROY:
	{
		KillTimer(hwnd, 1000);
		if (oldFont != nullptr) {
			DeleteObject(oldFont);
		}
	} break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	return 0;
}
void Init()
{
	std::time_t t = std::time(0);
	struct tm now;
	localtime_s(&now, &t);
	today = currDate = { now.tm_year + 1900, now.tm_mon + 1, now.tm_mday };
}

#ifdef _DEBUG
int main()
#else
int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
#endif // _DEBUG
{
	Init();
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	RegisterWindowClass("BaseWindow", BaseWindowProc);
	HWND base = CreateWindowA("BaseWindow", nullptr, WS_POPUP | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	ShowWindow(base, SW_SHOW);

	if (base == nullptr) {
		UnregisterClassA("BaseWindow", hInstance);
		return -1;
	}

	RegisterWindowClass("CalendarWindow", CalendarWindowProc);
	HWND calendar = CreateWindowA("CalendarWindow", nullptr, WS_CHILD | WS_VISIBLE,
		CALENDAR_POS_X, CALENDAR_POS_Y, CALENDAR_WIDTH, CALENDAR_HEIGHT,
		base, (HMENU)CALENDAR_ID, hInstance, nullptr);

	HWND closeButton = GetDlgItem(base, CLOSE_BUTTON_ID);

	MSG msg{};
	while (GetMessageA(&msg, nullptr, 0, 0)) {
#ifdef _DEBUG
		loopCount++;
		if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) >= std::chrono::microseconds{ 1000 }) {
			start = std::chrono::high_resolution_clock::now();
			std::cout << "Loop count: " << loopCount << '\r';
			loopCount = 0;
		}
#endif // _DEBUG

		switch (msg.message)
		{
		case WM_KEYDOWN:
		{
			if (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT) {
				UpdateCurrentDate(msg.wParam == VK_LEFT ? -1 : 1);
				calendarRedraw = true;
			}
#ifdef _DEBUG
			std::cout << msg.wParam << "(WPARAM)\n";
#endif // _DEBUG
		} break;
		case WM_MOUSEWHEEL:
		{
			short zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
			UpdateCurrentDate(zDelta > 0 ? -12 : 12);
		} break;
		case WM_LBUTTONDOWN:
		{
			if (msg.hwnd == calendar) {
				int x = ((int)(short)LOWORD(msg.lParam)), y = ((int)(short)HIWORD(msg.lParam));
				if ((x % (CELL_WIDTH + CELL_PADDING_X)) > CELL_PADDING_X && (y % (CELL_HEIGHT + CELL_PADDING_Y)) > CELL_PADDING_Y) {
					POINT newCell = { x / (CELL_WIDTH + CELL_PADDING_X), y / (CELL_HEIGHT + CELL_PADDING_Y) };
					if ((newCell.x != clickedCell.x && newCell.y != clickedCell.y) &&
						((newCell.x >= Date{ currDate.GetYear(), currDate.GetMonth(), 1 }.GetWeekDay() && newCell.y == 1) ||
							(newCell.x + (newCell.y - 1) * 7 < Date{ currDate.GetYear(), currDate.GetMonth(), 1 }.GetWeekDay() + DaysInMonth(currDate.GetYear(), currDate.GetMonth()) && newCell.y >= 5) ||
							(newCell.y >= 2 && newCell.y <= 4))) {
						clickedCell.x = newCell.x;
						clickedCell.y = newCell.y;
						calendarRedraw = true;
					}
#ifdef _DEBUG
					std::cout << x << "    " << y << '\t' << clickedCell.x << "     " << clickedCell.y << "\n";
#endif // _DEBUG
				}
			}
		} break;
		case WM_MOUSEMOVE:
		{
			if (msg.hwnd == closeButton) {
				if (isMouseLeave) {
					TRACKMOUSEEVENT tme{};
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.dwHoverTime = 1;
					tme.hwndTrack = closeButton;
					TrackMouseEvent(&tme);
					isMouseLeave = false;
					RedrawWindow(closeButton, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
				}
			}
		} break;
		case WM_MOUSELEAVE:
		{
			if (!isMouseLeave) {
				isMouseLeave = true;
				RedrawWindow(closeButton, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		} break;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
