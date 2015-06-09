#pragma once

#include "types.h"

extern int HashSizeMB;
extern std::string BOOK_FILE;
extern bool USE_BOOK;
extern int HelperThreads;

const int EmergencyTime = 100; //50 ms Emergency time
const int PV_MAX_LENGTH = 32; //Maximum Length of displayed Principal Variation
const int MAX_DEPTH = 128; //Maximal search depth
const int MASK_TIME_CHECK = (1 << 14) - 1;

const Value PieceValuesMG[] { Value(950), Value(520), Value(325), Value(325), Value(80), VALUE_ZERO, VALUE_ZERO };
const Value PieceValuesEG[] { Value(950), Value(520), Value(325), Value(325), Value(100), VALUE_ZERO, VALUE_ZERO };

const int PAWN_TABLE_SIZE = 1 << 14; //has to be power of 2

const eval MOBILITY_BONUS_KNIGHT[] = { eval(-21, -17), eval(-14, -10), eval(-3, -3), eval(1, 0), eval(5, 3), eval(9, 7), eval(12, 9), eval(14, 10), eval(15, 11) };
const eval MOBILITY_BONUS_BISHOP[] = { eval(-17, -16), eval(-9, -8), eval(2, 0), eval(7, 5), eval(11, 10), eval(16, 14),  eval(20, 18), eval(23, 21), eval(24, 23), 
                                       eval(26, 24), eval(27, 25), eval(27, 26), eval(28, 26), eval(29, 27) };
const eval MOBILITY_BONUS_ROOK[] = { eval(-16, -18), eval(-10, -9), eval(-2, 0), eval(0, 5), eval(2, 11), eval(4, 16), eval(6, 21), eval(7, 27), eval(9, 32),
                                     eval(10, 36), eval(10, 38), eval(11, 40), eval(12, 41), eval(12, 41), eval(12, 42) };
const eval MOBILITY_BONUS_QUEEN[] = { eval(-14, -13), eval(-9, -8), eval(-2, -2), eval(0, 0), eval(2, 3), eval(4, 6), eval(4, 10), eval(6, 13), eval(7, 13), eval(7, 14), 
                                      eval(7, 14), eval(7, 14), eval(7, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), 
									  eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14), eval(8, 14) };

const int KING_SAFETY_MAXVAL = 500;
const int KING_SAFETY_MAXINDEX = 61;
const double KING_SAFETY_LINEAR = 0.5;

#define KSV(I) Value(std::min(KING_SAFETY_MAXVAL, int(I*I / (KING_SAFETY_MAXINDEX * KING_SAFETY_MAXINDEX / KING_SAFETY_MAXVAL) + KING_SAFETY_LINEAR * I)))

const Value KING_SAFETY[100] = {
	Value(0), KSV(1), KSV(2), KSV(3), KSV(4), KSV(5), KSV(6), KSV(7), KSV(8), KSV(9),
	KSV(10), KSV(11), KSV(12), KSV(13), KSV(14), KSV(15), KSV(16), KSV(17), KSV(18), KSV(19),
	KSV(20), KSV(21), KSV(22), KSV(23), KSV(24), KSV(25), KSV(26), KSV(27), KSV(28), KSV(29),
	KSV(30), KSV(31), KSV(32), KSV(33), KSV(34), KSV(35), KSV(36), KSV(37), KSV(38), KSV(39),
	KSV(40), KSV(41), KSV(42), KSV(43), KSV(44), KSV(45), KSV(46), KSV(47), KSV(48), KSV(49),
	KSV(50), KSV(51), KSV(52), KSV(53), KSV(54), KSV(55), KSV(56), KSV(57), KSV(58), KSV(59),
	KSV(60), KSV(61), KSV(62), KSV(63), KSV(64), KSV(65), KSV(66), KSV(67), KSV(68), KSV(69),
	KSV(70), KSV(71), KSV(72), KSV(73), KSV(74), KSV(75), KSV(76), KSV(77), KSV(78), KSV(79),
	KSV(80), KSV(81), KSV(82), KSV(83), KSV(84), KSV(85), KSV(86), KSV(87), KSV(88), KSV(89),
	KSV(90), KSV(91), KSV(92), KSV(93), KSV(94), KSV(95), KSV(96), KSV(97), KSV(98), KSV(99)
};

