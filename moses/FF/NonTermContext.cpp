#include <vector>
#include "NonTermContext.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"
#include "moses/ChartCellLabel.h"

using namespace std;

namespace Moses
{
NonTermContext::NonTermContext(const std::string &line)
:StatelessFeatureFunction(2, line)
,m_const(1)
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
	for (size_t i = 0; i < stackVec->size(); ++i) {
		const ChartCellLabel &cell = *stackVec->at(i);
		SetScores(scoreBreakdown, cell, targetPhrase);
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
	  m_const = Scan<float>(value);
  }
  else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

void NonTermContext::SetScores(ScoreComponentCollection &scoreBreakdown,
							const ChartCellLabel &cell,
							const TargetPhrase &targetPhrase) const
{

}

}

