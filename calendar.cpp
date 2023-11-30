#include <windows.h>
#include <gdiplus.h>
#include <CommCtrl.h>
#include <memory>
#include <ctime>
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <locale>
#include <codecvt>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
bool RegisterWndClass(std::wstring, WNDPROC);

void DrawWindow(HWND Handle);
void DrawBase(Gdiplus::Graphics* g);
void DrawSysMenu(Gdiplus::Graphics* g);
void DrawTitleBar(Gdiplus::Graphics* g);
void DrawControlBox(Gdiplus::Graphics* g);
void DrawCalendar(Gdiplus::Graphics* g);
void DrawCalendarLabel(Gdiplus::Graphics* g);
void DrawCalendarLabel2(Gdiplus::Graphics* g);
void DrawToday(Gdiplus::Graphics* g);
void DrawClickedCell(Gdiplus::Graphics* g);
void DrawNotedCell(Gdiplus::Graphics* g);
void DrawNote(Gdiplus::Graphics* g);
void DrawNoteLabel(Gdiplus::Graphics* g);
void DrawNoteContent(Gdiplus::Graphics* g);
void DrawScrollbar(Gdiplus::Graphics* g);
void DrawAddNote(Gdiplus::Graphics* g);
void DrawChangeMonth(Gdiplus::Graphics* g);
void DrawPopup(Gdiplus::Graphics* g);
void DrawPopupShadow(Gdiplus::Graphics* g);
void DrawPopupBorder(Gdiplus::Graphics* g);
LRESULT CALLBACK PopupWindowProcedure(HWND, UINT, WPARAM, LPARAM);
void CreatePopup(HWND& popupWindow, HWND parentWindow, HINSTANCE hInstance);

void RoundedRect(Gdiplus::GraphicsPath* path, Gdiplus::RectF bounds, int radius);
void SaveNotes();
void LoadNotes();

constexpr auto ID_TIMER = 10000;
constexpr auto CLOSE_POPUP = 10001;
constexpr auto CONTENT_POPUP = 10002;
constexpr auto ADDNOTE_POPUP = 10003;
const std::string noteFile = "data.txt";
Gdiplus::SizeF windowSize = { 700, 500 };

struct Date {
	int year;
	int month;
	int day;
	int day_of_week;
};

size_t DateHash(Date key);
Date Today();
Date DateJump();
Date ToDate(int year, int month, int day);
std::wstring ToMonthName(int month);
int DaysInMonth(int year, int month);
int NotesInMonth(int year, int month);

Gdiplus::RectF exitWindow{ windowSize.Width - 25, 5, 20, 20 };
Gdiplus::RectF minimizeWindow{ exitWindow.X - exitWindow.Width - 10, exitWindow.Y, exitWindow.Width, exitWindow.Height };
Gdiplus::RectF titleBar{ 0, 0, windowSize.Width, exitWindow.GetBottom() + exitWindow.Y };
int MonthJump = 0;
Gdiplus::RectF calendar{ 200, 30, 480, windowSize.Height - 40 };
Gdiplus::SizeF cellSize{ calendar.Width / 7, calendar.Height / 8 };
Gdiplus::RectF prevMonth{ calendar.X + calendar.Width / 2 - 100, calendar.Y, 30, 30 };
Gdiplus::RectF nextMonth{ calendar.X + calendar.Width / 2 + 105 - prevMonth.Width, prevMonth.Y, prevMonth.Width, prevMonth.Height };
Gdiplus::RectF note{ 10, 30, 150, windowSize.Height - 200 };
Gdiplus::RectF scrollbar{ note.GetRight(), note.Y + (32 + 10), 8, note.Height - (32 + 10) };
Gdiplus::RectF thumb{ scrollbar.X, scrollbar.Y, 8, 0 };
Gdiplus::RectF popup{ windowSize.Width / 2 - 200, windowSize.Height / 2 - 100, 400, 200 };
HWND windowHwnd;
HWND popupHwnd;
Gdiplus::RectF addbutton{ 10, note.GetBottom() + 20, note.Width, 40 };

bool isPopup = false;

std::unordered_map<size_t, std::vector<std::wstring>> allNoteContent;
bool canMoveThumb = false;
float verticalScrollValue = 0;
float oldVerticalScrollValue = 0;
int verticalScrollMaxValue = 0;
float horizontalScrollValue = 0;
float horizontalScrollMaxValue = 0;
Gdiplus::PointF mouseLocation = { 0, 0 };
Gdiplus::PointF oldMouseLocation = { 0, 0 };

Date today = Today();
int todayCell[2] = { ((today.day_of_week - 1) % 7 + 7) % 7, (today.day - 1 + (6 + ToDate(today.year, today.month, 1).day_of_week) % 7) / 7 + 2 };
int clickedCell[2] = { todayCell[0], todayCell[1] };
int clickedDay = today.day;

Gdiplus::Color darkColor(0xf51e1e1e);
Gdiplus::Color lightColor(0xffffffff);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{

	ULONG_PTR gdiplusStartupToken;
	Gdiplus::GdiplusStartupInput gdiInput;
	Gdiplus::GdiplusStartup(&gdiplusStartupToken, &gdiInput, nullptr);
	MSG messages;
	{
		RegisterWndClass(L"TransparentDialog", WindowProcedure);

		windowHwnd = CreateWindowEx(
			WS_EX_LAYERED,
			L"TransparentDialog",
			L"Calendar",
			WS_POPUP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowSize.Width,
			windowSize.Height,
			HWND_DESKTOP,
			nullptr,
			hInstance,
			nullptr
		);

		LoadNotes();

		ShowWindow(windowHwnd, nCmdShow);

		SetTimer(windowHwnd, ID_TIMER, 1000 / 60, nullptr);

		CreatePopup(popupHwnd, windowHwnd, hInstance);
		while (GetMessage(&messages, nullptr, 0, 0))
		{
			if (IsDialogMessage(windowHwnd, &messages) == 0) {
				TranslateMessage(&messages);
				DispatchMessage(&messages);
			}
		}
	}
	Gdiplus::GdiplusShutdown(gdiplusStartupToken);
	return (int)messages.wParam;
}

