#include "MultiPassFF.h"
#include "moses/SearchGraph.h"
#include "moses/Hypothesis.h"
#include "moses/ScoreComponentCollection.h"

namespace Moses
{
void MultiPassFF::Evaluate(const Edge &edge ) const
{
	const Hypothesis &hypo = edge.GetHypothesis();
	Evaluate(hypo);

	const ArcList *arcList = hypo.GetArcList();
	if (arcList) {
		for (ArcList::const_iterator iter = arcList->begin(); iter != arcList->end(); ++iter) {
			const Hypothesis &arc = **iter;
			Evaluate(arc);
		}
	}
}

void MultiPassFF::Evaluate(const Hypothesis &hypo) const
{
	ScoreComponentCollection &scores = const_cast<ScoreComponentCollection&>(hypo.GetScoreBreakdown());
	scores.PlusEquals(this, 1.0);

}

}

