#include <vector>
#include "NonTermContextTarget.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"
#include "moses/ChartCellLabel.h"
#include "moses/ChartHypothesis.h"
#include "moses/InputType.h"
#include "moses/PP/NonTermContextTargetProperty.h"

using namespace std;

namespace Moses
{
int NonTermContextTargetState::Compare(const FFState& other) const
{
  const NonTermContextTargetState &otherState = static_cast<const NonTermContextTargetState&>(other);

  if (m_leftRight == otherState.m_leftRight) {
	  return 0;
  }
  else {
	  int ret = (m_leftRight < otherState.m_leftRight) ? -1 : +1;
	  return ret;
  }
}

////////////////////////////////////////////////////////////////////////

NonTermContextTarget::NonTermContextTarget(const std::string &line)
:StatefulFeatureFunction(1, line)
,m_smoothConst(1)
{
  ReadParameters();
}

void NonTermContextTarget::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{}

void NonTermContextTarget::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
}

FFState* NonTermContextTarget::Evaluate(
  const Hypothesis& cur_hypo,
  const FFState* prev_state,
  ScoreComponentCollection* accumulator) const
{
  abort();
}

FFState* NonTermContextTarget::EvaluateChart(
  const ChartHypothesis &hypo,
  int featureID /* used to index the state in the previous hypotheses */,
  ScoreComponentCollection* accumulator) const
{
	// property
	const TargetPhrase &targetPhrase = hypo.GetTranslationOption().GetPhrase();
	const PhraseProperty *prop = targetPhrase.GetProperty("NonTermContextTarget");
	if (prop == NULL) {
		return new NonTermContextTargetState();
	}
	const NonTermContextTargetProperty &ntContextProp = *static_cast<const NonTermContextTargetProperty*>(prop);

	//typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
	//AlignVec ntAlignments = targetPhrase.GetAlignNonTerm().GetSortedAlignments(1);

	// go thru each prev hypo & work out score
	const std::vector<const ChartHypothesis*> &prevHypos = hypo.GetPrevHypos();
	assert(ntAlignments.size() == prevHypos.size());

	for (size_t i = 0; i < prevHypos.size(); ++i) {
		const ChartHypothesis &prevHypo = *prevHypos[i];
		const FFState *temp = prevHypo.GetFFState(featureID);
		const NonTermContextTargetState *state = static_cast<const NonTermContextTargetState*>(temp);
		assert(state);

		size_t ntInd = i; // TODO have to change. Sorted by source
		const std::vector<const Factor*> &ntContext = state->GetWords();
		SetScores(ntInd, ntContextProp, ntContext, *accumulator);
	}
}

//! return the state associated with the empty hypothesis for a given sentence
const FFState* NonTermContextTarget::EmptyHypothesisState(const InputType &input) const
{
  abort();
}

void NonTermContextTarget::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "constant") {
	  m_smoothConst = Scan<float>(value);
  }
  else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}

void NonTermContextTarget::SetScores(size_t ntInd,
							const NonTermContextTargetProperty &ntContextProp,
							const std::vector<const Factor*> &ntContext,
							ScoreComponentCollection &scoreBreakdown) const
{
  float prob = ntContextProp.GetProb(ntInd, 1, ntContext[0], m_smoothConst);
  float prob2 = ntContextProp.GetProb(ntInd, 1, ntContext[1], m_smoothConst);
  float score = TransformScore(prob * prob2);
  scoreBreakdown.PlusEquals(this, score);
}

}

