#include <iostream>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <limits>
#include <iomanip>
using namespace std;

// ================= COLOR CLASS =================
class Color{
public:
    static void set(int c){
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
    }
    
    static void reset(){
        set(7);
    }
};

// ================= BASIC FUNCTIONS =================
void gotoxy(int x,int y){
    COORD c={short(x-1),short(y-1)};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),c);
}

void clearScreen(){ 
    system("cls"); 
}

void sleep_ms(int ms){ 
    Sleep(ms); 
}

void hideCursor(){
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = false;
    cursorInfo.dwSize = 100;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void showCursor(){
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = true;
    cursorInfo.dwSize = 100;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// ================= DRAWING FUNCTIONS =================
void drawBox(int x1, int y1, int x2, int y2, int style = 1, int color = 7){
    Color::set(color);
    
    char corners[4];
    char horizontal, vertical;
    
    switch(style){
        case 1: // Simple
            corners[0] = '+'; corners[1] = '+'; 
            corners[2] = '+'; corners[3] = '+';
            horizontal = '-'; vertical = '|';
            break;
        case 2: // Double line
            corners[0] = 201; corners[1] = 187; 
            corners[2] = 200; corners[3] = 188;
            horizontal = 205; vertical = 186;
            break;
        case 3: // Single line
            corners[0] = 218; corners[1] = 191; 
            corners[2] = 192; corners[3] = 217;
            horizontal = 196; vertical = 179;
            break;
        default:
            corners[0] = '+'; corners[1] = '+'; 
            corners[2] = '+'; corners[3] = '+';
            horizontal = '-'; vertical = '|';
    }
    
    // Draw horizontal lines
    for(int x = x1+1; x < x2; x++){
        gotoxy(x, y1); cout << horizontal;
        gotoxy(x, y2); cout << horizontal;
    }
    
    // Draw vertical lines
    for(int y = y1+1; y < y2; y++){
        gotoxy(x1, y); cout << vertical;
        gotoxy(x2, y); cout << vertical;
    }
    
    // Draw corners
    gotoxy(x1, y1); cout << corners[0];
    gotoxy(x2, y1); cout << corners[1];
    gotoxy(x1, y2); cout << corners[2];
    gotoxy(x2, y2); cout << corners[3];
    
    Color::reset();
}

void printCentered(int y, string text, int color = 7){
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    
    int x = (width - text.length()) / 2;
    gotoxy(max(1, x), y);
    Color::set(color);
    cout << text;
    Color::reset();
}

// ================= SPLIT SCREEN FUNCTIONS =================
void drawSplitScreen() {
    clearScreen();
    // Draw vertical line in middle
    Color::set(8);
    for(int y = 1; y <= 30; y++) {
        gotoxy(60, y);
        cout << char(179);
    }
    Color::reset();
}

// ================= BUFFERED OUTPUT FUNCTIONS =================
class BufferedOutput {
private:
    static const int MAX_WIDTH = 120;
    static const int MAX_HEIGHT = 30;
    char buffer[MAX_HEIGHT][MAX_WIDTH];
    int lastWidth, lastHeight;
    
public:
    BufferedOutput() {
        clearBuffer();
        lastWidth = 0;
        lastHeight = 0;
    }
    
    void clearBuffer() {
        for(int i = 0; i < MAX_HEIGHT; i++) {
            for(int j = 0; j < MAX_WIDTH; j++) {
                buffer[i][j] = ' ';
            }
        }
    }
    
    void writeToBuffer(int x, int y, char ch) {
        if(x >= 1 && x <= MAX_WIDTH && y >= 1 && y <= MAX_HEIGHT) {
            buffer[y-1][x-1] = ch;
        }
    }
    
    void writeToBuffer(int x, int y, string str) {
        for(int i = 0; i < str.length(); i++) {
            writeToBuffer(x + i, y, str[i]);
        }
    }
    
    void flush() {
        // Only update changed parts of screen
        for(int y = 0; y < MAX_HEIGHT; y++) {
            for(int x = 0; x < MAX_WIDTH; x++) {
                gotoxy(x + 1, y + 1);
                cout << buffer[y][x];
            }
        }
    }
};

// ================= LOADING SCREEN =================
void showLoading(){
    clearScreen();
    hideCursor();
    
    drawBox(30, 10, 90, 18, 2, 11);
    printCentered(12, "?? GAME ??", 14);
    printCentered(14, "Loading...", 11);
    
    // Simple progress bar
    gotoxy(32, 16);
    Color::set(8);
    cout << "[";
    for(int i=0; i<56; i++) cout << " ";
    cout << "]";
    
    gotoxy(33, 16);
    Color::set(10);
    for(int i=0; i<55; i++){
        cout << char(219);
        sleep_ms(20);
    }
    
    sleep_ms(500);
    showCursor();
    Color::reset();
}

// ================= GAME SETTINGS =================
struct TicTacToeSettings {
    int difficulty = 1; // 1=Easy, 2=Medium, 3=Hard
    int gridSize = 3;   // 3, 4, or 5
    int uiStyle = 1;    // 1=Simple, 2=Double, 3=Single
} tttSettings;

struct WordGuessSettings {
    int difficulty = 1;      // 1=Easy, 2=Medium, 3=Hard
    string category = "GENERAL"; // GENERAL, TECHNOLOGY, ENTERTAINMENT
    int uiStyle = 1;         // 1=Simple, 2=Double, 3=Single
} wgSettings;

// ================= GAME HISTORY =================
vector<string> gameHistory;

void saveToHistory(string gameType, string result){
    time_t now = time(0);
    tm* timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    
    string entry = string(buffer) + " | " + gameType + " | " + result;
    gameHistory.push_back(entry);
    
    // Keep only last 10 entries
    if(gameHistory.size() > 10){
        gameHistory.erase(gameHistory.begin());
    }
}

void showGameHistory(){
    clearScreen();
    drawBox(20, 3, 100, 25, 2, 13);
    printCentered(5, "?? GAME HISTORY ??", 13);
    
    if(gameHistory.empty()){
        gotoxy(45, 12);
        Color::set(14);
        cout << "No games played yet!";
    } else {
        for(int i=0; i<gameHistory.size(); i++){
            gotoxy(25, 8 + i);
            Color::set(7);
            cout << i+1 << ". " << gameHistory[i];
        }
    }
    
    gotoxy(40, 23);
    Color::set(8);
    cout << "Press any key to continue...";
    _getch();
}

// ================= TIC TAC TOE SETTINGS MENU =================
void showTicTacToeSettings(){
    clearScreen();
    drawBox(20, 3, 100, 20, tttSettings.uiStyle, 14);
    printCentered(5, "?? TIC TAC TOE SETTINGS ??", 14);
    
    vector<string> options = {
        "1. Difficulty Level: " + string(tttSettings.difficulty==1?"[EASY]":"Easy") + 
                            " " + (tttSettings.difficulty==2?"[MEDIUM]":"Medium") + 
                            " " + (tttSettings.difficulty==3?"[HARD]":"Hard"),
        "2. Grid Size: " + string(tttSettings.gridSize==3?"[3x3]":"3x3") + 
                        " " + (tttSettings.gridSize==4?"[4x4]":"4x4") + 
                        " " + (tttSettings.gridSize==5?"[5x5]":"5x5"),
        "3. UI Style: " + string(tttSettings.uiStyle==1?"[SIMPLE]":"Simple") + 
                       " " + (tttSettings.uiStyle==2?"[DOUBLE LINE]":"Double Line") + 
                       " " + (tttSettings.uiStyle==3?"[SINGLE LINE]":"Single Line"),
        "4. Save and Start Game"
    };
    
    for(int i=0; i<options.size(); i++){
        gotoxy(25, 8 + i*2);
        Color::set(11);
        cout << options[i];
    }
    
    gotoxy(25, 18);
    Color::set(8);
    cout << "Use number keys (1-4) to change settings";
    
    while(true){
        char ch = _getch();
        switch(ch){
            case '1':
                tttSettings.difficulty = (tttSettings.difficulty % 3) + 1;
                break;
            case '2':
                tttSettings.gridSize = (tttSettings.gridSize==3?4:(tttSettings.gridSize==4?5:3));
                break;
            case '3':
                tttSettings.uiStyle = (tttSettings.uiStyle % 3) + 1;
                break;
            case '4':
                return;
        }
        
        // Update display - partial update
        for(int i=0; i<options.size(); i++){
            gotoxy(25, 8 + i*2);
            Color::set(11);
            
            switch(i){
                case 0:
                    cout << "1. Difficulty Level: " << (tttSettings.difficulty==1?"[EASY]":"Easy") 
                         << " " << (tttSettings.difficulty==2?"[MEDIUM]":"Medium") 
                         << " " << (tttSettings.difficulty==3?"[HARD]":"Hard") << "          ";
                    break;
                case 1:
                    cout << "2. Grid Size: " << (tttSettings.gridSize==3?"[3x3]":"3x3") 
                         << " " << (tttSettings.gridSize==4?"[4x4]":"4x4") 
                         << " " << (tttSettings.gridSize==5?"[5x5]":"5x5") << "          ";
                    break;
                case 2:
                    cout << "3. UI Style: " << (tttSettings.uiStyle==1?"[SIMPLE]":"Simple") 
                         << " " << (tttSettings.uiStyle==2?"[DOUBLE LINE]":"Double Line") 
                         << " " << (tttSettings.uiStyle==3?"[SINGLE LINE]":"Single Line") << "          ";
                    break;
                case 3:
                    cout << "4. Save and Start Game                              ";
                    break;
            }
        }
    }
}

// ================= WORD GUESS SETTINGS MENU =================
void showWordGuessSettings(){
    clearScreen();
    drawBox(20, 3, 100, 20, wgSettings.uiStyle, 13);
    printCentered(5, "?? WORD GUESS SETTINGS ??", 13);
    
    vector<string> options = {
        "1. Difficulty Level: " + string(wgSettings.difficulty==1?"[EASY]":"Easy") + 
                            " " + (wgSettings.difficulty==2?"[MEDIUM]":"Medium") + 
                            " " + (wgSettings.difficulty==3?"[HARD]":"Hard"),
        "2. Word Category: " + string(wgSettings.category=="GENERAL"?"[GENERAL]":"General") + 
                          " " + (wgSettings.category=="TECHNOLOGY"?"[TECHNOLOGY]":"Technology") + 
                          " " + (wgSettings.category=="ENTERTAINMENT"?"[ENTERTAINMENT]":"Entertainment"),
        "3. UI Style: " + string(wgSettings.uiStyle==1?"[SIMPLE]":"Simple") + 
                       " " + (wgSettings.uiStyle==2?"[DOUBLE LINE]":"Double Line") + 
                       " " + (wgSettings.uiStyle==3?"[SINGLE LINE]":"Single Line"),
        "4. Save and Start Game"
    };
    
    for(int i=0; i<options.size(); i++){
        gotoxy(25, 8 + i*2);
        Color::set(11);
        cout << options[i];
    }
    
    gotoxy(25, 18);
    Color::set(8);
    cout << "Use number keys (1-4) to change settings";
    
    while(true){
        char ch = _getch();
        switch(ch){
            case '1':
                wgSettings.difficulty = (wgSettings.difficulty % 3) + 1;
                break;
            case '2':
                if(wgSettings.category == "GENERAL") wgSettings.category = "TECHNOLOGY";
                else if(wgSettings.category == "TECHNOLOGY") wgSettings.category = "ENTERTAINMENT";
                else wgSettings.category = "GENERAL";
                break;
            case '3':
                wgSettings.uiStyle = (wgSettings.uiStyle % 3) + 1;
                break;
            case '4':
                return;
        }
        
        // Partial update
        for(int i=0; i<options.size(); i++){
            gotoxy(25, 8 + i*2);
            Color::set(11);
            
            switch(i){
                case 0:
                    cout << "1. Difficulty Level: " << (wgSettings.difficulty==1?"[EASY]":"Easy") 
                         << " " << (wgSettings.difficulty==2?"[MEDIUM]":"Medium") 
                         << " " << (wgSettings.difficulty==3?"[HARD]":"Hard") << "          ";
                    break;
                case 1:
                    cout << "2. Word Category: " << (wgSettings.category=="GENERAL"?"[GENERAL]":"General") 
                         << " " << (wgSettings.category=="TECHNOLOGY"?"[TECHNOLOGY]":"Technology") 
                         << " " << (wgSettings.category=="ENTERTAINMENT"?"[ENTERTAINMENT]":"Entertainment") << "          ";
                    break;
                case 2:
                    cout << "3. UI Style: " << (wgSettings.uiStyle==1?"[SIMPLE]":"Simple") 
                         << " " << (wgSettings.uiStyle==2?"[DOUBLE LINE]":"Double Line") 
                         << " " << (wgSettings.uiStyle==3?"[SINGLE LINE]":"Single Line") << "          ";
                    break;
                case 3:
                    cout << "4. Save and Start Game                              ";
                    break;
            }
        }
    }
}

// ================= TOSS FUNCTION =================
bool toss(string player1, string player2){
    clearScreen();
    drawBox(35, 10, 85, 18, 2, 14);
    gotoxy(50, 12);
    Color::set(14);
    cout << "?? TOSSING...";
    sleep_ms(1500);
    
    srand(time(0));
    bool player1Wins = rand()%2;
    
    gotoxy(45, 14);
    if(player1Wins){
        Color::set(12);
        cout << player1 << " WINS TOSS!";
    } else {
        Color::set(9);
        cout << player2 << " WINS TOSS!";
    }
    
    gotoxy(48, 16);
    Color::set(8);
    cout << "Press any key to continue...";
    _getch();
    
    return player1Wins;
}

// ================= ENHANCED TIC TAC TOE =================
class TicTacToeGame {
private:
    vector<char> board;
    string player1, player2;
    bool currentTurn; // true = player1, false = player2/AI
    int cursorPos;
    bool vsAI;
    int winLength;
    
    // For optimized drawing
    bool firstDraw;
    int lastCursorPos;
    
public:
    TicTacToeGame() {
        winLength = (tttSettings.gridSize == 3) ? 3 : 4;
        firstDraw = true;
        lastCursorPos = -1;
    }
    
    void initializeGame(){
        board.clear();
        board.resize(tttSettings.gridSize * tttSettings.gridSize, ' ');
        cursorPos = 0;
        currentTurn = true;
        firstDraw = true;
        lastCursorPos = -1;
    }
    
    void setPlayers(string p1, string p2, bool aiMode){
        player1 = p1;
        player2 = p2;
        vsAI = aiMode;
        initializeGame();
    }
    
    void drawStaticElements(bool forceRedraw = false){
        if(firstDraw || forceRedraw){
            clearScreen();
            
            if(!vsAI) {
                drawSplitScreen();
                // Left panel for Player 1
                drawBox(5, 3, 55, 25, tttSettings.uiStyle, 12);
                // Right panel for Player 2
                drawBox(65, 3, 115, 25, tttSettings.uiStyle, 9);
                gotoxy(50, 5);
                Color::set(14);
                cout << "TIC TAC TOE " << tttSettings.gridSize << "x" << tttSettings.gridSize;
            } else {
                drawBox(5, 3, 115, 25, tttSettings.uiStyle, 11);
                gotoxy(50, 5);
                Color::set(14);
                cout << "TIC TAC TOE " << tttSettings.gridSize << "x" << tttSettings.gridSize;
            }
            
            // Draw grid (static part)
            drawGrid(false);
            
            firstDraw = false;
        }
    }
    
    void drawGrid(bool updateCellsOnly = false){
        int gridStartX, gridStartY;
        int cellWidth = 4;
        int cellHeight = 2;
        
        if(!vsAI) {
            gridStartX = 60 - ((tttSettings.gridSize * cellWidth) / 2);
            gridStartY = 13 - ((tttSettings.gridSize * cellHeight) / 2);
        } else {
            gridStartX = 60 - ((tttSettings.gridSize * cellWidth) / 2);
            gridStartY = 13 - ((tttSettings.gridSize * cellHeight) / 2);
        }
        
        if(!updateCellsOnly){
            // Draw grid lines
            Color::set(8);
            for(int i=0; i<=tttSettings.gridSize; i++){
                // Horizontal lines
                for(int j=0; j<tttSettings.gridSize*cellWidth; j++){
                    gotoxy(gridStartX + j, gridStartY + i*cellHeight);
                    if(i < tttSettings.gridSize && i > 0) cout << "-";
                }
            }
            
            for(int i=0; i<=tttSettings.gridSize; i++){
                // Vertical lines
                for(int j=0; j<=tttSettings.gridSize*cellHeight; j++){
                    gotoxy(gridStartX + i*cellWidth, gridStartY + j);
                    if(i < tttSettings.gridSize && i > 0 && j%cellHeight==0) cout << "|";
                }
            }
        }
        
        // Update cells
        updateGridCells();
    }
    
    void updateGridCells(){
        int gridStartX, gridStartY;
        int cellWidth = 4;
        int cellHeight = 2;
        
        if(!vsAI) {
            gridStartX = 60 - ((tttSettings.gridSize * cellWidth) / 2);
            gridStartY = 13 - ((tttSettings.gridSize * cellHeight) / 2);
        } else {
            gridStartX = 60 - ((tttSettings.gridSize * cellWidth) / 2);
            gridStartY = 13 - ((tttSettings.gridSize * cellHeight) / 2);
        }
        
        // Update cursor position
        if(lastCursorPos != -1 && lastCursorPos != cursorPos){
            // Clear previous cursor
            int prevRow = lastCursorPos / tttSettings.gridSize;
            int prevCol = lastCursorPos % tttSettings.gridSize;
            int prevX = gridStartX + prevCol*cellWidth + 2;
            int prevY = gridStartY + prevRow*cellHeight + 1;
            
            gotoxy(prevX, prevY);
            cout << " ";
            gotoxy(prevX + 2, prevY);
            cout << " ";
        }
        
        // Draw X and O with cursor
        for(int row=0; row<tttSettings.gridSize; row++){
            for(int col=0; col<tttSettings.gridSize; col++){
                int index = row * tttSettings.gridSize + col;
                int x = gridStartX + col*cellWidth + 2;
                int y = gridStartY + row*cellHeight + 1;
                
                gotoxy(x, y);
                if(index == cursorPos){
                    Color::set(15);
                    cout << "[";
                } else {
                    cout << " ";
                }
                
                if(board[index] == 'X'){
                    Color::set(12);
                    cout << "X";
                } else if(board[index] == 'O'){
                    Color::set(9);
                    cout << "O";
                } else {
                    Color::set(7);
                    cout << " ";
                }
                
                if(index == cursorPos){
                    Color::set(15);
                    cout << "]";
                } else {
                    cout << " ";
                }
            }
        }
        
        lastCursorPos = cursorPos;
        
        // Update player info (minimal updates)
        updatePlayerInfo();
    }
    
    void updatePlayerInfo(){
        // Player info
        if(!vsAI) {
            // Multiplayer - split screen info
            gotoxy(15, 20);
            Color::set(12);
            cout << player1 << " (X)";
            
            gotoxy(85, 20);
            Color::set(9);
            cout << player2 << " (O)";
            
            // Highlight current player
            if(currentTurn) {
                gotoxy(15, 22);
                Color::set(15);
                cout << "YOUR TURN!      ";
                gotoxy(85, 22);
                Color::set(8);
                cout << "Waiting...      ";
            } else {
                gotoxy(15, 22);
                Color::set(8);
                cout << "Waiting...      ";
                gotoxy(85, 22);
                Color::set(15);
                cout << "YOUR TURN!      ";
            }
        } else {
            // VS AI info
            gotoxy(20, 20);
            Color::set(12);
            cout << player1 << " (X)";
            
            gotoxy(50, 20);
            Color::set(13);
            cout << "COMPUTER (O)";
            
            // Current turn
            gotoxy(90, 20);
            Color::set(11);
            cout << "Turn: ";
            if(currentTurn){
                Color::set(12);
                cout << player1 << "      ";
            } else {
                Color::set(13);
                cout << "COMPUTER      ";
            }
        }
        
        // Settings info
        gotoxy(20, 22);
        Color::set(11);
        cout << "Difficulty: ";
        switch(tttSettings.difficulty){
            case 1: Color::set(10); cout << "EASY      "; break;
            case 2: Color::set(14); cout << "MEDIUM    "; break;
            case 3: Color::set(12); cout << "HARD      "; break;
        }
        
        gotoxy(50, 22);
        Color::set(11);
        cout << "Grid: " << tttSettings.gridSize << "x" << tttSettings.gridSize;
        
        // Controls
        gotoxy(20, 24);
        Color::set(8);
        cout << "CONTROLS: Arrow Keys = Move | Enter = Place | ESC = Exit";
        
        // If AI's turn, show thinking message
        if(vsAI && !currentTurn){
            gotoxy(90, 22);
            Color::set(13);
            cout << "COMPUTER thinking...";
        }
    }
    
    void drawGameScreen(){
        drawStaticElements();
    }
    
    int makeAIMove(){
        // Easy: Random moves
        if(tttSettings.difficulty == 1){
            vector<int> emptyCells;
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' ') emptyCells.push_back(i);
            }
            if(!emptyCells.empty()){
                return emptyCells[rand() % emptyCells.size()];
            }
        }
        
        // Medium: Try to win, then block, then random
        if(tttSettings.difficulty == 2){
            // Try to win
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' '){
                    board[i] = 'O';
                    if(checkWinner() == 'O'){
                        board[i] = ' ';
                        return i;
                    }
                    board[i] = ' ';
                }
            }
            
            // Block player
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' '){
                    board[i] = 'X';
                    if(checkWinner() == 'X'){
                        board[i] = ' ';
                        return i;
                    }
                    board[i] = ' ';
                }
            }
            
            // Take center if available
            if(tttSettings.gridSize % 2 == 1){
                int center = (tttSettings.gridSize/2) * tttSettings.gridSize + (tttSettings.gridSize/2);
                if(board[center] == ' ') return center;
            }
            
            // Random move
            vector<int> emptyCells;
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' ') emptyCells.push_back(i);
            }
            if(!emptyCells.empty()){
                return emptyCells[rand() % emptyCells.size()];
            }
        }
        
        // Hard: Advanced strategy
        if(tttSettings.difficulty == 3){
            // For larger grids, use a strategic approach
            // Priority: win > block > center > corners > edges
            
            // 1. Try to win
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' '){
                    board[i] = 'O';
                    if(checkWinner() == 'O'){
                        board[i] = ' ';
                        return i;
                    }
                    board[i] = ' ';
                }
            }
            
            // 2. Block player
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' '){
                    board[i] = 'X';
                    if(checkWinner() == 'X'){
                        board[i] = ' ';
                        return i;
                    }
                    board[i] = ' ';
                }
            }
            
            // 3. Take center
            if(tttSettings.gridSize % 2 == 1){
                int center = (tttSettings.gridSize/2) * tttSettings.gridSize + (tttSettings.gridSize/2);
                if(board[center] == ' ') return center;
            }
            
            // 4. Take corners
            vector<int> corners;
            if(tttSettings.gridSize == 3){
                corners = {0, 2, 6, 8};
            } else if(tttSettings.gridSize == 4){
                corners = {0, 3, 12, 15};
            } else if(tttSettings.gridSize == 5){
                corners = {0, 4, 20, 24};
            }
            
            for(int corner : corners){
                if(board[corner] == ' ') return corner;
            }
            
            // 5. Random move
            vector<int> emptyCells;
            for(int i=0; i<board.size(); i++){
                if(board[i] == ' ') emptyCells.push_back(i);
            }
            if(!emptyCells.empty()){
                return emptyCells[rand() % emptyCells.size()];
            }
        }
        
        return -1;
    }
    
    char checkWinner(){
        // Check rows
        for(int row=0; row<tttSettings.gridSize; row++){
            for(int col=0; col<=tttSettings.gridSize-winLength; col++){
                bool win = true;
                char first = board[row*tttSettings.gridSize + col];
                if(first == ' ') continue;
                
                for(int k=1; k<winLength; k++){
                    if(board[row*tttSettings.gridSize + col + k] != first){
                        win = false;
                        break;
                    }
                }
                if(win) return first;
            }
        }
        
        // Check columns
        for(int col=0; col<tttSettings.gridSize; col++){
            for(int row=0; row<=tttSettings.gridSize-winLength; row++){
                bool win = true;
                char first = board[row*tttSettings.gridSize + col];
                if(first == ' ') continue;
                
                for(int k=1; k<winLength; k++){
                    if(board[(row+k)*tttSettings.gridSize + col] != first){
                        win = false;
                        break;
                    }
                }
                if(win) return first;
            }
        }
        
        // Check diagonals (top-left to bottom-right)
        for(int row=0; row<=tttSettings.gridSize-winLength; row++){
            for(int col=0; col<=tttSettings.gridSize-winLength; col++){
                bool win = true;
                char first = board[row*tttSettings.gridSize + col];
                if(first == ' ') continue;
                
                for(int k=1; k<winLength; k++){
                    if(board[(row+k)*tttSettings.gridSize + col + k] != first){
                        win = false;
                        break;
                    }
                }
                if(win) return first;
            }
        }
        
        // Check diagonals (top-right to bottom-left)
        for(int row=0; row<=tttSettings.gridSize-winLength; row++){
            for(int col=winLength-1; col<tttSettings.gridSize; col++){
                bool win = true;
                char first = board[row*tttSettings.gridSize + col];
                if(first == ' ') continue;
                
                for(int k=1; k<winLength; k++){
                    if(board[(row+k)*tttSettings.gridSize + col - k] != first){
                        win = false;
                        break;
                    }
                }
                if(win) return first;
            }
        }
        
        // Check for draw
        for(char c : board){
            if(c == ' ') return ' ';
        }
        
        return 'D';
    }
    
    void play(){
        hideCursor();
        initializeGame();
        
        // Do toss for who goes first
        bool player1WinsToss;
        if(vsAI) {
            player1WinsToss = toss(player1, "COMPUTER");
        } else {
            player1WinsToss = toss(player1, player2);
        }
        currentTurn = player1WinsToss;
        
        // Show who goes first
        clearScreen();
        drawBox(40, 10, 80, 16, tttSettings.uiStyle, 10);
        gotoxy(45, 12);
        if(currentTurn) {
            Color::set(10);
            cout << player1 << " goes first! (X)";
        } else {
            if(vsAI) {
                Color::set(13);
                cout << "COMPUTER goes first! (O)";
            } else {
                Color::set(9);
                cout << player2 << " goes first! (O)";
            }
        }
        gotoxy(48, 14);
        Color::set(8);
        cout << "Press any key to start...";
        _getch();
        
        // Initial draw
        drawGameScreen();
        
        while(true){
            // AI's turn
            if(vsAI && !currentTurn){
                sleep_ms(1000); // Thinking time
                int aiMove = makeAIMove();
                if(aiMove != -1 && board[aiMove] == ' '){
                    board[aiMove] = 'O';
                    currentTurn = !currentTurn;
                    updateGridCells();
                }
            } else {
                // Player's turn
                if(kbhit()){
                    char key = _getch();
                    
                    if(key == -32){
                        key = _getch();
                        if(key == 72 && cursorPos >= tttSettings.gridSize) // Up
                            cursorPos -= tttSettings.gridSize;
                        if(key == 80 && cursorPos < tttSettings.gridSize*(tttSettings.gridSize-1)) // Down
                            cursorPos += tttSettings.gridSize;
                        if(key == 75 && cursorPos % tttSettings.gridSize != 0) // Left
                            cursorPos--;
                        if(key == 77 && cursorPos % tttSettings.gridSize != tttSettings.gridSize-1) // Right
                            cursorPos++;
                        
                        updateGridCells();
                    }
                    else if(key == 13){ // Enter
                        if(board[cursorPos] == ' '){
                            board[cursorPos] = currentTurn ? 'X' : 'O';
                            currentTurn = !currentTurn;
                            updateGridCells();
                        }
                    }
                    else if(key == 27){ // ESC
                        showCursor();
                        return;
                    }
                }
            }
            
            char winner = checkWinner();
            if(winner != ' '){
                showResult(winner);
                showCursor();
                return;
            }
            
            sleep_ms(50); // Small delay to reduce CPU usage
        }
    }
    
    void showResult(char winner){
        clearScreen();
        drawBox(30, 10, 90, 20, tttSettings.uiStyle, 14);
        
        gotoxy(45, 12);
        
        if(winner == 'X'){
            Color::set(12);
            cout << "+--------------------+";
            gotoxy(45, 13);
            cout << "¦      PLAYER 1      ¦";
            gotoxy(45, 14);
            cout << "¦      WINS! ??      ¦";
            gotoxy(45, 15);
            cout << "+--------------------+";
            
            gotoxy(50, 17);
            cout << player1 << " WINS!";
            
            saveToHistory("Tic Tac Toe " + to_string(tttSettings.gridSize) + "x" + to_string(tttSettings.gridSize), 
                         player1 + " beat " + (vsAI ? "COMPUTER" : player2));
        }
        else if(winner == 'O'){
            Color::set(vsAI ? 13 : 9);
            cout << "+--------------------+";
            gotoxy(45, 13);
            cout << "¦      PLAYER 2      ¦";
            gotoxy(45, 14);
            cout << "¦      WINS! ??      ¦";
            gotoxy(45, 15);
            cout << "+--------------------+";
            
            gotoxy(50, 17);
            if(vsAI) cout << "COMPUTER WINS!";
            else cout << player2 << " WINS!";
            
            saveToHistory("Tic Tac Toe " + to_string(tttSettings.gridSize) + "x" + to_string(tttSettings.gridSize), 
                         (vsAI ? "COMPUTER" : player2) + " beat " + player1);
        }
        else{ // Draw
            gotoxy(48, 12);
            Color::set(14);
            cout << "+------------------+";
            gotoxy(48, 13);
            cout << "¦      DRAW! ??     ¦";
            gotoxy(48, 14);
            cout << "+------------------+";
            
            gotoxy(52, 16);
            cout << "No Winner!";
            
            saveToHistory("Tic Tac Toe " + to_string(tttSettings.gridSize) + "x" + to_string(tttSettings.gridSize), "DRAW");
        }
        
        gotoxy(45, 19);
        Color::set(8);
        cout << "Press any key to continue...";
        _getch();
    }
};

