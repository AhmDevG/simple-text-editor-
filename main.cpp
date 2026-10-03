#include "./include/raylib.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iostream>
#include <cmath>

#define COLOR_BG        CLITERAL(Color){ 30, 30, 30, 255 }
#define COLOR_GUTTER    CLITERAL(Color){ 40, 40, 40, 255 }
#define COLOR_TEXT      CLITERAL(Color){ 220, 220, 220, 255 }
#define COLOR_NUMBERS   CLITERAL(Color){ 100, 100, 100, 255 }
#define COLOR_CURSOR    CLITERAL(Color){ 80, 160, 240, 255 }
#define COLOR_LINE_HL   CLITERAL(Color){ 45, 45, 45, 255 }
#define COLOR_SELECTION CLITERAL(Color){ 50, 90, 140, 180 }
#define COLOR_SCROLLBAR CLITERAL(Color){ 70, 70, 70, 255 }

using namespace std;

map<int, float> keyTimers;

struct TextPos {
    int row = 0;
    int col = 0;

    bool operator==(const TextPos &other) const { return row == other.row && col == other.col; }
    bool operator!=(const TextPos &other) const { return !(*this == other); }
    bool operator<(const TextPos &other) const {
        if (row != other.row) return row < other.row;
        return col < other.col;
    }
    bool operator<=(const TextPos &other) const { return *this < other || *this == other; }
};

struct VisualLine {
    int row = 0;
    int startCol = 0;
    int length = 0;
};

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

TextPos GetPrevWordPos(const vector<vector<char>> &buffer, int row, int col) {
    if (row < 0 || row >= (int)buffer.size()) return { 0, 0 };
    if (col == 0) {
        if (row > 0) return { row - 1, (int)buffer[row - 1].size() };
        return { 0, 0 };
    }
    int c = col;
    while (c > 0 && c <= (int)buffer[row].size() && buffer[row][c - 1] == ' ') c--;
    while (c > 0 && c <= (int)buffer[row].size() && buffer[row][c - 1] != ' ') c--;
    return { row, c };
}

TextPos GetNextWordPos(const vector<vector<char>> &buffer, int row, int col) {
    if (row < 0 || row >= (int)buffer.size()) return { 0, 0 };
    int lineSize = (int)buffer[row].size();
    if (col >= lineSize) {
        if (row < (int)buffer.size() - 1) return { row + 1, 0 };
        return { row, lineSize };
    }
    int c = col;
    while (c < lineSize && buffer[row][c] != ' ') c++;
    while (c < lineSize && buffer[row][c] == ' ') c++;
    return { row, c };
}

vector<VisualLine> GetVisualLines(const vector<vector<char>> &buffer, Font font, float fontSize, float fontSpacing, float maxTextWidth) {
    vector<VisualLine> visLines;
    if (maxTextWidth <= 40.0f) maxTextWidth = 40.0f;

    if (buffer.empty()) {
        visLines.push_back({ 0, 0, 0 });
        return visLines;
    }

    for (int r = 0; r < (int)buffer.size(); r++) {
        if (buffer[r].empty()) {
            visLines.push_back({ r, 0, 0 });
            continue;
        }

        int startCol = 0;
        int lineSize = (int)buffer[r].size();

        while (startCol < lineSize) {
            int count = 0;
            float currentWidth = 0.0f;

            while (startCol + count < lineSize) {
                char c = buffer[r][startCol + count];
                char str[2] = { c, '\0' };
                float charW = MeasureTextEx(font, str, fontSize, fontSpacing).x;

                if (count > 0 && currentWidth + charW > maxTextWidth) {
                    break;
                }
                currentWidth += charW;
                count++;
            }

            visLines.push_back({ r, startCol, count });
            startCol += count;
        }
    }
    if (visLines.empty()) {
        visLines.push_back({ 0, 0, 0 });
    }
    return visLines;
}

