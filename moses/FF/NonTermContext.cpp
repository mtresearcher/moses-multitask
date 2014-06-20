#include <vector>
#include "NonTermContext.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"
#include "moses/ChartCellLabel.h"
#include "moses/InputType.h"
#include "moses/PP/NonTermContextProperty.h"

using namespace std;

namespace Moses
{
NonTermContext::NonTermContext(const std::string &line)
:StatelessFeatureFunction(2, line)
,m_smoothConst(1)
{
  ReadParameters();
}

void NonTermContext::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{}

void NonTermContext::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
	assert(stackVec);

	const PhraseProperty *prop = targetPhrase.GetProperty("NonTermContext");
	if (prop == NULL) {
		return;
	}
	const NonTermContextProperty &ntContextProp = *static_cast<const NonTermContextProperty*>(prop);

	for (size_t i = 0; i < stackVec->size(); ++i) {
		const ChartCellLabel &cell = *stackVec->at(i);
		SetScores(i, input, ntContextProp, cell, targetPhrase, scoreBreakdown);
	}
}

void NonTermContext::Evaluate(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void NonTermContext::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void NonTermContext::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "constant") {
	  m_smoothConst = Scan<float>(value);
  }
  else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void NonTermContext::SetScores(size_t ntInd, const InputType &input,
							const NonTermContextProperty &ntContextProp,
							const ChartCellLabel &cell,
							const TargetPhrase &targetPhrase,
							ScoreComponentCollection &scoreBreakdown) const
{
	const WordsRange &range = cell.GetCoverage();

	const Word &leftOuter = input.GetWord(range.GetStartPos() - 1);
	const Word &leftInner = input.GetWord(range.GetStartPos());
	const Word &rightInner = input.GetWord(range.GetEndPos());
	const Word &rightOuter = input.GetWord(range.GetEndPos() + 1);

	float outer = ntContextProp.GetProb(ntInd, 0, leftOuter.GetFactor(0), m_smoothConst);
	outer *= ntContextProp.GetProb(ntInd, 3, rightOuter.GetFactor(0), m_smoothConst);

	float inner = ntContextProp.GetProb(ntInd, 1, leftInner.GetFactor(0), m_smoothConst);
	inner *= ntContextProp.GetProb(ntInd, 2, rightInner.GetFactor(0), m_smoothConst);

	vector<float> scores(2);
	scores[0] = TransformScore(outer);
	scores[1] = TransformScore(inner);

	scoreBreakdown.PlusEquals(this, scores);

}

}

