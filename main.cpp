#include "./include/raylib.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>


using namespace std;


#define COLOR_BG        CLITERAL(Color){ 30, 30, 30, 255 }
#define COLOR_GUTTER    CLITERAL(Color){ 40, 40, 40, 255 }
#define COLOR_TEXT      CLITERAL(Color){ 220, 220, 220, 255 }
#define COLOR_NUMBERS   CLITERAL(Color){ 100, 100, 100, 255 }
#define COLOR_CURSOR    CLITERAL(Color){ 80, 160, 240, 255 }
#define COLOR_LINE_HL   CLITERAL(Color){ 45, 45, 45, 255 }


map<int, float> keyTimers;


bool IsKeyTriggeredWithRepeat(int key, float dt) {
    const float INITIAL_DELAY = 0.35f;
    const float REPEAT_INTERVAL = 0.035f;

    if (IsKeyPressed(key)) {
        keyTimers[key] = 0.0f;
        return true;
    }
    if (IsKeyDown(key)) {
        keyTimers[key] += dt;
        if (keyTimers[key] >= INITIAL_DELAY) {
            keyTimers[key] -= REPEAT_INTERVAL;
            return true;
        }
    } else {
        keyTimers[key] = 0.0f;
    }
    return false;
}


void handleInput(vector<vector<char>> &buffer, int &row, int &col) {
    float dt = GetFrameTime();
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);


    if (ctrl && IsKeyPressed(KEY_C)) {
        string lineText(buffer[row].begin(), buffer[row].end());
        lineText += "\n";
        SetClipboardText(lineText.c_str());
        return;
    }

    if (ctrl && IsKeyPressed(KEY_X)) {
        string lineText(buffer[row].begin(), buffer[row].end());
        lineText += "\n";
        SetClipboardText(lineText.c_str());

        if (buffer.size() > 1) {
            buffer.erase(buffer.begin() + row);
            if (row >= (int)buffer.size()) row = (int)buffer.size() - 1;
            col = min(col, (int)buffer[row].size());
        } else {
            buffer[0].clear();
            col = 0;
        }
        return;
    }

    if (ctrl && IsKeyPressed(KEY_V)) {
        const char *clipText = GetClipboardText();
        if (clipText != nullptr) {
            string text(clipText);
            for (char c : text) {
                if (c == '\r') continue;
                if (c == '\n') {
                    vector<char> remaining(buffer[row].begin() + col, buffer[row].end());
                    buffer[row].erase(buffer[row].begin() + col, buffer[row].end());
                    buffer.insert(buffer.begin() + row + 1, remaining);
                    row++;
                    col = 0;
                } else {
                    buffer[row].insert(buffer[row].begin() + col, c);
                    col++;
                }
            }
        }
        return;
    }


    if (IsKeyTriggeredWithRepeat(KEY_HOME, dt)) {
        col = 0;
    }
    if (IsKeyTriggeredWithRepeat(KEY_END, dt)) {
        col = (int)buffer[row].size();
    }


    if (ctrl && IsKeyTriggeredWithRepeat(KEY_BACKSPACE, dt)) {
        if (col > 0) {
            int endCol = col;
            while (col > 0 && buffer[row][col - 1] == ' ') col--;
            while (col > 0 && buffer[row][col - 1] != ' ') col--;
            buffer[row].erase(buffer[row].begin() + col, buffer[row].begin() + endCol);
        } else if (row > 0) {
            int prevSize = (int)buffer[row - 1].size();
            buffer[row - 1].insert(buffer[row - 1].end(), buffer[row].begin(), buffer[row].end());
            buffer.erase(buffer.begin() + row);
            row--;
            col = prevSize;
        }
    }
    else if (!ctrl && IsKeyTriggeredWithRepeat(KEY_BACKSPACE, dt)) {
        if (col > 0) {
            buffer[row].erase(buffer[row].begin() + col - 1);
            col--;
        } else if (row > 0) {
            int prevSize = (int)buffer[row - 1].size();
            buffer[row - 1].insert(buffer[row - 1].end(), buffer[row].begin(), buffer[row].end());
            buffer.erase(buffer.begin() + row);
            row--;
            col = prevSize;
        }
    }

    if (IsKeyTriggeredWithRepeat(KEY_DELETE, dt)) {
        if (col < (int)buffer[row].size()) {
            buffer[row].erase(buffer[row].begin() + col);
        } else if (row < (int)buffer.size() - 1) {
            buffer[row].insert(buffer[row].end(), buffer[row + 1].begin(), buffer[row + 1].end());
            buffer.erase(buffer.begin() + row + 1);
        }
    }


    if (!ctrl) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                buffer[row].insert(buffer[row].begin() + col, (char)key);
                col++;
            }
            key = GetCharPressed();
        }
    }


    if (IsKeyTriggeredWithRepeat(KEY_ENTER, dt)) {
        vector<char> remainingText(buffer[row].begin() + col, buffer[row].end());
        buffer[row].erase(buffer[row].begin() + col, buffer[row].end());
        buffer.insert(buffer.begin() + row + 1, remainingText);
        row++;
        col = 0;
    }

    if (IsKeyTriggeredWithRepeat(KEY_LEFT, dt)) {
        if (col > 0) col--;
        else if (row > 0) { row--; col = (int)buffer[row].size(); }
    }
    if (IsKeyTriggeredWithRepeat(KEY_RIGHT, dt)) {
        if (col < (int)buffer[row].size()) col++;
        else if (row < (int)buffer.size() - 1) { row++; col = 0; }
    }
    if (IsKeyTriggeredWithRepeat(KEY_UP, dt)) {
        if (row > 0) { row--; col = min(col, (int)buffer[row].size()); }
    }
    if (IsKeyTriggeredWithRepeat(KEY_DOWN, dt)) {
        if (row < (int)buffer.size() - 1) { row++; col = min(col, (int)buffer[row].size()); }
    }
}


