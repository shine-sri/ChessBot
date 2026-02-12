// game.cpp - Single-file ChessBot
// Merged from main.cpp, chess.h, chess.cpp, player.cpp, path_node.cpp, bot.cpp

#include <iostream>
#include <string>
#include <forward_list>
#include <vector>
#include <map>
#include <algorithm>
#include <time.h>

// Platform-specific includes and functions
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else // macOS/Linux
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#endif

// --- Macros and Constants ---
#define BOARD_SIZE 8
#define BOX_WIDTH 10
#define DOWN 3
#define RIGHT 10
#define TO_DOWN std::string(DOWN, '\n')
#define TO_RIGHT std::string(RIGHT, ' ')
#define CLEAR_LINE std::string(100, ' ')
#define MOVES_PER_LINE 5

// --- Enums and Types ---
typedef enum {
    B_KING = -6, B_QUEEN, B_BISHOP, B_KNIGHT, B_ROOK, B_PAWN, EMPTY,
    W_KING, W_QUEEN, W_BISHOP, W_KNIGHT, W_ROOK, W_PAWN
} ChessPieces;

typedef enum {
    NORMAL, CASTLING, PROMOTION, EN_PASSANT
} Moves;

typedef enum {
    CHECKMATE, FIFTY_MOVES, THREEFOLD_REP, QUIT
} Endgame;

const char STARTING_BOARD[BOARD_SIZE][BOARD_SIZE] = {
    {B_ROOK, B_KNIGHT, B_BISHOP, B_QUEEN, B_KING, B_BISHOP, B_KNIGHT, B_ROOK},
    {B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN},
    {W_ROOK, W_KNIGHT, W_BISHOP, W_QUEEN, W_KING, W_BISHOP, W_KNIGHT, W_ROOK}
};

// --- Utility Functions ---
#ifdef _WIN32
void MoveCursorToXY(const short &x, const short &y) noexcept {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), (COORD){x, y});
}
#else
void MoveCursorToXY(const short &x, const short &y) noexcept {
    printf("\033[%d;%dH", y + 1, x + 1); // ANSI escape code, 1-based
    fflush(stdout);
}
#endif

std::string ToLowerString(std::string s) noexcept {
    std::transform(s.begin(), s.end(), s.begin(), [](const unsigned char &c){ return tolower(c); });
    return s;
}

// Cross-platform getch implementation
#ifdef _WIN32
// getch is available
#else
int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

template<class T> T GetRandomNumber(const T &min, const T &max) noexcept {
    return min + T(static_cast<double>(rand()) / static_cast<double>(RAND_MAX+1.0) * (max-min+1));
}

// --- Forward Declarations ---
class Chess;
class Player;
class PathNode;
class Bot;

// --- Player Class ---
class Player {
protected:
    std::string name;
    unsigned short score = 0;
    bool castling = true;
public:
    Player(const std::string &name) noexcept : name(name) {}
    std::string GetName() const noexcept { return name; }
    unsigned short GetScore() const noexcept { return score; }
    bool GetCastling() const noexcept { return castling; }
    void SetCastling(const bool &castling) noexcept { this->castling = castling; }
    void IncreaseScore(const unsigned short &inc) noexcept { score += inc; }
    void Reset() noexcept { score = 0; castling = true; }
    bool operator== (const Player &p) const noexcept { return !name.compare(p.name); }
};

// --- PathNode Class ---
class PathNode {
private:
    std::map<std::string, PathNode> child_node_list;
    void CreateSubtree(Chess &c) noexcept;
    float AlphaBeta(Chess &c, unsigned short &depth, float alpha, float beta, const bool &maximizing_player, const bool &initial_turn) noexcept;
public:
    std::string AlphaBetaRoot(Chess &c, unsigned short &difficulty) noexcept;
};

// --- Bot Class ---
class Bot : public Player {
private:
    PathNode root;
    unsigned short difficulty;
public:
    Bot(const std::string &name, const unsigned short &difficulty) noexcept : Player(name), difficulty(difficulty) {}
    unsigned short GetDifficulty() const noexcept { return difficulty; }
    std::string GetIdealMove(Chess &c) noexcept { return root.AlphaBetaRoot(c, difficulty); }
    std::string GetIdealMove(Chess &c, unsigned short difficulty) noexcept { return root.AlphaBetaRoot(c, difficulty); }
    bool operator== (const Bot &b) const noexcept { return !name.compare(b.name); }
};

// --- Chess Class Declaration (Implementation Follows) ---
class Chess {
private:
    char board[BOARD_SIZE][BOARD_SIZE];
    Bot white, black;
    std::vector<std::pair<Moves, std::string>> all_game_moves;
    bool whites_turn = true;
    unsigned short moves_after_last_pawn_move_or_capture = 0;
    bool white_bot_random;
    bool black_bot_random;
    static bool WithinBounds(const short &coord) noexcept;
    static void ChangeToString(char &x1, char &y1, char &x2, char &y2) noexcept;
    static std::string ToString(const short &x1, const short &y1, const short &x2, const short &y2) noexcept;
    static std::string PieceNameToString(const char &piece) noexcept;
    static float EvaluatePiece(const char &piece) noexcept;
    static void ClearAllMoves(const unsigned short &n) noexcept;
    static void PrintSeparator(const char &ch) noexcept;
    static void CopyBoard(const char from[BOARD_SIZE][BOARD_SIZE], char to[BOARD_SIZE][BOARD_SIZE]) noexcept;
    static bool AreBoardsEqual(const char board1[BOARD_SIZE][BOARD_SIZE], const char board2[BOARD_SIZE][BOARD_SIZE]) noexcept;
    static bool CanMovePiece(const short &x1, const short &y1, const short &x2, const short &y2, const std::forward_list<std::string> &all_moves) noexcept;
    Bot& GetCurrentPlayer() noexcept;
    Bot GetCurrentPlayerConst() const noexcept;
    Bot& GetOtherPlayer() noexcept;
    Bot GetOtherPlayerConst() const noexcept;
    void ChangeTurn() noexcept;
    void AppendToAllGameMoves(const short &x1, const short &y1, const short &x2, const short &y2) noexcept;
    void Reset() noexcept;
    void CheckCoordinates(const short &x, const short &y, const std::string &func_name) const noexcept(false);
    bool EndGameText(const unsigned short &n, const Endgame &end_game) const noexcept;
    short GetEnPassant(const short &x, const short &y) const noexcept;
    template<class Iterator> short GetEnPassant(const char board[BOARD_SIZE][BOARD_SIZE], const Iterator &it) const noexcept;
    bool ThreefoldRepetition() const noexcept;
    bool IsCheck(const bool &turn) const noexcept;
    bool IsCheck(std::string &move) noexcept;
    std::forward_list<std::string> PawnMoves(const short &x, const short &y) const noexcept;
    std::forward_list<std::string> RookMoves(const short &x, const short &y) const noexcept;
    std::forward_list<std::string> KnightMoves(const short &x, const short &y) const noexcept;
    std::forward_list<std::string> BishopMoves(const short &x, const short &y) const noexcept;
    std::forward_list<std::string> QueenMoves(const short &x, const short &y) const noexcept;
    std::forward_list<std::string> KingMoves(const short &x, const short &y) const noexcept;
    std::string GetRandomMove() noexcept;
    void ManuallyPromotePawn(const short &x, const short &y) noexcept;
    void UpdateBoard(const short &x, const short &y) const noexcept;
    void UpdateScore(const Bot &p) const noexcept;
    float EvaluatePosition(const short &x, const short &y) const noexcept;
    void PrintAllMovesMadeInOrder() const noexcept;
    bool CheckEndgame(const unsigned short &n = 0) noexcept;
public:
    Chess(const std::string &player1, const unsigned short &difficulty1, const std::string &player2, const unsigned short &difficulty2, bool white_bot_random = false, bool black_bot_random = false) noexcept;
    static void ChangeToRealCoordinates(char &x1, char &y1, char &x2, char &y2) noexcept;
    char GetPiece(const short &x, const short &y) const noexcept;
    bool GetTurn() const noexcept;
    std::forward_list<std::string> AllMoves() noexcept;
    void MovePiece(const short &x1, const short &y1, const short &x2, const short &y2, const bool &manual_promotion, const bool &update_board) noexcept;
    void MovePieceBack(const short &x1, const short &y1, const short &x2, const short &y2) noexcept;
    float EvaluateBoard(const bool &turn) const noexcept;
    void PrintBoard() const noexcept;
    bool PlayersTurn() noexcept;
    bool BotsTurn() noexcept;
    bool GameOver() noexcept;
};