void DrawWindow(HWND Handle)
{
	std::unique_ptr <Gdiplus::Bitmap> myBitmap = std::make_unique<Gdiplus::Bitmap>(windowSize.Width, windowSize.Height);
	std::unique_ptr<Gdiplus::Graphics> g = std::make_unique<Gdiplus::Graphics>(myBitmap.get());
	g->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
	g->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

	DrawBase(g.get());
	DrawSysMenu(g.get());
	DrawCalendar(g.get());
	DrawNote(g.get());
	DrawScrollbar(g.get());
	DrawAddNote(g.get());
	DrawPopup(g.get());

	RECT window;
	GetWindowRect(Handle, &window);
	HDC screenDc = GetWindowDC(nullptr);
	HDC memDc = CreateCompatibleDC(screenDc);
	HBITMAP hBitmap = nullptr;
	HGDIOBJ oldBitmap = nullptr;
	myBitmap->GetHBITMAP(Gdiplus::Color(0), &hBitmap);
	oldBitmap = SelectObject(memDc, hBitmap);
	SIZE size = { windowSize.Width, windowSize.Height };
	POINT pointSource = { 0, 0 };
	POINT topPos = { window.left, window.top };
	BLENDFUNCTION blend{};
	blend.AlphaFormat = AC_SRC_ALPHA;
	blend.BlendFlags = 0;
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;

	UpdateLayeredWindow(Handle, screenDc, &topPos, &size, memDc, &pointSource, 0, &blend, 0x00000002);
	ReleaseDC(nullptr, screenDc);
	SelectObject(memDc, oldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(memDc);
	DeleteObject(oldBitmap);

	g->Flush();
	g.reset();
	myBitmap.reset();
}

void DrawBase(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();
	RoundedRect(path.get(), Gdiplus::RectF(0, 0, windowSize.Width, windowSize.Height), 8);

	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(darkColor);

	g->FillPath(br.get(), path.get());

	br.reset();
	path.reset();
}

void DrawSysMenu(Gdiplus::Graphics* g)
{
	DrawTitleBar(g);
	DrawControlBox(g);
	DrawChangeMonth(g);
}

void DrawTitleBar(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(0xbb2f2f2f));
	std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();

	RoundedRect(path.get(), Gdiplus::RectF(0, 0, windowSize.Width, windowSize.Height), 10);
	g->SetClip(titleBar);
	g->Clear(0);
	g->FillPath(br.get(), path.get());

	g->ResetClip();
	path.reset();
	br.reset();

	std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	br = std::make_unique<Gdiplus::SolidBrush>(lightColor);
	std::unique_ptr<Gdiplus::StringFormat> fmt = std::make_unique<Gdiplus::StringFormat>();
	fmt->SetAlignment(Gdiplus::StringAlignmentCenter);
	fmt->SetLineAlignment(Gdiplus::StringAlignmentCenter);

	g->DrawString(L"Calendar", 9, font.get(), Gdiplus::RectF(8, 0, windowSize.Width - 8, titleBar.Height), fmt.get(), br.get());

	fmt.reset();
	br.reset();
	font.reset();
}

void DrawControlBox(Gdiplus::Graphics* g)
{
	int iconSize = 10;
	std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(0xffffffff, 2);
	g->DrawLine(p.get(), Gdiplus::PointF{ exitWindow.X + exitWindow.Width / 2 - iconSize / 2, exitWindow.Y + exitWindow.Height / 2 - iconSize / 2 }, Gdiplus::PointF{ exitWindow.X + exitWindow.Width / 2 + iconSize / 2, exitWindow.Y + exitWindow.Height / 2 + iconSize / 2 });
	g->DrawLine(p.get(), Gdiplus::PointF{ exitWindow.X + exitWindow.Width / 2 - iconSize / 2, exitWindow.Y + exitWindow.Height / 2 + iconSize / 2 }, Gdiplus::PointF{ exitWindow.X + exitWindow.Width / 2 + iconSize / 2, exitWindow.Y + exitWindow.Height / 2 - iconSize / 2 });

	g->DrawLine(p.get(), Gdiplus::PointF{ minimizeWindow.X + minimizeWindow.Width / 2 - iconSize / 2, minimizeWindow.Y + minimizeWindow.Height / 2 }, Gdiplus::PointF{ minimizeWindow.X + minimizeWindow.Width / 2 + iconSize / 2, minimizeWindow.Y + minimizeWindow.Height / 2 });

	p.reset();
}

