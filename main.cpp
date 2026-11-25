#include <array>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <cstdlib>
#include <cerrno>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#endif

using Board = std::array<std::array<int, 9>, 9>;

struct Puzzle { const char* name; const char* digits; };
const Puzzle puzzles[] = {
    {"Easy 1", "530070000600195000098000060800060003400803001700020006060000280000419005000080079"},
    {"Easy 2", "020810740700003100090002805009040087400208003160030200302700060005600008076051090"},
    {"Easy 3", "480006902002008001900370060840010200003704100001060049020085007700900600609200018"},
    {"Medium 1", "003020600900305001001806400008102900700000008006708200002609500800203009005010300"},
    {"Medium 2", "030050040008010500460000012070502080000603000040109030250000098001020600080060020"},
    {"Medium 3", "000900002050123400030000160908000000070000090000000205091000050007439020400007000"},
    {"Hard 1", "000000907000420180000705026100904000050000040000507009920108000034059000507000000"},
    {"Hard 2", "100920000524010000000000070050008102000000000402700090060000000000030945000071006"},
    {"Hard 3", "043080250600000000000001094900004070000608000010200003820500000000000005034090710"},
};

Board puzzleBoard(int index) {
    Board board{};
    for (int i = 0; i < 81; ++i) board[i / 9][i % 9] = puzzles[index].digits[i] - '0';
    return board;
}

bool allowed(const Board& board, int row, int col, int value) {
    for (int i = 0; i < 9; ++i) {
        if (i != col && board[row][i] == value) return false;
        if (i != row && board[i][col] == value) return false;
    }
    for (int r = row / 3 * 3; r < row / 3 * 3 + 3; ++r)
        for (int c = col / 3 * 3; c < col / 3 * 3 + 3; ++c)
            if ((r != row || c != col) && board[r][c] == value) return false;
    return true;
}

bool valid(const Board& board) {
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (board[r][c] < 0 || board[r][c] > 9 ||
                (board[r][c] && !allowed(board, r, c, board[r][c]))) return false;
    return true;
}

bool complete(const Board& board) {
    for (const auto& row : board)
        for (int value : row)
            if (value == 0) return false;
    return valid(board);
}

std::string move(Board& board, const Board& original, int row, int col, int value) {
    if (row < 0 || row > 8 || col < 0 || col > 8 || value < 0 || value > 9)
        return "Numbers must be 1-9.";
    if (original[row][col]) return "Original clues cannot be changed.";
    if (value && !allowed(board, row, col, value)) return "That move conflicts with the board.";
    board[row][col] = value;
    return "";
}

int shifted(int position, int delta) {
    int next = position + delta;
    return next < 0 ? 0 : next > 8 ? 8 : next;
}

// Screen coordinates shared by drawing and hit testing.
int cellX(int col) { return 4 + col * 4 + col / 3 * 2; }
int cellY(int row) { return 4 + row + row / 3; }
std::pair<int, int> hitCell(int x, int y) {
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (y == cellY(r) && x >= cellX(c) && x < cellX(c) + 3) return {r, c};
    return {-1, -1};
}

// shortcut: returns the first solution, add solution counting if uniqueness is needed.
bool solve(Board& board) {
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            if (board[r][c] != 0) continue;
            for (int value = 1; value <= 9; ++value) {
                if (!allowed(board, r, c, value)) continue;
                board[r][c] = value;
                if (solve(board)) return true;
                board[r][c] = 0;
            }
            return false;
        }
    }
    return valid(board);
}

void print(const Board& board, std::ostream& out) {
    out << "    1 2 3   4 5 6   7 8 9\n";
    for (int r = 0; r < 9; ++r) {
        if (r % 3 == 0) out << "  +-------+-------+-------+\n";
        out << r + 1 << ' ';
        for (int c = 0; c < 9; ++c) {
            if (c % 3 == 0) out << "| ";
            out << (board[r][c] ? char('0' + board[r][c]) : '.') << ' ';
        }
        out << "|\n";
    }
    out << "  +-------+-------+-------+\n";
    if (complete(board)) out << "Puzzle complete!\n";
}

