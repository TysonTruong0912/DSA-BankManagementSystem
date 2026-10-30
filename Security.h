#ifndef SECURITY_H
#define SECURITY_H

#include <string>

// Bao mat PIN: khong luu PIN goc, chi luu salt + ma bam SHA-256 (nhieu vong lap).

std::string sha256Hex(const std::string& data);
std::string generateSalt();                                     // 16 ky tu hex ngau nhien
std::string hashPin(const std::string& pin, const std::string& salt);
bool isValidPin(const std::string& pin);                        // 4-6 chu so

#endif
