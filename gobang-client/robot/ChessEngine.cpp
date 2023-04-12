
#define WIN32_LEAN_AND_MEAN // �� Windows ͷ���ų�����ʹ�õ�����
// Windows ͷ�ļ�:
#if WIN32
#include <SDKDDKVer.h>
#include <tchar.h>
#include <windows.h>
#endif

// C ����ʱͷ�ļ�
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>

#include "ACSearcher.h"
#include "ChessEngine.h"
#include "PossiblePositionManager.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <stack>
#include <vector>
using namespace std;

namespace ChessEngine {

#define BOARD_WIDTH 15
#define UNKNOWN_SCORE (10000001)
#define HASH_ITEM_INDEX_MASK (0xffff)
#define MAX_SCORE (10000000)
#define MIN_SCORE (-10000000)
int DEPTH = 7;

struct Pattern {
    string pattern;
    int score;
};

// ģʽ
vector<Pattern> patterns = {
    {"11111", 50000}, {"011110", 4320}, {"011100", 720}, {"001110", 720},
    {"011010", 720},  {"010110", 720},  {"11110", 720},  {"01111", 720},
    {"11011", 720},   {"10111", 720},   {"11101", 720},  {"001100", 120},
    {"001010", 120},  {"010100", 120},  {"000100", 20},  {"001000", 20},
};

// ������ֵĹ�ϣ����Ŀ
struct HashItem {
    long long checksum;
    int depth;
    int score;
    enum Flag { ALPHA = 0, BETA = 1, EXACT = 2, EMPTY = 3 } flag;
};

long long boardZobristValue[2][BOARD_WIDTH][BOARD_WIDTH];
long long currentZobristValue;
HashItem hashItems[HASH_ITEM_INDEX_MASK + 1];
char board[BOARD_WIDTH][BOARD_WIDTH];
int winner; // ʤ����

stack<Position> moves;
int scores[2][72]; // ������ַ�����2����ɫ72�У���������Ʋ�ࣩ
int allScore[2];   // ���������֣�2����ɫ��

// ac�㷨ʵ�ֵ�ģʽƥ����
ACSearcher acs;

PossiblePositionManager ppm;

// ��¼�������ڹ�ϣ����
void recordHashItem(int depth, int score, HashItem::Flag flag) {
    int index = (int)(currentZobristValue & HASH_ITEM_INDEX_MASK);
    HashItem *phashItem = &hashItems[index];

    if (phashItem->flag != HashItem::EMPTY && phashItem->depth > depth) {
        return;
    }

    phashItem->checksum = currentZobristValue;
    phashItem->score = score;
    phashItem->flag = flag;
    phashItem->depth = depth;
}

// �ڹ�ϣ����ȡ�ü���õľ���ķ���
int getHashItemScore(int depth, int alpha, int beta) {
    int index = (int)(currentZobristValue & HASH_ITEM_INDEX_MASK);
    HashItem *phashItem = &hashItems[index];

    if (phashItem->flag == HashItem::EMPTY)
        return UNKNOWN_SCORE;

    if (phashItem->checksum == currentZobristValue) {
        if (phashItem->depth >= depth) {
            if (phashItem->flag == HashItem::EXACT) {
                return phashItem->score;
            }
            if (phashItem->flag == HashItem::ALPHA &&
                phashItem->score <= alpha) {
                return alpha;
            }
            if (phashItem->flag == HashItem::BETA && phashItem->score >= beta) {
                return beta;
            }
        }
    }

    return UNKNOWN_SCORE;
}

// ����64λ�����
long long random64() {
    return (long long)rand() | ((long long)rand() << 15) |
           ((long long)rand() << 30) | ((long long)rand() << 45) |
           ((long long)rand() << 60);
}

// ����zobrist��ֵ
void randomBoardZobristValue() {
    int i, j, k;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            for (k = 0; k < BOARD_WIDTH; k++) {
                boardZobristValue[i][j][k] = random64();
            }
        }
    }
}

// ��ʼ����ʼ�����zobristֵ
void initCurrentZobristValue() { currentZobristValue = random64(); }

