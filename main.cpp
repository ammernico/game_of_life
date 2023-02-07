#include <sys/ioctl.h>
#include <unistd.h>
#include <ncurses.h>

#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>
#include <thread>


class Pos {
    void max_win_size() {
        struct winsize ws;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);

        x = ws.ws_col;
        y = ws.ws_row;
    }
    public:
        int x{};
        int y{};

        Pos() {
            max_win_size();
        }

        Pos(int x_par, int y_par) {
            x = x_par;
            y = y_par;
        }
        ~Pos() = default;
};

class Window {
    int start_x{};
    int start_y{};
    int end_x{};
    int end_y{};
    WINDOW *win;

    public:
        Window() = default;
        Window(int sx, int sy, int ex, int ey) {
            start_x = sx;
            start_y = sy;
            end_x = ex;
            end_y = ey;

            win = newwin(ey, ex, sy, sx);
        }
        ~Window() = default;

        Pos get_size() const {
            int x = end_x - start_x;
            int y = end_y - start_y;
            return Pos{x, y};
        }

        void refresh() {
            wrefresh(win);
        }

       int get_key() {
            int key = wgetch(win);
            return key;
       }

       void print_center(const char text[]) {
           int text_len = strlen(text);
           mvwprintw(win, (end_y / 2), ((end_x / 2) - (text_len / 2)), "%s", text);
       }

       void print_box() {
           wclear(win);
           box(win, 0, 0);
       }

       void print_2d(const std::vector<std::vector<int>> &field) {
           for (int y = 0; y < field.size(); y++) {
               for (int x = 0; x < field[y].size(); x++) {
                   char e;
                   if (field[y][x] == 0) {
                       e = ' ';
                   } else {
                       e = 'X';
                   }
                   mvwprintw(win, y, x + 1, "%c", e);
               }
           }
       }

       void print_lower_right(const char text[]) {
           int text_len = strlen(text);
           int x = ((end_x - text_len) - 1);
           int y = (end_y - 1);
           mvwprintw(win, y, x, "%s", text);
       }
};

class Screen {
    public:
        Pos screen_size = Pos{};
        // window covering the complete size of the terminal
        Window global_win;
        // window inside the outer box
        Window box_win;

        Screen() {
            global_win = Window(0, 0, screen_size.x, screen_size.y);
            box_win = Window(1, 1, (screen_size.x - 2), (screen_size.y - 2));

            refresh();
        }
        ~Screen() = default;
};

void generate_rand_start(std::vector<std::vector<int>> &big_bang) {
    srand(time(nullptr));

    for (int y = 0; y < big_bang.size(); y++) {
        for (int x = 0; x < big_bang[y].size(); x++) {
            // generate an random integer
            int random = rand() % 2;
            big_bang[y][x] = random;
        }
    }
}

int count_living_neighbours(std::vector<std::vector<int>> &matrix, Pos pos, Pos size) {
    int count = 0;
    for (int y = (pos.y - 1); y <= (pos.y + 1); y++) {
        for (int x = (pos.x - 1); x <= (pos.x + 1); x++) {
            // check if out of bounds
            if ((y < 0 || x < 0) || (y >= size.y || x >= size.x)) {
                continue;
            }
            // don't check the element itself
            if (x == pos.x && y == pos.y) {
                continue;
            }
            if (matrix[y][x] == 1) {
                count++;
            }
        }
    }
    return count;
}

void next_generation(std::vector<std::vector<int>> &old, std::vector<std::vector<int>> &next, Pos size) {
    for (int y = 0; y < old.size(); y++) {
        for (int x = 0; x < old[y].size(); x++) {
            Pos pos = Pos{x, y};
            int alive_neighbours = count_living_neighbours(old, pos, size);

            /* Rules
               1. Any live cell with fewer than two live neighbours dies, as if by underpopulation.
               2. Any live cell with two or three live neighbours lives on to the next generation.
               3. Any live cell with more than three live neighbours dies, as if by overpopulation.
               4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
             */

            if (old[y][x] == 1) {
                if (alive_neighbours == 2 || alive_neighbours == 3) {
                    next[y][x] = 1;
                } else {
                    next[y][x] = 0;
                }
            } else {
                if (alive_neighbours == 3) {
                    next[y][x] = 1;
                } else {
                    next[y][x] = 0;
                }
            }
        }
    }
}

int main() {
    // create the 'display'
    initscr();

    // has to be set to interrupt
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    raw();
    noecho();

    // get the dimensions of the terminal window
    Screen screen = Screen{};
    // print the box
    screen.global_win.print_box();
    screen.global_win.refresh();

    // get the size of the inner terminal
    Pos game_field_size{};
    {
        Pos size = screen.box_win.get_size();
        int x = size.x - 1;  // 232
        int y = size.y + 1;  // 47
        game_field_size = Pos{x, y};
    }

    std::vector<std::vector<int>> current(game_field_size.y, std::vector<int>(game_field_size.x, 0));
    std::vector<std::vector<int>> next(game_field_size.y, std::vector<int>(game_field_size.x, 0));

    generate_rand_start(current);
    int life_cycles = 0;
    int delay = 1000;

    int running = 1;
    while (running) {
        int key = getch();
        if (key != ERR) {
            // key is pressed
            switch(key) {
                case KEY_UP:
                    generate_rand_start(current);
                    life_cycles = 0;
                    screen.global_win.print_box();
                    screen.global_win.refresh();
                    break;
                case KEY_LEFT:
                    if (delay < 10000) {
                        delay += 100;
                    }
                    break;
                case KEY_RIGHT:
                    if (delay > 200) {
                        delay -= 100;
                    }
                    break;
                default:
                   running = 0;
            }
       } else {
            // print the life cycle count
            std::string count = std::to_string(life_cycles);
            char const *count_chars = count.c_str();
            screen.global_win.print_lower_right(count_chars);
            screen.global_win.refresh();

            // print the game
            screen.box_win.print_2d(current);

            // generate the next generation
            next_generation(current, next, game_field_size);
            current.swap(next);

            life_cycles++;
            screen.box_win.refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }
    }
    // restores the terminal
    endwin();
    return 0;
}