void DrawChangeMonth(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(0xffffffff, 2);
	g->DrawLine(p.get(), Gdiplus::PointF{ prevMonth.X + prevMonth.Width / 3, prevMonth.Y + prevMonth.Height / 2 }, Gdiplus::PointF{ prevMonth.X + 2 * prevMonth.Width / 4, prevMonth.Y + prevMonth.Height / 4 });
	g->DrawLine(p.get(), Gdiplus::PointF{ prevMonth.X + prevMonth.Width / 3, prevMonth.Y + prevMonth.Height / 2 }, Gdiplus::PointF{ prevMonth.X + 2 * prevMonth.Width / 4, prevMonth.Y + 3 * prevMonth.Height / 4 });
	
	g->DrawLine(p.get(), Gdiplus::PointF{ nextMonth.X + 2 * nextMonth.Width / 4, nextMonth.Y + nextMonth.Height / 2 }, Gdiplus::PointF{ nextMonth.X + nextMonth.Width / 3, nextMonth.Y + nextMonth.Height / 4 });
	g->DrawLine(p.get(), Gdiplus::PointF{ nextMonth.X + 2 * nextMonth.Width / 4, nextMonth.Y + nextMonth.Height / 2 }, Gdiplus::PointF{ nextMonth.X + nextMonth.Width / 3, nextMonth.Y + 3 * nextMonth.Height / 4 });

	p.reset();
}

void DrawCalendar(Gdiplus::Graphics* g)
{
	g->TranslateTransform(calendar.X, calendar.Y);
	DrawCalendarLabel(g);
	DrawCalendarLabel2(g);
	DrawToday(g);
	DrawClickedCell(g);
	DrawNotedCell(g);
	g->ResetTransform();
}

void DrawCalendarLabel(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 20, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(lightColor);
	std::unique_ptr<Gdiplus::StringFormat> fmt = std::make_unique<Gdiplus::StringFormat>();
	fmt->SetAlignment(Gdiplus::StringAlignmentCenter);
	fmt->SetLineAlignment(Gdiplus::StringAlignmentCenter);

	Date dateJump = DateJump();
	std::wstring label = ToMonthName(dateJump.month) + L" " + std::to_wstring(dateJump.year);
	g->DrawString(label.c_str(), -1, font.get(), Gdiplus::RectF{ 0, 0, calendar.Width, 20 * 96 / 72 },
		fmt.get(), br.get());

	std::wstring dayOfWeek[7] = { L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat", L"Sun" };
	for (int i = 0; i < 7; i++) {
		g->DrawString(dayOfWeek[i].c_str(), -1, font.get(), Gdiplus::RectF{ i * cellSize.Width, cellSize.Height, cellSize.Width, 20 * 96 / 72 }, fmt.get(), br.get());
	}

	fmt.reset();
	br.reset();
	font.reset();
}

void DrawCalendarLabel2(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(lightColor);
	std::unique_ptr<Gdiplus::StringFormat> fmt = std::make_unique<Gdiplus::StringFormat>();
	fmt->SetAlignment(Gdiplus::StringAlignmentCenter);

	Date dateJump = DateJump();
	int firstDayOfMonth = (dateJump.day_of_week - dateJump.day % 7 + 7) % 7;
	int numberOfDays = DaysInMonth(dateJump.year, dateJump.month);
	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			int row = i + firstDayOfMonth, col = j;
			if (row >= 7)
			{
				col++;
				row %= 7;
			}
			if (i + 7 * j + 1 > numberOfDays) break;

			g->DrawString(std::to_wstring(i + 7 * j + 1).c_str(), -1, font.get(), Gdiplus::RectF{ row * cellSize.Width, (col + 2) * cellSize.Height, cellSize.Width, cellSize.Height }, fmt.get(), br.get());
		}
	}

	fmt.reset();
	br.reset();
	font.reset();
}

void DrawToday(Gdiplus::Graphics* g)
{
	if (MonthJump == 0) {
		std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(lightColor);
		std::unique_ptr<Gdiplus::StringFormat> fmt = std::make_unique<Gdiplus::StringFormat>();
		fmt->SetAlignment(Gdiplus::StringAlignmentCenter);

		Date new_date = ToDate(today.year, today.month, 1);
		int row = (((int)today.day_of_week - 1) % 7 + 7) % 7;
		int col = (today.day - 1 + (6 + new_date.day_of_week) % 7) / 7;

		Gdiplus::RectF bounds{ row * cellSize.Width, (col + 2) * cellSize.Height, cellSize.Width, cellSize.Height };
		g->FillRectangle(br.get(), bounds);

		std::unique_ptr<Gdiplus::SolidBrush> _br = std::make_unique<Gdiplus::SolidBrush>(darkColor);
		g->DrawString(std::to_wstring(today.day).c_str(), -1, font.get(), bounds, fmt.get(), _br.get());

		fmt.reset();
		br.reset();
		font.reset();
	}
}

void DrawClickedCell(Gdiplus::Graphics* g)
{
	if (clickedCell[0] != -1) {
		std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(lightColor, 2);
		Gdiplus::RectF clickedCellRect{ clickedCell[0] * cellSize.Width, clickedCell[1] * cellSize.Height, cellSize.Width, cellSize.Height };

		g->DrawRectangle(p.get(), clickedCellRect);

		p.reset();
		font.reset();

		int glowDist = 50;
		double glowOpacity = 2.0;
		std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();
		path.get()->AddRectangle(clickedCellRect);
		for (int i = 1; i <= glowDist; i += 3)
		{
			int alpha = (int)round(glowOpacity - glowOpacity / glowDist * i);
			std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(Gdiplus::Color(alpha, 255, 255, 255), i);
			p.get()->SetLineJoin(Gdiplus::LineJoinRound);

			g->DrawPath(p.get(), path.get());

			p.reset();
		}

		path.reset();
	}
}