int GetVisualIndexFromPos(const vector<VisualLine> &visLines, int row, int col) {
    if (visLines.empty()) return 0;
    int bestIdx = 0;
    for (int i = 0; i < (int)visLines.size(); i++) {
        if (visLines[i].row == row) {
            bestIdx = i;
            if (col >= visLines[i].startCol && col <= visLines[i].startCol + visLines[i].length) {
                if (col < visLines[i].startCol + visLines[i].length || i == (int)visLines.size() - 1 || visLines[i + 1].row != row) {
                    return i;
                }
            }
        }
    }
    return bestIdx;
}

TextPos GetTextPosFromMouse(Vector2 mousePos, const vector<vector<char>> &buffer, Font font, float fontSize, float fontSpacing, int lineSpacing, int paddingTop, int gutterWidth, int textPaddingLeft, float scrollOffsetY, const vector<VisualLine> &visLines) {
    TextPos pos = { 0, 0 };
    if (visLines.empty() || buffer.empty()) return pos;

    float adjustedY = mousePos.y + scrollOffsetY - paddingTop;
    int visIdx = (int)(adjustedY / lineSpacing);

    if (visIdx < 0) visIdx = 0;
    if (visIdx >= (int)visLines.size()) visIdx = (int)visLines.size() - 1;

    const VisualLine &v = visLines[visIdx];
    if (v.row < 0 || v.row >= (int)buffer.size()) return pos;
    pos.row = v.row;

    float textStartX = (float)(gutterWidth + textPaddingLeft);
    float relativeX = mousePos.x - textStartX;

    if (relativeX <= 0) {
        pos.col = v.startCol;
        return pos;
    }

    int bestCol = v.startCol;
    float minDiff = 99999.0f;
    int rowLen = (int)buffer[v.row].size();
    int endC = min(v.startCol + v.length, rowLen);

    for (int c = v.startCol; c <= endC; c++) {
        string sub(buffer[v.row].begin() + v.startCol, buffer[v.row].begin() + c);
        float width = MeasureTextEx(font, sub.c_str(), fontSize, fontSpacing).x;
        float diff = abs(relativeX - width);
        if (diff < minDiff) {
            minDiff = diff;
            bestCol = c;
        }
    }
    pos.col = bestCol;
    return pos;
}

void GetNormalizedSelection(TextPos start, TextPos end, TextPos &outStart, TextPos &outEnd) {
    if (start <= end) {
        outStart = start;
        outEnd = end;
    } else {
        outStart = end;
        outEnd = start;
    }
}

bool DeleteSelection(vector<vector<char>> &buffer, TextPos &selStart, TextPos &selEnd, int &row, int &col) {
    if (selStart == selEnd) return false;

    TextPos s, e;
    GetNormalizedSelection(selStart, selEnd, s, e);

    if (s.row < 0 || s.row >= (int)buffer.size() || e.row < 0 || e.row >= (int)buffer.size()) return false;

    if (s.row == e.row) {
        int sCol = clamp(s.col, 0, (int)buffer[s.row].size());
        int eCol = clamp(e.col, sCol, (int)buffer[s.row].size());
        buffer[s.row].erase(buffer[s.row].begin() + sCol, buffer[s.row].begin() + eCol);
    } else {
        int sCol = clamp(s.col, 0, (int)buffer[s.row].size());
        int eCol = clamp(e.col, 0, (int)buffer[e.row].size());

        buffer[s.row].erase(buffer[s.row].begin() + sCol, buffer[s.row].end());
        buffer[s.row].insert(buffer[s.row].end(), buffer[e.row].begin() + eCol, buffer[e.row].end());
        buffer.erase(buffer.begin() + s.row + 1, buffer.begin() + e.row + 1);
    }

    row = s.row;
    col = s.col;
    selStart = selEnd = { row, col };
    return true;
}