// ================= ENHANCED WORD GUESS =================
class WordGuessGame {
private:
    vector<string> generalWords = {
        "APPLE", "HOUSE", "TABLE", "CHAIR", "PHONE", "WATER", "SMILE", "BRAIN",
        "CLOUD", "EARTH", "FRUIT", "GRASS", "HEART", "JUICE", "LIGHT", "MONEY",
        "NIGHT", "OCEAN", "PAPER", "QUEEN", "RIVER", "SNAKE", "TIGER", "UNITY",
        "VOICE", "WATCH", "YOUTH", "ZEBRA", "BREAD", "CHAIR", "DREAM", "FLOWER"
    };
    
    vector<string> techWords = {
        "LAPTOP", "MOBILE", "SERVER", "ROUTER", "CABLE", "SCREEN", "KEYBOARD",
        "MOUSE", "PRINTER", "SCANNER", "MEMORY", "STORAGE", "NETWORK", "WIFI",
        "BLUETOOTH", "BATTERY", "CHARGER", "CAMERA", "SENSOR", "SPEAKER",
        "MICROPHONE", "MONITOR", "PROCESSOR", "GRAPHICS", "SOFTWARE", "HARDWARE"
    };
    
    vector<string> entertainmentWords = {
        "MOVIE", "MUSIC", "DANCE", "SINGER", "ACTOR", "COMEDY", "DRAMA",
        "THEATER", "CINEMA", "RADIO", "TVSHOW", "CONCERT", "FESTIVAL",
        "GAMING", "CARTOON", "ANIME", "SERIES", "EPISODE", "SEASON",
        "AWARD", "TICKET", "STAGE", "SCREEN", "SCRIPT", "DIRECTOR"
    };
    