bool readBoard(std::istream& in, std::ostream& out, Board& board) {
    out << "Enter 9 rows of 9 digits. Use 0 for empty cells.\n";
    Board input{};
    for (int r = 0; r < 9;) {
        out << "Row " << r + 1 << ": ";
        std::string line;
        if (!std::getline(in, line)) return false;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() != 9 || line.find_first_not_of("0123456789") != std::string::npos) {
            out << "Enter exactly 9 digits (0-9), without spaces.\n";
            continue;
        }
        for (int c = 0; c < 9; ++c) input[r][c] = line[c] - '0';
        if (!valid(input)) {
            input[r].fill(0);
            out << "Conflicting clues. Enter this row again.\n";
            continue;
        }
        ++r;
    }
    board = input;
    return true;
}

bool command(const std::string& line, Board& board, const Board& original, std::ostream& out) {
    std::istringstream input(line);
    std::string name, extra;
    input >> name;
    if (name == "set" || name == "clear") {
        int row = 0, col = 0, value = 0;
        if (!(input >> row >> col) || (name == "set" && !(input >> value)) ||
            (input >> extra) || row < 1 || row > 9 || col < 1 || col > 9 ||
            (name == "set" && (value < 1 || value > 9))) {
            out << "Use set row col value or clear row col; numbers must be 1-9.\n";
            return true;
        }
        --row;
        --col;
        std::string error = move(board, original, row, col, value);
        if (!error.empty()) out << error << '\n';
        else print(board, out);
        return true;
    }
    if (input >> extra) {
        out << "Unexpected arguments. Type help for commands.\n";
        return true;
    }
    if (name == "quit") return false;
    if (name == "help") {
        out << "set row col value | clear row col | solve | reset | help | quit\n"
            << "Coordinates and values are 1-9. Solve includes your current entries.\n";
    } else if (name == "reset") {
        board = original;
        print(board, out);
    } else if (name == "solve") {
        Board solution = board;
        if (solve(solution)) {
            board = solution;
            print(board, out);
        } else out << "No solution for the current board. Clear entries or reset to try again.\n";
    } else out << "Unknown command. Type help for commands.\n";
    return true;
}

enum Key { None = 0, Backspace = 8, Enter = 13, EscapeKey = 27,
           Up = 256, Down, Left, Right, Delete, Click, InputError };
struct Input { int code = None, x = 0, y = 0; };
struct Pixel { char ch = ' '; int color = 7; };

Input decodeInput(const std::string& bytes) {
    if (bytes.size() == 1) {
        unsigned char ch = static_cast<unsigned char>(bytes[0]);
        if (ch == 3 || ch == 4) return {'Q'};
        if (ch == 127) return {Backspace};
        if (ch == '\n') return {Enter};
        return {ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch};
    }
    if (bytes == "\x1b[A" || bytes == "\x1bOA") return {Up};
    if (bytes == "\x1b[B" || bytes == "\x1bOB") return {Down};
    if (bytes == "\x1b[C" || bytes == "\x1bOC") return {Right};
    if (bytes == "\x1b[D" || bytes == "\x1bOD") return {Left};
    if (bytes == "\x1b[3~") return {Delete};
    if (bytes.rfind("\x1b[<", 0) == 0) {
        std::istringstream in(bytes.substr(3));
        int button, x, y;
        char a, b, end, extra;
        if ((in >> button >> a >> x >> b >> y >> end) && !(in >> extra) &&
            a == ';' && b == ';' && end == 'M' && button == 0 && x > 0 && y > 0)
            return {Click, x - 1, y - 1};
    }
    return {};
}

#ifndef _WIN32
int readByte(int timeout) {
    pollfd fd{STDIN_FILENO, POLLIN, 0};
    int ready;
    do { ready = poll(&fd, 1, timeout); } while (ready < 0 && errno == EINTR);
    if (ready == 0) return -1;
    if (ready < 0 || !(fd.revents & POLLIN)) return -2;
    unsigned char ch;
    ssize_t count;
    do { count = read(STDIN_FILENO, &ch, 1); } while (count < 0 && errno == EINTR);
    return count == 1 ? ch : -2;
}

