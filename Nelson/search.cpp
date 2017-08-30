#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <math.h>
#include <cstring>
#include <thread>
#include <chrono>
#include "search.h"
#include "hashtables.h"

void Search::Reset() {
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_ZERO;
	NodeCount = 0;
	QNodeCount = 0;
	MaxDepth = 0;
	cutoffAt1stMove = 0;
	cutoffCount = 0;
	cutoffMoveIndexSum = 0;
	Stop.store(false);
	PonderMode.store(false);
	threadLocalData.History.age();
	searchMoves.clear();
	threadLocalData.cmHistory.age();
	threadLocalData.followupHistory.age();
	threadLocalData.killerManager.clear();
	for (int i = 0; i < PV_MAX_LENGTH; ++i) PVMoves[i] = MOVE_NONE;
}

void Search::NewGame() {
	Reset();
	for (int i = 0; i < 12; ++i) {
		for (int j = 0; j < 64; ++j)counterMove[i][j] = MOVE_NONE;
	}
	threadLocalData.cmHistory.initialize();
	threadLocalData.History.initialize();
	threadLocalData.followupHistory.initialize();
}

std::string Search::PrincipalVariation(Position & pos, int depth) {
	std::stringstream ss;
	//When pondering start PV with ponder move 
	if (PonderMode.load() && pos.GetLastAppliedMove() != MOVE_NONE) ss << toString(pos.GetLastAppliedMove()) << " ";
	int i = 0;
	ponderMove = MOVE_NONE;
	//First get PV from PV array...
	for (; i < depth && i < PV_MAX_LENGTH; ++i) {
		if (PVMoves[i] == MOVE_NONE || !pos.validateMove(PVMoves[i])) break;
		Position next(pos);
		if (!next.ApplyMove(PVMoves[i])) break;
		pos = next;
		if (i > 0) ss << " ";
		ss << toString(PVMoves[i]);
		if (i == 1) ponderMove = PVMoves[i];
	}
	//...then continue with moves from transposition table
	for (; i < depth; ++i) {
		Move hashmove = settings::parameter.HelperThreads == 0 ? tt::hashmove<tt::UNSAFE>(pos.GetHash()) : tt::hashmove<tt::THREAD_SAFE>(pos.GetHash());
		if (hashmove == MOVE_NONE || !pos.validateMove(hashmove)) break;
		Position next(pos);
		if (!next.ApplyMove(hashmove)) break;
		pos = next;
		if (i > 0) ss << " ";
		ss << toString(hashmove);
		if (i == 1) ponderMove = hashmove;
	}
	return ss.str();
}

//Creates the "thinking output" while running in UCI or XBoard mode
void Search::info(Position &pos, int pvIndx, SearchResultType srt) {
	if ((UciOutput || XBoardOutput)) {
		Position npos(pos);
		npos.copy(pos);
		if (UciOutput) {
			std::string srtString;
			if (srt == SearchResultType::FAIL_LOW) srtString = " upperbound"; else if (srt == SearchResultType::FAIL_HIGH) srtString = " lowerbound";
			if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD))
				sync_cout << "info depth " << _depth << " seldepth " << std::max(MaxDepth, _depth) << " multipv " << pvIndx + 1 << " score cp " << (int)BestMove.score << srtString << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
				<< " hashfull " << tt::GetHashFull()
				<< " tbhits " << tbHits
				<< " time " << _thinkTime
				<< " pv " << PrincipalVariation(npos, _depth) << sync_endl;
			else {
				int pliesToMate;
				if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
				sync_cout << "info depth " << _depth << " seldepth " << std::max(MaxDepth, _depth) << " multipv " << pvIndx + 1 << " score mate " << pliesToMate / 2 << srtString << " nodes " << NodeCount << " nps " << NodeCount * 1000 / _thinkTime
					<< " hashfull " << tt::GetHashFull()
					<< " tbhits " << tbHits
					<< " time " << _thinkTime
					<< " pv " << PrincipalVariation(npos, _depth) << sync_endl;
			}
		}
		else if (XBoardOutput) {
			if (_depth < 5) return;
			const char srtChar[3] = { ' ', '?', '!' };
			int xscore = BestMove.score;
			if (abs(int(BestMove.score)) > int(VALUE_MATE_THRESHOLD)) {
				if (int(BestMove.score) > 0) {
					int pliesToMate = VALUE_MATE - BestMove.score;
					xscore = 100000 + pliesToMate;
				}
				else {
					int pliesToMate = -BestMove.score - VALUE_MATE;
					xscore = -100000 - pliesToMate;
				}
			}
			sync_cout << _depth << " " << xscore << " " << _thinkTime / 10 << " " << NodeCount << " " /*<< std::max(MaxDepth, _depth) << " " << (NodeCount / _thinkTime) * 1000
				<< " " << tbHits
				<< "\t"*/ << PrincipalVariation(npos, _depth) << srtChar[srt] << sync_endl;
		}
	}
}