    // For optimized drawing
    bool firstDraw;
    string lastHidden;
    int lastChances;
    vector<char> lastGuessedLetters;
    
    vector<string> getWordList(){
        if(wgSettings.category == "TECHNOLOGY") return techWords;
        if(wgSettings.category == "ENTERTAINMENT") return entertainmentWords;
        return generalWords;
    }
    
public:
    WordGuessGame() {
        firstDraw = true;
        lastChances = -1;
    }
    
    void playMultiplayer(){
        clearScreen();
        
        // Split screen for multiplayer
        drawSplitScreen();
        
        // Player 1 enters word
        drawBox(5, 3, 55, 15, wgSettings.uiStyle, 12);
        gotoxy(20, 5);
        Color::set(12);
        cout << "PLAYER 1 - ENTER WORD";
        
        gotoxy(10, 8);
        Color::set(11);
        cout << "Enter word (any case): ";
        Color::set(7);
        hideCursor();
        
        string word;
        char ch;
        int pos = 0;
        
        // Hide input for word entry
        while(true) {
            ch = _getch();
            if(ch == 13) break; // Enter
            if(ch == 27) {
                showCursor();
                return;
            }
            if(ch == 8 && pos > 0) { // Backspace
                pos--;
                word = word.substr(0, pos);
                gotoxy(33 + pos, 8);
                cout << " ";
                gotoxy(33 + pos, 8);
            }
            else if(ch >= 'a' && ch <= 'z' && pos < 20) {
                word += toupper(ch);
                gotoxy(33 + pos, 8);
                cout << "*";
                pos++;
            }
            else if(ch >= 'A' && ch <= 'Z' && pos < 20) {
                word += ch;
                gotoxy(33 + pos, 8);
                cout << "*";
                pos++;
            }
        }
        showCursor();
        
        // Show word was entered
        gotoxy(10, 10);
        Color::set(10);
        cout << "Word saved! Player 2's turn to guess.";
        sleep_ms(1500);
        
        // Clear Player 2's screen
        drawBox(65, 3, 115, 15, wgSettings.uiStyle, 9);
        gotoxy(80, 5);
        Color::set(9);
        cout << "PLAYER 2";
        gotoxy(70, 8);
        Color::set(11);
        cout << "Get ready to guess...";
        sleep_ms(1500);
        
        // Toss to decide who guesses first
        bool player1Guesses = toss("Player 1", "Player 2");
        
        // Now play the game
        if(player1Guesses){
            playGame(word, "Player 1", "Player 2", false);
        } else {
            playGame(word, "Player 2", "Player 1", false);
        }
    }
    