// �洢�������������һ�����ӵ�λ��
Position searchResult;

// ����λ�����֣�����board�ǵ�ǰ���̣�p��λ�ã�role�����ֽ�ɫ������role��Human��������������֣�����role��computer���Ƕ��ڵ�������
int evaluatePoint(char board[BOARD_WIDTH][BOARD_WIDTH], Position p) {
    int result;
    unsigned int i, j;
    int role;

    result = 0;
    role = HUMAN;

    string lines[4];
    string lines1[4];
    for (i = max(0, p.x - 5); i < (unsigned int)min(BOARD_WIDTH, p.x + 6);
         i++) {
        if (i != p.x) {
            lines[0].push_back(board[i][p.y] == role ? '1'
                               : board[i][p.y] == 0  ? '0'
                                                     : '2');
            lines1[0].push_back(board[i][p.y] == role ? '2'
                                : board[i][p.y] == 0  ? '0'
                                                      : '1');
        } else {
            lines[0].push_back('1');
            lines1[0].push_back('1');
        }
    }
    for (i = max(0, p.y - 5); i < (unsigned int)min(BOARD_WIDTH, p.y + 6);
         i++) {
        if (i != p.y) {
            lines[1].push_back(board[p.x][i] == role ? '1'
                               : board[p.x][i] == 0  ? '0'
                                                     : '2');
            lines1[1].push_back(board[p.x][i] == role ? '2'
                                : board[p.x][i] == 0  ? '0'
                                                      : '1');
        } else {
            lines[1].push_back('1');
            lines1[1].push_back('1');
        }
    }
    for (i = p.x - min(min(p.x, p.y), 5), j = p.y - min(min(p.x, p.y), 5);
         i < (unsigned int)min(BOARD_WIDTH, p.x + 6) &&
         j < (unsigned int)min(BOARD_WIDTH, p.y + 6);
         i++, j++) {
        if (i != p.x) {
            lines[2].push_back(board[i][j] == role ? '1'
                               : board[i][j] == 0  ? '0'
                                                   : '2');
            lines1[2].push_back(board[i][j] == role ? '2'
                                : board[i][j] == 0  ? '0'
                                                    : '1');
        } else {
            lines[2].push_back('1');
            lines1[2].push_back('1');
        }
    }
    for (i = p.x + min(min(p.y, BOARD_WIDTH - 1 - p.x), 5),
        j = p.y - min(min(p.y, BOARD_WIDTH - 1 - p.x), 5);
         i >= (unsigned int)max(0, p.x - 5) && i < BOARD_WIDTH &&
         j < (unsigned int)min(BOARD_WIDTH, p.y + 6);
         i--, j++) {
        if (i != p.x) {
            lines[3].push_back(board[i][j] == role ? '1'
                               : board[i][j] == 0  ? '0'
                                                   : '2');
            lines1[3].push_back(board[i][j] == role ? '2'
                                : board[i][j] == 0  ? '0'
                                                    : '1');
        } else {
            lines[3].push_back('1');
            lines1[3].push_back('1');
        }
    }

    for (i = 0; i < 4; i++) {
        vector<int> tmp = acs.ACSearch(lines[i]);
        for (j = 0; j < tmp.size(); j++) {
            result += patterns[tmp[j]].score;
        }

        tmp = acs.ACSearch(lines1[i]);
        for (j = 0; j < tmp.size(); j++) {
            result += patterns[tmp[j]].score;
        }
    }

    return result;
}

// ����������������һ����������
int evaluate(char board[BOARD_WIDTH][BOARD_WIDTH], Role role) {

    if (role == COMPUTOR) {
        return allScore[1];
    } else if (role == HUMAN) {
        return allScore[0];
    }

    cout << "error" << endl;

    return 0;
}