// --- PathNode Implementation ---
void PathNode::CreateSubtree(Chess &c) noexcept {
    auto all_moves = c.AllMoves();
    for(auto &move : all_moves) {
        Chess::ChangeToRealCoordinates(move[0], move[1], move[2], move[3]);
        child_node_list.emplace(move, PathNode());
    }
}

float PathNode::AlphaBeta(Chess &c, unsigned short &depth, float alpha, float beta, const bool &maximizing_player, const bool &initial_turn) noexcept {
    if(!depth)
        return c.EvaluateBoard(initial_turn);
    CreateSubtree(c);
    float points = maximizing_player ? -9999 : 9999;
    for(auto &node : child_node_list) {
        if(c.GetPiece(node.first[2], node.first[3]) == W_KING - 7*c.GetTurn()) {
            child_node_list.clear();
            return maximizing_player ? 9999 : -9999;
        }
        c.MovePiece(node.first[0], node.first[1], node.first[2], node.first[3], false, false);
        points = maximizing_player ? std::max(points, node.second.AlphaBeta(c, --depth, alpha, beta, false, initial_turn))
        : std::min(points, node.second.AlphaBeta(c, --depth, alpha, beta, true, initial_turn));
        maximizing_player ? alpha = std::max(alpha, points) : beta = std::min(beta, points);
        ++depth;
        c.MovePieceBack(node.first[0], node.first[1], node.first[2], node.first[3]);
        if(alpha >= beta)
            break;
    }
    child_node_list.clear();
    return points;
}

std::string PathNode::AlphaBetaRoot(Chess &c, unsigned short &difficulty) noexcept {
    CreateSubtree(c);
    std::vector<std::string> ideal_moves;
    float max_move_score = -9999;
    for(auto &node : child_node_list) {
        if(c.GetPiece(node.first[2], node.first[3]) == W_KING - 7*c.GetTurn()) {
            child_node_list.clear();
            return node.first;
        }
        c.MovePiece(node.first[0], node.first[1], node.first[2], node.first[3], false, false);
        float move_score = node.second.AlphaBeta(c, difficulty, -10000, 10000, false, !c.GetTurn());
        if(move_score > max_move_score) {
            max_move_score = move_score;
            ideal_moves.clear();
            ideal_moves.emplace_back(node.first);
        }
        else if(move_score == max_move_score)
            ideal_moves.emplace_back(node.first);
        c.MovePieceBack(node.first[0], node.first[1], node.first[2], node.first[3]);
    }
    child_node_list.clear();
    auto move = ideal_moves.cbegin();
    advance(move, GetRandomNumber<unsigned short>(0, ideal_moves.size()-1));
    return *move;
}

// --- Chess Implementation ---

// constructor of chess class
Chess::Chess(const std::string &player1, const unsigned short &difficulty1, const std::string &player2, const unsigned short &difficulty2, bool white_bot_random, bool black_bot_random) noexcept
: white(player1, difficulty1), black(player2, difficulty2), white_bot_random(white_bot_random), black_bot_random(black_bot_random) {
    CopyBoard(STARTING_BOARD, board);
}

// checks whether the given coordinate is within board boundaries or not
bool Chess::WithinBounds(const short &coord) noexcept {
    return coord>=0 && coord<BOARD_SIZE;
}

// changes the given board coordinates from ASCII to numerical, e.g. ('d', '3') -> (3, 5)
void Chess::ChangeToRealCoordinates(char &x1, char &y1, char &x2, char &y2) noexcept {
    x1 -= 'a', x2 -= 'a';
    y1 = '8'-y1, y2 = '8'-y2;
}

// changes the given board coordinates from numerical to ASCII, e.g. (3, 5) -> ('d', '3')
void Chess::ChangeToString(char &x1, char &y1, char &x2, char &y2) noexcept {
    x1 += 'a', x2 += 'a';
    y1 = '8'-y1, y2 = '8'-y2;
}

// returns the given numerical board coordinates as a string
std::string Chess::ToString(const short &x1, const short &y1, const short &x2, const short &y2) noexcept {
    return {static_cast<char>(x1+'a'), static_cast<char>('8'-y1), static_cast<char>(x2+'a'), static_cast<char>('8'-y2)};
}

// returns the name that is displayed on the terminal for the given piece
std::string Chess::PieceNameToString(const char &piece) noexcept {
    switch(piece) {
        case W_PAWN:    return "W_PAWN";
        case B_PAWN:    return "B_PAWN";
        case W_ROOK:    return "W_ROOK";
        case B_ROOK:    return "B_ROOK";
        case W_KNIGHT:  return "W_KNIGHT";
        case B_KNIGHT:  return "B_KNIGHT";
        case W_BISHOP:  return "W_BISHOP";
        case B_BISHOP:  return "B_BISHOP";
        case W_QUEEN:   return "W_QUEEN";
        case B_QUEEN:   return "B_QUEEN";
        case W_KING:    return "W_KING";
        case B_KING:    return "B_KING";
        default:        return "";
    }
}

// returns the worth of the given piece in terms of points
float Chess::EvaluatePiece(const char &piece) noexcept {
    switch(piece) {
        case W_PAWN:
        case B_PAWN:    return 10;
        case W_ROOK:
        case B_ROOK:    return 50;
        case W_KNIGHT:
        case B_KNIGHT:
        case W_BISHOP:
        case B_BISHOP:  return 30;
        case W_QUEEN:
        case B_QUEEN:   return 90;
        case W_KING:
        case B_KING:    return 900;
        default:        return 0;
    }
}

void Chess::ClearAllMoves(const unsigned short &n) noexcept {
    MoveCursorToXY(0, DOWN + 3*BOARD_SIZE + 9);
    for(unsigned short i=0;i<n;++i)
        std::cout << CLEAR_LINE << std::endl;
}