    void playVsAI(){
        clearScreen();
        drawBox(30, 5, 90, 15, wgSettings.uiStyle, 13);
        
        gotoxy(40, 7);
        Color::set(13);
        cout << "WORD GUESS - VS AI";
        
        gotoxy(35, 9);
        Color::set(11);
        cout << "1. You think, AI guesses";
        gotoxy(35, 11);
        cout << "2. AI thinks, You guess";
        
        gotoxy(35, 13);
        Color::set(14);
        cout << "Choice: ";
        Color::set(7);
        
        char mode;
        cin >> mode;
        cin.ignore();
        
        if(mode == '1'){
            // Player thinks, AI guesses
            clearScreen();
            drawBox(30, 5, 90, 15, wgSettings.uiStyle, 10);
            gotoxy(40, 7);
            Color::set(10);
            cout << "ENTER YOUR WORD";
            
            gotoxy(35, 10);
            Color::set(11);
            cout << "Word (any case): ";
            Color::set(7);
            
            string word;
            getline(cin, word);
            transform(word.begin(), word.end(), word.begin(), ::toupper);
            
            // Toss for who guesses first
            bool playerGuesses = toss("COMPUTER", "Player");
            
            if(playerGuesses) {
                playGame(word, "COMPUTER", "Player", true);
            } else {
                playGame(word, "COMPUTER", "Player", true);
            }
        } else {
            // AI thinks, Player guesses
            vector<string> wordList = getWordList();
            srand(time(0));
            string word = wordList[rand() % wordList.size()];
            
            // Toss for who guesses first
            bool playerGuesses = toss("Player", "COMPUTER");
            
            if(playerGuesses) {
                playGame(word, "Player", "COMPUTER", true);
            } else {
                playGame(word, "Player", "COMPUTER", true);
            }
        }
    }
    