void updateScore(char board[BOARD_WIDTH][BOARD_WIDTH], Position p) {

    string lines[4];
    string lines1[4];
    unsigned int i, j;
    int role = HUMAN;

    // ��
    for (i = 0; i < BOARD_WIDTH; i++) {

        lines[0].push_back(board[i][p.y] == role ? '1'
                           : board[i][p.y] == 0  ? '0'
                                                 : '2');
        lines1[0].push_back(board[i][p.y] == role ? '2'
                            : board[i][p.y] == 0  ? '0'
                                                  : '1');
    }
    // ��
    for (i = 0; i < BOARD_WIDTH; i++) {

        lines[1].push_back(board[p.x][i] == role ? '1'
                           : board[p.x][i] == 0  ? '0'
                                                 : '2');
        lines1[1].push_back(board[p.x][i] == role ? '2'
                            : board[p.x][i] == 0  ? '0'
                                                  : '1');
    }
    // ��б��
    for (i = p.x - min(p.x, p.y), j = p.y - min(p.x, p.y);
         i < BOARD_WIDTH && j < BOARD_WIDTH; i++, j++) {

        lines[2].push_back(board[i][j] == role ? '1'
                           : board[i][j] == 0  ? '0'
                                               : '2');
        lines1[2].push_back(board[i][j] == role ? '2'
                            : board[i][j] == 0  ? '0'
                                                : '1');
    }
    // б��
    for (i = p.x + min(p.y, BOARD_WIDTH - 1 - p.x),
        j = p.y - min(p.y, BOARD_WIDTH - 1 - p.x);
         i < BOARD_WIDTH && j < BOARD_WIDTH; i--, j++) {

        lines[3].push_back(board[i][j] == role ? '1'
                           : board[i][j] == 0  ? '0'
                                               : '2');
        lines1[3].push_back(board[i][j] == role ? '2'
                            : board[i][j] == 0  ? '0'
                                                : '1');
    }

    int lineScore[4];
    int line1Score[4];
    memset(lineScore, 0, sizeof(lineScore));
    memset(line1Score, 0, sizeof(line1Score));

    // �������
    for (i = 0; i < 4; i++) {
        vector<int> result = acs.ACSearch(lines[i]);
        for (j = 0; j < result.size(); j++) {
            lineScore[i] += patterns[result[j]].score;
        }

        result = acs.ACSearch(lines1[i]);
        for (j = 0; j < result.size(); j++) {
            line1Score[i] += patterns[result[j]].score;
        }
    }

    int a = p.y;
    int b = BOARD_WIDTH + p.x;
    int c = 2 * BOARD_WIDTH + (p.y - p.x + 10);
    int d = 2 * BOARD_WIDTH + 21 + (p.x + p.y - 4);
    // ��ȥ��ǰ�ļ�¼
    for (i = 0; i < 2; i++) {
        allScore[i] -= scores[i][a];
        allScore[i] -= scores[i][b];
    }

    // scores˳�� �����ᡢ\��/
    scores[0][a] = lineScore[0];
    scores[1][a] = line1Score[0];
    scores[0][b] = lineScore[1];
    scores[1][b] = line1Score[1];

    // �����µļ�¼
    for (i = 0; i < 2; i++) {
        allScore[i] += scores[i][a];
        allScore[i] += scores[i][b];
    }

    if (p.y - p.x >= -10 && p.y - p.x <= 10) {

        for (i = 0; i < 2; i++)
            allScore[i] -= scores[i][c];

        scores[0][c] = lineScore[2];
        scores[1][c] = line1Score[2];

        for (i = 0; i < 2; i++)
            allScore[i] += scores[i][c];
    }

    if (p.x + p.y >= 4 && p.x + p.y <= 24) {

        for (i = 0; i < 2; i++)
            allScore[i] -= scores[i][d];

        scores[0][d] = lineScore[3];
        scores[1][d] = line1Score[3];

        for (i = 0; i < 2; i++)
            allScore[i] += scores[i][d];
    }
}

