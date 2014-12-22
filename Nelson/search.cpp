#include "search.h"
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h> 

ValuatedMove search::Think(position &pos, SearchStopCriteria ssc) {
	searchStopCriteria = ssc;
	int64_t thinkTime = 1;
	int64_t * nodeCounts = new int64_t[ssc.MaxDepth+1];
	nodeCounts[0] = 0;
	//Iterativ Deepening Loop
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	rootMoves = new ValuatedMove[rootMoveCount];
	memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	for (int depth = 1; depth <= ssc.MaxDepth; ++depth) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		Search<ROOT>(alpha, beta, pos, depth, &PVMoves[0]);
		stable_sort(rootMoves, &rootMoves[rootMoveCount], sortByScore);
		BestMove = rootMoves[0];
		if (Stop) break;
		int64_t tNow = now();
		thinkTime = tNow - ssc.StartTime;
		if (Stop || tNow > ssc.SoftStopTime || (3 * (tNow - ssc.StartTime) + ssc.StartTime) > ssc.SoftStopTime || (abs(int(BestMove.score)) > int(VALUE_MATE_THRESHOLD) && abs(int(BestMove.score)) <= int(VALUE_MATE))) break;
		nodeCounts[depth] = NodeCount - nodeCounts[depth - 1];
		if (depth > 3) BranchingFactor = sqrt(1.0 * nodeCounts[depth] / nodeCounts[depth - 2]);
		if (UciOutput && thinkTime > 200) {
			if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
				cout << "info depth " << depth << " nodes " << NodeCount << " score cp " << BestMove.score << " nps " << NodeCount * 1000 / thinkTime
				//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits() 
				<< " pv " << PrincipalVariation(depth) << endl;
			else {
				int pliesToMate;
				if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
				cout << "info depth " << depth << " nodes " << NodeCount << " score mate " << pliesToMate / 2 << " nps " << NodeCount * 1000 / thinkTime
					//<< " hashfull " << tt::Hashfull() << " tbhits " << tablebase::GetTotalHits() 
					<< " pv " << PrincipalVariation(depth) << endl;
			}
		}
	}
	delete[] rootMoves;
	delete[] nodeCounts;
	return BestMove;
}

template<> Value search::Search<ROOT>(Value alpha, Value beta, position &pos, int depth, Move * pv) {
	Value score;
	Value bestScore = -VALUE_MATE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	for (int i = 0; i < rootMoveCount; ++i) {
		position next(pos);
		next.ApplyMove(rootMoves[i].move);
		score = -Search<PV>(-beta, -alpha, next, depth - 1, &subpv[0]);
		rootMoves[i].score = score;
		if (score >= beta) {
			return score;
		}
		if (score > bestScore) {
			bestScore = score;
			if (score > alpha)
			{
				alpha = score;
				pv[0] = rootMoves[i].move;
				memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
			}
		}
	}
	return bestScore;
}

template<NodeType NT> Value search::Search(Value alpha, Value beta, position &pos, int depth, Move * pv) {
	if (pos.GetResult() != OPEN) {
		++NodeCount;
		return pos.evaluate();
	}
	if (depth <= 0) {
		return QSearch<STANDARD>(alpha, beta, pos, depth);
	}
	++NodeCount;
	Stop = Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime);
	if (Stop) return VALUE_ZERO;
	Value score;
	Value bestScore = -VALUE_MATE;
	Move subpv[PV_MAX_LENGTH];
	pv[0] = MOVE_NONE;
	pos.InitializeMoveIterator<MAIN_SEARCH>();
	Move move;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -Search<PV>(-beta, -alpha, next, depth - 1, &subpv[0]);
			if (score >= beta) {
				return score;
			}
			if (score > bestScore) {
				bestScore = score;
				if (score > alpha)
				{
					alpha = score;
					pv[0] = move;
					memcpy(pv + 1, subpv, (PV_MAX_LENGTH - 1)*sizeof(Move));
				}
			}
		}
	}
	return bestScore;
}

template<NodeType NT> Value search::QSearch(Value alpha, Value beta, position &pos, int depth) {
	++QNodeCount;
	++NodeCount;
	Stop = Stop || ((NodeCount & MASK_TIME_CHECK) == 0 && now() >= searchStopCriteria.HardStopTime);
	if (Stop) return VALUE_ZERO;
	Value standPat = pos.evaluate();
	if (standPat > beta || pos.GetResult() != OPEN) return beta;
	if (alpha < standPat) alpha = standPat;
	pos.InitializeMoveIterator<QSEARCH>();
	Move move;
	Value score;
	while ((move = pos.NextMove())) {
		position next(pos);
		if (next.ApplyMove(move)) {
			score = -QSearch<STANDARD>(-beta, -alpha, next, depth - 1);
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}
	}
	return alpha;
}

string search::PrincipalVariation(int depth) {
	std::stringstream ss;
	for (int i = 0; i < depth; ++i) {
		if (PVMoves[i] == MOVE_NONE) break;
		if (i>0) ss << " ";
		ss << toString(PVMoves[i]);
	}
	return ss.str();
}