Input terminalInput() {
    int ch = readByte(100);
    if (ch == -2) return {InputError};
    if (ch == -1) return {};
    std::string bytes(1, char(ch));
    if (ch != EscapeKey) return decodeInput(bytes);
    // shortcut: 100 ms separates EscapeKey from key sequences; increase for slow remote terminals.
    ch = readByte(100);
    if (ch == -2) return {InputError};
    if (ch == -1) return {EscapeKey};
    bytes += char(ch);
    if (ch != '[' && ch != 'O') return {};
    while (bytes.size() < 64) {
        ch = readByte(100);
        if (ch == -2) return {InputError};
        if (ch == -1) return {};
        bytes += char(ch);
        if (ch >= 0x40 && ch <= 0x7e) return decodeInput(bytes);
    }
    return {};
}
#endif

bool interactive() {
#ifdef _WIN32
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE), out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD inputMode;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleMode(in, &inputMode) || !GetConsoleScreenBufferInfo(out, &info)) return false;
    if (!SetConsoleMode(in, (inputMode | ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT |
        ENABLE_EXTENDED_FLAGS) & ~(ENABLE_QUICK_EDIT_MODE | ENABLE_LINE_INPUT |
        ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT))) return false;
    CONSOLE_CURSOR_INFO cursor{};
    GetConsoleCursorInfo(out, &cursor);
    auto hidden = cursor;
    hidden.bVisible = FALSE;
    SetConsoleCursorInfo(out, &hidden);
#else
    const char* term = std::getenv("TERM");
    termios saved{};
    winsize size{};
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) || !term ||
        std::string(term) == "dumb" || tcgetattr(STDIN_FILENO, &saved) != 0 ||
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) return false;
    termios raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return false;
    std::cout << "\x1b[?1049h\x1b[?25l\x1b[?1000h\x1b[?1006h\x1b[2J" << std::flush;
#endif

    constexpr int width = 76, height = 24;
    int selection = 0, row = 0, col = 0, customRow = 0;
    enum { Menu, Game, Custom } screen = Menu;
    Board board{}, original{};
    std::string title, status, digits;
    bool running = true, failed = false;
    while (running) {
#ifdef _WIN32
        if (!GetConsoleScreenBufferInfo(out, &info)) { failed = true; break; }
        bool fits = info.srWindow.Right - info.srWindow.Left + 1 >= width &&
                    info.srWindow.Bottom - info.srWindow.Top + 1 >= height;
#else
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0) { failed = true; break; }
        bool fits = size.ws_col >= width && size.ws_row >= height;
#endif
        std::array<Pixel, width * height> frame{};
        auto text = [&](int x, int y, const std::string& value, int color = 7) {
            for (char ch : value) {
                if (x >= 0 && x < width && y >= 0 && y < height) {
                    frame[y * width + x].ch = ch;
                    frame[y * width + x].color = color;
                }
                ++x;
            }
        };
        text(2, 0, "TERMINAL SUDOKU", 15);
        if (!fits) text(0, 1, "Resize terminal to at least 76 columns and 24 rows. Q quits.");
        else if (screen == Menu) {
            text(2, 2, "Choose a puzzle: arrows + Enter, or click. Q quits.");
            for (int i = 0; i < 10; ++i)
                text(2, 4 + i, std::to_string(i + 1) + ". " +
                    (i < 9 ? puzzles[i].name : "Custom puzzle"), i == selection ? 112 : 7);
            text(2, 16, "Three puzzles per difficulty. Difficulty labels are approximate.");
        } else if (screen == Custom) {
            text(2, 2, "Enter 9 digits per row (0 = empty), then Enter. Esc returns.");
            for (int r = 0; r < customRow; ++r) {
                std::string value;
                for (int n : board[r]) value += char('0' + n);
                text(4, 4 + r, value);
            }
            text(2, 4 + customRow, "> " + digits, 15);
            text(2, 17, status, 14);
        } else {
            text(2, 2, title + " - click a cell or use arrow keys", 15);
            for (int y : {3, 7, 11, 15}) text(2, y, "+-------------+-------------+-------------+");
            for (int r = 0; r < 9; ++r) {
                for (int x : {2, 16, 30, 44}) text(x, cellY(r), "|");
                for (int c = 0; c < 9; ++c) {
                    std::string value = " ";
                    value += board[r][c] ? char('0' + board[r][c]) : '.';
                    value += ' ';
                    text(cellX(c), cellY(r), value, r == row && c == col ? 112 :
                        original[r][c] ? 15 : 11);
                }
            }
            text(2, 17, complete(board) ? "Puzzle complete!" : status, 14);
            text(2, 19, "1-9: enter   0 / Backspace / Delete: clear");
            text(2, 20, "S: solve   R: reset   Esc: menu (discards progress)   Q: quit");
            text(2, 22, "Selected: row " + std::to_string(row + 1) + ", column " + std::to_string(col + 1));
        }
