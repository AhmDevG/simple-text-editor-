#pragma once
#include <vector>
#include <iostream>
#include "./constants.hpp"

using namespace std;

class EditorActions {
    public :
        // TODO(DONE) : handle the enter when you are a middle of line
        // get the current col and from this col to the end of the line shift it in the bottom line
        static void handleEnter(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (col == charBuffer[row].size()) {
                row += 1;
                col = 0;
                charBuffer.push_back(vector<char>());
            }
            else {
                // slice from the current col to the end of the line and move it to the next line
                vector<char> newLine(charBuffer[row].begin() + col, charBuffer[row].end());
                charBuffer[row].erase(charBuffer[row].begin() + col, charBuffer[row].end());
                charBuffer.insert(charBuffer.begin() + row + 1, newLine);
                row += 1;
                col = 0 ;
            }
        }

        static void handleBackspace(vector<vector<char>>& charBuffer, int& row, int& col) {
            if (col > 0) {
                col--;
                charBuffer[row].erase(charBuffer[row].begin() + col);
            }
            else if (row > 0) {
                col = charBuffer[row - 1].size();
                charBuffer[row - 1].insert(charBuffer[row - 1].end(), charBuffer[row].begin(), charBuffer[row].end());
                charBuffer.erase(charBuffer.begin() + row);
                row--;
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
