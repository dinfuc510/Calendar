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
#define _CRT_SECURE_NO_WARNINGS
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

#define CALENDAR_POS_X 200
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
	size_t date = 0;

	Date(size_t d) {
		date = d;
	}
	Date(int year, int month, int day) {
		date = (size_t)(year * 10000 + month * 100 + day);
	}
	int GetYear() const {
		return (int)(date / 10000);
	}

	int GetMonth() const {
		return (date / 100) % 100;
	}

	int GetMonthDay() const {
		return date % 100;
	}

	int GetWeekDay() const {
		//	Gauss's algorithm
		int year = GetYear(), month = GetMonth(), mday = GetMonthDay();
		int offsets[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
		int offset = offsets[month - 1];
		if (month > 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
			offset = (offset + 1) % 7;
		return (mday + offset + 5 * ((year - 1) % 4) + 4 * ((year - 1) % 100) + 6 * ((year - 1) % 400)) % 7;
	}
};
static Date today = 0;
static Date currDate = 0;

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
inline int DaysInMonth(int year, int month)
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
		calendarRedraw = true;
	}
}

LRESULT BaseWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static bool firstDraw = true;
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
			int fontSize = WINDOW_HEIGHT / 32 + 10;
			HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			HFONT olfhfont = (HFONT)SelectObject(hdc, globalFont);
			GetTextExtentPoint32A(hdc, "September", 9, &septemberSz);
			GetTextExtentPoint32A(hdc, "9999", 4, &year9999Sz);
			SelectObject(hdc, olfhfont);
			DeleteObject(globalFont);
			ReleaseDC(hwnd, hdc);
		}

		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)PREVMONTH_BUTTON_ID, hInstance, nullptr);
		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X + buttonSize + 4, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)NEXTMONTH_BUTTON_ID, hInstance, nullptr);
		CreateWindowA("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, WINDOW_WIDTH - TITLEBAR_HEIGHT * 3 / 2, 0, TITLEBAR_HEIGHT * 3 / 2, TITLEBAR_HEIGHT, hwnd, (HMENU)CLOSE_BUTTON_ID, hInstance, nullptr);
		CreateWindowA("Static", listMonthName[today.GetMonth() - 1], WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, CALENDAR_POS_X + CALENDAR_WIDTH / 2 - septemberSz.cx, TITLEBAR_HEIGHT, septemberSz.cx, septemberSz.cy, hwnd, (HMENU)MONTH_LABEL_ID, hInstance, nullptr);
		CreateWindowA("Static", year, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, CALENDAR_POS_X + CALENDAR_WIDTH / 2 + 10, TITLEBAR_HEIGHT, septemberSz.cx, septemberSz.cy, hwnd, (HMENU)YEAR_LABEL_ID, hInstance, nullptr);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps{};
		HDC hdc = BeginPaint(hwnd, &ps);
		
		if (firstDraw) {
			firstDraw = false;
#ifdef _DEBUG
			std::cout << "Redraw Window!!!\n";
#endif // _DEBUG

			RECT rc;
			GetWindowRect(hwnd, &rc);
			OffsetRect(&rc, -rc.left, -rc.top);

			RECT wndRect{ 0, TITLEBAR_HEIGHT, rc.right, rc.bottom };
			SetBkColor(hdc, 0x1e1e1e);
			ExtTextOutA(hdc, 0, TITLEBAR_HEIGHT, ETO_OPAQUE, &wndRect, nullptr, 0, nullptr);

			RECT titleBarRect{ 0, 0, rc.right, TITLEBAR_HEIGHT };
			SetBkColor(hdc, 0x2f2f2f);
			ExtTextOutA(hdc, 0, 0, ETO_OPAQUE, &titleBarRect, nullptr, 0, nullptr);
		}
		EndPaint(hwnd, &ps);
	} break;
	case WM_DRAWITEM:
	{
		const DRAWITEMSTRUCT* item = (const DRAWITEMSTRUCT*)lparam;
		RECT rc = item->rcItem;
		OffsetRect(&rc, -rc.left, -rc.top);

		switch (wparam)
		{
		case PREVMONTH_BUTTON_ID:
		case NEXTMONTH_BUTTON_ID:
		{
			if ((item->itemAction == ODA_DRAWENTIRE	&& item->itemState == 0) ||
				(item->itemAction == ODA_SELECT		&& item->itemState & ODS_FOCUS)) {
#ifdef _DEBUG
				std::cout << (PREVMONTH_BUTTON_ID == wparam ? "Prev redraw" : "Next redraw") << item->itemAction << " " << item->itemState << "\n";
#endif // _DEBUG

				SetBkColor(item->hDC, item->itemState & ODS_SELECTED ? 0x606060 : 0x2f2f2f);
				ExtTextOutA(item->hDC, 0, 0, ETO_OPAQUE, &rc, nullptr, 0, nullptr);

				HPEN hpen = CreatePen(PS_SOLID, 2, 0xffffff);
				HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);

				int offsetX = (wparam == PREVMONTH_BUTTON_ID ? -4 : 4), offsetY = 0, direction = (wparam == PREVMONTH_BUTTON_ID ? rc.right / 5 : -rc.right / 5);
				MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
				LineTo(item->hDC, rc.right / 2 + direction + offsetX, rc.bottom / 2 - rc.right / 5 + offsetY);						// 4 sqrt(2) ~ 5.6 ~ 6
				MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, nullptr);
				LineTo(item->hDC, rc.right / 2 + direction + offsetX, rc.bottom / 2 + rc.right / 5 + offsetY);
				SelectObject(item->hDC, oldhpen);
				DeleteObject(oldhpen);
			}
		} break;
		case CLOSE_BUTTON_ID:
		{
#ifdef _DEBUG
			std::cout << item->itemAction << " " << item->itemState << "\n";
#endif // _DEBUG

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
		case MONTH_LABEL_ID:
		case YEAR_LABEL_ID:
		{
			SetTextColor(item->hDC, 0xffffff);
			SetBkColor(item->hDC, 0x1e1e1e);
			
			char text[MAX_PATH] = { 0 };
			GetWindowTextA(item->hwndItem, text, MAX_PATH);
			size_t len = GetWindowTextLengthA(item->hwndItem);

			int fontSize = WINDOW_HEIGHT / 32 + 10;
			HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			HGDIOBJ oldHfont = SelectObject(item->hDC, globalFont);

			SIZE textSz = { 0 };
			GetTextExtentPointA(item->hDC, text, (int) len, &textSz);

			ExtTextOutA(item->hDC, wparam == MONTH_LABEL_ID ? rc.right - textSz.cx : 0, rc.bottom / 2 - textSz.cy / 2, ETO_OPAQUE, &rc, text, (int) len, nullptr);

			SelectObject(item->hDC, oldHfont);
			DeleteObject(globalFont);
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
		PostQuitMessage(0);
		break;
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
		SetTimer(hwnd, 1000, 1, nullptr);
		break;
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

			if (currDate.GetYear() == today.GetYear() && currDate.GetMonth() == today.GetMonth()) {
				int todayRow = today.GetWeekDay();
				int todayCol = (today.GetMonthDay() + firstDayOfMonth - 1) / 7 + 1;
				SetTextColor(hdc, 0);
				SetBkColor(hdc, 0xffffff);

				cellRect = { 0, 0, CELL_WIDTH, CELL_HEIGHT };
				OffsetRect(&cellRect, todayRow * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, todayCol * (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
				const char* text = mday[today.GetMonthDay() - 1];
				size_t len = strlen(text);
				GetTextExtentPoint32A(hdc, text, (int) len, &textSz);
				ExtTextOutA(hdc,cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int) len, nullptr);
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
	case WM_LBUTTONDOWN:
	{
		int x = ((int)(short)LOWORD(lparam)), y = ((int)(short)HIWORD(lparam));
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
	} break;
	case WM_MOUSEWHEEL:
	{
		short zDelta = GET_WHEEL_DELTA_WPARAM(wparam);
		UpdateCurrentDate(zDelta > 0 ? -12 : 12);
	} break;
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
	struct tm *now = localtime(&t);
	today = currDate = { now->tm_year + 1900, now->tm_mon + 1, now->tm_mday };
}

#ifdef _DEBUG
int main()
#else
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int)
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

	MSG msg{};
	while (GetMessageA(&msg, nullptr, 0, 0)) {
#ifdef _DEBUG
		loopCount++;
		if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) >= std::chrono::microseconds{ 1000 }) {
			start = std::chrono::high_resolution_clock::now();
			//std::cout << "Loop count: " << loopCount << '\r';
			loopCount = 0;
		}
#endif // _DEBUG

		if (msg.message == WM_KEYDOWN) {
			if (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT) {
				UpdateCurrentDate(msg.wParam == VK_LEFT ? -1 : 1);
			}
			else if (msg.wParam == VK_UP || msg.wParam == VK_DOWN) {
				UpdateCurrentDate(msg.wParam == VK_UP ? -12 : 12);
			}
#ifdef _DEBUG
			std::cout << msg.wParam << "(WPARAM)\n";
#endif // _DEBUG
		}
		else if (msg.message == WM_MOUSEMOVE) {
			if (GetDlgCtrlID(msg.hwnd) == CLOSE_BUTTON_ID && isMouseLeave) {
				TRACKMOUSEEVENT tme{};
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE | TME_HOVER;
				tme.dwHoverTime = 1;
				tme.hwndTrack = msg.hwnd;
				TrackMouseEvent(&tme);
				isMouseLeave = false;
				RedrawWindow(msg.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}
		else if (msg.message == WM_MOUSELEAVE) {
			if (!isMouseLeave) {
				isMouseLeave = true;
				RedrawWindow(msg.hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
