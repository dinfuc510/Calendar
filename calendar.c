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
#define NOMETAFILE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>

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

#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

#ifndef SRCCOPY
#define SRCCOPY 0x00CC0020
#endif

static bool isMouseLeave = true;
static bool calendarRedraw = true;
static POINT clickedCell = { -1, -1 };

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t mday;
} Date;

//	Gauss's algorithm
int GetWeekDay(const Date* date) {
	int year = date->year, month = date->month;
	int offsets[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
	int offset = offsets[month - 1];
	if (month > 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))) {
		offset = (offset + 1) % 7;
	}
	return (date->mday + offset + 5 * ((year - 1) % 4) + 4 * ((year - 1) % 100) + 6 * ((year - 1) % 400)) % 7;
}

static Date today;
static Date currDate;

static const char listMonthName[12][12] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
static const char mday[31][3] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31" };
static const char dayOfWeek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static inline bool RegisterWindowClass(const char* lpszClassName, WNDPROC lpfnWndProc) {
	WNDCLASSA wc = {0};
	wc.lpfnWndProc = lpfnWndProc;
	wc.lpszClassName = lpszClassName;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = GetModuleHandle(NULL);
	wc.style = CS_CLASSDC;

	return RegisterClass(&wc);
}
static inline unsigned long RGB2BGR(unsigned long color)
{
	return RGB((BYTE)(color >> 16), (BYTE)(color >> 8), (BYTE)(color >> 0));
}
static uint8_t DaysInMonth(uint16_t year, uint8_t month)
{
	if ((month <= 7 && month & 1) || (month >= 8 && month % 2 == 0)) return 31;
	else if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) return 29;

	return 28;
}
static void UpdateCurrentDate(int month)
{
	int newMonth = currDate.month + month;
	int newYear = currDate.year;
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
		HWND base = FindWindow("BaseWindow", NULL);
		if (base != NULL) {
			if (newMonth != currDate.month) {
				if (newMonth >= 1 && newMonth <= 12) {
					SetWindowText(GetDlgItem(base, MONTH_LABEL_ID), listMonthName[newMonth - 1]);
				}
			}
			if (newYear != currDate.year) {
				char year[6] = { 0 };
				snprintf(year, 6, "%u", newYear);
				SetWindowText(GetDlgItem(base, YEAR_LABEL_ID), year);
			}
		}
		uint8_t maximumDayInMonth = DaysInMonth((uint16_t) newYear, (uint8_t) newMonth);
		currDate.year = (uint16_t) newYear;
		currDate.month = (uint8_t) newMonth;
		currDate.mday = currDate.mday <= maximumDayInMonth ? currDate.mday : maximumDayInMonth;
		calendarRedraw = true;
	}
}

