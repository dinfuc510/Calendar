#define NOGDICAPMASKS
// #define NOSYSMETRICS
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

#define TITLEBAR_HEIGHT 	32
#define CALENDAR_POS_X 		200
#define CALENDAR_POS_Y 		(TITLEBAR_HEIGHT + 700 / 16 + 4)
#define CLOSE_BUTTON_WIDTH 	48
#define CALENDAR_ROWS 		7
#define CALENDAR_COLS 		7
#define CELL_PADDING_X 		2
#define CELL_PADDING_Y 		2
#define PREVMONTH_BUTTON_ID 2000
#define NEXTMONTH_BUTTON_ID 2001
#define CLOSE_BUTTON_ID		2002
#define MONTH_LABEL_ID		2003
#define YEAR_LABEL_ID		2004
#define CALENDAR_ID			2005

// redefinition
#ifndef SRCCOPY
#define SRCCOPY 0x00CC0020
#endif
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#ifndef MK_LBUTTON
#define MK_LBUTTON 0x0001
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
uint8_t GetWeekDay(const Date* date) {
	uint16_t year = date->year;
	uint8_t month = date->month;
	uint8_t offsets[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
	uint8_t offset = offsets[month - 1];
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
	WNDCLASSA wc = { 0 };
	wc.lpfnWndProc = lpfnWndProc;
	wc.lpszClassName = lpszClassName;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = GetModuleHandle(NULL);
	wc.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

	return RegisterClass(&wc);
}
static inline uint32_t RGB2BGR(uint32_t color)
{
	return RGB((uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)(color >> 0));
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
		clickedCell.x = clickedCell.y = -1;
		uint8_t maximumDayInMonth = DaysInMonth((uint16_t) newYear, (uint8_t) newMonth);
		currDate.year = (uint16_t) newYear;
		currDate.month = (uint8_t) newMonth;
		currDate.mday = currDate.mday <= maximumDayInMonth ? currDate.mday : maximumDayInMonth;
		calendarRedraw = true;
	}
}
#define GetWindowSize(hwnd) _GetWindowSize(hwnd, __FILE__, __LINE__)
SIZE _GetWindowSize(HWND hwnd, const char* filename, int line) {
	if (hwnd == NULL) {
		fprintf(stderr, "%s:%d %s\n", filename, line, "ERROR: hwnd can not NULL");
		return (SIZE){0, 0};
	}
	RECT rc;
	GetWindowRect(hwnd, &rc);
	OffsetRect(&rc, -rc.left, -rc.top);
	return (SIZE){rc.right, rc.bottom};
}

