#include "UI.h"
#include "UI_Internal.h"
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#endif
using namespace std;
using namespace UI::Internal;

namespace UI {
    int selectMenu(const string& title, const vector<string>& options, int defaultIndex)
    {
        if (options.empty()) return -1;
        int selected = defaultIndex;
        if (selected < 0) selected = 0;
        if (selected >= (int)options.size()) selected = (int)options.size() - 1;

        // Pipe / test tu dong: nhap so (1-based), 0 = huy -> -1
        if (!isInteractive())
        {
            drawMenuBox(title, options);
            cout << "  " << FG << "> " << RESET << FG_TEXT << "Nhap so (1-" << options.size() << "): " << RESET;
            string line;
            if (!getline(cin, line)) return -1;
            // trim
            size_t a = 0, b = line.size();
            while (a < b && isspace((unsigned char)line[a])) ++a;
            while (b > a && isspace((unsigned char)line[b - 1])) --b;
            line = line.substr(a, b - a);
            if (line.empty()) return selected;
            int n = atoi(line.c_str());
            if (n >= 1 && n <= (int)options.size()) return n - 1;
            return -1;
        }

        while (true)
        {
            // ve lai menu (khong clear toan man de giu header phia tren neu co)
            // Dung cuon lenh: in lai block menu
            paintVerticalMenu(title, options, selected);

            int key = readKey();
            if (key < 0) return selected;

            if (key == 1000) // UP
            {
                selected = (selected - 1 + (int)options.size()) % (int)options.size();
            }
            else if (key == 1001) // DOWN
            {
                selected = (selected + 1) % (int)options.size();
            }
            else if (key == '\r' || key == '\n') // Enter
            {
                return selected;
            }
            else if (key >= '1' && key <= '9')
            {
                int n = key - '0';
                if (n >= 1 && n <= (int)options.size())
                    return n - 1;
            }
            else if (key == '0' && options.size() >= 10)
            {
                // khong map 0 -> index; user dung Enter
            }
            // xoa block menu cu: di chuyen con tro len
            int linesDrawn = (int)options.size() + 7;
            cout << "\033[" << linesDrawn << "A";
            cout << "\033[J";
        }
    }


}