#undef KSV

/*const Value KING_SAFETY[100] = {
	Value(0), KSV(1), KSV(2), KSV(3), KSV(4), KSV(5), KSV(6), KSV(7), KSV(8), KSV(9),
	Value(0), Value(0), Value(1), Value(2), Value(3), Value(5), Value(7), Value(9), Value(12), Value(15),
	Value(18), Value(22), Value(26), Value(30), Value(35), Value(39), Value(44), Value(50), Value(56), Value(62),
	Value(68), Value(75), Value(82), Value(85), Value(89), Value(97), Value(105), Value(113), Value(122), Value(131),
	Value(140), Value(150), Value(169), Value(180), Value(191), Value(202), Value(213), Value(225), Value(237), Value(248),
	Value(260), Value(272), Value(283), Value(295), Value(307), Value(319), Value(330), Value(342), Value(354), Value(366),
	Value(377), Value(389), Value(401), Value(412), Value(424), Value(436), Value(448), Value(459), Value(471), Value(483),
	Value(494), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500),
	Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500), Value(500)
}*/;

// Threat[defended/weak][minor/major attacking][attacked PieceType] contains
// bonuses according to which piece type attacks which one.
const eval Threat[][2][6] = {
	{ { eval(0, 0), eval(0, 0), eval(10, 18), eval(12, 19), eval(22, 49), eval(18, 53) }, // Defended Minor
	{ eval(0, 0), eval(0, 0), eval(5, 7), eval(5, 7), eval(4, 7), eval(12, 24) } }, // Defended Major
	{ { eval(0, 0), eval(0, 16), eval(17, 21), eval(16, 25), eval(21, 50), eval(18, 52) }, // Weak Minor
	{ eval(0, 0), eval(0, 14), eval(13, 29), eval(13, 29), eval(0, 22), eval(12, 26) } } // Weak Major
};

const eval Hanging(16, 13);
const eval KingOnOne(1, 29);
const eval KingOnMany(3, 63);
const eval ROOK_ON_OPENFILE(10, 0);
const eval ROOK_ON_SEMIOPENFILE(10, 0);
const eval ROOK_ON_7TH(20, 0);

const Value BONUS_KNIGHT_OUTPOST = Value(5);
const Value BONUS_BISHOP_OUTPOST = Value(0);

//extern Value PASSED_PAWN_BONUS[4];
const Value PASSED_PAWN_BONUS[4] = { Value(30), Value(37), Value(77), Value(162) };
const Value BONUS_PROTECTED_PASSED_PAWN = Value(30);
const Value MALUS_ISOLATED_PAWN = Value(25);
const Value BONUS_BISHOP_PAIR = Value(0);
const Value BONUS_CASTLING = Value(10);

const Value DELTA_PRUNING_SAFETY_MARGIN = Value(PieceValuesEG[PAWN] >> 1);

const Value PAWN_SHELTER_2ND_RANK = Value(10); 
const Value PAWN_SHELTER_3RD_RANK = Value(5);

const Value BETA_PRUNING_MARGIN[8] = { Value(0), Value(200), Value(400), Value(600), Value(800), Value(1000), Value(1200), Value(1400) };
const Value BETA_PRUNING_FACTOR = Value(120);

const int FULTILITY_PRUNING_DEPTH = 3;
const Value FUTILITY_PRUNING_LIMIT[4] = { VALUE_ZERO, PieceValuesMG[BISHOP], PieceValuesMG[ROOK], PieceValuesMG[QUEEN] };
const Value FUTILITY_PRUNING_MARGIN[4] = { VALUE_ZERO, PieceValuesMG[BISHOP], PieceValuesMG[ROOK], PieceValuesMG[QUEEN] };