void Search::debugInfo(std::string info)
{
	if (UciOutput) {
		sync_cout << "info string " << info << sync_endl;
	}
	else if (XBoardOutput) {
		sync_cout << "# " << info << sync_endl;
	}
	//else
	//{
	//	sync_cout << info << sync_endl;
	//}
}

Move Search::GetBestBookMove(Position& pos, ValuatedMove * moves, int moveCount) {
	if (settings::options.getBool(settings::OPTION_OWN_BOOK) && BookFile != nullptr) {
		if (book == nullptr) book = new polyglot::Book(*BookFile);
		book->probe(pos, true, moves, moveCount);
	}
	return MOVE_NONE;
}

std::string Search::GetXAnalysisOutput() {
	std::lock_guard<std::mutex> lck(mtxXAnalysisOutput);
	if (XAnalysisOutput == nullptr) XAnalysisOutput = new std::string();
	return *XAnalysisOutput;
}

Search::Search() {
	PonderMode.store(false);
	Stop.store(false);
	for (int i = 0; i < 12; ++i)
		for (int j = 0; j < 64; ++j)
			counterMove[i][j] = MOVE_NONE;
	BestMove.move = MOVE_NONE;
	BestMove.score = VALUE_NOTYETDETERMINED;
	threadLocalData.cmHistory.initialize();
	threadLocalData.followupHistory.initialize();
	threadLocalData.History.initialize();
	_thinkTime = 0;
	for (int i = 0; i < PV_MAX_LENGTH; ++i) PVMoves[i] = MOVE_NONE;
}

Search::~Search() {
	//stop and delete Slave Threads
	for (int i = 0; i < settings::parameter.HelperThreads; ++i) {
		if (subThreads[i].joinable()) subThreads[i].join();
	}
	if (book != nullptr) {
		delete book;
		book = nullptr;
	}
	if (BookFile != nullptr) {
		delete BookFile;
		BookFile = nullptr;
	}
	if (XAnalysisOutput != nullptr) {
		delete XAnalysisOutput;
		XAnalysisOutput = nullptr;
	}
}