void Chess::PrintSeparator(const char &ch) noexcept {
    for(unsigned short i=1;i<BOARD_SIZE;++i)
        std::cout << std::string(BOX_WIDTH, ch) << "|";
    std::cout << std::string(BOX_WIDTH, ch) << std::endl << TO_RIGHT;
}

void Chess::CopyBoard(const char from[BOARD_SIZE][BOARD_SIZE], char to[BOARD_SIZE][BOARD_SIZE]) noexcept {
    std::copy(*from, *from + BOARD_SIZE*BOARD_SIZE, *to);
}

bool Chess::AreBoardsEqual(const char board1[BOARD_SIZE][BOARD_SIZE], const char board2[BOARD_SIZE][BOARD_SIZE]) noexcept {
    return std::equal(*board1, *board1 + BOARD_SIZE*BOARD_SIZE, *board2);
}

bool Chess::CanMovePiece(const short &x1, const short &y1, const short &x2, const short &y2, const std::forward_list<std::string> &all_moves) noexcept {
    return std::find(all_moves.cbegin(), all_moves.cend(), ToString(x1, y1, x2, y2)) != all_moves.cend();
}

char Chess::GetPiece(const short &x, const short &y) const noexcept {
    return board[y][x];
}

bool Chess::GetTurn() const noexcept {
    return whites_turn;
}

Bot& Chess::GetCurrentPlayer() noexcept {
    return whites_turn ? white : black;
}

Bot Chess::GetCurrentPlayerConst() const noexcept {
    return whites_turn ? white : black;
}

Bot& Chess::GetOtherPlayer() noexcept {
    return whites_turn ? black : white;
}

Bot Chess::GetOtherPlayerConst() const noexcept {
    return whites_turn ? black : white;
}

void Chess::ChangeTurn() noexcept {
    whites_turn = !whites_turn;
}

void Chess::AppendToAllGameMoves(const short &x1, const short &y1, const short &x2, const short &y2) noexcept {
    if(GetCurrentPlayerConst().GetCastling() && (board[y1][x1] == B_KING + 7*whites_turn) && (x2 == 2 || x2 == 6))
        all_game_moves.emplace_back(CASTLING, std::string(1, x2));
    else
        all_game_moves.emplace_back(NORMAL, ToString(x1, y1, x2, y2) + board[y1][x1] + board[y2][x2]);
}

void Chess::Reset() noexcept {
    CopyBoard(STARTING_BOARD, board);
    white.Reset();
    black.Reset();
    all_game_moves.clear();
    whites_turn = true;
    moves_after_last_pawn_move_or_capture = 0;
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void Chess::CheckCoordinates(const short &x, const short &y, const std::string &func_name) const noexcept(false) {
    try {
        if(!WithinBounds(x))        throw x;
        if(!WithinBounds(y))        throw y;
    }
    catch(const short &coord) {
        std::cerr << std::endl << std::endl << TO_RIGHT << "!ERROR!\t\tInvalid coordinate: '" << coord << "'.\t\t!ERROR!";
        std::cerr << std::endl << TO_RIGHT << "      \t\tException occurred in \"" << func_name << "\".";
        PrintAllMovesMadeInOrder();
        exit(1);
    }
}

bool Chess::EndGameText(const unsigned short &n, const Endgame &end_game) const noexcept {
    ClearAllMoves(n);
    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 7);
    switch(end_game) {
        case CHECKMATE:
            std::cout << "!!!Checkmate!!!" << CLEAR_LINE << std::endl << TO_RIGHT << GetOtherPlayerConst().GetName() << " wins!";
            return true;
        default:
            std::cout << "!!!Draw!!!" << CLEAR_LINE << std::endl << TO_RIGHT;
            switch(end_game) {
                case FIFTY_MOVES:
                    std::cout << "Fifty-move rule: No capture has been made and no pawn has been moved in the last 50 moves.";
                    return true;
                case THREEFOLD_REP:
                    std::cout << "Threefold repetition: Last position occured 3 times during the game.";
                    return true;
                default:
                    return false;
            }
    }
}

short Chess::GetEnPassant(const short &x, const short &y) const noexcept {
    if(all_game_moves.empty())
        return -1;
    if(all_game_moves.back().first != NORMAL)
        return -1;
    auto last_move = all_game_moves.back().second;
    ChangeToRealCoordinates(last_move[0], last_move[1], last_move[2], last_move[3]);
    return ((last_move[4] == W_PAWN - 7*whites_turn) && (abs(last_move[0] - x) == 1) && (last_move[3]-last_move[1] == 2*(whites_turn ? 1 : -1)) && (y == 4 - whites_turn)) ? last_move[0] : -1;
}

template<class Iterator> short Chess::GetEnPassant(const char board[BOARD_SIZE][BOARD_SIZE], const Iterator &it) const noexcept {
    if(it->first != NORMAL)
        return -1;
    auto last_move = it->second;
    ChangeToRealCoordinates(last_move[0], last_move[1], last_move[2], last_move[3]);
    for(short x=0;x<BOARD_SIZE;++x)
        if(board[3 + whites_turn][x] == W_PAWN - 7*whites_turn)
            if((last_move[4] == B_PAWN + 7*whites_turn) && (abs(last_move[0] - x) == 1) && (last_move[3] - last_move[1] == 2*(whites_turn ? -1 : 1)))
                return last_move[0];
    return -1;
}

bool Chess::ThreefoldRepetition() const noexcept {
    static char prev_board[BOARD_SIZE][BOARD_SIZE];
    CopyBoard(board, prev_board);
    unsigned short position_count = 1;
    auto it = all_game_moves.crbegin();
    auto last_move = it->second;
    while(true) {
        for(unsigned short i=0;i<2;++i)    {
            switch(it->first) {
                case CASTLING:
                    return false;
                default:
                    if(last_move[4] == W_PAWN || last_move[4] == B_PAWN || last_move[5] != EMPTY)
                        return false;
                    ChangeToRealCoordinates(last_move[0], last_move[1], last_move[2], last_move[3]);
                    prev_board[short(last_move[1])][short(last_move[0])] = last_move[4], prev_board[short(last_move[3])][short(last_move[2])] = EMPTY;
                    if(it->first == EN_PASSANT)
                        prev_board[short(last_move[1])][short(last_move[2])] = i == whites_turn ? B_PAWN : W_PAWN;
            }
            if((++it) == all_game_moves.crend())
                return false;
            last_move = it->second;
        }
        if(AreBoardsEqual(prev_board, board))
            if(GetOtherPlayerConst().GetCastling() == (it->first == CASTLING ? false : last_move[6 + (it->first == PROMOTION)]))
                if((all_game_moves.size() > 1 ? GetEnPassant(board, prev(all_game_moves.cend(), 2)) : -1)
                == (next(it) == all_game_moves.crend() ? -1 : GetEnPassant(prev_board, next(it))))
                    if((++position_count) == 3)
                        return true;
    }
}