static LRESULT BaseWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static bool canDraw = true;
	switch (msg)
	{
	case WM_CREATE:
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);
		char year[6];
		snprintf(year, 6, "%u", today.year);
		CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)PREVMONTH_BUTTON_ID, hInstance, NULL);
		CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)NEXTMONTH_BUTTON_ID, hInstance, NULL);
		CreateWindow("Button", "", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)CLOSE_BUTTON_ID, hInstance, NULL);
		CreateWindow("Static", listMonthName[today.month - 1], WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)MONTH_LABEL_ID, hInstance, NULL);
		CreateWindow("Static", year, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, 0, 0, 0, 0, hwnd, (HMENU)YEAR_LABEL_ID, hInstance, NULL);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		if (canDraw) {
			canDraw = false;
			SIZE size = GetWindowSize(hwnd);

			RECT wndRect = { 0, TITLEBAR_HEIGHT, size.cx, size.cy };
			SetBkColor(hdc, 0x1e1e1e);
			ExtTextOut(hdc, 0, TITLEBAR_HEIGHT, ETO_OPAQUE, &wndRect, NULL, 0, NULL);

			RECT titleBarRect = { 0, 0, size.cx, TITLEBAR_HEIGHT };
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
			if ((item->itemAction == ODA_DRAWENTIRE && item->itemState == 0) ||
				(item->itemAction == ODA_SELECT && item->itemState & ODS_FOCUS)) {

#ifdef _DEBUG
				printf("%lld %d %d\n", wparam, item->itemAction, item->itemState);
#endif

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

			int offset = 6;
			HPEN hpen = CreatePen(PS_SOLID, 2, isMouseLeave ? 0x7f7f7f : 0xffffff);
			HPEN oldhpen = (HPEN)SelectObject(item->hDC, hpen);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, NULL);
			LineTo(item->hDC, rc.right / 2 - offset, rc.bottom / 2 - offset);						// 4 sqrt(2) ~ 5.6 ~ 6
			LineTo(item->hDC, rc.right / 2 + offset, rc.bottom / 2 + offset);
			MoveToEx(item->hDC, rc.right / 2, rc.bottom / 2, NULL);
			LineTo(item->hDC, rc.right / 2 - offset, rc.bottom / 2 + offset);
			LineTo(item->hDC, rc.right / 2 + offset, rc.bottom / 2 - offset);
			SelectObject(item->hDC, oldhpen);
			DeleteObject(oldhpen);
		} break;
		case MONTH_LABEL_ID:
		case YEAR_LABEL_ID:
		{
			SIZE size = GetWindowSize(hwnd);
			SetTextColor(item->hDC, 0xffffff);
			SetBkColor(item->hDC, 0x1e1e1e);

			char text[MAX_PATH] = { 0 };
			GetWindowText(item->hwndItem, text, MAX_PATH);
			size_t len = GetWindowTextLength(item->hwndItem);

			int fontSize = size.cy / 32 + 10;
			HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			HGDIOBJ oldHfont = SelectObject(item->hDC, globalFont);

			SIZE textSz = { 0 };
			GetTextExtentPoint(item->hDC, text, (int)len, &textSz);

			ExtTextOut(item->hDC, wparam == MONTH_LABEL_ID ? rc.right - textSz.cx : 0, rc.bottom / 2 - textSz.cy / 2, ETO_OPAQUE, &rc, text, (int)len, NULL);

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
		if (GET_Y_LPARAM(lparam) <= TITLEBAR_HEIGHT) {
			PostMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		}
	} break;
	case WM_LBUTTONDBLCLK:
	{
		if (wparam == MK_LBUTTON) {
			if (GET_Y_LPARAM(lparam) <= TITLEBAR_HEIGHT) {
				PostMessage(hwnd, WM_NCLBUTTONDBLCLK, HTCAPTION, 0);
			}
		}
	} break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO info = (LPMINMAXINFO) lparam;
		info->ptMinTrackSize = (POINT){600, 300};
		info->ptMaxTrackSize = (POINT){GetSystemMetrics(SM_CXSCREEN) - 1, GetSystemMetrics(SM_CYSCREEN) - 1};
		info->ptMaxPosition = (POINT){0, 1};
	} break;
	case WM_SIZE:
	{
		int width = LOWORD(lparam), height = HIWORD(lparam);
		SetFocus(hwnd);
		if (width < height) {
			width = height;
			height = LOWORD(lparam);
		}
		SetWindowPos(GetDlgItem(hwnd, CALENDAR_ID), NULL, CALENDAR_POS_X, CALENDAR_POS_Y, LOWORD(lparam) - CALENDAR_POS_X, height - CALENDAR_POS_Y, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
		SetWindowPos(GetDlgItem(hwnd, CLOSE_BUTTON_ID), NULL, LOWORD(lparam) - CLOSE_BUTTON_WIDTH, 0, CLOSE_BUTTON_WIDTH, TITLEBAR_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);

		SIZE septemberSz = {0}, year9999Sz = {0};
		int fontSize = width / 32 + 10;
		if (fontSize > CALENDAR_POS_Y - TITLEBAR_HEIGHT - 10) {
			fontSize = CALENDAR_POS_Y - TITLEBAR_HEIGHT - 10;
		}
		{
			HDC hdc = GetDC(hwnd);
			HFONT globalFont = CreateFont(-fontSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
			HFONT olfhfont = (HFONT)SelectObject(hdc, globalFont);
			GetTextExtentPoint32(hdc, "September", 9, &septemberSz);
			GetTextExtentPoint32(hdc, "9999", 4, &year9999Sz);
			SelectObject(hdc, olfhfont);
			DeleteObject(globalFont);
			ReleaseDC(hwnd, hdc);
		}
		SetWindowPos(GetDlgItem(hwnd, MONTH_LABEL_ID), HWND_BOTTOM, CALENDAR_POS_X + width / 2 - septemberSz.cx, TITLEBAR_HEIGHT, septemberSz.cx, septemberSz.cy, SWP_NOACTIVATE);
		SetWindowPos(GetDlgItem(hwnd, YEAR_LABEL_ID), NULL, CALENDAR_POS_X + width / 2 + 10, TITLEBAR_HEIGHT, year9999Sz.cx, year9999Sz.cy, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(GetDlgItem(hwnd, PREVMONTH_BUTTON_ID), NULL, CALENDAR_POS_X, TITLEBAR_HEIGHT + 4, fontSize, fontSize, SWP_NOZORDER | SWP_NOACTIVATE);
		SetWindowPos(GetDlgItem(hwnd, NEXTMONTH_BUTTON_ID), NULL, CALENDAR_POS_X + fontSize + 4, TITLEBAR_HEIGHT + 4, fontSize, fontSize, SWP_NOZORDER | SWP_NOACTIVATE);

		calendarRedraw = true;
		canDraw = true;
	} break;
	case WM_NCCALCSIZE:
	{
		int border = 2;
		LPNCCALCSIZE_PARAMS params = (LPNCCALCSIZE_PARAMS)lparam;
		params->rgrc[0].left += border;
		// params->rgrc[0].top += border;
		params->rgrc[0].right -= border;
		params->rgrc[0].bottom -= border;

	} break;
	case WM_NCACTIVATE:
		lparam = -1;
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
		RECT rc;
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);

		if (calendarRedraw) {
			calendarRedraw = false;
			uint32_t cell_width = (rc.right - (CALENDAR_ROWS + 1) * CELL_PADDING_X) / CALENDAR_ROWS;
			uint32_t cell_height = (rc.bottom - (CALENDAR_COLS + 1) * CELL_PADDING_Y) / CALENDAR_COLS;

			uint32_t calendar_padding_x = (rc.right - (CALENDAR_ROWS - 1) * (cell_width + CELL_PADDING_X) - cell_width) / 2;
			uint32_t calendar_padding_y = (rc.bottom - (CALENDAR_COLS - 1) * (cell_height + CELL_PADDING_Y) - cell_height) / 2;

			HDC memDc = CreateCompatibleDC(hdc);
			if (memDc != NULL) {
				HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
				if (memBitmap != NULL) {
					HGDIOBJ oldBitmap = SelectObject(memDc, memBitmap);

					RECT fillRect = { 0, 0, rc.right, rc.bottom };
					SetBkColor(memDc, 0x1e1e1e);
					ExtTextOut(memDc, 0, 0, ETO_OPAQUE, &fillRect, NULL, 0, NULL);

					SetTextColor(memDc, 0xa1a1a1);
					HFONT hfont = CreateFont(-fillRect.bottom / 24, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
					SIZE textSz = { 0 };
					RECT cellRect = { calendar_padding_x, calendar_padding_y, cell_width, cell_height };
					oldFont = (HFONT)SelectObject(memDc, hfont);
					for (int i = 0; i < CALENDAR_ROWS; i++) {
						GetTextExtentPoint32(memDc, dayOfWeek[i], 3, &textSz);
						ExtTextOut(memDc, cellRect.left + cell_width / 2 - textSz.cx / 2, cellRect.top + cell_height / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, dayOfWeek[i], 3, NULL);
						OffsetRect(&cellRect, cell_width + CELL_PADDING_X, 0);
					}
					SelectObject(memDc, oldFont);
					DeleteObject(hfont);

					uint8_t firstDayOfMonth = GetWeekDay(&(Date) { currDate.year, currDate.month, 1 });
					uint8_t numberOfDays = DaysInMonth(currDate.year, currDate.month);
					uint8_t numberOfDaysInPrevMonth = 31;
					if (currDate.month > 1) {
						numberOfDaysInPrevMonth = DaysInMonth(currDate.year, currDate.month - 1);
					}

					hfont = CreateFont(-fillRect.bottom / 28, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
					oldFont = (HFONT)SelectObject(memDc, hfont);
					SetTextColor(memDc, 0x4f4f4f);
					for (int i = 0; i < firstDayOfMonth; ++i) {
						cellRect = (RECT) { 0, 0, cell_width, cell_height };
						OffsetRect(&cellRect, i * (cell_width + CELL_PADDING_X) + calendar_padding_x, (cell_height + CELL_PADDING_Y) + calendar_padding_y);
						const char* text = mday[numberOfDaysInPrevMonth - firstDayOfMonth + i];
						size_t len = strlen(text);
						GetTextExtentPoint32(memDc, text, (int)len, &textSz);
						ExtTextOut(memDc, cellRect.left + cell_width / 2 - textSz.cx / 2, cellRect.top + cell_height / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int)len, NULL);
					}
					SetTextColor(memDc, 0xffffff);
					for (int i = 0; i < CALENDAR_ROWS; i++) {
						for (int j = 0; j < 6; j++) {
							int row = i + firstDayOfMonth, col = j + 1;
							if (row >= 7) {
								col++;
								row %= 7;
							}
							if (i + 7 * j >= numberOfDays) {
								break;
							}

							cellRect = (RECT) { 0, 0, cell_width, cell_height };
							OffsetRect(&cellRect, row * (cell_width + CELL_PADDING_X) + calendar_padding_x, col * (cell_height + CELL_PADDING_Y) + calendar_padding_y);
							const char* text = mday[i + 7 * j];
							size_t len = strlen(text);
							GetTextExtentPoint32(memDc, text, (int)len, &textSz);
							ExtTextOut(memDc, cellRect.left + cell_width / 2 - textSz.cx / 2, cellRect.top + cell_height / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int)len, NULL);
						}
					}
					SetTextColor(memDc, 0x4f4f4f);
					for (int i = 0; i < CALENDAR_ROWS; i++) {
						for (int j = 4; j < 6; j++) {
							int row = i + firstDayOfMonth, col = j + 1;
							if (row >= 7) {
								col++;
								row %= 7;
							}
							if (i + 7 * j >= numberOfDays) {
								cellRect = (RECT) { 0, 0, cell_width, cell_height };
								OffsetRect(&cellRect, row * (cell_width + CELL_PADDING_X) + calendar_padding_x, col * (cell_height + CELL_PADDING_Y) + calendar_padding_y);
								const char* text = mday[i - numberOfDays % 7 + 7 * (j - 4)];
								size_t len = strlen(text);
								GetTextExtentPoint32(memDc, text, (int) len, &textSz);
								ExtTextOut(memDc, cellRect.left + cell_width / 2 - textSz.cx / 2, cellRect.top + cell_height / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int)len, NULL);
							}
						}
					}
					if (currDate.year == today.year && currDate.month == today.month) {
						int todayRow = GetWeekDay(&today);
						int todayCol = (today.mday + firstDayOfMonth - 1) / 7 + 1;
						SetTextColor(memDc, 0);
						SetBkColor(memDc, 0xffffff);

						cellRect = (RECT) { 1, 1, cell_width - 1, cell_height - 1 };
						OffsetRect(&cellRect, todayRow * (cell_width + CELL_PADDING_X) + calendar_padding_x, todayCol * (cell_height + CELL_PADDING_Y) + calendar_padding_y);
						const char* text = mday[today.mday - 1];
						size_t len = strlen(text);
						GetTextExtentPoint32(memDc, text, (int)len, &textSz);
						ExtTextOut(memDc, cellRect.left + cell_width / 2 - textSz.cx / 2, cellRect.top + cell_height / 2 - textSz.cy / 2, ETO_OPAQUE, &cellRect, text, (int)len, NULL);
					}

					if (clickedCell.x != -1) {
						int cellBorderWidth = 1;

						cellRect = (RECT) { 0, 0, cell_width, cell_height };
						OffsetRect(&cellRect, clickedCell.x * (cell_width + CELL_PADDING_X) + calendar_padding_x, clickedCell.y * (cell_height + CELL_PADDING_Y) + calendar_padding_y);
						HPEN p = CreatePen(PS_SOLID, cellBorderWidth, 0xffffff);
						HPEN oldPen = (HPEN)SelectObject(memDc, GetStockObject(NULL_BRUSH));
						SelectObject(memDc, p);
						Rectangle(memDc, cellRect.left, cellRect.top, cellRect.right, cellRect.bottom);
						SelectObject(memDc, oldPen);
						DeleteObject(p);
					}

					SelectObject(memDc, oldFont);
					DeleteObject(hfont);

					BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDc, 0, 0, SRCCOPY);

					SelectObject(memDc, oldBitmap);
					DeleteObject(memBitmap);
				}
				else {
					fprintf(stderr, "%s\n", "Could not create memory bitmap");
				}
				DeleteObject(memDc);
			}
			else {
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
		RECT rc;
		GetWindowRect(hwnd, &rc);
		OffsetRect(&rc, -rc.left, -rc.top);

		uint32_t cell_width = ((rc.right - (CALENDAR_ROWS + 1) * CELL_PADDING_X) / CALENDAR_ROWS);
		uint32_t cell_height = ((rc.bottom - (CALENDAR_COLS + 1) * CELL_PADDING_Y) / CALENDAR_COLS);

		int firstDayOfMonth = GetWeekDay(&(Date) { currDate.year, currDate.month, 1 });
		int x = GET_X_LPARAM(lparam), y = GET_Y_LPARAM(lparam);
		if ((x % (cell_width + CELL_PADDING_X)) > CELL_PADDING_X && (y % (cell_height + CELL_PADDING_Y)) > CELL_PADDING_Y) {
			POINT newCell = { x / (cell_width + CELL_PADDING_X), y / (cell_height + CELL_PADDING_Y) };
#ifdef _DEBUG
		printf("%ld %ld - ", clickedCell.x, clickedCell.y);
		printf("%ld %ld\n", newCell.x, newCell.y);
#endif
			if ((newCell.x != clickedCell.x || newCell.y != clickedCell.y) &&
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
	struct tm* now = localtime(&t);
	today = currDate = (Date) { (uint16_t)(now->tm_year + 1900), (uint8_t)(now->tm_mon + 1), (uint8_t)now->tm_mday };
}

#if !defined(_DEBUG) && defined(_WIN32)
int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, LPSTR _lpCmdLine, int _nCmdShow)
{
	(void)_hInstance; (void)_hPrevInstance; (void)_lpCmdLine; (void)_nCmdShow;
#else
int main(void)
{
#endif

	Init();
	HINSTANCE hInstance = GetModuleHandle(NULL);

	RegisterWindowClass("BaseWindow", BaseWindowProc);
	HWND base = CreateWindow("BaseWindow", NULL, WS_THICKFRAME | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (base == NULL) {
		UnregisterClass("BaseWindow", hInstance);
		return -1;
	}
	ShowWindow(base, SW_SHOW);

	SIZE size = GetWindowSize(base);
	RegisterWindowClass("CalendarWindow", CalendarWindowProc);
	CreateWindow("CalendarWindow", NULL, WS_CHILD | WS_VISIBLE,
		CALENDAR_POS_X, CALENDAR_POS_Y, size.cx - CALENDAR_POS_X, size.cy - CALENDAR_POS_Y,
		base, (HMENU) CALENDAR_ID, hInstance, NULL);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_KEYDOWN) {
			if (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT) {
				UpdateCurrentDate(msg.wParam == VK_LEFT ? -1 : 1);
			}
			if (msg.wParam == VK_UP || msg.wParam == VK_DOWN) {
				UpdateCurrentDate(msg.wParam == VK_UP ? -12 : 12);
			}
		}
		else if (msg.message == WM_MOUSEMOVE) {
			if (GetDlgCtrlID(msg.hwnd) == CLOSE_BUTTON_ID && isMouseLeave) {
				TRACKMOUSEEVENT tme = { 0 };
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE;
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