void DrawNotedCell(Gdiplus::Graphics* g)
{
	Date dateJump = DateJump();
	Date firstDayOfMonth = ToDate(dateJump.year, dateJump.month, 1);
	int beginningEmptyCells = (6 + firstDayOfMonth.day_of_week) % 7;
	std::unique_ptr<Gdiplus::LinearGradientBrush> br = std::make_unique<Gdiplus::LinearGradientBrush>(Gdiplus::RectF{ 0, 0, cellSize.Width, cellSize.Height }, 0xffffa500, 0xff0000ff, Gdiplus::LinearGradientModeForwardDiagonal);
	br.get()->SetGammaCorrection(TRUE);
	std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(br.get(), 4);
	for (int i = 1; i <= DaysInMonth(dateJump.year, dateJump.month); i++)
	{
		if (allNoteContent.find(DateHash({ dateJump.year, dateJump.month, i, -1 })) != allNoteContent.end())
		{
			int row = (((firstDayOfMonth.day_of_week - 1) % 7 + 7) % 7 + (i - 1)) % 7;
			int col = (i - 1 + beginningEmptyCells) / 7 + 2;

			g->DrawRectangle(p.get(), Gdiplus::RectF{ row * cellSize.Width, col * cellSize.Height, cellSize.Width, cellSize.Height });
		}
	}

	p.reset();
	br.reset();
}

void DrawNote(Gdiplus::Graphics* g)
{
	g->TranslateTransform(note.X, note.Y);

	DrawNoteLabel(g);
	DrawNoteContent(g);

	g->ResetTransform();
}

void DrawNoteLabel(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(lightColor);
	std::wstring label = L"";

	if (clickedCell[0] == -1)
	{
		int numberOfNotes = NotesInMonth(DateJump().year, DateJump().month);
		label = L"You have ";
		label += std::to_wstring(numberOfNotes);
		label += L" note(s)";
		std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
		g->DrawString(label.c_str(), -1, font.get(), Gdiplus::PointF{ 0, 0 }, br.get());
		font.reset();
	}
	else
	{
		std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 32, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);

		if (MonthJump == 0 && clickedCell[0] == todayCell[0] && clickedCell[1] == todayCell[1])
			label = L"Today";
		else {
			label = std::to_wstring(clickedDay) + L"/";
			label += std::to_wstring(DateJump().month);
		}
		g->DrawString(label.c_str(), -1, font.get(), Gdiplus::PointF{ 0, 0 }, br.get());
		font.reset();
	}

	br.reset();
}

void DrawNoteContent(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::Pen> p = std::make_unique<Gdiplus::Pen>(0x33ffffff, 2);
	g->DrawRectangle(p.get(), Gdiplus::RectF{0, 32 + 10 - 1, note.Width + scrollbar.Width, note.Height - 32 - 10 + 2});
	p.reset();

	if (clickedCell[0] == -1) return;

	size_t key{ DateHash(Date{DateJump().year, DateJump().month, clickedDay, -1}) };
	if (allNoteContent.find(key) != allNoteContent.end())
	{
		std::vector<std::wstring> noteContent{ allNoteContent[key] };
		std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
		std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(lightColor);

		int noteContentHeight = (10 + 16) * (int)noteContent.size();
		thumb.Height = note.Height - (32 + 10) - 0.3 * noteContentHeight;
		if (thumb.Height < 40) thumb.Height = 40;
		verticalScrollMaxValue = noteContentHeight - scrollbar.Height;
		horizontalScrollMaxValue = 0;

		HRGN clip = CreateRectRgn(note.X, note.Y + (32 + 10), note.GetRight(), note.GetBottom());
		g->TranslateTransform(horizontalScrollValue, verticalScrollValue);
		g->SetClip(clip);
		DeleteObject(clip);
		Gdiplus::RectF bounds;
		for (int i = 0; i < (int)noteContent.size(); i++) {
			g->MeasureString((L" \u25aa " + noteContent[i]).c_str(), -1, font.get(), Gdiplus::PointF{0, 0}, &bounds);
			if (bounds.Width > horizontalScrollMaxValue) horizontalScrollMaxValue = bounds.Width;
			g->DrawString((L" \u25aa " + noteContent[i]).c_str(), -1, font.get(), Gdiplus::PointF{0, Gdiplus::REAL((16 + 10) * i + (32 + 10)) }, br.get());
		}
		g->ResetClip();
		g->ResetTransform();

		br.reset();
		font.reset();
	}
}

void DrawScrollbar(Gdiplus::Graphics* g)
{
	if (verticalScrollMaxValue <= 0) return;

	thumb.Y = -verticalScrollValue * (note.Height - thumb.Height - (32 + 10)) / verticalScrollMaxValue + note.Y + (32 + 10);
	std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();
	RoundedRect(path.get(), thumb, thumb.Width / 2);
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(50, lightColor.GetR(), lightColor.GetG(), lightColor.GetB()));

	g->FillRectangle(br.get(), scrollbar);
	br.reset();
	br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(200, lightColor.GetR(), lightColor.GetG(), lightColor.GetB()));
	g->FillPath(br.get(), path.get());

	br.reset();
	path.reset();
}

void DrawAddNote(Gdiplus::Graphics* g)
{
	std::unique_ptr<Gdiplus::Font>font = std::make_unique<Gdiplus::Font>(L"Segoe UI", 16, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(clickedCell[0] != -1 ? 0xff808080 : 0x33808080));

	g->FillRectangle(br.get(), addbutton);
	Gdiplus::RectF boundsText;
	g->MeasureString(L"Add Note", 8, font.get(), addbutton, &boundsText);
	br.reset();
	br = std::make_unique<Gdiplus::SolidBrush>(clickedCell[0] != -1 ? lightColor : 0x33ffffff);
	g->DrawString(
		L"Add Note",
		8,
		font.get(),
		Gdiplus::PointF{ addbutton.X + addbutton.Width / 2 - boundsText.Width / 2, addbutton.Y + addbutton.Height / 2 - boundsText.Height / 2 },
		br.get());

	br.reset();
	font.reset();
}