    void drawWordGuessScreen(string hidden, int chances, vector<char> guessedLetters, 
                            string guesser, string opponent, bool vsAI, string word = "") {
        
        if(firstDraw) {
            clearScreen();
            
            if(!vsAI) {
                drawSplitScreen();
                // Left panel for guesser
                if(guesser == "Player 1") {
                    drawBox(5, 3, 55, 25, wgSettings.uiStyle, 12);
                    gotoxy(20, 5);
                    Color::set(12);
                    cout << guesser << " - GUESSING";
                } else {
                    drawBox(65, 3, 115, 25, wgSettings.uiStyle, 9);
                    gotoxy(80, 5);
                    Color::set(9);
                    cout << guesser << " - GUESSING";
                }
            } else {
                drawBox(5, 3, 115, 25, wgSettings.uiStyle, 11);
                gotoxy(50, 5);
                Color::set(14);
                cout << "WORD GUESS GAME";
            }
            
            firstDraw = false;
        }
        
        // Update only changed parts
        if(lastHidden != hidden || lastChances != chances) {
            // Word display - centered
            if(!vsAI) {
                // Center between both panels
                gotoxy(40, 9);
                Color::set(11);
                cout << "Word: ";
                Color::set(10);
                for(int i=0; i<hidden.length(); i++){
                    cout << hidden[i] << " ";
                }
                // Clear any leftover characters
                for(int i=hidden.length(); i<15; i++) {
                    cout << "  ";
                }
            } else {
                gotoxy(40, 9);
                Color::set(11);
                cout << "Category: ";
                Color::set(10);
                cout << wgSettings.category << "      ";
                
                gotoxy(40, 11);
                Color::set(11);
                cout << "Word: ";
                Color::set(10);
                for(int i=0; i<hidden.length(); i++){
                    cout << hidden[i] << " ";
                }
                for(int i=hidden.length(); i<15; i++) {
                    cout << "  ";
                }
            }
            
            // Chances
            if(!vsAI) {
                gotoxy(40, 11);
            } else {
                gotoxy(40, 13);
            }
            Color::set(11);
            cout << "Chances left: ";
            Color::set(chances > 5 ? 10 : (chances > 2 ? 14 : 12));
            cout << chances << "   ";
            Color::set(11);
            cout << " (";
            for(int i=0; i<chances; i++) cout << "? ";
            for(int i=chances; i<10; i++) cout << "  ";
            cout << ")";
            
            lastHidden = hidden;
            lastChances = chances;
        }
        
        // Update guessed letters if changed
        if(guessedLetters != lastGuessedLetters) {
            if(!vsAI) {
                gotoxy(40, 13);
            } else {
                gotoxy(40, 15);
            }
            Color::set(11);
            cout << "Guessed letters: ";
            for(int i=0; i<min((int)guessedLetters.size(), 10); i++){
                Color::set(8);
                cout << guessedLetters[i] << " ";
            }
            for(int i=guessedLetters.size(); i<10; i++) {
                cout << "  ";
            }
            
            lastGuessedLetters = guessedLetters;
        }
        
        // Player info (static)
        if(!vsAI) {
            gotoxy(10, 18);
            Color::set(12);
            cout << "Guesser: " << guesser << "      ";
            
            gotoxy(70, 18);
            Color::set(9);
            cout << "Opponent: " << opponent << "      ";
        } else {
            gotoxy(20, 18);
            Color::set(12);
            cout << "Guesser: " << guesser << "      ";
            
            gotoxy(60, 18);
            Color::set(9);
            cout << "Opponent: " << opponent << "      ";
        }
        
        // Settings info (static)
        if(!vsAI) {
            gotoxy(10, 20);
            Color::set(11);
            cout << "Difficulty: ";
            switch(wgSettings.difficulty){
                case 1: Color::set(10); cout << "EASY      "; break;
                case 2: Color::set(14); cout << "MEDIUM    "; break;
                case 3: Color::set(12); cout << "HARD      "; break;
            }
            
            gotoxy(70, 20);
            Color::set(11);
            cout << "Category: " << wgSettings.category << "      ";
        } else {
            gotoxy(20, 20);
            Color::set(11);
            cout << "Difficulty: ";
            switch(wgSettings.difficulty){
                case 1: Color::set(10); cout << "EASY      "; break;
                case 2: Color::set(14); cout << "MEDIUM    "; break;
                case 3: Color::set(12); cout << "HARD      "; break;
            }
            
            gotoxy(60, 20);
            Color::set(11);
            cout << "Category: " << wgSettings.category << "      ";
        }
        
        // Word length (static)
        if(!vsAI) {
            gotoxy(40, 22);
        } else {
            gotoxy(90, 22);
        }
        Color::set(11);
        cout << "Length: " << word.length() << " letters      ";
    }
    
