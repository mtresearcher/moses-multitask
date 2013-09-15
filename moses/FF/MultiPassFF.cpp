#include "MultiPassFF.h"
#include "moses/SearchGraph.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"

namespace Moses
{
void MultiPassFF::Evaluate(Edge &edge ) const
{
	const Hypothesis &hypo = edge.GetHypothesis();
	ScoreComponentCollection &scores = const_cast<ScoreComponentCollection&>(hypo.GetScoreBreakdown());
	scores.PlusEquals(this, 1.0);
}

}

