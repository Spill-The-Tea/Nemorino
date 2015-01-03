#pragma once

#include "types.h"
#include "board.h"

struct evaluation
{
public:
	Value Material;
	eval Mobility;
	eval KingSafety;

	inline Value GetScore(const Phase_t phase, const Color sideToMove) {
		return (Material + (Mobility + KingSafety).getScore(phase)) * (1 - 2 * sideToMove);
	}
};

const eval DrawEval;
const evaluation DrawEvaluation{ Value(0), DrawEval };

typedef evaluation(*EvalFunction)(const position&);

evaluation evaluate(const position& pos);
evaluation evaluateDraw(const position& pos);
evaluation evaluateFromScratch(const position& pos);
eval evaluateMobility(const position& pos);
eval evaluateMobility2(const position& pos);
eval evaluateKingSafety(const position& pos);

 