    void playGame(string word, string guesser, string opponent, bool vsAI){
        string hidden(word.length(), '_');
        int chances = word.length() + 3;
        vector<char> guessedLetters;
        bool hintUsed = false;
        
        firstDraw = true;
        lastHidden = "";
        lastChances = -1;
        lastGuessedLetters.clear();
        
        hideCursor();
        
        // Initial draw
        drawWordGuessScreen(hidden, chances, guessedLetters, guesser, opponent, vsAI, word);
        
        while(chances > 0 && hidden != word){
            
            // If AI's turn
            if(vsAI && guesser == "COMPUTER"){
                // AI's turn to guess
                gotoxy(40, 24);
                Color::set(14);
                cout << "AI is making a guess...                      ";
                
                sleep_ms(1500);
                
                char aiGuess = getAIGuess(guessedLetters, word);
                guessedLetters.push_back(aiGuess);
                
                // Check if correct
                bool correct = false;
                for(int i=0; i<word.length(); i++){
                    if(word[i] == aiGuess){
                        hidden[i] = aiGuess;
                        correct = true;
                    }
                }
                
                if(!correct) chances--;
                
                // Update display
                drawWordGuessScreen(hidden, chances, guessedLetters, guesser, opponent, vsAI, word);
                
                // Show result
                gotoxy(40, 25);
                Color::set(13);
                cout << "AI guessed: " << aiGuess;
                if(correct){
                    Color::set(10);
                    cout << " (Correct!)                        ";
                } else {
                    Color::set(12);
                    cout << " (Wrong!)                          ";
                }
                sleep_ms(2000);
                
                // Clear message
                gotoxy(40, 25);
                cout << "                                        ";
            } else {
                // Player's turn
                gotoxy(40, 24);
                Color::set(14);
                cout << "Your guess (A-Z or ? for hint): ";
                Color::set(7);
                
                showCursor();
                char guess;
                cin >> guess;
                cin.ignore();
                hideCursor();
                
                // Handle hint request
                if(guess == '?'){
                    if(!hintUsed){
                        for(int i=0; i<word.length(); i++){
                            if(hidden[i] == '_'){
                                hidden[i] = word[i];
                                break;
                            }
                        }
                        chances -= 2;
                        hintUsed = true;
                        
                        // Update display
                        drawWordGuessScreen(hidden, chances, guessedLetters, guesser, opponent, vsAI, word);
                        continue;
                    }
                }
                
                guess = toupper(guess);
                
                // Validate input
                if(guess < 'A' || guess > 'Z'){
                    gotoxy(40, 25);
                    Color::set(12);
                    cout << "Invalid! Use A-Z or ? for hint              ";
                    sleep_ms(1500);
                    gotoxy(40, 25);
                    cout << "                                        ";
                    continue;
                }
                
                // Check if already guessed
                if(find(guessedLetters.begin(), guessedLetters.end(), guess) != guessedLetters.end()){
                    gotoxy(40, 25);
                    Color::set(14);
                    cout << "Already guessed!                            ";
                    sleep_ms(1500);
                    gotoxy(40, 25);
                    cout << "                                        ";
                    continue;
                }
                
                guessedLetters.push_back(guess);
                
                // Check if correct
                bool correct = false;
                for(int i=0; i<word.length(); i++){
                    if(word[i] == guess){
                        hidden[i] = guess;
                        correct = true;
                    }
                }
                
                if(!correct) chances--;
                
                // Update display
                drawWordGuessScreen(hidden, chances, guessedLetters, guesser, opponent, vsAI, word);
                
                // Show result briefly
                gotoxy(40, 25);
                if(!correct){
                    Color::set(12);
                    cout << "Wrong guess!                                ";
                } else {
                    Color::set(10);
                    cout << "Good guess!                                 ";
                }
                sleep_ms(1500);
                gotoxy(40, 25);
                cout << "                                        ";
            }
        }
        
        showCursor();
        
        // Show result
        showWordResult(hidden == word, word, guesser, opponent, vsAI);
    }
    
