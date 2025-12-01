#include <SFML/Graphics.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <queue>
#include <string>
#include <vector>

using namespace std;
using namespace sf;

const int cell_size = 32;
const int high = 16;
const int width = 16;
const int total_mines = 40;

struct Cell {
    bool mine = false;
    bool revealed = false;
    bool flagged = false;
    int adj = 0;
};

vector<vector<Cell>> board;
bool started = false;
bool game_over = false;
bool win = false;
int revealed_count = 0;
int flags_left = total_mines;
int final_time = 0;
Clock game_clock;
Time paused_time = Time::Zero;

void init_board() {
    board.assign(high, vector<Cell>(width));
    started = false;
    game_over = false;
    win = false;
    revealed_count = 0;
    flags_left = total_mines;
    paused_time = Time::Zero;
    game_clock.restart();
}

bool in_bounds(int x, int y) {
    return x >= 0 && x < width && y >= 0 && y < high;
}

void place_mines(int safe_x, int safe_y) {
    vector<int> cells;
    for (int y = 0; y < high; y++) {
        for (int x = 0; x < width; x++) {
            cells.push_back(y * high + x);
        }
    }

    mt19937 rng((unsigned)chrono::system_clock::now().time_since_epoch().count());
    shuffle(cells.begin(), cells.end(), rng);
    int placed = min((int)sizeof(cells), total_mines);
    for (int i = 0; i < placed; i++) {
        int x = cells[i] % high;
        int y = cells[i] / high;
        board[y][x].mine = true;
    }

    for (int y = 0; y < high; y++) {
        for (int x = 0; x < width; x++) {
            if (board[y][x].mine) continue;
            int cnt = 0;
            for (int oy = -1; oy <= 1; ++oy) for (int ox = -1; ox <= 1; ++ox) {
                if (ox == 0 && oy == 0) continue;
                int nx = x + ox, ny = y + oy;
                if (in_bounds(nx, ny) && board[ny][nx].mine) cnt++;
            }
            board[y][x].adj = cnt;
        }
    }
}

void reveal_all_mines() {
    for (int y = 0; y < high; y++) {
        for (int x = 0; x < width; x++) {
            if (board[y][x].mine) board[y][x].revealed = true;
        }
    }
}

void check_win_condition() {
    int total_cells = high * width;
    if (revealed_count == total_cells - total_mines) {
        win = true;
        game_over = false;
        final_time = game_clock.getElapsedTime().asSeconds();
        for (int y = 0; y < high; y++)
            for (int x = 0; x < width; x++)
                board[y][x].revealed = true;
    }
    return;
}

void flood_fill_reveal(int sx, int sy) {
    queue<pair<int, int> > q;
    q.push({sx, sy});
    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();
        for (int oy = -1; oy <= 1; oy++)
            for (int ox = -1; ox <= 1; ox++) {
                int nx = x + ox, ny = y + oy;
                if (!in_bounds(nx, ny)) continue;
                Cell &c = board[ny][nx];
                if (!c.revealed && !c.flagged && !c.mine) {
                    c.revealed = true;
                    revealed_count++;
                    if(c.adj == 0) q.push({nx, ny});
                }
            }
    }
}

bool reveal_cell(int x, int y) {
    if (game_over || win) return false;
    if (!in_bounds(x, y)) return false;
    if (!started) {
        place_mines(x, y);
        started = true;
        game_clock.restart();
    }
    Cell &c = board[y][x];
    if (c.revealed || c.flagged) return false;
    c.revealed = true;
    revealed_count++;
    if (c.mine) {
        game_over = true;
        reveal_all_mines();
        final_time = game_clock.getElapsedTime().asSeconds();
        return true;
    }
    if (c.adj == 0) flood_fill_reveal(x, y);
    check_win_condition();
    return false;
}

void toggle_flag(int x, int y) {
    if (game_over || win) return;
    if (!in_bounds(x, y)) return;
    Cell &c = board[y][x];
    if (c.revealed) return;
    if (c.flagged) {
        c.flagged = false;
        flags_left++;
    } else {
        if (flags_left <= 0) return;
        c.flagged = true;
        flags_left--;
    }
    check_win_condition();
}