void DrawPopup(Gdiplus::Graphics* g)
{
	DrawPopupShadow(g);
	DrawPopupBorder(g);
}

void DrawPopupShadow(Gdiplus::Graphics* g)
{
	if (!isPopup) return;

	std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();
	RoundedRect(path.get(), Gdiplus::RectF(0, 0, windowSize.Width, windowSize.Height), 10);

	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(100, 0, 0, 0));

	g->SetClip(titleBar, Gdiplus::CombineModeExclude);
	g->FillPath(br.get(), path.get());

	g->ResetClip();
	path.reset();
	br.reset();

	for (int i = popup.X - 40; i < popup.X; i += 3)
	{
		path = std::make_unique<Gdiplus::GraphicsPath>();
		br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color((i - (popup.X - 40)) / 2, 0, 0, 0));
		RoundedRect(path.get(), Gdiplus::RectF(i, i, windowSize.Width - i * 2, windowSize.Height - i * 2), 40);

		g->FillPath(br.get(), path.get());

		path.reset();
		br.reset();
	}
}

void DrawPopupBorder(Gdiplus::Graphics* g)
{
	if (!isPopup) return;

	std::unique_ptr<Gdiplus::GraphicsPath> path = std::make_unique<Gdiplus::GraphicsPath>();
	RoundedRect(
		path.get(),
		Gdiplus::RectF(
			popup.X, popup.Y,
			popup.Width,
			popup.Height
		),
		10);

	std::unique_ptr<Gdiplus::SolidBrush> br = std::make_unique<Gdiplus::SolidBrush>(darkColor);
	g->FillPath(br.get(), path.get());
	br.reset();
	path.reset();

	float padding = 10;
	br = std::make_unique<Gdiplus::SolidBrush>(Gdiplus::Color(0xffffffff));
	path = std::make_unique<Gdiplus::GraphicsPath>();
	RoundedRect(path.get(), Gdiplus::RectF{ popup.X + popup.Width / 2 - 200 / 2 - padding,
		popup.Y + popup.Height / 2 - 40 / 2 - 4,
		200 + padding * 2,
		24 + 4 * 2 }, 12);
	g->FillPath(br.get(), path.get());
	path.reset();
	br.reset();
}

std::wstring ToMonthName(int month)
{
	std::wstring list[12] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December" };
	return list[month - 1];
}

int DaysInMonth(int year, int month)
{
	if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) return 31;
	else if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) return 29;
	
	return 28;
}

Date Today()
{
	std::unique_ptr<std::time_t> t = std::make_unique<std::time_t>(std::time(0));
	struct tm now;
	localtime_s(&now, t.get());
	Date today{ now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_wday };

	t.reset();
	return today;
}

Date DateJump()
{
	int new_month = today.month + MonthJump, new_year = today.year;
	if (new_month > 12)
	{
		new_year += new_month / 12;
		new_month %= 12;
		if (new_month == 0)
		{
			new_month = 12;
			new_year--;
		}
	}
	else if (new_month < 1)
	{
		new_year += new_month / 12 - 1;
		new_month = 12 + new_month % 12;
	}

	int maximumDayInMonth = DaysInMonth(new_year, new_month);
	return ToDate(new_year, new_month, today.day <= maximumDayInMonth ? today.day : maximumDayInMonth);
}

Date ToDate(int year, int month, int day)
{
	std::tm time_in = { 0, 0, 0, day, month - 1, year - 1900 };
	std::time_t time_temp = std::mktime(&time_in);

	struct tm new_date;
	localtime_s(&new_date, &time_temp);

	return Date{ new_date.tm_year + 1900, new_date.tm_mon + 1, new_date.tm_mday, new_date.tm_wday };
}

size_t DateHash(Date key)
{
	std::tm curr{ 0 };
	curr.tm_year = key.year - 1900;
	curr.tm_mon = key.month - 1;
	curr.tm_mday = key.day;

	return std::mktime(&curr);
}

int NotesInMonth(int year, int month)
{
	int count = 0;
	for (int i = 1; i <= 31; i++)
	{
		size_t key{ DateHash(Date{year, month, i, -1}) };
		if (allNoteContent.find(key) != allNoteContent.end()) count++;
	}

	return count;
}

void RoundedRect(Gdiplus::GraphicsPath* path, Gdiplus::RectF bounds, int radius)
{
	int diameter = radius * 2;
	Gdiplus::RectF arc = Gdiplus::RectF(bounds.X, bounds.Y, diameter, diameter);
	path->AddArc(arc, 180, 90);
	arc.X = bounds.GetRight() - diameter;
	path->AddArc(arc, 270, 90);
	arc.Y = bounds.GetBottom() - diameter;
	path->AddArc(arc, 0, 90);
	arc.X = bounds.GetLeft();
	path->AddArc(arc, 90, 90);
	path->CloseFigure();
}

static std::wstring Utf8ToWstr(const std::string& utf8) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return wcu8.from_bytes(utf8);
}

static std::string WstrToUtf8(const std::wstring& utf16) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wcu8;
	return wcu8.to_bytes(utf16);
}

void LoadNotes()
{
	std::ifstream ifs{ noteFile, std::ios::in };

	if (ifs.is_open())
	{
		std::string line;

		while (std::getline(ifs, line))
		{
			size_t delimiterIdx = line.find(":");
			time_t key = std::stoull(line.substr(0, delimiterIdx));

			std::vector<std::wstring> value{};
			size_t start = delimiterIdx + 1, end{};
			while ((end = line.find("\u200b", start)) != std::string::npos)
			{
				std::string note = line.substr(start, end - start);
				if (note != "")
					value.emplace_back(Utf8ToWstr(note));

				start = end + 1;
			}

			allNoteContent.emplace(key, value);
		}

		ifs.close();
	}
}