bool Chess::IsCheck(const bool &turn) const noexcept {
    short x = -1, y = -1;
    for(short i=0;x==-1;++i)
        for(short j=0;j<BOARD_SIZE;++j)
            if(board[j][i] == B_KING + 7*turn) {
                x = i, y = j;
                break;
            }
    for(short i=x+1;i<BOARD_SIZE;++i)
        if(board[y][i] == W_ROOK - 7*turn)            return true;
        else if(board[y][i] == W_QUEEN - 7*turn)    return true;
        else if(board[y][i] != EMPTY)    break;
    for(short i=x-1;i>=0;--i)
        if(board[y][i] == W_ROOK - 7*turn)            return true;
        else if(board[y][i] == W_QUEEN - 7*turn)    return true;
        else if(board[y][i] != EMPTY)    break;
    for(short i=y+1;i<BOARD_SIZE;++i)
        if(board[i][x] == W_ROOK - 7*turn)            return true;
        else if(board[i][x] == W_QUEEN - 7*turn)    return true;
        else if(board[i][x] != EMPTY)    break;
    for(short i=y-1;i>=0;--i)
        if(board[i][x] == W_ROOK - 7*turn)            return true;
        else if(board[i][x] == W_QUEEN - 7*turn)    return true;
        else if(board[i][x] != EMPTY)    break;
    for(short i=x-1, j=y-1; i>=0 && j>=0; --i, --j)
        if(board[j][i] == W_BISHOP - 7*turn)        return true;
        else if(board[j][i] == W_QUEEN - 7*turn)    return true;
        else if(board[j][i] != EMPTY)    break;
    for(short i=x-1, j=y+1; i>=0 && j<BOARD_SIZE; --i, ++j)
        if(board[j][i] == W_BISHOP - 7*turn)        return true;
        else if(board[j][i] == W_QUEEN - 7*turn)    return true;
        else if(board[j][i] != EMPTY)    break;
    for(short i=x+1, j=y-1; i<BOARD_SIZE && j>=0; ++i, --j)
        if(board[j][i] == W_BISHOP - 7*turn)        return true;
        else if(board[j][i] == W_QUEEN - 7*turn)    return true;
        else if(board[j][i] != EMPTY)    break;
    for(short i=x+1, j=y+1; i<BOARD_SIZE && j<BOARD_SIZE; ++i, ++j)
        if(board[j][i] == W_BISHOP - 7*turn)        return true;
        else if(board[j][i] == W_QUEEN - 7*turn)    return true;
        else if(board[j][i] != EMPTY)    break;
    for(short i=x-1;i<x+2;++i)
        for(short j=y-1;j<y+2;++j)
            if((board[j][i] == W_KING - 7*turn) && WithinBounds(i) && WithinBounds(j))            return true;
    if((board[y-1][x-2] == W_KNIGHT - 7*turn) && (y > 0) && (x > 1))                            return true;
    else if((board[y-1][x+2] == W_KNIGHT - 7*turn) && (y > 0) && (x < BOARD_SIZE-2))            return true;
    else if((board[y+1][x-2] == W_KNIGHT - 7*turn) && (y < BOARD_SIZE-1) && (x > 1))            return true;
    else if((board[y+1][x+2] == W_KNIGHT - 7*turn) && (y < BOARD_SIZE-1) && (x < BOARD_SIZE-2))    return true;
    else if((board[y-2][x-1] == W_KNIGHT - 7*turn) && (y > 1) && (x > 0))                        return true;
    else if((board[y-2][x+1] == W_KNIGHT - 7*turn) && (y > 1) && (x < BOARD_SIZE-1))            return true;
    else if((board[y+2][x-1] == W_KNIGHT - 7*turn) && (y < BOARD_SIZE-2) && (x > 0))            return true;
    else if((board[y+2][x+1] == W_KNIGHT - 7*turn) && (y < BOARD_SIZE-2) && (x < BOARD_SIZE-1))    return true;
    else if((board[y + (turn ? -1 : 1)][x+1] == W_PAWN - 7*turn) && (x < BOARD_SIZE-1))            return true;
    else if((board[y + (turn ? -1 : 1)][x-1] == W_PAWN - 7*turn) && (x > 0))                    return true;
    return false;
}

bool Chess::IsCheck(std::string &move) noexcept {
    ChangeToRealCoordinates(move[0], move[1], move[2], move[3]);
    MovePiece(move[0], move[1], move[2], move[3], false, false);
    const bool &is_check = IsCheck(!whites_turn);
    MovePieceBack(move[0], move[1], move[2], move[3]);
    ChangeToString(move[0], move[1], move[2], move[3]);
    return is_check;
}

std::forward_list<std::string> Chess::PawnMoves(const short &x, const short &y) const noexcept {
    const auto &IsValid = whites_turn ? [](const char &ch){ return ch < 0; } : [](const char &ch){ return ch > 0; };
    const short &inc = whites_turn ? -1 : 1;
    std::forward_list<std::string> all_moves;
    if(board[y+inc][x] == EMPTY) {
        all_moves.emplace_front(ToString(x, y, x, y+inc));
        if((y == 1 + 5*whites_turn) && (board[y + 2*inc][x] == EMPTY))
            all_moves.emplace_front(ToString(x, y, x, y + 2*inc));
    }
    if(GetEnPassant(x, y) != -1)
        all_moves.emplace_front(ToString(x, y, GetEnPassant(x, y), y+inc));
    if(IsValid(board[y+inc][x+1]) && (x < BOARD_SIZE-1))
        all_moves.emplace_front(ToString(x, y, x+1, y+inc));
    if(IsValid(board[y+inc][x-1]) && (x > 0))
        all_moves.emplace_front(ToString(x, y, x-1, y+inc));
    return all_moves;
}

std::forward_list<std::string> Chess::RookMoves(const short &x, const short &y) const noexcept {
    const auto &IsValid = whites_turn ? [](const char &ch){ return ch < 0; } : [](const char &ch){ return ch > 0; };
    std::forward_list<std::string> all_moves;
    for(short i=x+1;i<BOARD_SIZE;++i)
        if(board[y][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, y));
        else {
            if(IsValid(board[y][i]))
                all_moves.emplace_front(ToString(x, y, i, y));
            break;
        }
    for(short i=x-1;i>=0;--i)
        if(board[y][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, y));
        else {
            if(IsValid(board[y][i]))
                all_moves.emplace_front(ToString(x, y, i, y));
            break;
        }
    for(short i=y+1;i<BOARD_SIZE;++i)
        if(board[i][x] == EMPTY)
            all_moves.emplace_front(ToString(x, y, x, i));
        else {
            if(IsValid(board[i][x]))
                all_moves.emplace_front(ToString(x, y, x, i));
            break;
        }
    for(short i=y-1;i>=0;--i)
        if(board[i][x] == EMPTY)
            all_moves.emplace_front(ToString(x, y, x, i));
        else {
            if(IsValid(board[i][x]))
                all_moves.emplace_front(ToString(x, y, x, i));
            break;
        }
    return all_moves;
}

