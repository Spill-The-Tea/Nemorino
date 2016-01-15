#pragma once
#include <vector>
#include "types.h"
#include "board.h"
#include "position.h"
#include "pgn.h"

enum PerftType { BASIC,  //All moves are generated together
	             P1, //tactical and Quiet Moves are generated seperately
                 P2, //Winning, Equal, Loosing Captures and Quiets are generated separately
                 P3,  //Move iterator is used
	             P4   //Legal move generation
};

int64_t bench(std::vector<std::string> fens, int depth);
int64_t bench(int depth); //Benchmark positions from SF
int64_t bench2(int depth); //Random positions
int64_t bench(std::string filename, int depth);


uint64_t perft(position &pos, int depth);
uint64_t perft3(position &pos, int depth);
uint64_t perft4(position &pos, int depth);
uint64_t perftcomb(position &pos, int depth);

void divide(position &pos, int depth);
void divide3(position &pos, int depth);
bool testPerft(PerftType perftType = BASIC);
void testPolyglotKey();
bool testSEE();
std::vector<std::string> readTextFile(std::string file);
void testCheckQuietCheckMoveGeneration();
void testTacticalMoveGeneration();
void testSearch(position &pos, int depth);
void testFindMate();
void testResult();
void testMateInDos();
void testRepetition();
void testKPK();

#ifdef TB
void testTB();
#endif