signed main() {
    init_board();

    int win_w = width * cell_size, win_h = high * cell_size + 48;
    RenderWindow window(VideoMode(win_w, win_h), "Gongbing");
    window.setFramerateLimit(60);

    Font font;
    if (!font.loadFromFile("DejaVuSans.ttf")) {
        if (!font.loadFromFile("arial.ttf")) {
            printf("Error: could not load DejaVuSans.ttf or arial.ttf\n");
            return 0;
        }
    }


    RectangleShape cellShape(Vector2f((float)cell_size - 1.f, (float)cell_size - 1.f));
    Text text("", font, 18);
    text.setStyle(Text::Bold);

    Text infoText("", font, 18);
    infoText.setPosition(6.f, high * cell_size + 6.f);

    while(window.isOpen()) {
        Event ev;
        while(window.pollEvent(ev)) {
            if (ev.type == Event::Closed) window.close();

            if (ev.type == Event::KeyPressed) if (ev.key.code == Keyboard::R) init_board();

            if (ev.type == Event::MouseButtonPressed) {
                if (ev.mouseButton.button == Mouse::Left || ev.mouseButton.button == Mouse::Right) {
                    int mx = ev.mouseButton.x;
                    int my = ev.mouseButton.y;
                    if (my < high * cell_size) {
                        int cx = mx / cell_size, cy = my / cell_size;
                        if (in_bounds(cx, cy)) {
                            if (ev.mouseButton.button == Mouse::Left) reveal_cell(cx, cy);
                            else toggle_flag(cx, cy);
                        }
                    }
                }
            }
        }

        window.clear(Color(200, 200, 200));

        for (int y = 0; y < high; ++y) {
            for (int x = 0; x < width; ++x) {
                const Cell &c = board[y][x];
                cellShape.setPosition((float)x * cell_size + 0.5f, (float)y * cell_size + 0.5f);
                if (c.revealed) {
                    cellShape.setFillColor(Color(180,180,180));
                    cellShape.setOutlineThickness(0.f);
                    window.draw(cellShape);
                    if (c.mine) {
                        CircleShape ms(cell_size * 0.28f);
                        ms.setOrigin(ms.getRadius(), ms.getRadius());
                        ms.setPosition(x * cell_size + cell_size / 2.f + 0.5f, y * cell_size + cell_size / 2.f + 0.5f);
                        ms.setOutlineColor(Color::Black);
                        ms.setOutlineThickness(1.f);
                        ms.setFillColor(Color::Black);
                        window.draw(ms);
                    } else if (c.adj > 0) {
                        text.setString(to_string(c.adj));
                        text.setCharacterSize(18);
                        text.setPosition(x * cell_size + 7.f, y * cell_size + 4.f);
                        switch (c.adj) {
                            case 1: text.setFillColor(Color(0,0,200)); break;
                            case 2: text.setFillColor(Color(0,150,0)); break;
                            case 3: text.setFillColor(Color(200,0,0)); break;
                            case 4: text.setFillColor(Color(100,0,100)); break;
                            default: text.setFillColor(Color::Black); break;
                        }
                        window.draw(text);
                    }
                } else {
                    cellShape.setFillColor(Color(120,120,120));
                    cellShape.setOutlineColor(Color::Black);
                    cellShape.setOutlineThickness(1.f);
                    window.draw(cellShape);
                    if (c.flagged) {
                        text.setString("F");
                        text.setCharacterSize(18);
                        text.setFillColor(Color::Red);
                        text.setPosition(x * cell_size + 8.f, y * cell_size + 4.f);
                        window.draw(text);
                    }
                }
            }
        }

        int flags_now = flags_left;
        
        int seconds = 0;
        if (started && !game_over && !win) {
            seconds = game_clock.getElapsedTime().asSeconds();
        } else {
            seconds = final_time;
        }


        string status;
        if (game_over) status = "Game over! Press R to restart.";
        else if (win) status = "You win! Press R to restart.";
        else status = "Press R to restart";

        infoText.setString("Mines: " + to_string(total_mines) + "   Flags: " + to_string(flags_now) + "   Time" + to_string(seconds) + "   " + status);
        
        window.draw(infoText);

        window.display();
    }
    return 0;
}