#include <windows.h>
#include <gdiplus.h>
#include <fstream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <vector>

const std::string noteFile = "note.txt";
std::unordered_map<size_t, std::vector<std::wstring>> allNoteContent;

struct Date {
	size_t date;				//yyyyMMdd

	Date(int year, int month, int day)
	{
		date = (size_t)(year * 10000 + month * 100 + day);
	}

	int GetYear() const
	{
		return date / 10000;
	}

	int GetMonth() const
	{
		return (date / 100) % 100;
	}

	int GetMonthDay() const
	{
		return date % 100;
	}

	int GetWeekDay() const
	{
		int year = GetYear(), month = GetMonth(), mday = GetMonthDay();
		int offsets[12] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
		int offset = offsets[month - 1];
		if (month > 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
			offset = (offset + 1) % 7;
		return (mday + offset + 5 * ((year - 1) % 4) + 4 * ((year - 1) % 100) + 6 * ((year - 1) % 400)) % 7;
	}
};


//https://stackoverflow.com/questions/6693010/how-do-i-use-multibytetowidechar
static std::wstring Utf8ToWstr(const std::string& utf8) {
	int count = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.length(), nullptr, 0);
	std::wstring wstr(count, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.length(), &wstr[0], count);
	return wstr;
}
static std::string WstrToUtf8(const std::wstring& utf16) {
	int count = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), utf16.length(), nullptr, 0, nullptr, nullptr);
	std::string str(count, 0);
	WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &str[0], count, nullptr, nullptr);
	return str;
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

	if (ofs.is_open()) {
		for (const auto& notes : allNoteContent) {
			ofs << notes.first << ":";

			for (const std::wstring& s : notes.second) {
				ofs << WstrToUtf8(s) << "\u200b";
			}

			ofs << "\n";
		}

		ofs.close();
	}
}

std::wstring ToMonthName(int month)
{
	static std::wstring list[12] = { L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September", L"October", L"November", L"December" };
	return list[month - 1];
}

int DaysInMonth(int year, int month)
{
	if ((month <= 7 && month & 1) || (month >= 8 && month % 2 == 0)) return 31;
	else if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
	if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) return 29;

	return 28;
}

Date Today()
{
	time_t t = std::time(0);
	struct tm now = *localtime(&t);
	Date today{ now.tm_year + 1900, now.tm_mon + 1, now.tm_mday };
	return today;
}