string GetSelectedText(const vector<vector<char>> &buffer, TextPos selStart, TextPos selEnd) {
    if (selStart == selEnd) return "";

    TextPos s, e;
    GetNormalizedSelection(selStart, selEnd, s, e);
    if (s.row < 0 || s.row >= (int)buffer.size() || e.row < 0 || e.row >= (int)buffer.size()) return "";

    string result = "";

    if (s.row == e.row) {
        int sCol = clamp(s.col, 0, (int)buffer[s.row].size());
        int eCol = clamp(e.col, sCol, (int)buffer[s.row].size());
        result.assign(buffer[s.row].begin() + sCol, buffer[s.row].begin() + eCol);
    } else {
        int sCol = clamp(s.col, 0, (int)buffer[s.row].size());
        int eCol = clamp(e.col, 0, (int)buffer[e.row].size());

        result.assign(buffer[s.row].begin() + sCol, buffer[s.row].end());
        result += "\n";
        for (int r = s.row + 1; r < e.row; r++) {
            result.append(buffer[r].begin(), buffer[r].end());
            result += "\n";
        }
        result.append(buffer[e.row].begin(), buffer[e.row].begin() + eCol);
    }
    return result;
}

void handleInput(vector<vector<char>> &buffer, int &row, int &col, TextPos &selStart, TextPos &selEnd, Font font, float &scrollOffsetY, const vector<VisualLine> &visLines) {
    float dt = GetFrameTime();
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

    const float fontSize = 22.0f;
    const float fontSpacing = 1.0f;
    const int lineSpacing = 28;
    const int paddingTop = 20;
    const int gutterWidth = 60;
    const int textPaddingLeft = 15;

    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        TextPos clickedPos = GetTextPosFromMouse(mousePos, buffer, font, fontSize, fontSpacing, lineSpacing, paddingTop, gutterWidth, textPaddingLeft, scrollOffsetY, visLines);
        row = clickedPos.row;
        col = clickedPos.col;
        selStart = selEnd = clickedPos;
    }
    else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        selEnd = GetTextPosFromMouse(mousePos, buffer, font, fontSize, fontSpacing, lineSpacing, paddingTop, gutterWidth, textPaddingLeft, scrollOffsetY, visLines);
        row = selEnd.row;
        col = selEnd.col;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        scrollOffsetY -= wheel * 30.0f;
    }

    if (ctrl && IsKeyPressed(KEY_C)) {
        string selected = GetSelectedText(buffer, selStart, selEnd);
        if (selected.empty() && row >= 0 && row < (int)buffer.size()) {
            selected = string(buffer[row].begin(), buffer[row].end()) + "\n";
        }
        SetClipboardText(selected.c_str());
        return;
    }

    if (ctrl && IsKeyPressed(KEY_X)) {
        string selected = GetSelectedText(buffer, selStart, selEnd);
        if (!selected.empty()) {
            SetClipboardText(selected.c_str());
            DeleteSelection(buffer, selStart, selEnd, row, col);
        } else if (row >= 0 && row < (int)buffer.size()) {
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
        }
        return;
    }

    if (ctrl && IsKeyPressed(KEY_V)) {
        DeleteSelection(buffer, selStart, selEnd, row, col);
        const char *clipText = GetClipboardText();
        if (clipText != nullptr && row >= 0 && row < (int)buffer.size()) {
            string text(clipText);
            for (char c : text) {
                if (c == '\r') continue;
                if (c == '\n') {
                    col = clamp(col, 0, (int)buffer[row].size());
                    vector<char> remaining(buffer[row].begin() + col, buffer[row].end());
                    buffer[row].erase(buffer[row].begin() + col, buffer[row].end());
                    buffer.insert(buffer.begin() + row + 1, remaining);
                    row++;
                    col = 0;
                } else {
                    col = clamp(col, 0, (int)buffer[row].size());
                    buffer[row].insert(buffer[row].begin() + col, c);
                    col++;
                }
            }
            selStart = selEnd = { row, col };
        }
        return;
    }

    if (ctrl && IsKeyPressed(KEY_A)) {
        selStart = { 0, 0 };
        selEnd = { (int)buffer.size() - 1, (int)buffer.back().size() };
        row = selEnd.row;
        col = selEnd.col;
        return;
    }

    if (ctrl && IsKeyPressed(KEY_BACKSPACE)) {
        TextPos newPos = GetPrevWordPos(buffer, row, col);
        selStart = newPos;
        selEnd = { row, col };
        DeleteSelection(buffer, selStart, selEnd, row, col);
        return;
    }

    if (IsKeyTriggeredWithRepeat(KEY_HOME, dt)) {
        col = 0;
        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }
    if (IsKeyTriggeredWithRepeat(KEY_END, dt)) {
        if (row >= 0 && row < (int)buffer.size()) col = (int)buffer[row].size();
        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }

    if (IsKeyTriggeredWithRepeat(KEY_BACKSPACE, dt)) {
        if (!DeleteSelection(buffer, selStart, selEnd, row, col)) {
            if (col > 0 && row >= 0 && row < (int)buffer.size()) {
                buffer[row].erase(buffer[row].begin() + col - 1);
                col--;
            } else if (row > 0) {
                int prevSize = (int)buffer[row - 1].size();
                buffer[row - 1].insert(buffer[row - 1].end(), buffer[row].begin(), buffer[row].end());
                buffer.erase(buffer.begin() + row);
                row--;
                col = prevSize;
            }
            selStart = selEnd = { row, col };
        }
    }

    if (IsKeyTriggeredWithRepeat(KEY_TAB, dt)) {
        DeleteSelection(buffer, selStart, selEnd, row, col);
        if (row >= 0 && row < (int)buffer.size()) {
            col = clamp(col, 0, (int)buffer[row].size());
            buffer[row].insert(buffer[row].begin() + col, 4, ' ');
            col += 4;
            selStart = selEnd = { row, col };
        }
    }

    if (!ctrl) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126) {
                DeleteSelection(buffer, selStart, selEnd, row, col);
                if (row >= 0 && row < (int)buffer.size()) {
                    col = clamp(col, 0, (int)buffer[row].size());
                    buffer[row].insert(buffer[row].begin() + col, (char)key);
                    col++;
                    selStart = selEnd = { row, col };
                }
            }
            key = GetCharPressed();
        }
    }

    if (IsKeyTriggeredWithRepeat(KEY_ENTER, dt)) {
        DeleteSelection(buffer, selStart, selEnd, row, col);
        if (row >= 0 && row < (int)buffer.size()) {
            col = clamp(col, 0, (int)buffer[row].size());
            vector<char> remainingText(buffer[row].begin() + col, buffer[row].end());
            buffer[row].erase(buffer[row].begin() + col, buffer[row].end());
            buffer.insert(buffer.begin() + row + 1, remainingText);
            row++;
            col = 0;
            selStart = selEnd = { row, col };
        }
    }

    if (IsKeyTriggeredWithRepeat(KEY_LEFT, dt)) {
        if (ctrl) {
            TextPos newPos = GetPrevWordPos(buffer, row, col);
            row = newPos.row;
            col = newPos.col;
        } else {
            if (col > 0) col--;
            else if (row > 0) { row--; col = (int)buffer[row].size(); }
        }

        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }

    if (IsKeyTriggeredWithRepeat(KEY_RIGHT, dt)) {
        if (ctrl) {
            TextPos newPos = GetNextWordPos(buffer, row, col);
            row = newPos.row;
            col = newPos.col;
        } else {
            if (row >= 0 && row < (int)buffer.size()) {
                if (col < (int)buffer[row].size()) col++;
                else if (row < (int)buffer.size() - 1) { row++; col = 0; }
            }
        }

        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }

    if (IsKeyTriggeredWithRepeat(KEY_UP, dt)) {
        int curVisIdx = GetVisualIndexFromPos(visLines, row, col);
        if (curVisIdx > 0 && curVisIdx < (int)visLines.size()) {
            int targetVisIdx = curVisIdx - 1;
            const VisualLine &targetVis = visLines[targetVisIdx];

            if (targetVis.row >= 0 && targetVis.row < (int)buffer.size()) {
                int currentVisStart = visLines[curVisIdx].startCol;
                int maxColCurrent = min(col, currentVisStart + visLines[curVisIdx].length);
                string currentTextBefore(buffer[row].begin() + currentVisStart, buffer[row].begin() + maxColCurrent);
                float currentX = MeasureTextEx(font, currentTextBefore.c_str(), fontSize, fontSpacing).x;

                int bestCol = targetVis.startCol;
                float minDiff = 99999.0f;
                int targetRowLen = (int)buffer[targetVis.row].size();
                int endC = min(targetVis.startCol + targetVis.length, targetRowLen);

                for (int testCol = targetVis.startCol; testCol <= endC; testCol++) {
                    string targetTextBefore(buffer[targetVis.row].begin() + targetVis.startCol, buffer[targetVis.row].begin() + testCol);
                    float targetX = MeasureTextEx(font, targetTextBefore.c_str(), fontSize, fontSpacing).x;
                    float diff = abs(currentX - targetX);
                    if (diff < minDiff) {
                        minDiff = diff;
                        bestCol = testCol;
                    }
                }
                row = targetVis.row;
                col = bestCol;
            }
        }
        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }

    if (IsKeyTriggeredWithRepeat(KEY_DOWN, dt)) {
        int curVisIdx = GetVisualIndexFromPos(visLines, row, col);
        if (curVisIdx >= 0 && curVisIdx < (int)visLines.size() - 1) {
            int targetVisIdx = curVisIdx + 1;
            const VisualLine &targetVis = visLines[targetVisIdx];

            if (targetVis.row >= 0 && targetVis.row < (int)buffer.size()) {
                int currentVisStart = visLines[curVisIdx].startCol;
                int maxColCurrent = min(col, currentVisStart + visLines[curVisIdx].length);
                string currentTextBefore(buffer[row].begin() + currentVisStart, buffer[row].begin() + maxColCurrent);
                float currentX = MeasureTextEx(font, currentTextBefore.c_str(), fontSize, fontSpacing).x;

                int bestCol = targetVis.startCol;
                float minDiff = 99999.0f;
                int targetRowLen = (int)buffer[targetVis.row].size();
                int endC = min(targetVis.startCol + targetVis.length, targetRowLen);

                for (int testCol = targetVis.startCol; testCol <= endC; testCol++) {
                    string targetTextBefore(buffer[targetVis.row].begin() + targetVis.startCol, buffer[targetVis.row].begin() + testCol);
                    float targetX = MeasureTextEx(font, targetTextBefore.c_str(), fontSize, fontSpacing).x;
                    float diff = abs(currentX - targetX);
                    if (diff < minDiff) {
                        minDiff = diff;
                        bestCol = testCol;
                    }
                }
                row = targetVis.row;
                col = bestCol;
            }
        }
        if (shift) selEnd = { row, col };
        else selStart = selEnd = { row, col };
    }
}

