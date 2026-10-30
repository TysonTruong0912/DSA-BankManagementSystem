#ifndef UTILS_H
#define UTILS_H

#include <string>

// Cac ham tien ich dung chung cho ca chuong trinh

std::string getCurrentDateTime();          // dd/mm/yyyy hh:mm:ss
std::string getCurrentDate();              // dd/mm/yyyy
std::string formatMoney(double value);     // 10000000 -> "10,000,000"
std::string trim(const std::string& s);    // bo khoang trang dau/cuoi
bool isAllDigits(const std::string& s);    // chuoi khong rong, chi gom chu so

std::string toLower(const std::string& s);                          // ASCII, de tim kiem khong phan biet hoa thuong
bool containsIgnoreCase(const std::string& hay, const std::string& needle);
double round2(double v);                                            // lam tron 2 chu so thap phan
int dateKey(const std::string& s);                                  // "dd/mm/yyyy ..." -> yyyymmdd (0 neu sai)
bool parseDateInput(const std::string& s, int& key);                // chi nhan dung "dd/mm/yyyy" hop le
int todayKey();                                                     // hom nay dang yyyymmdd
std::string getTimestampForFile();                                  // yyyymmdd_hhmmss
std::string csvEscape(const std::string& s);                        // boc "..." neu can

#endif