std::forward_list<std::string> Chess::KnightMoves(const short &x, const short &y) const noexcept {
    const auto &IsValid = whites_turn ? [](const char &ch){ return ch <= 0; } : [](const char &ch){ return ch >= 0; };
    std::forward_list<std::string> all_moves;
    if(IsValid(board[y-1][x-2]) && (y > 0) && (x > 1))
        all_moves.emplace_front(ToString(x, y, x-2, y-1));
    if(IsValid(board[y-1][x+2]) && (y > 0) && (x < BOARD_SIZE-2))
        all_moves.emplace_front(ToString(x, y, x+2, y-1));
    if(IsValid(board[y+1][x-2]) && (y < BOARD_SIZE-1) && (x > 1))
        all_moves.emplace_front(ToString(x, y, x-2, y+1));
    if(IsValid(board[y+1][x+2]) && (y < BOARD_SIZE-1) && (x < BOARD_SIZE-2))
        all_moves.emplace_front(ToString(x, y, x+2, y+1));
    if(IsValid(board[y-2][x-1]) && (y > 1) && (x > 0))
        all_moves.emplace_front(ToString(x, y, x-1, y-2));
    if(IsValid(board[y-2][x+1]) && (y > 1) && (x < BOARD_SIZE-1))
        all_moves.emplace_front(ToString(x, y, x+1, y-2));
    if(IsValid(board[y+2][x-1]) && (y < BOARD_SIZE-2) && (x > 0))
        all_moves.emplace_front(ToString(x, y, x-1, y+2));
    if(IsValid(board[y+2][x+1]) && (y < BOARD_SIZE-2) && (x < BOARD_SIZE-1))
        all_moves.emplace_front(ToString(x, y, x+1, y+2));
    return all_moves;
}

std::forward_list<std::string> Chess::BishopMoves(const short &x, const short &y) const noexcept {
    const auto &IsValid = whites_turn ? [](const char &ch){ return ch < 0; } : [](const char &ch){ return ch > 0; };
    std::forward_list<std::string> all_moves;
    for(short i=x-1, j=y-1; i>=0 && j>=0; --i, --j)
        if(board[j][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, j));
        else {
            if(IsValid(board[j][i]))
                all_moves.emplace_front(ToString(x, y, i, j));
            break;
        }
    for(short i=x-1, j=y+1; i>=0 && j<BOARD_SIZE; --i, ++j)
        if(board[j][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, j));
        else {
            if(IsValid(board[j][i]))
                all_moves.emplace_front(ToString(x, y, i, j));
            break;
        }
    for(short i=x+1, j=y-1; i<BOARD_SIZE && j>=0; ++i, --j)
        if(board[j][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, j));
        else {
            if(IsValid(board[j][i]))
                all_moves.emplace_front(ToString(x, y, i, j));
            break;
        }
    for(short i=x+1, j=y+1; i<BOARD_SIZE && j<BOARD_SIZE; ++i, ++j)
        if(board[j][i] == EMPTY)
            all_moves.emplace_front(ToString(x, y, i, j));
        else {
            if(IsValid(board[j][i]))
                all_moves.emplace_front(ToString(x, y, i, j));
            break;
        }
    return all_moves;
}

std::forward_list<std::string> Chess::QueenMoves(const short &x, const short &y) const noexcept {
    auto all_moves = RookMoves(x, y);
    all_moves.merge(BishopMoves(x, y));
    return all_moves;
}

std::forward_list<std::string> Chess::KingMoves(const short &x, const short &y) const noexcept {
    const auto &IsValid = whites_turn ? [](const char &ch){ return ch <= 0; } : [](const char &ch){ return ch >= 0; };
    std::forward_list<std::string> all_moves;
    for(short i=x-1;i<x+2;++i)
        for(short j=y-1;j<y+2;++j)
            if(IsValid(board[j][i]) && WithinBounds(i) && WithinBounds(j))
                all_moves.emplace_front(ToString(x, y, i, j));
    if(GetCurrentPlayerConst().GetCastling())
        if(!IsCheck(whites_turn)) {
            const short &line = (BOARD_SIZE-1)*whites_turn;
            if((board[line][0] == B_ROOK + 7*whites_turn) && board[line][1] == EMPTY && board[line][2] == EMPTY && board[line][3] == EMPTY)
                all_moves.emplace_front(ToString(4, line, 2, line));
            else if((board[line][7] == B_ROOK + 7*whites_turn) && board[line][5] == EMPTY && board[line][6] == EMPTY)
                all_moves.emplace_front(ToString(4, line, 6, line));
        }
    return all_moves;
}

std::forward_list<std::string> Chess::AllMoves() noexcept {
    std::forward_list<std::string> all_moves;
    for(short y=0;y<BOARD_SIZE;++y)
        for(short x=0;x<BOARD_SIZE;++x) {
            if((board[y][x] < 0) == whites_turn)
                continue;
            switch(board[y][x]) {
                case W_PAWN:
                case B_PAWN:
                    all_moves.merge(PawnMoves(x, y));
                    break;
                case W_ROOK:
                case B_ROOK:
                    all_moves.merge(RookMoves(x, y));
                    break;
                case W_KNIGHT:
                case B_KNIGHT:
                    all_moves.merge(KnightMoves(x, y));
                    break;
                case W_BISHOP:
                case B_BISHOP:
                    all_moves.merge(BishopMoves(x, y));
                    break;
                case W_QUEEN:
                case B_QUEEN:
                    all_moves.merge(QueenMoves(x, y));
                    break;
                case W_KING:
                case B_KING:
                    all_moves.merge(KingMoves(x, y));
            }
        }
    for(auto it = all_moves.begin(), prev = all_moves.before_begin(); it != all_moves.cend();)        // if the possible move makes me checkmate after the opponent's turn, remove it from the list
        if(IsCheck(*it))
            it = all_moves.erase_after(prev);
        else
            ++it, ++prev;
    return all_moves;
}

std::string Chess::GetRandomMove() noexcept {
    auto all_moves = AllMoves();
    auto move = all_moves.begin();
    advance(move, GetRandomNumber<unsigned short>(0, distance(all_moves.cbegin(), all_moves.cend()) - 1));
    ChangeToRealCoordinates((*move)[0], (*move)[1], (*move)[2], (*move)[3]);
    return *move;
}

void Chess::ManuallyPromotePawn(const short &x, const short &y) noexcept {
    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 7);
    std::cout << "Enter your choice of promotion [(r)ook, (k)night, (b)ishop, (q)ueen]";
    char key = getch();
    while(true)
        switch(key = tolower(key)) {
            case 'r':    board[y][x] = whites_turn ? W_ROOK : B_ROOK;        return;
            case 'k':    board[y][x] = whites_turn ? W_KNIGHT : B_KNIGHT;    return;
            case 'b':    board[y][x] = whites_turn ? W_BISHOP : B_BISHOP;    return;
            case 'q':    board[y][x] = whites_turn ? W_QUEEN : B_QUEEN;        return;
            default:    key = getch();
        }
}