void drawTextEditor(const vector<vector<char>> &buffer, int row, int col) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const int fontSize = 20;
    const int lineSpacing = 28;
    const int paddingTop = 20;
    const int gutterWidth = 60;
    const int textPaddingLeft = 15;

    ClearBackground(COLOR_BG);


    int highlightY = paddingTop + (row * lineSpacing);
    DrawRectangle(gutterWidth, highlightY, screenWidth - gutterWidth, lineSpacing, COLOR_LINE_HL);


    DrawRectangle(0, 0, gutterWidth, screenHeight, COLOR_GUTTER);


    for (size_t i = 0; i < buffer.size(); i++) {
        int yPos = paddingTop + (i * lineSpacing);

        DrawText(TextFormat("%2zu", i + 1), 15, yPos, fontSize, COLOR_NUMBERS);

        string lineText(buffer[i].begin(), buffer[i].end());
        DrawText(lineText.c_str(), gutterWidth + textPaddingLeft, yPos, fontSize, COLOR_TEXT);
    }


    if (row >= 0 && row < (int)buffer.size()) {
        int safeCol = min(col, (int)buffer[row].size());
        string textBeforeCursor(buffer[row].begin(), buffer[row].begin() + safeCol);

        int cursorX = gutterWidth + textPaddingLeft + MeasureText(textBeforeCursor.c_str(), fontSize);
        int cursorY = paddingTop + (row * lineSpacing);

        if ((int)(GetTime() * 2.5f) % 2 == 0) {
            DrawRectangle(cursorX, cursorY, 2, fontSize, COLOR_CURSOR);
        }
    }
}


int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "simple editor with highlighting");
    SetTargetFPS(60);

    vector<vector<char>> charBuffer = { {} };
    int cursorRow = 0;
    int cursorCol = 0;

    while (!WindowShouldClose()) {
        handleInput(charBuffer, cursorRow, cursorCol);

        BeginDrawing();
        drawTextEditor(charBuffer, cursorRow, cursorCol);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