void SaveNotes()
{
	std::ofstream ofs{ noteFile, std::ios::out };

	if (ofs.is_open())
	{
		for (const auto& notes : allNoteContent)
		{
			ofs << notes.first << ":";

			for (const std::wstring& s : notes.second)
			{
				ofs << WstrToUtf8(s) << "\u200b";
			}

			ofs << "\n";
		}

		ofs.close();
	}
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		KillTimer(hWnd, ID_TIMER);
		DestroyWindow(popupHwnd);
		SaveNotes();
		PostQuitMessage(0);
		break;
	}
	case WM_TIMER:
	{
		switch (wParam)
		{
		case ID_TIMER:
		{
			DrawWindow(hWnd);
			KillTimer(hWnd, ID_TIMER);
			break;
		}
		default:
			break;
		}
		break;
	}
	case WM_KEYUP:
	{
		if (!isPopup)
		{
			int tilt = (int)wParam;
			if (tilt == 37 || tilt == 39)
			{
				if (calendar.Contains(mouseLocation))
				{
					MonthJump += (tilt == 37) ? -1 : 1;
					clickedCell[0] = clickedCell[1] = -1;
					verticalScrollMaxValue = 0;
					horizontalScrollValue = 0;
					SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
				}
				else if (note.Contains(mouseLocation))
				{
					if (horizontalScrollMaxValue > note.Width)
					{
						horizontalScrollValue += (tilt == 37) ? 20 : -20;
						if (horizontalScrollValue > 0) horizontalScrollValue = 0;
						else if (horizontalScrollValue < -(horizontalScrollMaxValue - note.Width)) horizontalScrollValue = -(horizontalScrollMaxValue - note.Width);
						SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
					}
				}
			}
		}
		break;
	}
	case WM_MOUSEWHEEL:
	{
		if (verticalScrollMaxValue > 0)
		{
			POINT e;
			if (GetCursorPos(&e))
			{
				if (ScreenToClient(hWnd, &e))
				{
					if (e.x >= note.X && e.x <= note.X + note.Width && e.y >= note.Y && e.y <= note.Y + note.Height)
					{
						short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
						if (zDelta != 0)
						{
							if (zDelta > 0)
							{
								if (verticalScrollValue + 10 < 0)
									verticalScrollValue += 10;
								else
									verticalScrollValue = 0;
							}
							else if (zDelta < 0)
							{
								if (verticalScrollValue - 10 > -verticalScrollMaxValue)
									verticalScrollValue -= 10;
								else
									verticalScrollValue = -verticalScrollMaxValue;
							}

							SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
						}
					}
				}
			}
		}
		break;
	}
	case WM_LBUTTONDOWN:
	{
		POINT e;
		if (GetCursorPos(&e))
		{
			if (ScreenToClient(hWnd, &e))
			{
				if (!isPopup)
				{
					if (prevMonth.Contains(mouseLocation))
					{
						MonthJump--;
						clickedCell[0] = clickedCell[1] = -1;
						verticalScrollMaxValue = 0;
						SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
					}
					else if (nextMonth.Contains(mouseLocation))
					{
						MonthJump++;
						clickedCell[0] = clickedCell[1] = -1;
						verticalScrollMaxValue = 0;
						SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
					}
				}

				if (titleBar.Contains(mouseLocation))
				{
					if (minimizeWindow.Contains(mouseLocation))
					{
						if (isPopup)
							ShowWindow(windowHwnd, SW_FORCEMINIMIZE);
						else
							ShowWindow(windowHwnd, SW_MINIMIZE);
					}
					else if (exitWindow.Contains(mouseLocation))
					{
						DestroyWindow(windowHwnd);
					}
					else
					{
						ReleaseCapture();
						SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
					}
				}

				if (!isPopup)
				{
					if (thumb.Contains(mouseLocation))
					{
						canMoveThumb = true;
						oldVerticalScrollValue = verticalScrollValue;
						oldMouseLocation = mouseLocation;
					}

					Gdiplus::PointF pt{ e.x - calendar.X, e.y - calendar.Y };
					if (pt.X > 0 && pt.Y > 0)
					{
						if (pt.Y > 2 * cellSize.Height)
						{
							int oldCell[2] = { clickedCell[0], clickedCell[1] };
							clickedCell[0] = (int)(pt.X / cellSize.Width);
							clickedCell[1] = (int)(pt.Y / cellSize.Height);
							Date dateJump = DateJump();
							Date new_date = ToDate(dateJump.year, dateJump.month, 1);
							int beginningEmptyCells = (6 + new_date.day_of_week) % 7;

							if (clickedCell[0] >= 7)
							{
								clickedCell[0] = oldCell[0];
								clickedCell[1] = oldCell[1];
							}
							else if (clickedCell[1] == 2 || clickedCell[1] >= 6)
							{
								if (clickedCell[1] == 2 && clickedCell[0] < beginningEmptyCells)
								{
									clickedCell[0] = oldCell[0];
									clickedCell[1] = oldCell[1];
								}
								else
								{
									int endEmptyCells = beginningEmptyCells + DaysInMonth(dateJump.year, dateJump.month);
									if (clickedCell[0] + (clickedCell[1] - 2) * 7 >= endEmptyCells)
									{
										clickedCell[0] = oldCell[0];
										clickedCell[1] = oldCell[1];
									}
								}
							}

							if (clickedCell[0] != oldCell[0] || clickedCell[1] != oldCell[1]) {
								clickedDay = clickedCell[0] + (clickedCell[1] - 2) * 7 - beginningEmptyCells + 1;
								verticalScrollValue = 0;
								verticalScrollMaxValue = 0;
								horizontalScrollValue = 0;
								SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
							}
						}
					}
				}

				if (addbutton.Contains(mouseLocation))
				{
					if (clickedCell[0] != -1)
					{
						isPopup = true;
						SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
						ShowWindow(popupHwnd, SW_SHOW);
					}
				}
			}
		}
		break;
	}
	case WM_LBUTTONUP:
	{
		canMoveThumb = false;
		break;
	}
	case WM_MOUSEMOVE:
	{
		POINT e;
		if (GetCursorPos(&e))
		{
			if (ScreenToClient(hWnd, &e))
			{
				mouseLocation = Gdiplus::PointF{ Gdiplus::REAL(e.x), Gdiplus::REAL(e.y) };
				if (canMoveThumb)
				{
					verticalScrollValue = -(mouseLocation.Y - oldMouseLocation.Y) / scrollbar.Height * verticalScrollMaxValue + oldVerticalScrollValue;
					if (verticalScrollValue <= -verticalScrollMaxValue)
						verticalScrollValue = -verticalScrollMaxValue;
					else if (verticalScrollValue >= 0)
						verticalScrollValue = 0;

					SetTimer(hWnd, ID_TIMER, 1000 / 60, nullptr);
				}
			}
		}
		break;
	}
	case WM_SETCURSOR:
	{
		if (minimizeWindow.Contains(mouseLocation) || exitWindow.Contains(mouseLocation) ||
			(!isPopup && clickedCell[0] != -1 && addbutton.Contains(mouseLocation)))
		{
			SetCursor(LoadCursor(nullptr, IDC_HAND));
		}
		else
		{
			SetCursor(LoadCursor(nullptr, IDC_ARROW));
		}
		return TRUE;

	} break;
	case WM_MOVE:
	{
		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		SetWindowPos(
			popupHwnd,
			HWND_NOTOPMOST,
			popup.X + windowRect.left,
			popup.Y + windowRect.top,
			popup.Width,
			popup.Height,
			SWP_NOACTIVATE);
		break;
	}
	case WM_KILLFOCUS:
	{
		SetWindowPos(popupHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void CreatePopup(HWND& popupWindow, HWND parentWindow, HINSTANCE hInstance)
{
	RegisterWndClass(L"Popup", PopupWindowProcedure);

	popupWindow = CreateWindowEx(
		WS_EX_TOPMOST,
		L"Popup",
		L"Popup",
		WS_POPUP,
		windowSize.Width / 2 - popup.Width / 2,
		windowSize.Height / 2 - popup.Height / 2,
		popup.Width,
		popup.Height,
		HWND_DESKTOP,
		nullptr,
		hInstance,
		nullptr
	);
	SetWindowLongPtr(popupWindow, -8, (LONG)parentWindow);

	ShowWindow(popupWindow, SW_HIDE);
}

LRESULT CALLBACK PopupWindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND contentHwnd;
	static HWND buttonHwnd;
	static HWND closeHwnd;
	static SUBCLASSPROC subclassEdit;
	static SUBCLASSPROC subclassButton;
	static HFONT hFont = CreateFont(24, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Segoe UI"));
	static HBRUSH popupBackColor = CreateSolidBrush(0x1e1e1e);
	switch (message)
	{
	case WM_CREATE:
	{
		subclassEdit = [](HWND hWnd, UINT uMsg, WPARAM wParam,
			LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
				switch (uMsg)
				{
				case WM_KEYDOWN:
				{
					if (wParam == VK_RETURN)
					{
						const int len = GetWindowTextLength(hWnd) + 1;
						wchar_t* text = new wchar_t[len];
						GetWindowText(hWnd, text, len);
						if (len > 1 || text[0] != L'\0')
						{
							size_t key{ DateHash(Date{DateJump().year, DateJump().month, clickedDay, -1}) };
							if (allNoteContent.find(key) != allNoteContent.end())
							{
								allNoteContent[key].emplace_back(text);
							}
							else
							{
								allNoteContent.emplace(key, std::vector<std::wstring>{ text });
							}
							SetWindowText(hWnd, L"");

							isPopup = false;

							if (verticalScrollMaxValue + (10 + 16) + scrollbar.Height > note.Height)
								verticalScrollValue = -verticalScrollMaxValue - (10 + 16);

							SetWindowPos(GetParent(hWnd), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
							SetTimer(windowHwnd, ID_TIMER, 1000 / 60, nullptr);
							ShowWindow(GetParent(hWnd), SW_HIDE);
						}
						delete[] text;
					}
				} break;
				default:
					return DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}
				return 0;
			};
		subclassButton = [](HWND hWnd, UINT uMsg, WPARAM wParam,
			LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
				static bool isMouseDown = false;
				static bool isMouseLeave = true;
				switch (uMsg)
				{
				case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC hdc = BeginPaint(hWnd, &ps);

					RECT rect;
					GetWindowRect(hWnd, &rect);
					RECT rcItem{ 0, 0, rect.right - rect.left, rect.bottom - rect.top };

					HBRUSH hBrush = CreateSolidBrush(isMouseDown ? 0xa1470d : isMouseLeave ? 0xc06515 : 0xd27619);
					FillRect(hdc, &rcItem, hBrush);
					DeleteObject(hBrush);

					SetBkMode(hdc, TRANSPARENT);
					SetTextColor(hdc, 0xffffff);

					HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
					SelectObject(hdc, hFont);
					int len = GetWindowTextLength(hWnd) + 1;
					LPTSTR lpBuff = new TCHAR[len];
					len = GetWindowText(hWnd, lpBuff, len);
					DrawText(hdc, lpBuff, len, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
					delete[] lpBuff;

					EndPaint(hWnd, &ps);

				} break;
				case WM_MOUSEMOVE:
				{
					if (isMouseLeave)
					{
						TRACKMOUSEEVENT tme{};
						tme.cbSize = sizeof(TRACKMOUSEEVENT);
						tme.dwFlags = TME_LEAVE;
						tme.dwHoverTime = 1;
						tme.hwndTrack = hWnd;
						TrackMouseEvent(&tme);
						isMouseLeave = false;
						SetTimer(hWnd, 1000, 1000 / 60, nullptr);
					}
				} break;
				case WM_MOUSELEAVE:
				{
					isMouseLeave = true;
					SetTimer(hWnd, 1000, 1000 / 60, nullptr);
				} break;
				case WM_LBUTTONDOWN:
				{
					isMouseDown = true;
					SetTimer(hWnd, 1000, 1000 / 60, nullptr);
				} break;
				case WM_LBUTTONUP:
				{
					isMouseDown = false;
					const int len = GetWindowTextLength(contentHwnd) + 1;
					wchar_t* text = new wchar_t[len];
					GetWindowText(contentHwnd, text, len);
					if (len > 1 || text[0] != L'\0')
					{
						size_t key{ DateHash(Date{DateJump().year, DateJump().month, clickedDay, -1}) };
						if (allNoteContent.find(key) != allNoteContent.end())
						{
							allNoteContent[key].emplace_back(text);
						}
						else
						{
							allNoteContent.emplace(key, std::vector<std::wstring>{ text });
						}
						SetWindowText(contentHwnd, L"");

						isPopup = false;

						if (verticalScrollMaxValue + (10 + 16) + scrollbar.Height > note.Height)
							verticalScrollValue = -verticalScrollMaxValue - (10 + 16);

						SetWindowPos(GetParent(hWnd), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
						SetTimer(windowHwnd, ID_TIMER, 1000 / 60, nullptr);
						ShowWindow(GetParent(hWnd), SW_HIDE);
					}
					delete[] text;
					SetTimer(hWnd, 1000, 1000 / 60, nullptr);
				} break;
				case WM_TIMER:
				{
					InvalidateRect(hWnd, nullptr, FALSE);
					KillTimer(hWnd, wParam);

					return 0;
				} break;
				case WM_SETCURSOR:
					SetCursor(LoadCursor(nullptr, IDC_HAND));
					break;
				default:
					return DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}
				return 0;
			};

		contentHwnd = CreateWindow(
			L"EDIT",
			L"",
			WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
			popup.Width / 2 - 200 / 2,
			popup.Height / 2 - 40 / 2,
			200,
			24,
			hWnd,
			(HMENU)CONTENT_POPUP,
			GetModuleHandle(nullptr),
			nullptr);
		SendMessage(contentHwnd, WM_SETFONT, (LPARAM)hFont, TRUE);
		SendMessage(contentHwnd, EM_SETCUEBANNER, 1, (LPARAM)L"Write your note");
		SetWindowSubclass(contentHwnd, subclassEdit, 0, 0);

		buttonHwnd = CreateWindow(
			L"BUTTON",
			L"Add Note",
			WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
			popup.Width / 2 - 100 / 2,
			popup.Height - 50,
			100,
			30,
			hWnd,
			(HMENU)ADDNOTE_POPUP,
			(HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE),
			nullptr);
		SendMessage(buttonHwnd, WM_SETFONT, (LPARAM)hFont, TRUE);
		SetWindowSubclass(buttonHwnd, subclassButton, 0, 0);

		closeHwnd = CreateWindow(
			L"STATIC",
			L"x",
			WS_VISIBLE | WS_CHILD | SS_NOTIFY | SS_CENTER,
			popup.Width - 20,
			0,
			12,
			20,
			hWnd,
			(HMENU)CLOSE_POPUP,
			GetModuleHandle(nullptr),
			nullptr);
		SendMessage(closeHwnd, WM_SETFONT, (LPARAM)hFont, TRUE);
		SetClassLongPtr(closeHwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(nullptr, IDC_HAND));

		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case CLOSE_POPUP:
		{
			isPopup = false;
			SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetTimer(windowHwnd, ID_TIMER, 1000 / 60, nullptr);
			ShowWindow(hWnd, SW_HIDE);
			break;
		}
		default:
			break;
		}
		break;
	}
	case WM_CTLCOLORSTATIC:
	{
		SetTextColor((HDC)wParam, 0xffffff);
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (LRESULT)popupBackColor;
	}
	case WM_SETFOCUS:
		SetFocus(contentHwnd);
		break;
	case WM_DESTROY:
	{
		RemoveWindowSubclass(contentHwnd, subclassEdit, 0);
		RemoveWindowSubclass(buttonHwnd, subclassButton, 0);
		DeleteObject(hFont);
		DeleteObject(popupBackColor);
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool RegisterWndClass(std::wstring className, WNDPROC wndProc)
{
	WNDCLASS wincl{};
	wincl.hInstance = GetModuleHandle(nullptr);
	wincl.lpszClassName = className.c_str();
	wincl.lpfnWndProc = wndProc;
	wincl.style = CS_CLASSDC;
	wincl.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wincl.lpszMenuName = nullptr;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = nullptr;

	return RegisterClass(&wincl);
}
