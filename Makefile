# Cach dung:
#   make                 tu dong chon: SQLite he thong -> sqlite3.c cung thu muc -> khong SQLite (file .txt)
#   make SQLITE=system   ep dung SQLite cai san (libsqlite3-dev / brew install sqlite)
#   make SQLITE=amalgam  ep bien dich sqlite3.c (tai tu https://sqlite.org/download.html, ban "amalgamation")
#   make SQLITE=none     bien dich KHONG co SQLite -> chay che do file .txt nhu ban cu
#   make clean | make run

CXX      ?= g++
CC       ?= gcc
CXXFLAGS ?= -std=c++11 -Wall -Wextra -pedantic -O2
SQLITE   ?= auto

ifeq ($(SQLITE),auto)
  ifneq ($(wildcard sqlite3.c),)
    SQLITE := amalgam
  else ifneq ($(or $(wildcard /usr/include/sqlite3.h),$(wildcard /usr/local/include/sqlite3.h),$(wildcard /opt/homebrew/include/sqlite3.h),$(wildcard /mingw64/include/sqlite3.h),$(wildcard /ucrt64/include/sqlite3.h)),)
    SQLITE := system
  else
    SQLITE := none
  endif
endif

SRC := $(filter-out sqlite3.c,$(wildcard *.cpp))
OBJ := $(SRC:.cpp=.o)
LIBS :=
DEFS :=
EXTRA_OBJ :=

ifeq ($(SQLITE),system)
  DEFS := -DUSE_SQLITE
  LIBS := -lsqlite3
  ifneq ($(wildcard /opt/homebrew/include/sqlite3.h),)
    CXXFLAGS += -I/opt/homebrew/include
    LIBS := -L/opt/homebrew/lib -lsqlite3
  endif
endif
ifeq ($(SQLITE),amalgam)
  DEFS := -DUSE_SQLITE -I.
  EXTRA_OBJ := sqlite3.o
  ifneq ($(OS),Windows_NT)
    LIBS := -lpthread
  endif
endif

bank: $(OBJ) $(EXTRA_OBJ)
	$(CXX) $(OBJ) $(EXTRA_OBJ) -o bank $(LIBS)
	@echo "Da bien dich xong (che do SQLite = $(SQLITE))"

%.o: %.cpp $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(DEFS) -c $< -o $@

sqlite3.o: sqlite3.c
	$(CC) -O2 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o sqlite3.o

run: bank
	./bank

clean:
	rm -f bank bank.exe *.o

.PHONY: run clean