// alpha-beta��֦
int abSearch(char board[BOARD_WIDTH][BOARD_WIDTH], int depth, int alpha,
             int beta, Role currentSearchRole) {
    HashItem::Flag flag = HashItem::ALPHA;
    int score = getHashItemScore(depth, alpha, beta);
    if (score != UNKNOWN_SCORE && depth != DEPTH) {
        return score;
    }

    int score1 = evaluate(board, currentSearchRole);
    int score2 = evaluate(board, currentSearchRole == HUMAN ? COMPUTOR : HUMAN);

    if (score1 >= 50000) {
        return MAX_SCORE - 1000 - (DEPTH - depth);
    }
    if (score2 >= 50000) {
        return MIN_SCORE + 1000 + (DEPTH - depth);
    }

    if (depth == 0) {
        recordHashItem(depth, score1 - score2, HashItem::EXACT);
        return score1 - score2;
    }

    // set<Position> possiblePossitions = createPossiblePosition(board);

    int count = 0;
    set<Position> possiblePositions;
    const set<Position> &tmpPossiblePositions =
        ppm.GetCurrentPossiblePositions();

    // �Ե�ǰ���ܳ��ֵ�λ�ý��д�������
    set<Position>::iterator iter;
    for (iter = tmpPossiblePositions.begin();
         iter != tmpPossiblePositions.end(); iter++) {
        possiblePositions.insert(
            Position(iter->x, iter->y, evaluatePoint(board, *iter)));
    }

    while (!possiblePositions.empty()) {
        Position p = *possiblePositions.begin();

        possiblePositions.erase(possiblePositions.begin());

        // ��������
        board[p.x][p.y] = currentSearchRole;
        currentZobristValue ^=
            boardZobristValue[currentSearchRole - 1][p.x][p.y];
        updateScore(board, p);

        // ���ӿ��ܳ��ֵ�λ��
        p.score = 0;
        ppm.AddPossiblePositions(board, p);

        int val = -abSearch(board, depth - 1, -beta, -alpha,
                            currentSearchRole == HUMAN ? COMPUTOR : HUMAN);
        if (depth == DEPTH) {
            // cout << "score(" << p.x << "," << p.y << "):" << val << endl;
        }

        // ȡ����һ�����ӵĿ��ܳ��ֵ�λ��
        ppm.Rollback();

        // ȡ������
        board[p.x][p.y] = 0;
        currentZobristValue ^=
            boardZobristValue[currentSearchRole - 1][p.x][p.y];
        updateScore(board, p);

        if (val >= beta) {
            recordHashItem(depth, beta, HashItem::BETA);
            return beta;
        }
        if (val > alpha) {
            flag = HashItem::EXACT;
            alpha = val;
            if (depth == DEPTH) {
                searchResult = p;
            }
        }

        count++;
        if (count >= 9) {
            break;
        }
    }

    recordHashItem(depth, alpha, flag);
    return alpha;
}

// �����һ�����߷�
Position getAGoodMove(char board[BOARD_WIDTH][BOARD_WIDTH]) {
    int score = abSearch(board, DEPTH, MIN_SCORE, MAX_SCORE, COMPUTOR);
    if (score >= MAX_SCORE - 1000 - 1) {
        winner = COMPUTOR;
    } else if (score <= MIN_SCORE + 1000 + 1) {
        winner = HUMAN;
    }
    return searchResult;
}

// ��ʼ�����������������ͷ�ֵ
void init() {
    vector<string> patternStrs;
    for (size_t i = 0; i < patterns.size(); i++) {
        patternStrs.push_back(patterns[i].pattern);
    }

    // ��ʼ��ACSearcher
    acs.LoadPattern(patternStrs);
    acs.BuildGotoTable();
    acs.BuildFailTable();

    randomBoardZobristValue();
    currentZobristValue = random64();
}

// �������
void printBoard(char board[BOARD_WIDTH][BOARD_WIDTH]) {
    int i, j;
    for (i = 0; i < BOARD_WIDTH; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            cout << (int)board[i][j] << " ";
        }
        cout << endl;
    }
}

////�����Ƕ���ӿڵ�ʵ��

// �ڿ�ʼ֮ǰ��һЩ��ʼ������
void beforeStart() {
    memset(board, EMPTY, BOARD_WIDTH * BOARD_WIDTH * sizeof(char));
    memset(scores, 0, sizeof(scores));
    init();
}

// �ж��Ƿ���ĳһ��Ӯ��
int isSomeOneWin() {
    if (winner == HUMAN)
        return 0;
    if (winner == COMPUTOR)
        return 1;

    return -1;
}