void Chess::MovePiece(const short &x1, const short &y1, const short &x2, const short &y2, const bool &manual_promotion, const bool &update_board) noexcept {
    AppendToAllGameMoves(x1, y1, x2, y2);
    switch(board[y1][x1]) {
        case W_PAWN:
        case B_PAWN:
            if(y2 == ((BOARD_SIZE-1) * !whites_turn)) {
                if(manual_promotion) {
                    ManuallyPromotePawn(x1, y1);
                    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 7);
                    std::cout << "All possible moves:" << CLEAR_LINE;
                }
                else if(whites_turn ? white_bot_random : black_bot_random)
                    board[y1][x1] = (whites_turn ? 1 : -1) * GetRandomNumber(2, 5);
                else
                    board[y1][x1] = whites_turn ? W_QUEEN : B_QUEEN;
                all_game_moves.back().first = PROMOTION;
                all_game_moves.back().second.push_back(board[y1][x1]);
            }
            else if(x1 != x2 && board[y2][x2] == EMPTY) {
                board[y1][x2] = EMPTY;
                if(update_board) {
                    GetCurrentPlayer().IncreaseScore(EvaluatePiece(W_PAWN));
                    UpdateScore(GetCurrentPlayerConst());
                    UpdateBoard(x2, y1);
                }
                all_game_moves.back().first = EN_PASSANT;
            }
            break;
        case W_KING:
        case B_KING:
            if(GetCurrentPlayerConst().GetCastling()) {
                const short &line = (BOARD_SIZE-1) * whites_turn;
                switch(x2) {
                    case 2:
                        board[line][3] = board[line][0], board[line][0] = EMPTY;
                        if(update_board) {
                            UpdateBoard(0, line);
                            UpdateBoard(3, line);
                        }
                        break;
                    case 6:
                        board[line][5] = board[line][7], board[line][7] = EMPTY;
                        if(update_board) {
                            UpdateBoard(7, line);
                            UpdateBoard(5, line);
                        }
                }
            }
        case W_ROOK:
        case B_ROOK:
            GetCurrentPlayer().SetCastling(false);
    }
    if(all_game_moves.back().first != CASTLING)                all_game_moves.back().second.push_back(GetCurrentPlayerConst().GetCastling());
    board[y2][x2] = board[y1][x1], board[y1][x1] = EMPTY;
    if(update_board) {
        if(all_game_moves.back().first != CASTLING)
            if(all_game_moves.back().second[5] != EMPTY) {
                GetCurrentPlayer().IncreaseScore(EvaluatePiece(all_game_moves.back().second[5]));
                UpdateScore(GetCurrentPlayerConst());
                moves_after_last_pawn_move_or_capture = 0;
            }
        UpdateBoard(x1, y1);
        UpdateBoard(x2, y2);
    }
    ChangeTurn();
}

void Chess::MovePieceBack(const short &x1, const short &y1, const short &x2, const short &y2) noexcept {
    ChangeTurn();
    board[y1][x1] = board[y2][x2], board[y2][x2] = all_game_moves.back().first == CASTLING ? static_cast<char>(EMPTY) : all_game_moves.back().second[5];
    switch(board[y1][x1]) {
        case W_PAWN:
        case B_PAWN:
            if(x1 != x2 && board[y2][x2] == EMPTY)
                board[y1][x2] = whites_turn ? B_PAWN : W_PAWN;
            break;
        case W_ROOK:
        case B_ROOK:
            if(prev(all_game_moves.cend(), 3)->first != CASTLING)
                if(prev(all_game_moves.cend(), 3)->second[6 + (prev(all_game_moves.cend(), 3)->first == PROMOTION)])
                    GetCurrentPlayer().SetCastling(true);
            break;
        case W_QUEEN:
        case B_QUEEN:
            if(all_game_moves.back().first == PROMOTION)
                board[y1][x1] = whites_turn ? W_PAWN : B_PAWN;
            break;
        case W_KING:
        case B_KING:
            if(all_game_moves.back().first == CASTLING) {
                GetCurrentPlayer().SetCastling(true);
                const short line = (BOARD_SIZE-1) * whites_turn;
                switch(x2) {
                    case 2:
                        board[line][0] = board[line][3], board[line][3] = EMPTY;
                        break;
                    case 6:
                        board[line][5] = board[line][7], board[line][7] = EMPTY;
                }
            }
            else if(prev(all_game_moves.cend(), 3)->first != CASTLING)
                if(prev(all_game_moves.cend(), 3)->second[6 + (prev(all_game_moves.cend(), 3)->first == PROMOTION)])
                    GetCurrentPlayer().SetCastling(true);
    }
    all_game_moves.pop_back();
}

void Chess::UpdateBoard(const short &x, const short &y) const noexcept {
    const unsigned short &diff = BOX_WIDTH - PieceNameToString(board[y][x]).length();
    MoveCursorToXY(RIGHT + (BOX_WIDTH+1)*x, DOWN + 3*y + 1);
    std::cout << std::string(diff/2, ' ') << PieceNameToString(board[y][x]) << std::string(diff/2, ' ');
    if(diff%2)    std::cout << " ";
}

void Chess::UpdateScore(const Bot &p) const noexcept {
    const unsigned short &dx = p==white ? white.GetName().length() + 2 : (BOX_WIDTH+1)*BOARD_SIZE - 5;
    MoveCursorToXY(RIGHT+dx, DOWN + 3*BOARD_SIZE + 2);
    std::cout << std::string(std::to_string(p.GetScore()).length(), ' ');
    MoveCursorToXY(RIGHT+dx, DOWN + 3*BOARD_SIZE + 2);
    std::cout << p.GetScore();
}

float Chess::EvaluatePosition(const short &x, const short &y) const noexcept {
    if(board[y][x] == EMPTY)
        return 0;
    static float PIECE_POS_POINTS[6][BOARD_SIZE][BOARD_SIZE] =
    {{{-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0},
    {-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0},
    {-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0},
    {-3.0, -4.0, -4.0, -5.0, -5.0, -4.0, -4.0, -3.0},
    {-2.0, -3.0, -3.0, -4.0, -4.0, -3.0, -3.0, -2.0},
    {-1.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -1.0},
    {2.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 2.0},
    {2.0, 3.0, 1.0, 0.0, 0.0, 1.0, 3.0, 2.0}}
    ,
    {{-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0},
    {-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0},
    {-1.0, 0.0, 0.5, 0.5, 0.5, 0.5, 0.0, -1.0},
    {-0.5, 0.0, 0.5, 0.5, 0.5, 0.5, 0.0, -0.5},
    {0.0, 0.0, 0.5, 0.5, 0.5, 0.5, 0.0, -0.5},
    {-1.0, 0.5, 0.5, 0.5, 0.5, 0.5, 0.0, -1.0},
    {-1.0, 0.0, 0.5, 0.0, 0.0, 0.0, 0.0, -1.0},
    {-2.0, -1.0, -1.0, -0.5, -0.5, -1.0, -1.0, -2.0}}
    ,
    {{-2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0},
    {-1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0},
    {-1.0, 0.0, 0.5, 1.0, 1.0, 0.5, 0.0, -1.0},
    {-1.0, 0.5, 0.5, 1.0, 1.0, 0.5, 0.5, -1.0},
    {-1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, -1.0},
    {-1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -1.0},
    {-1.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.5, -1.0},
    {-2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0}}
    ,
    {{-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0},
    {-4.0, -2.0, 0.0, 0.0, 0.0, 0.0, -2.0, -4.0},
    {-3.0, 0.0, 1.0, 1.5, 1.5, 1.0, 0.0, -3.0},
    {-3.0, 0.5, 1.5, 2.0, 2.0, 1.5, 0.5, -3.0},
    {-3.0, 0.0, 1.5, 2.0, 2.0, 1.5, 0.0, -3.0},
    {-3.0, 0.5, 1.0, 1.5, 1.5, 1.0, 0.5, -3.0},
    {-4.0, -2.0, 0.0, 0.5, 0.5, 0.0, -2.0, -4.0},
    {-5.0, -4.0, -3.0, -3.0, -3.0, -3.0, -4.0, -5.0}}
    ,
    {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.5},
    {-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5},
    {-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5},
    {-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5},
    {-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5},
    {-0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.5},
    {0.0, 0.0, 0.0, 0.5, 0.5, 0.0, 0.0, 0.0}}
    ,
    {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0},
    {1.0, 1.0, 2.0, 3.0, 3.0, 2.0, 1.0, 1.0},
    {0.5, 0.5, 1.0, 2.5, 2.5, 1.0, 0.5, 0.5},
    {0.0, 0.0, 0.0, 2.0, 2.0, 0.0, 0.0, 0.0},
    {0.5, -0.5, -1.0, 0.0, 0.0, -1.0, -0.5, 0.5},
    {0.5, 1.0, 1.0, -2.0, -2.0, 1.0, 1.0, 0.5},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}};
    return (board[y][x]<0 ? -1 : 1) * (EvaluatePiece(board[y][x]) + PIECE_POS_POINTS[board[y][x] + 7*(board[y][x]<0) - 1][board[y][x]<0 ? BOARD_SIZE-y-1 : y][x]);
}

