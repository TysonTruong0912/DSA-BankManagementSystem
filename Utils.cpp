#include "Utils.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace std;

// Lay thoi gian hien tai (an toan luong tren Windows va Linux)
static std::tm getLocalTime()
{
    time_t now = time(nullptr);
    std::tm result;
#ifdef _WIN32
    localtime_s(&result, &now);
#else
    localtime_r(&now, &result);
#endif
    return result;
}

string getCurrentDate()
{
    std::tm t = getLocalTime();
    ostringstream oss;
    oss << setfill('0')
        << setw(2) << t.tm_mday << "/"
        << setw(2) << (t.tm_mon + 1) << "/"
        << (t.tm_year + 1900);
    return oss.str();
}

string getCurrentDateTime()
{
    std::tm t = getLocalTime();
    ostringstream oss;
    oss << getCurrentDate() << " "
        << setfill('0')
        << setw(2) << t.tm_hour << ":"
        << setw(2) << t.tm_min << ":"
        << setw(2) << t.tm_sec;
    return oss.str();
}

string formatMoney(double value)
{
    bool negative = value < 0;
    if (negative) value = -value;

    double rounded = std::round(value * 100.0) / 100.0;
    ostringstream oss;
    oss << fixed << setprecision(2) << rounded;
    string s = oss.str();                       // vi du "10000000.00"

    size_t dot = s.find('.');
    string intPart = s.substr(0, dot);
    string fracPart = s.substr(dot);            // ".00"

    // Chen dau phay moi 3 chu so tu phai sang trai
    string out;
    int count = 0;
    for (int i = (int)intPart.size() - 1; i >= 0; --i)
    {
        out.insert(out.begin(), intPart[i]);
        ++count;
        if (count % 3 == 0 && i > 0)
            out.insert(out.begin(), ',');
    }

    if (fracPart != ".00")      // so nguyen thi khong in phan thap phan
        out += fracPart;

    return negative ? "-" + out : out;
}

string trim(const string& s)
{
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) ++start;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) --end;
    return s.substr(start, end - start);
}

bool isAllDigits(const string& s)
{
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!isdigit((unsigned char)s[i])) return false;
    return true;
}

string toLower(const string& s)
{
    string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = (char)tolower((unsigned char)r[i]);
    return r;
}

bool containsIgnoreCase(const string& hay, const string& needle)
{
    return toLower(hay).find(toLower(needle)) != string::npos;
}

double round2(double v)
{
    return std::round(v * 100.0) / 100.0;
}

int dateKey(const string& s)
{
    if (s.size() < 10 || s[2] != '/' || s[5] != '/') return 0;
    string d = s.substr(0, 2), m = s.substr(3, 2), y = s.substr(6, 4);
    if (!isAllDigits(d) || !isAllDigits(m) || !isAllDigits(y)) return 0;
    return atoi(y.c_str()) * 10000 + atoi(m.c_str()) * 100 + atoi(d.c_str());
}

bool parseDateInput(const string& s, int& key)
{
    if (s.size() != 10) return false;
    int k = dateKey(s);
    if (k == 0) return false;
    int y = k / 10000, m = (k / 100) % 100, d = k % 100;
    static const int dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (y < 1900 || y > 2200 || m < 1 || m > 12 || d < 1) return false;
    int maxDay = dim[m - 1];
    if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) maxDay = 29;
    if (d > maxDay) return false;
    key = k;
    return true;
}

int todayKey()
{
    return dateKey(getCurrentDate());
}

string getTimestampForFile()
{
    std::tm t = getLocalTime();
    ostringstream oss;
    oss << setfill('0') << (t.tm_year + 1900) << setw(2) << (t.tm_mon + 1) << setw(2) << t.tm_mday
        << "_" << setw(2) << t.tm_hour << setw(2) << t.tm_min << setw(2) << t.tm_sec;
    return oss.str();
}

string csvEscape(const string& s)
{
    if (s.find_first_of(",\"\n\r") == string::npos) return s;
    string out = "\"";
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '"') out += "\"\"";
        else out += s[i];
    }
    return out + "\"";
}