// ����
string takeBack() {
    if (moves.size() < 2) {
        cout << "can not take back" << endl;

        string resultStr;
        int i, j;
        for (i = 0; i < BOARD_WIDTH; i++) {
            for (j = 0; j < BOARD_WIDTH; j++) {
                resultStr.push_back(board[i][j] + 48);
            }
        }

        // printBoard(board);

        return resultStr;
    }

    Position previousPosition = moves.top();
    moves.pop();
    currentZobristValue ^=
        boardZobristValue[COMPUTOR - 1][previousPosition.x][previousPosition.y];
    board[previousPosition.x][previousPosition.y] = EMPTY;
    updateScore(board, previousPosition);

    previousPosition = moves.top();
    moves.pop();
    currentZobristValue ^=
        boardZobristValue[HUMAN - 1][previousPosition.x][previousPosition.y];
    board[previousPosition.x][previousPosition.y] = EMPTY;
    updateScore(board, previousPosition);

    ppm.Rollback();
    ppm.Rollback();

    string resultStr;
    int i, j;
    for (i = 0; i < BOARD_WIDTH; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            resultStr.push_back(board[i][j] + 48);
        }
    }

    // printBoard(board);

    winner = -1;

    return resultStr;
}

// ���֮ǰ�ļ�¼�����¿���
string reset(int role) {
    char chs[15 * 15 + 1];
    memset(chs, '0', 15 * 15);
    memset(scores, 0, sizeof(scores));
    memset(allScore, 0, sizeof(allScore));

    winner = -1;
    // ��ʼ�������ܷ���Ϊ0

    while (!moves.empty()) {
        moves.pop();
    }

    int i;
    for (i = 0; i < HASH_ITEM_INDEX_MASK + 1; i++) {
        hashItems[i].flag = HashItem::EMPTY;
    }

    // ��ʼ������
    memset(board, EMPTY, BOARD_WIDTH * BOARD_WIDTH * sizeof(char));

    // �����һ�ֿ��ܳ��ֵ�λ��
    ppm.RemoveAll();

    // �û�����
    if (role == 0) {
        // do nothing
    }
    // ��������
    else if (role == 1) {
        currentZobristValue ^= boardZobristValue[COMPUTOR - 1][7][7];
        board[7][7] = COMPUTOR;
        updateScore(board, Position(7, 7));

        moves.push(Position(7, 7));
        searchResult = Position(7, 7);

        ppm.AddPossiblePositions(board, Position(7, 7));

        // ��һ��Ĭ����7��7��λ��
        chs[7 + 7 * 15] = '2';
    }

    winner = -1;

    return string(chs);
}

// �������ò���
void setLevel(int level) { DEPTH = level; }

// ȡ�øղŵ����µ���һ�����ӵ�λ��
Position getLastPosition() { return searchResult; }

// �������壬�������̣���������
string nextStep(int x, int y) {

    moves.push(Position(x, y));

    board[x][y] = HUMAN;
    currentZobristValue ^= boardZobristValue[HUMAN - 1][x][y];
    updateScore(board, Position(x, y));

    // ���ӿ��ܳ��ֵ�λ��
    ppm.AddPossiblePositions(board, Position(x, y));

    Position result = getAGoodMove(board);

    board[result.x][result.y] = COMPUTOR;
    currentZobristValue ^= boardZobristValue[COMPUTOR - 1][result.x][result.y];
    updateScore(board, result);

    // ���ӿ��ܳ��ֵ�λ��
    ppm.AddPossiblePositions(board, result);

    // ��˫����δ����ʤ�����������λ�ü��뵽��ʷ��¼��
    if (winner == -1)
        moves.push(Position(result.x, result.y));

    string resultStr;
    int i, j;
    for (i = 0; i < BOARD_WIDTH; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            resultStr.push_back(board[i][j] + 48);
        }
    }

    // printBoard(board);

    return resultStr;
}

// ��ȡ����
vector<Position> getChessManual() {
    vector<Position> result;

    while (!moves.empty()) {
        result.insert(result.begin(), moves.top());
        moves.pop();
    }

    return result;
}

}; // namespace ChessEngine