    char getAIGuess(vector<char> guessedLetters, string word){
        // Easy: Random guess
        if(wgSettings.difficulty == 1){
            char guess;
            do {
                guess = 'A' + rand() % 26;
            } while(find(guessedLetters.begin(), guessedLetters.end(), guess) != guessedLetters.end());
            return guess;
        }
        
        // Medium: Common letters first
        if(wgSettings.difficulty == 2){
            string common = "ETAOINSHRDLCUMWFGYPBVKJXQZ";
            for(char c : common){
                if(find(guessedLetters.begin(), guessedLetters.end(), c) == guessedLetters.end()){
                    return c;
                }
            }
        }
        
        // Hard: Smart guessing based on word list
        if(wgSettings.difficulty == 3){
            vector<string> wordList = getWordList();
            vector<int> freq(26, 0);
            
            // Count frequency of letters in possible words
            for(const string& w : wordList){
                if(w.length() == word.length()){
                    for(char c : w){
                        if(find(guessedLetters.begin(), guessedLetters.end(), c) == guessedLetters.end()){
                            freq[c-'A']++;
                        }
                    }
                }
            }
            
            // Find most common unguessed letter
            int maxFreq = 0;
            char bestGuess = 'A';
            for(int i=0; i<26; i++){
                if(freq[i] > maxFreq && find(guessedLetters.begin(), guessedLetters.end(), 'A'+i) == guessedLetters.end()){
                    maxFreq = freq[i];
                    bestGuess = 'A' + i;
                }
            }
            
            return bestGuess;
        }
        
        // Fallback
        return 'A';
    }
    