static LRESULT BaseWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static bool firstDraw = true;
	switch (msg)
	{
	case WM_CREATE:
		{
			HINSTANCE hInstance = GetModuleHandle(NULL);
			int buttonSize = CALENDAR_POS_Y - TITLEBAR_HEIGHT - 4;
			SIZE septemberSz = {0}, year9999Sz = {0};
			char year[6] = { 0 };
			snprintf(year, 6, "%u", currDate.year);
			{
				HDC hdc = GetDC(hwnd);
				int fontSize = WINDOW_HEIGHT / 32 + 10;
				HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
				HFONT olfhfont = (HFONT)SelectObject(hdc, globalFont);
				GetTextExtentPoint32(hdc, "September", 9, &septemberSz);
				GetTextExtentPoint32(hdc, "9999", 4, &year9999Sz);
				SelectObject(hdc, olfhfont);
				DeleteObject(globalFont);
				ReleaseDC(hwnd, hdc);
			}

			CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)PREVMONTH_BUTTON_ID, hInstance, NULL);
			CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, CALENDAR_POS_X + buttonSize + 4, TITLEBAR_HEIGHT + 4, buttonSize, buttonSize, hwnd, (HMENU)NEXTMONTH_BUTTON_ID, hInstance, NULL);
			CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, WINDOW_WIDTH - TITLEBAR_HEIGHT * 3 / 2, 0, TITLEBAR_HEIGHT * 3 / 2, TITLEBAR_HEIGHT, hwnd, (HMENU)CLOSE_BUTTON_ID, hInstance, NULL);
			CreateWindow("Static", listMonthName[today.month - 1], WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, CALENDAR_POS_X + CALENDAR_WIDTH / 2 - septemberSz.cx, TITLEBAR_HEIGHT, septemberSz.cx, septemberSz.cy, hwnd, (HMENU)MONTH_LABEL_ID, hInstance, NULL);
			CreateWindow("Static", year, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, CALENDAR_POS_X + CALENDAR_WIDTH / 2 + 10, TITLEBAR_HEIGHT, septemberSz.cx, septemberSz.cy, hwnd, (HMENU)YEAR_LABEL_ID, hInstance, NULL);
		} break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps = {0};
			HDC hdc = BeginPaint(hwnd, &ps);

			if (firstDraw) {
				firstDraw = false;
				RECT rc;
				GetWindowRect(hwnd, &rc);
				OffsetRect(&rc, -rc.left, -rc.top);

				RECT wndRect = { 0, TITLEBAR_HEIGHT, rc.right, rc.bottom };
				SetBkColor(hdc, 0x1e1e1e);
				ExtTextOut(hdc, 0, TITLEBAR_HEIGHT, ETO_OPAQUE, &wndRect, NULL, 0, NULL);

				RECT titleBarRect = { 0, 0, rc.right, TITLEBAR_HEIGHT };
				SetBkColor(hdc, 0x2f2f2f);
				ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &titleBarRect, NULL, 0, NULL);
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

						SetBkColor(item->hDC, item->itemState & ODS_SELECTED ? 0x606060 : 0x2f2f2f);
					ExtTextOut(item->hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

					HPEN hpen = CreatePen(PS_SOLID, 2, item->itemState & ODS_SELECTED ? 0xffffff : 0xe3e3e3);
					HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);

					int offsetX = (wparam == PREVMONTH_BUTTON_ID ? -4 : 4), offsetY = 0, direction = (wparam == PREVMONTH_BUTTON_ID ? rc.right / 5 : -rc.right / 5);
					MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, NULL);
				LineTo(item->hDC, rc.right / 2 + direction + offsetX, rc.bottom / 2 - rc.right / 5 + offsetY);						// 4 sqrt(2) ~ 5.6 ~ 6
				MoveToEx(item->hDC, rc.right / 2 + offsetX, rc.bottom / 2 + offsetY, NULL);
				LineTo(item->hDC, rc.right / 2 + direction + offsetX, rc.bottom / 2 + rc.right / 5 + offsetY);
				SelectObject(item->hDC, oldhpen);
				DeleteObject(oldhpen);
			}
		} break;
	case CLOSE_BUTTON_ID:
		{
			SetBkColor(item->hDC, isMouseLeave ? 0x2f2f2f : 0x3030ff);
			ExtTextOut(item->hDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

			HPEN hpen = CreatePen(PS_SOLID, 2, isMouseLeave ? 0x7f7f7f : 0xffffff);
			HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, NULL);
			LineTo(item->hDC, rc.right / 2 - 6, rc.bottom / 2 - 6);						// 4 sqrt(2) ~ 5.6 ~ 6
			LineTo(item->hDC, rc.right / 2 + 6, rc.bottom / 2 + 6);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, NULL);
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
			GetWindowText(item->hwndItem, text, MAX_PATH);
			size_t len = GetWindowTextLength(item->hwndItem);

			int fontSize = WINDOW_HEIGHT / 32 + 10;
			HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			HGDIOBJ oldHfont = SelectObject(item->hDC, globalFont);

			SIZE textSz = { 0 };
			GetTextExtentPoint(item->hDC, text, (int) len, &textSz);

			ExtTextOut(item->hDC, wparam == MONTH_LABEL_ID ? rc.right - textSz.cx : 0, rc.bottom / 2 - textSz.cy / 2, ETO_OPAQUE, &rc, text, (int) len, NULL);

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
}