#ifdef _WIN32
        std::array<CHAR_INFO, width * height> output{};
        for (int i = 0; i < width * height; ++i) {
            output[i].Char.AsciiChar = frame[i].ch;
            output[i].Attributes = WORD(frame[i].color);
        }
        SMALL_RECT area{info.srWindow.Left, info.srWindow.Top,
            SHORT(info.srWindow.Left + width - 1), SHORT(info.srWindow.Top + height - 1)};
        if (!WriteConsoleOutputA(out, output.data(), {width, height}, {0, 0}, &area)) {
            failed = true; break;
        }
        INPUT_RECORD event{};
        DWORD count;
        if (!ReadConsoleInputW(in, &event, 1, &count)) { failed = true; break; }
        if (event.EventType == WINDOW_BUFFER_SIZE_EVENT) continue;
        Input action;
        if (event.EventType == MOUSE_EVENT) {
            const auto& mouse = event.Event.MouseEvent;
            if (mouse.dwEventFlags != 0 || !(mouse.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)) continue;
            action = {Click, mouse.dwMousePosition.X - info.srWindow.Left,
                mouse.dwMousePosition.Y - info.srWindow.Top};
        } else if (event.EventType == KEY_EVENT && event.Event.KeyEvent.bKeyDown) {
            const auto& key = event.Event.KeyEvent;
            switch (key.wVirtualKeyCode) {
                case VK_UP: action.code = Up; break;
                case VK_DOWN: action.code = Down; break;
                case VK_LEFT: action.code = Left; break;
                case VK_RIGHT: action.code = Right; break;
                case VK_DELETE: action.code = Delete; break;
                default:
                    if (key.uChar.UnicodeChar > 0 && key.uChar.UnicodeChar < 128)
                        action = decodeInput(std::string(1, char(key.uChar.UnicodeChar)));
            }
        }
#else
        std::string output = "\x1b[H";
        if (!fits) output += "\x1b[2JResize terminal to 76x24. Q quits.";
        else {
            int color = -1;
            for (int y = 0; y < height; ++y) {
                output += "\x1b[" + std::to_string(y + 1) + ";1H";
                for (int x = 0; x < width; ++x) {
                    const auto& pixel = frame[y * width + x];
                    if (pixel.color != color) {
                        color = pixel.color;
                        output += color == 112 ? "\x1b[0;30;47m" : color == 15 ? "\x1b[0;97m" :
                                  color == 11 ? "\x1b[0;96m" : color == 14 ? "\x1b[0;93m" : "\x1b[0;37m";
                    }
                    output += pixel.ch;
                }
            }
        }
        std::cout << output << "\x1b[0m" << std::flush;
        if (!std::cout) { failed = true; break; }
        Input action = terminalInput();
        if (action.code == InputError) { failed = true; break; }