    void showWordResult(bool won, string word, string guesser, string opponent, bool vsAI){
        clearScreen();
        drawBox(30, 10, 90, 20, wgSettings.uiStyle, 14);
        
        gotoxy(45, 12);
        
        if(won){
            Color::set(10);
            cout << "+--------------------+";
            gotoxy(45, 13);
            cout << "¦      VICTORY!      ¦";
            gotoxy(45, 14);
            cout << "+--------------------+";
            
            gotoxy(50, 16);
            cout << guesser << " WINS!";
            
            saveToHistory("Word Guess (" + wgSettings.category + ")", 
                         guesser + " guessed the word: " + word);
        } else {
            Color::set(12);
            cout << "+--------------------+";
            gotoxy(45, 13);
            cout << "¦      DEFEAT!       ¦";
            gotoxy(45, 14);
            cout << "+--------------------+";
            
            gotoxy(50, 16);
            cout << opponent << " WINS!";
            
            saveToHistory("Word Guess (" + wgSettings.category + ")", 
                         guesser + " failed, word was: " + word);
        }
        
        gotoxy(45, 18);
        Color::set(14);
        cout << "The word was: ";
        Color::set(11);
        cout << word;
        
        gotoxy(45, 20);
        Color::set(8);
        cout << "Press any key to continue...";
        _getch();
    }
};

// ================= MAIN MENU =================
void showMainMenu(){
    clearScreen();
    
    drawBox(30, 5, 90, 18, 2, 14);
    
    printCentered(7, "?? GAME ??", 14);
    
    vector<string> menuItems = {
        "1. Play Tic Tac Toe",
        "2. Play Word Guess",
        "3. Game History",
        "4. Exit"
    };
    
    for(int i=0; i<menuItems.size(); i++){
        gotoxy(40, 10 + i*2);
        Color::set(11);
        cout << menuItems[i];
    }
    
    gotoxy(40, 17);
    Color::set(8);
    cout << "Enter choice (1-4): ";
    Color::set(7);
}

// ================= MAIN FUNCTION =================
int main(){
    // Set console size
    system("mode con: cols=120 lines=30");
    system("title GAME");
    
    // Show loading screen
    showLoading();
    
    // Set random seed
    srand(time(0));
    
    TicTacToeGame tttGame;
    WordGuessGame wgGame;
    
    while(true){
        showMainMenu();
        
        char choice;
        cin >> choice;
        cin.ignore();
        
        if(choice == '4'){
            break;
        }
        
        switch(choice){
            case '1': // Tic Tac Toe
            {
                // Show Tic Tac Toe settings first
                showTicTacToeSettings();
                
                // Ask for game mode
                clearScreen();
                drawBox(30, 5, 90, 15, tttSettings.uiStyle, 11);
                
                gotoxy(40, 7);
                Color::set(11);
                cout << "TIC TAC TOE MODE";
                
                gotoxy(35, 9);
                Color::set(14);
                cout << "1. Play with Friend";
                gotoxy(35, 10);
                cout << "2. Play vs Computer";
                gotoxy(35, 11);
                cout << "3. Back to Main Menu";
                
                gotoxy(35, 13);
                Color::set(11);
                cout << "Choice: ";
                Color::set(7);
                
                char mode;
                cin >> mode;
                cin.ignore();
                
                if(mode == '3') break;
                
                // Get player names
                string p1, p2;
                bool vsAI = (mode == '2');
                
                if(vsAI){
                    clearScreen();
                    drawBox(30, 5, 90, 10, tttSettings.uiStyle, 10);
                    gotoxy(35, 7);
                    Color::set(10);
                    cout << "Enter your name: ";
                    Color::set(7);
                    getline(cin, p1);
                    p2 = "COMPUTER";
                } else {
                    clearScreen();
                    drawBox(30, 5, 90, 12, tttSettings.uiStyle, 11);
                    gotoxy(35, 7);
                    Color::set(11);
                    cout << "Player 1 name: ";
                    Color::set(7);
                    getline(cin, p1);
                    gotoxy(35, 9);
                    Color::set(11);
                    cout << "Player 2 name: ";
                    Color::set(7);
                    getline(cin, p2);
                }
                
                // Play game
                tttGame.setPlayers(p1, p2, vsAI);
                tttGame.play();
                break;
            }
            
            case '2': // Word Guess
            {
                // Show Word Guess settings first
                showWordGuessSettings();
                
                // Ask for game mode
                clearScreen();
                drawBox(30, 5, 90, 15, wgSettings.uiStyle, 13);
                
                gotoxy(40, 7);
                Color::set(13);
                cout << "WORD GUESS MODE";
                
                gotoxy(35, 9);
                Color::set(14);
                cout << "1. Play with Friend";
                gotoxy(35, 10);
                cout << "2. Play vs Computer";
                gotoxy(35, 11);
                cout << "3. Back to Main Menu";
                
                gotoxy(35, 13);
                Color::set(11);
                cout << "Choice: ";
                Color::set(7);
                
                char mode;
                cin >> mode;
                cin.ignore();
                
                if(mode == '3') break;
                
                // Play game
                if(mode == '1'){
                    wgGame.playMultiplayer();
                } else {
                    wgGame.playVsAI();
                }
                break;
            }
            
            case '3': // History
                showGameHistory();
                break;
        }
    }
    
    // Exit screen
    clearScreen();
    drawBox(35, 10, 85, 18, 2, 14);
    
    gotoxy(45, 12);
    Color::set(14);
    cout << "THANK YOU FOR PLAYING!";
    
    gotoxy(48, 14);
    Color::set(11);
    cout << "GAME";
    
    gotoxy(50, 16);
    Color::set(7);
    cout << "Goodbye!";
    
    sleep_ms(2000);
    return 0;
}
