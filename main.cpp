#include "./constants.hpp"
#include "./editor_actions.hpp"
#include "./include/raylib.h"
#include <functional>
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using Action = void (*)(vector<vector<char>> &, int &, int &);

// Font lnuFont  ;
// Font textFont ;

// g++ main.cpp -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm -o main.exe && main.exe

bool caretVisible = true;
float caretTimer = 0.0f;

void showKeyboardCursor(int row, int col) {
  caretTimer += GetFrameTime();

  if (caretTimer >= 0.2f) {
    caretVisible = !caretVisible;
    caretTimer = 0.0f;
  }

  if (caretVisible) {
    int x = (col * CHAR_WIDTH) + CARRET_OFFSET;
    int y = (row * CHAR_HEIGHT) + CARRET_OFFSET;

    DrawLine(x, y, x, y + CHAR_HEIGHT, WHITE);
  }
}

void showTextBuffer(const vector<vector<char>> &charBuffer) {
  for (int row = 0; row < charBuffer.size(); ++row) {
    for (int col = 0; col < charBuffer[row].size(); ++col) {
      char c = charBuffer[row][col];
      if (c != '\0') {
        float x = (col * CHAR_WIDTH) + CARRET_OFFSET;
        float y = (row * CHAR_HEIGHT) + CARRET_OFFSET;

        char text[2] = {charBuffer[row][col], '\0'};

        // DrawTextEx(textFont , text, {x, y}, 20, 0 ,  WHITE);
        DrawText(text, x, y, 20, WHITE);
      }
    }
  }
}

void showLineNumber(const vector<vector<char>> &charBuffer) {
  for (int row = 0; row < charBuffer.size(); ++row) {
    float x = CARRET_OFFSET - 20;
    float y = (row * CHAR_HEIGHT) + CARRET_OFFSET;

    char text[10];
    sprintf(text, "%d", row + 1);

    DrawText(text, x, y, 20, YELLOW);
  }
}

// the key and the action that can perform with it
map<int, Action> keyActionMap = {
    {KEY_ENTER, EditorActions::handleEnter},
    {KEY_BACKSPACE, EditorActions::handleBackspace},
    {KEY_LEFT, EditorActions::handleKeyLeft},
    {KEY_RIGHT, EditorActions::handleKeyRight},
    {KEY_UP, EditorActions::handleKeyUp},
    {KEY_DOWN, EditorActions::handleKeyDown}};

int main() {
  InitWindow(800, 600, "simple text editor");
  SetTargetFPS(60);
  vector<vector<char>> charBuffer(1);

  int row = 0;
  int col = 0;

  // lnuFont = LoadFontEx("./fonts/Roboto-Bold.ttf" , 30 , nullptr , 0);
  // textFont = LoadFontEx("./fonts/Roboto-Regular.ttf" , 30 , nullptr , 0);

  while (!WindowShouldClose()) {
    BeginDrawing();

    int key = GetCharPressed();
    int theKey = GetKeyPressed();

    if (theKey > 0) {

      auto it = keyActionMap.find(theKey);

      if (it != keyActionMap.end()) {
        Action action = it->second;
        action(charBuffer, row, col);
      }
      else {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
          if (IsKeyPressed(KEY_BACKSPACE)) {
            while (col > 0 && charBuffer[row][col - 1] != ' ') {
              col -= 1;
              charBuffer[row].pop_back();
            }
          }
        }
        else {
          EditorActions::handleCharacterInput(charBuffer, row, col, key);
        }
      }
    }

    ClearBackground(BLACK);
    showKeyboardCursor(row, col);
    showTextBuffer(charBuffer);
    showLineNumber(charBuffer);

    EndDrawing();
  }

  // UnloadFont(lnuFont) ;
  // UnloadFont(textFont) ;

  CloseWindow();

  return 0;
}