#endif
        bool activate = false;
        int code = action.code;
        if (code == 'Q') { running = false; continue; }
        if (!fits) continue;
        if (code == Click) {
            int x = action.x, y = action.y;
            if (screen == Menu && x >= 2 && x < 24 && y >= 4 && y < 14) {
                selection = y - 4;
                activate = true;
            } else if (screen == Game) {
                auto cell = hitCell(x, y);
                if (cell.first >= 0) { row = cell.first; col = cell.second; }
            }
        } else {
            if (code == EscapeKey) { screen = Menu; status.clear(); continue; }
            if (screen == Menu) {
                if (code == Up && selection > 0) --selection;
                if (code == Down && selection < 9) ++selection;
                activate = code == Enter;
            } else if (screen == Custom) {
                if (code >= '0' && code <= '9' && digits.size() < 9) digits += char(code);
                if (code == Backspace && !digits.empty()) digits.pop_back();
                if (code == Enter) {
                    status = "Enter exactly 9 digits.";
                    if (digits.size() == 9) {
                        for (int c = 0; c < 9; ++c) board[customRow][c] = digits[c] - '0';
                        if (!valid(board)) {
                            board[customRow].fill(0);
                            status = "Conflicting clues. Backspace to correct this row.";
                        } else {
                            ++customRow; digits.clear(); status.clear();
                            if (customRow == 9) { original = board; screen = Game; title = "Custom puzzle"; }
                        }
                    }
                }
            } else {
                if (code == Up) row = shifted(row, -1);
                if (code == Down) row = shifted(row, 1);
                if (code == Left) col = shifted(col, -1);
                if (code == Right) col = shifted(col, 1);
                if (code >= '0' && code <= '9') status = move(board, original, row, col, code - '0');
                if (code == Backspace || code == Delete) status = move(board, original, row, col, 0);
                if (code == 'R') { board = original; status = "Puzzle reset."; }
                if (code == 'S') {
                    Board result = board;
                    if (solve(result)) { board = result; status.clear(); }
                    else status = "No solution. Clear entries or reset.";
                }
            }
        }
        if (activate && screen == Menu) {
            row = col = 0; status.clear();
            if (selection < 9) {
                board = original = puzzleBoard(selection);
                title = puzzles[selection].name;
                screen = Game;
            } else {
                board = {}; customRow = 0; digits.clear(); screen = Custom;
            }
        }
    }
#ifdef _WIN32
    SetConsoleMode(in, inputMode);
    SetConsoleCursorInfo(out, &cursor);
    SetConsoleCursorPosition(out, {info.srWindow.Left, SHORT(info.srWindow.Top + 23 < info.dwSize.Y ? info.srWindow.Top + 23 : info.dwSize.Y - 1)});
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    std::cout << "\x1b[?1006l\x1b[?1000l\x1b[0m\x1b[?25h\x1b[?1049l" << std::flush;
#endif
    std::cout << (failed ? "\nConsole input/output failed. Restart with --text.\n" : "\n");
    return true;
}

// Test-only search stops at two; gameplay still returns the first solution.
int solutionCount(Board& board) {
    for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) {
        if (board[r][c]) continue;
        int total = 0;
        for (int value = 1; value <= 9 && total < 2; ++value) {
            if (!allowed(board, r, c, value)) continue;
            board[r][c] = value;
            total += solutionCount(board);
            board[r][c] = 0;
        }
        return total > 1 ? 2 : total;
    }
    return valid(board) ? 1 : 0;
}

