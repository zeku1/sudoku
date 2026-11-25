# Terminal Sudoku

A small C++20 Sudoku solver with manual play. One source file, no external libraries. Windows uses native console APIs; Linux uses POSIX terminal input and ANSI screen/mouse sequences.

## Build and run on Linux

Copy `main.cpp` to your Linux machine and open a terminal in that folder. Compile the source on Linux; the Windows `.exe` and `.obj` files are not needed.

```bash
g++ -std=c++20 -Wall -Wextra -pedantic -O2 main.cpp -o sudoku
./sudoku --self-test
./sudoku
```

If `g++` is missing, install your distribution's C++ compiler. On Ubuntu or Debian:

```bash
sudo apt update
sudo apt install g++
```

Use a terminal emulator with ANSI and SGR mouse support, such as an xterm-compatible terminal, with at least 76 columns and 24 rows. Arrow keys also work when the terminal does not forward mouse clicks. Run `./sudoku --text` for a basic typed-command interface, including on a plain Linux virtual console or an online runner without an interactive terminal.

The Linux backend uses [POSIX terminal settings](https://man7.org/linux/man-pages/man3/termios.3.html) for immediate key input and [xterm mouse sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.html) for clicks. It restores terminal input settings on normal exit, including Q or Ctrl+C. Escape waits briefly to distinguish it from arrow-key sequences.

## Click or use arrow keys (Windows and Linux)

Run `.\sudoku.exe` on Windows or `./sudoku` on Linux in a terminal with at least 76 columns and 24 rows. Choose from nine premade puzzles (three each under Easy, Medium, and Hard), or select Custom puzzle. Use Up/Down and Enter, or click a menu item. Difficulty labels are approximate; every built-in puzzle has one solution, verified by the self-test.

Click a board cell or use arrow keys to select it, then press a number immediately:

| Key | Action |
| --- | --- |
| `1-9` | Enter a number |
| `0`, Backspace, Delete | Clear the selected player entry |
| `S` | Solve the current board |
| `R` | Reset the puzzle |
| Escape | Return to the menu and discard progress |
| `Q` or Ctrl+C | Quit |

The selected cell is highlighted. Original clues are white; player entries are cyan. Invalid moves show a message without changing the board. Arrow movement stops at the edges. Custom input accepts nine digits per row followed by Enter; use Backspace to correct a row.

Console settings are restored on normal exit. Mouse selection requires a terminal that forwards mouse events. Use `--text` if your terminal does not support these controls.

## Build and run on Windows

With GCC in PowerShell:

```powershell
g++ -std=c++20 -Wall -Wextra -pedantic -O2 main.cpp -o sudoku.exe
.\sudoku.exe --self-test
.\sudoku.exe
```

With MSVC in Developer PowerShell:

```powershell
cl /std:c++20 /W4 /EHsc main.cpp /Fe:sudoku.exe
.\sudoku.exe --self-test
.\sudoku.exe
```

Build without `-DNDEBUG` or `/DNDEBUG` for the assert-based self-test. The POSIX backend may also build on macOS, but macOS has not been verified.

## Text mode / redirected input

Run `.\sudoku.exe --text` on Windows or `./sudoku --text` on Linux for the original typed-command interface. This mode is also selected automatically when interactive terminal initialization fails or input/output is redirected; it starts with custom puzzle entry rather than the premade menu. On Linux, an unset `TERM` or `TERM=dumb` also selects text mode.

Enter nine rows of exactly nine digits without spaces. Use `0` for empty cells. Invalid or conflicting rows are rejected so you can retry that row.

```text
530070000
600195000
098000060
800060003
400803001
700020006
060000280
000419005
000080079
```

## Commands

| Command | Action |
| --- | --- |
| `set 1 3 4` | Put 4 in row 1, column 3 |
| `clear 1 3` | Erase that player entry |
| `solve` | Solve the current board, including player entries |
| `reset` | Restore the original puzzle |
| `help` | Show commands |
| `quit` | Exit |

Coordinates and values are 1-9. Original clues cannot change. Invalid commands and conflicting moves leave the board unchanged. Completion is announced automatically; reset and quit remain available afterward.

If solving fails, the board stays unchanged. Clear player entries or reset and try again. A locally legal move can still make a puzzle unsolvable.

Recursive backtracking returns the first solution without checking uniqueness during play and may be slow on difficult puzzles. No generation, hints, animation, or saving. Quitting, returning to the menu, or ending text input discards progress.