float Chess::EvaluateBoard(const bool &turn) const noexcept {
    float total_evaluation = 0.0;
    for(short y=0;y<BOARD_SIZE;++y)
        for(short x=0;x<BOARD_SIZE;++x)
            total_evaluation += EvaluatePosition(x, y);
    return (turn ? 1 : -1) * total_evaluation;
}

void Chess::PrintBoard() const noexcept {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    std::cout << TO_DOWN << TO_RIGHT;
    for(short y=0;y<BOARD_SIZE;++y) {
        PrintSeparator(' ');
        std::cout << "\b\b\b" << BOARD_SIZE-y << "  ";
        for(short x=0;x<BOARD_SIZE;++x) {
            const unsigned short &diff = BOX_WIDTH - PieceNameToString(board[y][x]).length();
            std::cout << std::string(diff/2, ' ') << PieceNameToString(board[y][x]) << std::string(diff/2, ' ');
            if(diff%2)                std::cout << " ";
            if(x < BOARD_SIZE-1)    std::cout << "|";
        }
        if(y < BOARD_SIZE-1) {
            std::cout << std::endl << TO_RIGHT;
            PrintSeparator('_');
        }
    }
    std::cout << std::endl << TO_RIGHT;
    PrintSeparator(' ');
    for(char ch='a';ch<'a'+BOARD_SIZE;++ch)
        std::cout << std::string(BOX_WIDTH/2, ' ') << ch << std::string(BOX_WIDTH/2, ' ');
    std::cout << std::endl << std::endl << TO_RIGHT << white.GetName() << ": 0";
    std::cout << std::string((BOX_WIDTH+1)*BOARD_SIZE - white.GetName().length() - black.GetName().length() - 10, ' ') << black.GetName() << ": 0";
    std::cout << std::endl << std::endl << TO_RIGHT << white.GetName() << "'s turn...";
    std::cout << std::endl << TO_RIGHT << "Enter move coordinates (x1,y1)->(x2,y2):";
    std::cout << std::endl << std::endl << TO_RIGHT << "All possible moves:" << std::endl;
}

void Chess::PrintAllMovesMadeInOrder() const noexcept {
    std::cout << std::endl << std::endl << TO_RIGHT << "All moves made in order:" << std::endl;
    bool turn = true;
    for(const auto &game_move : all_game_moves) {
        std::cout << std::endl << TO_RIGHT << (turn ? white : black).GetName() << ": ";
        switch(game_move.first) {
            case CASTLING:
                std::cout << "castling " << (game_move.second[0] == 2 ? "long" : "short");    break;
            default:
                std::cout << ToLowerString(PieceNameToString(game_move.second[4])).substr(2) << " '" << game_move.second.substr(0, 2) << "' to ";
                if(game_move.second[5] != EMPTY)
                    std::cout << ToLowerString(PieceNameToString(game_move.second[5])).substr(2) + " ";
                std::cout << "'" << game_move.second.substr(2, 2) << "'";
                switch(game_move.first) {
                    case PROMOTION:
                        std::cout << " promoted to " << ToLowerString(PieceNameToString(game_move.second[6])).substr(2);
                        break;
                    case EN_PASSANT:
                        std::cout << " (en passant)";
                    default:
                        break;
                }
        }
        turn = !turn;
    }
}

bool Chess::CheckEndgame(const unsigned short &n) noexcept {
    if(AllMoves().empty()) {
        GetOtherPlayer().IncreaseScore(EvaluatePiece(W_KING));
        UpdateScore(GetOtherPlayerConst());
        return EndGameText(n, CHECKMATE);
    }
    else if(all_game_moves.back().first != CASTLING) {
        if(all_game_moves.back().second[4] == W_PAWN - 7*whites_turn)
            moves_after_last_pawn_move_or_capture = 0;
        else if(all_game_moves.back().second[5] != EMPTY)
            moves_after_last_pawn_move_or_capture = 0;
        else if((++moves_after_last_pawn_move_or_capture) == 50)
            return EndGameText(n, FIFTY_MOVES);
    }
    else if((++moves_after_last_pawn_move_or_capture) == 50)
        return EndGameText(n, FIFTY_MOVES);
    if(ThreefoldRepetition())
        return EndGameText(n, THREEFOLD_REP);
    return false;
}

