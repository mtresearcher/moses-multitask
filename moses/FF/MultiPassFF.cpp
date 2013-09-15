#include "MultiPassFF.h"
#include "moses/SearchGraph.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"

namespace Moses
{
void MultiPassFF::Evaluate(SearchGraph::Edge &edge ) const
{
	Hypothesis &hypo = edge.GetHypothesis();
	ScoreComponentCollection &scores = hypo.GetScoreBreakdown();
	scores.PlusEquals(this, 1.8);
}

}

