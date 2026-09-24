#pragma once
#include <vector>
#include "./constants.hpp"

using namespace std;

class EditorActions {
    public :
        static void handleEnter(vector<vector<char>>& charBuffer, int& row, int& col) {
            row += 1;
            col = 0;
            charBuffer.push_back(vector<char>());
        }

        static void handleBackspace(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (col > 0) {
                col--;

                charBuffer[row].erase(charBuffer[row].begin() + col);
            }
        }

        static void handleKeyLeft(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (col > 0) {
                col -= 1;
            } else if (row > 0) {
                row -= 1;
                col = charBuffer[row].size();
            }
        }

        static void handleKeyRight(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (col < charBuffer[row].size()) {
                col += 1;
            } else if (row < charBuffer.size() - 1) {
                row += 1;
                col = 0;
            }
        }

        static void handleKeyUp(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (row > 0) {
                row -= 1;
                col = std::min(col, static_cast<int>(charBuffer[row].size()));
            }
        }

        static void handleKeyDown(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (row < charBuffer.size() - 1) {
                row += 1;
                col = min(col, static_cast<int>(charBuffer[row].size()));
            }
        }


        static void handleCharacterInput(vector<vector<char>>& charBuffer, int& row, int& col, int key) {
            if (col >= MAX_CHARS_PER_LINE) {
                row += 1;
                col = 0;
                charBuffer.push_back(vector<char>());
            }

            charBuffer[row].insert(
                    charBuffer[row].begin() + col,
                    static_cast<char>(key)
                    );

            col++ ;
        }
};