static LRESULT CalendarWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HFONT oldFont;
	switch (msg)
	{
	case WM_CREATE:
		SetTimer(hwnd, 1000, 1, NULL);
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			if (calendarRedraw) {
				calendarRedraw = false;

				HDC memDc = CreateCompatibleDC(hdc);
				if (memDc != NULL) {
					HBITMAP memBitmap = CreateCompatibleBitmap(hdc, CALENDAR_WIDTH, CALENDAR_HEIGHT);
					if (memBitmap != NULL) {
						HGDIOBJ oldBitmap = SelectObject(memDc, memBitmap);

						RECT fillRect = { 0, 0, CALENDAR_WIDTH, CALENDAR_HEIGHT };
						SetBkColor(memDc, 0x1e1e1e);
						ExtTextOut(memDc, 0, 0, ETO_OPAQUE, &fillRect, NULL, 0, NULL);

						SetTextColor(memDc, 0xffffff);
						HFONT hfont = CreateFont(-fillRect.bottom / 24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
						SIZE textSz = {0};
						RECT cellRect = { CALENDAR_PADDING_X, CALENDAR_PADDING_Y, CELL_WIDTH, CELL_HEIGHT };
						oldFont = (HFONT)SelectObject(memDc, hfont);
						for (int i = 0; i < CALENDAR_ROWS; i++) {
							GetTextExtentPoint32(memDc, dayOfWeek[i], 3, &textSz);
							ExtTextOut(memDc, cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, dayOfWeek[i], 3, NULL);
							OffsetRect(&cellRect, CELL_WIDTH + CELL_PADDING_X, 0);
						}
						SelectObject(memDc, oldFont);
						DeleteObject(hfont);

						int firstDayOfMonth = GetWeekDay(&CLITERAL(Date){currDate.year, currDate.month, 1});
						int numberOfDays = DaysInMonth(currDate.year, currDate.month);

						hfont = CreateFont(-fillRect.bottom / 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
						oldFont = (HFONT)SelectObject(memDc, hfont);
						for (int i = 0; i < CALENDAR_ROWS; i++) {
							for (int j = 0; j < 6; j++) {
								int row = i + firstDayOfMonth, col = j + 1;
								if (row >= 7) {
									col++;
									row %= 7;
								}

								if (i + 7 * j >= numberOfDays)
									break;

								cellRect = CLITERAL(RECT){ 0, 0, CELL_WIDTH, CELL_HEIGHT };
								OffsetRect(&cellRect, row * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, col * (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
								const char* text = mday[i + 7 * j];
								size_t len = strlen(text);
								GetTextExtentPoint32(memDc, text, (int) len, &textSz);
								ExtTextOut(memDc, cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int) len, NULL);
							}
						}

						if (currDate.year == today.year && currDate.month == today.month) {
							int todayRow = GetWeekDay(&today);
							int todayCol = (today.mday + firstDayOfMonth - 1) / 7 + 1;
							SetTextColor(memDc, 0);
							SetBkColor(memDc, 0xffffff);

							cellRect = CLITERAL(RECT){ 0, 0, CELL_WIDTH, CELL_HEIGHT };
							OffsetRect(&cellRect, todayRow * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, todayCol * (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
							const char* text = mday[today.mday - 1];
							size_t len = strlen(text);
							GetTextExtentPoint32(memDc, text, (int) len, &textSz);
							ExtTextOut(memDc,cellRect.left + CELL_WIDTH / 2 - textSz.cx / 2, cellRect.top + CELL_HEIGHT / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int) len, NULL);
						}

						if (clickedCell.x != -1) {
							int cellBorderWidth = 1;

							cellRect = CLITERAL(RECT){ 0, 0, CELL_WIDTH, CELL_HEIGHT };
							OffsetRect(&cellRect, clickedCell.x * (CELL_WIDTH + CELL_PADDING_X) + CALENDAR_PADDING_X, clickedCell.y* (CELL_HEIGHT + CELL_PADDING_Y) + CALENDAR_PADDING_Y);
							HPEN p = CreatePen(PS_SOLID, cellBorderWidth, 0xffffff);
							HPEN oldPen = (HPEN)SelectObject(memDc, GetStockObject(NULL_BRUSH));
							SelectObject(memDc, p);
							Rectangle(memDc, cellRect.left - cellBorderWidth, cellRect.top - cellBorderWidth, cellRect.right + cellBorderWidth, cellRect.bottom + cellBorderWidth);
							SelectObject(memDc, oldPen);
							DeleteObject(p);
							clickedCell.x = clickedCell.y = -1;
						}

						SelectObject(memDc, oldFont);
						DeleteObject(hfont);

						BitBlt(hdc, 0, 0, CALENDAR_WIDTH, CALENDAR_HEIGHT, memDc, 0, 0, SRCCOPY);

						SelectObject(memDc, oldBitmap);
						DeleteObject(memBitmap);
					} else {
						fprintf(stderr, "%s\n", "Could not create memory bitmap");
					}
					DeleteObject(memDc);
				} else {
					fprintf(stderr, "%s\n", "Could not create memory dc");
				}
			}
			EndPaint(hwnd, &ps);
		} break;
	case WM_TIMER:
		RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		break;
	case WM_LBUTTONDOWN:
		{
			int firstDayOfMonth = GetWeekDay(&CLITERAL(Date){currDate.year, currDate.month, 1});
			int x = ((int)(short)LOWORD(lparam)), y = ((int)(short)HIWORD(lparam));
			if ((x % (CELL_WIDTH + CELL_PADDING_X)) > CELL_PADDING_X && (y % (CELL_HEIGHT + CELL_PADDING_Y)) > CELL_PADDING_Y) {
				POINT newCell = { x / (CELL_WIDTH + CELL_PADDING_X), y / (CELL_HEIGHT + CELL_PADDING_Y) };
				if ((newCell.x != clickedCell.x && newCell.y != clickedCell.y) &&
					((newCell.x >= firstDayOfMonth && newCell.y == 1) ||
						(newCell.x + (newCell.y - 1) * 7 < firstDayOfMonth + DaysInMonth(currDate.year, currDate.month) && newCell.y >= 5) ||
						(newCell.y >= 2 && newCell.y <= 4))) {
					clickedCell.x = newCell.x;
				clickedCell.y = newCell.y;
				calendarRedraw = true;
			}
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
		if (oldFont != NULL) {
			DeleteObject(oldFont);
		}
	} break;
default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

return 0;
}

static inline void Init(void)
{
	time_t t = time(0);
	struct tm *now = localtime(&t);
	today = currDate = CLITERAL(Date){ (uint16_t)(now->tm_year + 1900), (uint8_t)(now->tm_mon + 1), (uint8_t) now->tm_mday };
}

#if !defined(_DEBUG) && defined(_WIN32)
int WINAPI WinMain (HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _lpCmdLine, int _nCmdShow)
{
	(void)_hInstance; (void)_hPrevInstance; (void)_lpCmdLine; (void)_nCmdShow;
#else
	int main(void)
	{
#endif

		Init();
		HINSTANCE hInstance = GetModuleHandle(NULL);

		RegisterWindowClass("BaseWindow", BaseWindowProc);
		HWND base = CreateWindow("BaseWindow", NULL, WS_POPUP | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
		ShowWindow(base, SW_SHOW);

		if (base == NULL) {
			UnregisterClass("BaseWindow", hInstance);
			return -1;
		}

		RegisterWindowClass("CalendarWindow", CalendarWindowProc);
		CreateWindow("CalendarWindow", NULL, WS_CHILD | WS_VISIBLE,
			CALENDAR_POS_X, CALENDAR_POS_Y, CALENDAR_WIDTH, CALENDAR_HEIGHT,
			base, (HMENU)CALENDAR_ID, hInstance, NULL);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT) {
					UpdateCurrentDate(msg.wParam == VK_LEFT ? -1 : 1);
				}
				else if (msg.wParam == VK_UP || msg.wParam == VK_DOWN) {
					UpdateCurrentDate(msg.wParam == VK_UP ? -12 : 12);
				}
			}
			else if (msg.message == WM_MOUSEMOVE) {
				if (GetDlgCtrlID(msg.hwnd) == CLOSE_BUTTON_ID && isMouseLeave) {
					TRACKMOUSEEVENT tme = {0};
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE | TME_HOVER;
					tme.dwHoverTime = 1;
					tme.hwndTrack = msg.hwnd;
					TrackMouseEvent(&tme);
					isMouseLeave = false;
					RedrawWindow(msg.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				}
			}
			else if (msg.message == WM_MOUSELEAVE) {
				if (!isMouseLeave) {
					isMouseLeave = true;
					RedrawWindow(msg.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
				}
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return 0;
	}