ValuatedMove Search::Think(Position &pos) {
	std::lock_guard<std::mutex> lgStart(mtxSearch);
#ifdef TRACE
	//pos.move_history->clear();
	utils::clearSearchTree();
#endif
	//Initialize Engine before starting the new search
	_thinkTime = 1; //to avoid divide by 0 errors
	ponderMove = MOVE_NONE;
	Value score = VALUE_ZERO;
	rootPosition = pos;
	pos.ResetPliesFromRoot();
	settings::parameter.EngineSide = pos.GetSideToMove();
	tt::newSearch();
	//Get all root moves
	ValuatedMove * generatedMoves = pos.GenerateMoves<LEGAL>();
	rootMoveCount = pos.GeneratedMoveCount();
	if (rootMoveCount == 0) {
		BestMove.move = MOVE_NONE;
		BestMove.score = VALUE_ZERO;
		info(pos, 0, SearchResultType::UNIQUE_MOVE);
		utils::debugInfo("No valid move!");
		return BestMove;
	}
	if (rootMoveCount == 1) {
		BestMove = *generatedMoves; //if there is only one legal move save time and return move immediately (although there is no score assigned)
		utils::debugInfo("Only one valid move!");
		goto END;
	}
	//check if book is available
	if (settings::options.getBool(settings::OPTION_OWN_BOOK) && BookFile != nullptr) {
		if (book == nullptr) book = new polyglot::Book(*BookFile);
		//Currently engine isn't looking for the best book move, but selects on of the available bookmoves by random, with entries weight used to define the
		//probability 
		Move bookMove = book->probe(pos, false, generatedMoves, rootMoveCount);
		if (bookMove != MOVE_NONE) {
			BestMove.move = bookMove;
			BestMove.score = VALUE_ZERO;
			utils::debugInfo("Book move");
			//Try to find a suitable move for pondering
			Position next(pos);
			if (next.ApplyMove(bookMove)) {
				ValuatedMove* replies = next.GenerateMoves<LEGAL>();
				ponderMove = book->probe(next, true, replies, next.GeneratedMoveCount());
				if (ponderMove == MOVE_NONE && next.GeneratedMoveCount() > 0) ponderMove = replies->move;
			}
			info(pos, 0, SearchResultType::BOOK_MOVE);
			goto END;
		}
	}
	//If a search move list is provided replace root moves by search moves
	if (searchMoves.size()) {
		rootMoveCount = int(searchMoves.size());
		for (int i = 0; i < rootMoveCount; ++i) rootMoves[i].move = searchMoves[i];
		searchMoves.clear();
	}
	else {
		memcpy(rootMoves, generatedMoves, rootMoveCount * sizeof(ValuatedMove));
	}
	//Root probing of tablebases. This is done as suggested by SF: Keep only the winning, resp. drawing, moves in the move list
	//and then search normally. This way the engine will play "better" than by simply choosing the "best" tablebase move (which is
	//the move which minimizes the number until drawPlyCount is reset without changing the result
	tbHits = 0;
	probeTB = tablebases::MaxCardinality > 0;
	if (pos.GetMaterialTableEntry()->IsTablebaseEntry()) {
		std::vector<ValuatedMove> tbMoves(rootMoves, rootMoves + rootMoveCount);
		bool tbHit = tablebases::root_probe(pos, tbMoves, score);
		if (tbHit) {
			tbHits++;
			probeTB = false;
			rootMoveCount = (int)tbMoves.size();
			std::copy(tbMoves.begin(), tbMoves.end(), rootMoves);
			if (rootMoveCount == 1) {
				rootMoves[0].score = score;
				BestMove = rootMoves[0]; //if tablebase probe only returns one move => play it and done!
				info(pos, 0, SearchResultType::TABLEBASE_MOVE);
				utils::debugInfo("Tablebase move", toString(BestMove.move));
				goto END;
			}
		}
	}
	//Initialize PV-Array
	std::fill_n(PVMoves, PV_MAX_LENGTH, MOVE_NONE);
	//SMP active => start helper threads (if not yet done)
	for (int i = 0; i < settings::parameter.HelperThreads; ++i)
		subThreads.push_back(std::thread(&Search::startHelper, this));
	//Special logic to get static evaluation via UCI: if go depth 0 is requested simply return static evaluation
	if (timeManager.GetMaxDepth() == 0) {
		BestMove.move = MOVE_NONE;
		BestMove.score = pos.evaluate();
		if (abs(int(BestMove.score)) <= int(VALUE_MATE_THRESHOLD)) sync_cout << "info score cp " << (int)BestMove.score << sync_endl;
		else {
			int pliesToMate;
			if (int(BestMove.score) > 0) pliesToMate = VALUE_MATE - BestMove.score; else pliesToMate = -BestMove.score - VALUE_MATE;
			sync_cout << "info score mate " << pliesToMate / 2 << sync_endl;
		}
		return BestMove;
	}
	//Iterativ Deepening Loop
	for (_depth = 1; _depth < timeManager.GetMaxDepth(); ++_depth) {
		Value alpha, beta, delta = Value(20);
		for (int pvIndx = 0; pvIndx < MultiPv && pvIndx < rootMoveCount; ++pvIndx) {
			if (_depth >= 5 && MultiPv == 1 && std::abs(int16_t(score)) < VALUE_KNOWN_WIN) {
				//set aspiration window
				alpha = std::max(score - delta, -VALUE_INFINITE);
				beta = std::min(score + delta, VALUE_INFINITE);
			}
			else {
				alpha = -VALUE_MATE;
				beta = VALUE_MATE;
			}
			while (true) {
				if (settings::parameter.HelperThreads > 0)
					score = SearchRoot<MASTER>(alpha, beta, pos, _depth, rootMoves, PVMoves, threadLocalData, pvIndx);
				else
					score = SearchRoot<SINGLE>(alpha, beta, pos, _depth, rootMoves, PVMoves, threadLocalData, pvIndx);
				//Best move is already in first place, this is assured by SearchRoot
				//therefore we sort only the other moves
				std::stable_sort(rootMoves + pvIndx + 1, &rootMoves[rootMoveCount], sortByScore);
				if (Stopped()) {
					break;
				}
				if (score <= alpha) {
					//fail-low
					beta = (alpha + beta) / 2;
					alpha = std::max(score - delta, -VALUE_INFINITE);
					info(pos, pvIndx, SearchResultType::FAIL_LOW);
					//inform timemanager to assigne more time
					if (!PonderMode.load()) timeManager.reportFailLow();
				}
				else if (score >= beta) {
					//fail-high
					alpha = (alpha + beta) / 2;
					beta = std::min(score + delta, VALUE_INFINITE);
					info(pos, pvIndx, SearchResultType::FAIL_HIGH);
				}
				else {
					//Iteration completed
					BestMove = rootMoves[pvIndx];
					score = BestMove.score;
					break;
				}
				if (std::abs(int16_t(score)) >= VALUE_KNOWN_WIN) {
					alpha = -VALUE_MATE;
					beta = VALUE_MATE;
				}
				else delta += delta / 4 + 5; //widening formula is from SF - very small widening of the aspiration window size, more might be better => let's tune it some day
			}
			Time_t tNow = now();
			_thinkTime = std::max(tNow - timeManager.GetStartTime(), int64_t(1));
			Stop.load();
			if (!Stopped()) {
				//check if new deeper iteration shall be started
				if (!timeManager.ContinueSearch(_depth, BestMove, NodeCount, tNow, PonderMode)) {
					Stop.store(true);
				}
			}
			else debugInfo("Iteration cancelled!");
			//send information to GUI
			info(pos, pvIndx);
			if (Stopped()) break;
		}
		if (Stopped()) break;
	}
	Stop.store(true);
END://when pondering engine must not return a best move before opponent moved => therefore let main thread wait	
	bool infoSent = false;
	while (PonderMode.load()) {
		if (!infoSent) utils::debugInfo("Waiting for opponent..");
		infoSent = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	if (PVMoves[0] != MOVE_NONE && PVMoves[0] != BestMove.move) {
		std::stringstream ss;
		ss << "PV Move: " << toString(PVMoves[0]) << " Bestmove: " << toString(BestMove.move);
		utils::debugInfo(ss.str());
		BestMove.move = PVMoves[0];
	}
	//If for some reason search did not find a best move return the  first one (to avoid loss and it's anyway the best guess then)
	if (BestMove.move == MOVE_NONE) BestMove = rootMoves[0];
	if (settings::parameter.HelperThreads > 0 && rootMoveCount > 1) {
		for (int i = 0; i < subThreads.size(); ++i) subThreads[i].join();
		subThreads.clear();
	}
	return BestMove;
}

//slave thread
void Search::startHelper() {
	int depth = 1;
	Move * PVMovesLocal = new Move[PV_MAX_LENGTH];
	ValuatedMove * moves = new ValuatedMove[MAX_MOVE_COUNT];
	memcpy(moves, rootMoves, MAX_MOVE_COUNT * sizeof(ValuatedMove));
	ThreadData * h = new ThreadData;
	//Iterative Deepening Loop
	while (!Stop.load() && depth < MAX_DEPTH) {
		Value alpha = -VALUE_MATE;
		Value beta = VALUE_MATE;
		SearchRoot<SLAVE>(alpha, beta, rootPosition, depth, moves, PVMovesLocal, *h);
		++depth;
	}
	delete h;
	delete[] moves;
	delete[] PVMovesLocal;
}

bool Search::isQuiet(Position &pos) {
	Value evaluationDiff = pos.GetStaticEval() - QSearch<SINGLE>(-VALUE_MATE, VALUE_MATE, pos, 0, threadLocalData);
	return std::abs(int16_t(evaluationDiff)) <= 30;
}

Value Search::qscore(Position * pos)
{
	return QSearch<SINGLE>(-VALUE_MATE, VALUE_MATE, *pos, 0, threadLocalData);
}

void Search::updateCutoffStats(ThreadData& tlData, const Move cutoffMove, int depth, Position &pos, int moveIndex) {
	cutoffMoveIndexSum += std::max(0, moveIndex);
	cutoffCount++;
	cutoffAt1stMove += moveIndex <= 0;
	if (moveIndex == -1 || pos.IsQuiet(cutoffMove)) {
		Piece movingPiece = pos.GetPieceOnSquare(from(cutoffMove));
		Square toSquare = to(cutoffMove);
		if (moveIndex >= 0) {
			tlData.killerManager.store(pos, cutoffMove);
		}
		Value v = Value(depth * depth);
		tlData.History.update(-depth * tlData.History.getValue(movingPiece, cutoffMove) / 64, movingPiece, cutoffMove);
		tlData.History.update(v, movingPiece, cutoffMove);
		Piece prevPiece = BLANK;
		Square prevTo = OUTSIDE;
		Piece prev2Piece = BLANK;
		Square prev2To = OUTSIDE;
		//Piece ownPrevPiece = BLANK;
		//Square ownPrevTo = OUTSIDE;
		Move lastApplied;
		if ((lastApplied = FixCastlingMove(pos.GetLastAppliedMove())) != MOVE_NONE) {
			prevTo = to(lastApplied);
			prevPiece = pos.GetPieceOnSquare(prevTo);
			counterMove[int(pos.GetPieceOnSquare(prevTo))][prevTo] = cutoffMove;
			tlData.cmHistory.update(-depth * tlData.cmHistory.getValue(prevPiece, prevTo, movingPiece, toSquare) / 64, prevPiece, prevTo, movingPiece, toSquare);
			tlData.cmHistory.update(v, prevPiece, prevTo, movingPiece, toSquare);
			Move lastApplied2;
			if (pos.Previous() && (lastApplied2 = FixCastlingMove(pos.Previous()->GetLastAppliedMove())) != MOVE_NONE) {
				prev2To = to(lastApplied2);
				prev2Piece = pos.Previous()->GetPieceOnSquare(prev2To);
				tlData.followupHistory.update(-depth *tlData.followupHistory.getValue(prev2Piece, prev2To, movingPiece, toSquare) / 64, prev2Piece, prev2To, movingPiece, toSquare);
				tlData.followupHistory.update(v, prev2Piece, prev2To, movingPiece, toSquare);
			}
		}
		if (moveIndex > 0) {
			int moveCount;
			ValuatedMove * alreadyProcessedQuiets = pos.GetMoves(moveCount);
			moveCount = std::min(moveIndex, moveCount);
			for (int i = 0; i < moveCount; ++i) {
				if (alreadyProcessedQuiets->move != cutoffMove && pos.IsQuiet(alreadyProcessedQuiets->move)) {
					Move alreadyProcessedMove = FixCastlingMove(alreadyProcessedQuiets->move);
					movingPiece = pos.GetPieceOnSquare(from(alreadyProcessedMove));
					toSquare = to(alreadyProcessedMove);
					tlData.History.update(-depth * tlData.History.getValue(movingPiece, alreadyProcessedMove) / 64, movingPiece, alreadyProcessedMove);
					tlData.History.update(-v, movingPiece, alreadyProcessedMove);
					if (pos.GetLastAppliedMove() != MOVE_NONE) {
						tlData.cmHistory.update(-depth * tlData.cmHistory.getValue(prevPiece, prevTo, movingPiece, toSquare) / 64, prevPiece, prevTo, movingPiece, toSquare);
						tlData.cmHistory.update(-v, prevPiece, prevTo, movingPiece, toSquare);
					}
					if (prev2To != Square::OUTSIDE) {
						tlData.followupHistory.update(-depth *tlData.followupHistory.getValue(prev2Piece, prev2To, movingPiece, toSquare) / 64, prev2Piece, prev2To, movingPiece, toSquare);
						tlData.followupHistory.update(-v, prev2Piece, prev2To, movingPiece, toSquare);
					}
				}
				//if (ownPrevTo != OUTSIDE) {
				//	cmHistory.update(-v, ownPrevPiece, ownPrevTo, movingPiece, toSquare);
				//}
				alreadyProcessedQuiets++;
			}
		}
	}
}
