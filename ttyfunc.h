// This file is part of fstt.
// Copyright (C) 2022, github.com/CubedProgrammer, owner of said account.

// fstt is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

// fstt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// You shouu have received a copy of the GNU General Public License along with fstt. If not, see <https://www.gnu.org/licenses/>. 

#ifndef Included_ttyfunc_h
#define Included_ttyfunc_h
#define TMPPATH "/tmp/fixed_size_terminal_terminal%u"
#define CACHEPATH "/tmp/fixed_size_terminal_terminal%u/cache"
#define IPIPEPATH "/tmp/fixed_size_terminal_terminal%u/inamed_pipes"
#define OPIPEPATH "/tmp/fixed_size_terminal_terminal%u/onamed_pipes"

int maketty(const char *name, const char *rstr, const char *cstr, const char *shell, unsigned *restrict ttynumptr, const char *log);
void list_tty(char l);

#endif