bool Chess::PlayersTurn() noexcept {
    auto all_moves = AllMoves();
    all_moves.sort();
    unsigned short i=0;
    for(const auto &move : all_moves) {
        if(!((i++)%MOVES_PER_LINE))    std::cout << std::endl;
        std::cout << TO_RIGHT << move.substr(0, 2) << " " << move.substr(2);
    }
    if(IsCheck(whites_turn)) {
        std::cout << std::endl << std::endl << TO_RIGHT << "Check!";
        i += 2*MOVES_PER_LINE;
    }
    MoveCursorToXY(RIGHT+41, DOWN + 3*BOARD_SIZE + 5);
    while(true) {
        std::string from, to;
        std::cin >> from;
        if(!ToLowerString(from).compare("quit"))
            return EndGameText(i/MOVES_PER_LINE + 1, QUIT);
        if(!ToLowerString(from).compare("exit"))
            return EndGameText(i/MOVES_PER_LINE + 1, QUIT);
        std::cin >> to;
        from.resize(2);
        to.resize(2);
        from.shrink_to_fit();
        to.shrink_to_fit();
        from[0] = tolower(from[0]), to[0] = tolower(to[0]);
        ChangeToRealCoordinates(from[0], from[1], to[0], to[1]);
        if((from[0]!=to[0] || from[1]!=to[1]) && WithinBounds(from[0]) && WithinBounds(from[1]) && WithinBounds(to[0]) && WithinBounds(to[1]))
            if(CanMovePiece(from[0], from[1], to[0], to[1], all_moves)) {
                MovePiece(from[0], from[1], to[0], to[1], true, true);
                if(CheckEndgame(i/MOVES_PER_LINE + 1))
                    return false;
                break;
            }
        MoveCursorToXY(RIGHT+41, DOWN + 3*BOARD_SIZE + 5);
        std::cout << CLEAR_LINE << std::endl << CLEAR_LINE;
        MoveCursorToXY(RIGHT+41, DOWN + 3*BOARD_SIZE + 5);
    }
    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 4);
    std::cout << GetCurrentPlayerConst().GetName() << "'s turn..." << CLEAR_LINE;
    MoveCursorToXY(RIGHT+41, DOWN + 3*BOARD_SIZE + 5);
    std::cout << CLEAR_LINE << std::endl << CLEAR_LINE;
    ClearAllMoves(i/MOVES_PER_LINE + 1);
    MoveCursorToXY(0, DOWN + 3*BOARD_SIZE + 8);
    return true;
}

bool Chess::BotsTurn() noexcept {
    const auto &move = (whites_turn ? white_bot_random : black_bot_random) ? GetRandomMove() : GetCurrentPlayer().GetIdealMove(*this);
    std::cout << "Bot moves: " << move.substr(0,2) << " to " << move.substr(2,2) << std::endl;
    MovePiece(move[0], move[1], move[2], move[3], false, true);
    PrintBoard();
    if(CheckEndgame())
        return false;
    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 4);
    std::cout << GetCurrentPlayerConst().GetName() << "'s turn..." << CLEAR_LINE;
    return true;
}

bool Chess::GameOver() noexcept {
    std::cout << std::endl << std::endl << std::endl << TO_RIGHT << "Press R to play again.";
    std::cout << std::endl << TO_RIGHT << "Press any other key to quit.";
    PrintAllMovesMadeInOrder();
    char key = getch();
    switch(key = tolower(key)) {
        case 'r':
            Reset();
            return true;
        default:
            return false;
    }
}

// --- main() ---
int main() {
    std::cout << "Welcome to ChessBot!" << std::endl;
    srand((unsigned int)time(NULL));

    int game_mode = 0;
    while (true) {
        std::cout << "\nChoose game mode:" << std::endl;
        std::cout << "1. Play against Bot" << std::endl;
        std::cout << "2. Play against another Person" << std::endl;
        std::cout << "3. Bot vs Bot" << std::endl;
        std::cout << "Enter 1, 2, or 3: ";
        std::cin >> game_mode;
        if (game_mode >= 1 && game_mode <= 3) break;
        std::cout << "Invalid input. Please try again." << std::endl;
    }

    bool against_bot = (game_mode == 1);
    bool two_bots = (game_mode == 3);
    bool bot_is_white = false;
    int bot_difficulty = 1;
    std::string player1 = "Player1", player2 = "Player2";
    bool white_bot_random = false, black_bot_random = false;
    int white_bot_difficulty = 1, black_bot_difficulty = 1;

    if (against_bot) {
        std::string color_choice;
        while (true) {
            std::cout << "\nDo you want to play as white or black? (w/b): ";
            std::cin >> color_choice;
            if (color_choice == "w" || color_choice == "W") {
                bot_is_white = false;
                player1 = "You";
                player2 = "Bot";
                break;
            } else if (color_choice == "b" || color_choice == "B") {
                bot_is_white = true;
                player1 = "Bot";
                player2 = "You";
                break;
            } else {
                std::cout << "Invalid input. Please enter 'w' or 'b'." << std::endl;
            }
        }
        while (true) {
            std::cout << "\nChoose bot difficulty:" << std::endl;
            std::cout << "1. Easy" << std::endl;
            std::cout << "2. Medium" << std::endl;
            std::cout << "3. Hard" << std::endl;
            std::cout << "Enter 1, 2, or 3: ";
            std::cin >> bot_difficulty;
            if (bot_difficulty >= 1 && bot_difficulty <= 3) break;
            std::cout << "Invalid input. Please try again." << std::endl;
        }
        if (bot_is_white) {
            white_bot_random = false;
            black_bot_random = false;
            white_bot_difficulty = bot_difficulty;
            black_bot_difficulty = 1;
        } else {
            white_bot_random = false;
            black_bot_random = false;
            white_bot_difficulty = 1;
            black_bot_difficulty = bot_difficulty;
        }
    } else if (two_bots) {
        player1 = "Bot1";
        player2 = "Bot2";
        white_bot_random = false;
        black_bot_random = false;
        while (true) {
            std::cout << "\nChoose white bot difficulty (1=Easy, 2=Medium, 3=Hard): ";
            std::cin >> white_bot_difficulty;
            if (white_bot_difficulty >= 1 && white_bot_difficulty <= 3) break;
            std::cout << "Invalid input. Please try again." << std::endl;
        }
        while (true) {
            std::cout << "\nChoose black bot difficulty (1=Easy, 2=Medium, 3=Hard): ";
            std::cin >> black_bot_difficulty;
            if (black_bot_difficulty >= 1 && black_bot_difficulty <= 3) break;
            std::cout << "Invalid input. Please try again." << std::endl;
        }
    } else {
        player1 = "Player1";
        player2 = "Player2";
    }

    Chess c(player1, white_bot_difficulty, player2, black_bot_difficulty, white_bot_random, black_bot_random);

    if (against_bot) {
        do {
            c.PrintBoard();
            if (bot_is_white) {
                while (true) {
                    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 5);
                    std::cout << CLEAR_LINE << std::endl << std::endl << CLEAR_LINE;
                    if (!c.BotsTurn())
                        break;
                    std::cout << std::endl << TO_RIGHT << "Enter move coordinates (x1,y1)->(x2,y2):";
                    std::cout << std::endl << std::endl << TO_RIGHT << "All possible moves:" << std::endl;
                    if (!c.PlayersTurn())
                        break;
                }
            } else {
                while (true) {
                    if (!c.PlayersTurn())
                        break;
                    MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 5);
                    std::cout << CLEAR_LINE << std::endl << std::endl << CLEAR_LINE;
                    if (!c.BotsTurn())
                        break;
                    std::cout << std::endl << TO_RIGHT << "Enter move coordinates (x1,y1)->(x2,y2):";
                    std::cout << std::endl << std::endl << TO_RIGHT << "All possible moves:" << std::endl;
                }
            }
        } while (c.GameOver());
        exit(0);
    } else if (two_bots) {
        do {
            c.PrintBoard();
            MoveCursorToXY(RIGHT, DOWN + 3*BOARD_SIZE + 5);
            std::cout << CLEAR_LINE << std::endl << std::endl << CLEAR_LINE;
            while (c.BotsTurn());
        } while (c.GameOver());
        exit(0);
    } else {
        do {
            c.PrintBoard();
            while (c.PlayersTurn());
        } while (c.GameOver());
    }
}