void selfTest() {
    assert(decodeInput("\x1b[A").code == Up && decodeInput("\x1bOB").code == Down);
    assert(decodeInput("\x1b[C").code == Right && decodeInput("\x1b[D").code == Left);
    assert(decodeInput("\x1b").code == EscapeKey && decodeInput("\x1b[3~").code == Delete);
    assert(decodeInput("\x7f").code == Backspace && decodeInput("\n").code == Enter);
    assert(decodeInput("q").code == 'Q' && decodeInput("\x03").code == 'Q');
    Input click = decodeInput("\x1b[<0;20;5M");
    assert(click.code == Click && click.x == 19 && click.y == 4);
    for (const auto* bytes : {"\x1b[<0;20;5m", "\x1b[<1;20;5M", "\x1b[<0;0;5M",
                             "\x1b[<0;999999999999999;5M", "\x1b[<bad", "\x1b[99~"})
        assert(decodeInput(bytes).code == None);
    for (int i = 0; i < 9; ++i) {
        Board board = puzzleBoard(i), original = board;
        assert(valid(board));
        int count = solutionCount(board);
        assert(count == 1 && board == original);
        bool solved = solve(board);
        assert(solved && complete(board));
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c)
            assert(!original[r][c] || original[r][c] == board[r][c]);
    }
    assert(shifted(0, -1) == 0 && shifted(8, 1) == 8 && shifted(4, 1) == 5);
    for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c) {
        for (int dx = 0; dx < 3; ++dx)
            assert((hitCell(cellX(c) + dx, cellY(r)) == std::pair{r, c}));
    }
    assert(hitCell(2, 4).first == -1 && hitCell(4, 7).first == -1 && hitCell(75, 23).first == -1);
    const std::string puzzle =
        "530070000\n600195000\n098000060\n800060003\n400803001\n"
        "700020006\n060000280\n000419005\n000080079\n";
    std::ostringstream out;
    std::istringstream input(puzzle);
    Board original{};
    bool read = readBoard(input, out, original);
    assert(read && valid(original));
    Board solved = original;
    bool found = solve(solved);
    assert(found && complete(solved));
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            assert(!original[r][c] || original[r][c] == solved[r][c]);
    Board copy = solved;
    found = solve(copy);
    assert(found && copy == solved);
    for (auto cell : {std::pair{0, 2}, std::pair{2, 0}, std::pair{1, 1}}) {
        copy = original;
        copy[cell.first][cell.second] = 5;
        assert(!valid(copy));
    }
    std::istringstream malformed("abc\n123\n1234567890\n550000000\n" + puzzle);
    read = readBoard(malformed, out, copy);
    assert(read && copy == original);
    std::istringstream eof("530070000\n");
    read = readBoard(eof, out, copy);
    assert(!read && copy == original);
    Board board = original;
    for (const auto* line : {"set 1 1 4", "clear 1 1", "set 1 3 5", "set 0 3 4",
                             "set 1 3 10", "set 1 x 4", "set 1 3 4 extra", "clear 1",
                             "set 999999999999999 3 4", "reset extra", "quit extra", ""}) {
        bool running = command(line, board, original, out);
        assert(running && board == original);
    }
    command("set 1 3 4", board, original, out);
    assert(board[0][2] == 4);
    command("clear 1 3", board, original, out);
    assert(board == original);
    command("set 1 3 4", board, original, out);
    command("reset", board, original, out);
    assert(board == original);
    command("solve", board, original, out);
    assert(board == solved);
    Board impossible{};
    impossible[0] = {1, 2, 3, 4, 5, 6, 7, 8, 0};
    impossible[1][8] = 9;
    assert(valid(impossible));
    copy = impossible;
    found = solve(copy);
    assert(!found && copy == impossible);
    command("solve", copy, impossible, out);
    assert(copy == impossible);
    bool running = command("quit", board, original, out);
    assert(!running);
    std::cout << "All self-tests passed.\n";
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--self-test") {
#ifdef NDEBUG
        std::cerr << "Rebuild without -DNDEBUG to enable self-test assertions.\n";
        return 1;
#else
        selfTest();
        return 0;
#endif
    }
    if (argc != 1 && !(argc == 2 && std::string(argv[1]) == "--text")) {
        std::cerr << "Usage: sudoku [--text | --self-test]\n";
        return 1;
    }
    if (argc == 1 && interactive()) return 0;
    Board board{};
    if (!readBoard(std::cin, std::cout, board)) return 0;
    const Board original = board;
    print(board, std::cout);
    command("help", board, original, std::cout);
    std::string line;
    while (std::cout << "sudoku> " && std::getline(std::cin, line))
        if (!command(line, board, original, std::cout)) break;
}