void drawTextEditor(const vector<vector<char>> &buffer, int row, int col, TextPos selStart, TextPos selEnd, Font font, float &scrollOffsetY, const vector<VisualLine> &visLines) {
    if (buffer.empty() || visLines.empty()) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    const float fontSize = 22.0f;
    const float fontSpacing = 1.0f;
    const int lineSpacing = 28;
    const int paddingTop = 20;
    const int gutterWidth = 60;
    const int textPaddingLeft = 15;
    const int scrollbarWidth = 12;

    float totalContentHeight = (float)(visLines.size() * lineSpacing + paddingTop * 2);
    float maxScroll = max(0.0f, totalContentHeight - (float)screenHeight);
    scrollOffsetY = clamp(scrollOffsetY, 0.0f, maxScroll);

    int curVisIdx = GetVisualIndexFromPos(visLines, row, col);
    curVisIdx = clamp(curVisIdx, 0, (int)visLines.size() - 1);

    float cursorY = (float)(paddingTop + curVisIdx * lineSpacing) - scrollOffsetY;
    if (cursorY < paddingTop) scrollOffsetY = (float)(curVisIdx * lineSpacing);
    if (cursorY > screenHeight - lineSpacing) scrollOffsetY = (float)(curVisIdx * lineSpacing - screenHeight + lineSpacing * 2);

    ClearBackground(COLOR_BG);
    BeginScissorMode(0, 0, screenWidth, screenHeight);

    int highlightY = (int)(paddingTop + (curVisIdx * lineSpacing) - scrollOffsetY);
    DrawRectangle(gutterWidth, highlightY, screenWidth - gutterWidth, lineSpacing, COLOR_LINE_HL);

    if (selStart != selEnd) {
        TextPos s, e;
        GetNormalizedSelection(selStart, selEnd, s, e);

        for (size_t i = 0; i < visLines.size(); i++) {
            const VisualLine &v = visLines[i];
            if (v.row < 0 || v.row >= (int)buffer.size()) continue;
            if (v.row < s.row || v.row > e.row) continue;

            int rowSize = (int)buffer[v.row].size();
            int visStartCol = clamp(v.startCol, 0, rowSize);
            int visEndCol = clamp(v.startCol + v.length, visStartCol, rowSize);

            int selStartColInRow = (v.row == s.row) ? clamp(s.col, 0, rowSize) : 0;
            int selEndColInRow = (v.row == e.row) ? clamp(e.col, 0, rowSize) : rowSize;

            int drawStartCol = max(visStartCol, selStartColInRow);
            int drawEndCol = min(visEndCol, selEndColInRow);

            if (drawStartCol < drawEndCol) {
                string textBefore(buffer[v.row].begin() + visStartCol, buffer[v.row].begin() + drawStartCol);
                string textSelected(buffer[v.row].begin() + drawStartCol, buffer[v.row].begin() + drawEndCol);

                float xStart = (float)(gutterWidth + textPaddingLeft) + MeasureTextEx(font, textBefore.c_str(), fontSize, fontSpacing).x;
                float selWidth = MeasureTextEx(font, textSelected.c_str(), fontSize, fontSpacing).x;

                float yPos = (float)(paddingTop + i * lineSpacing) - scrollOffsetY;
                DrawRectangle((int)xStart, (int)yPos, (int)selWidth, lineSpacing, COLOR_SELECTION);
            } else if (drawStartCol == drawEndCol && v.row != e.row && drawEndCol == visEndCol && visEndCol == rowSize) {
                string textBefore(buffer[v.row].begin() + visStartCol, buffer[v.row].begin() + visEndCol);
                float xStart = (float)(gutterWidth + textPaddingLeft) + MeasureTextEx(font, textBefore.c_str(), fontSize, fontSpacing).x;
                float yPos = (float)(paddingTop + i * lineSpacing) - scrollOffsetY;
                DrawRectangle((int)xStart, (int)yPos, 10, lineSpacing, COLOR_SELECTION);
            }
        }
    }

    for (size_t i = 0; i < visLines.size(); i++) {
        float yPos = (float)(paddingTop + (i * lineSpacing)) - scrollOffsetY;
        if (yPos + lineSpacing < 0 || yPos > screenHeight) continue;

        const VisualLine &v = visLines[i];
        if (v.row < 0 || v.row >= (int)buffer.size()) continue;

        int rowSize = (int)buffer[v.row].size();
        int startC = clamp(v.startCol, 0, rowSize);
        int endC = clamp(v.startCol + v.length, startC, rowSize);

        string lineSub(buffer[v.row].begin() + startC, buffer[v.row].begin() + endC);
        DrawTextEx(font, lineSub.c_str(), (Vector2){ (float)(gutterWidth + textPaddingLeft), yPos }, fontSize, fontSpacing, COLOR_TEXT);
    }

    DrawRectangle(0, 0, gutterWidth, screenHeight, COLOR_GUTTER);
    for (size_t i = 0; i < visLines.size(); i++) {
        float yPos = (float)(paddingTop + (i * lineSpacing)) - scrollOffsetY;
        if (yPos + lineSpacing < 0 || yPos > screenHeight) continue;

        const VisualLine &v = visLines[i];
        if (v.row < 0 || v.row >= (int)buffer.size()) continue;

        if (v.startCol == 0) {
            const char *lineNumStr = TextFormat("%2d", v.row + 1);
            DrawTextEx(font, lineNumStr, (Vector2){ 15.0f, yPos }, fontSize, fontSpacing, COLOR_NUMBERS);
        }
    }

    if (row >= 0 && row < (int)buffer.size()) {
        const VisualLine &curVis = visLines[curVisIdx];
        int rowSize = (int)buffer[row].size();
        int visStart = clamp(curVis.startCol, 0, rowSize);
        int safeCol = clamp(col, visStart, clamp(curVis.startCol + curVis.length, visStart, rowSize));

        string textBeforeCursor(buffer[row].begin() + visStart, buffer[row].begin() + safeCol);

        Vector2 textSize = MeasureTextEx(font, textBeforeCursor.c_str(), fontSize, fontSpacing);
        float curX = (float)(gutterWidth + textPaddingLeft) + textSize.x;
        float curY = (float)(paddingTop + (curVisIdx * lineSpacing)) - scrollOffsetY;

        if ((int)(GetTime() * 2.5f) % 2 == 0) {
            DrawRectangle((int)curX, (int)curY, 2, (int)fontSize, COLOR_CURSOR);
        }
    }

    if (maxScroll > 0) {
        float thumbHeight = max(30.0f, ((float)screenHeight / totalContentHeight) * screenHeight);
        float thumbY = (scrollOffsetY / maxScroll) * (screenHeight - thumbHeight);

        DrawRectangle(screenWidth - scrollbarWidth, 0, scrollbarWidth, screenHeight, CLITERAL(Color){ 35, 35, 35, 255 });
        DrawRectangle(screenWidth - scrollbarWidth, (int)thumbY, scrollbarWidth, (int)thumbHeight, COLOR_SCROLLBAR);
    }

    EndScissorMode();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "simple editor");
    SetTargetFPS(60);

    Font font = LoadFontEx("./fonts/JetBrainsMono-Medium.ttf", 44, 0, 250);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    vector<vector<char>> charBuffer = { {} };
    int cursorRow = 0;
    int cursorCol = 0;
    TextPos selStart = { 0, 0 };
    TextPos selEnd = { 0, 0 };
    float scrollOffsetY = 0.0f;

    while (!WindowShouldClose()) {
        int screenWidth = GetScreenWidth();
        const int gutterWidth = 60;
        const int textPaddingLeft = 15;
        const int scrollbarWidth = 12;
        const float fontSize = 22.0f;
        const float fontSpacing = 1.0f;
        float maxTextWidth = (float)(screenWidth - gutterWidth - textPaddingLeft - scrollbarWidth - 25);

        vector<VisualLine> visLines = GetVisualLines(charBuffer, font, fontSize, fontSpacing, maxTextWidth);

        handleInput(charBuffer, cursorRow, cursorCol, selStart, selEnd, font, scrollOffsetY, visLines);


        visLines = GetVisualLines(charBuffer, font, fontSize, fontSpacing, maxTextWidth);

        BeginDrawing();
        drawTextEditor(charBuffer, cursorRow, cursorCol, selStart, selEnd, font, scrollOffsetY, visLines);
        EndDrawing();
    }

    UnloadFont(font);
    CloseWindow();
    return 0;
}
