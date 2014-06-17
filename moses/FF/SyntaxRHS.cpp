#include <iostream>
#include <vector>
#include "SyntaxRHS.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/StackVec.h"
#include "moses/ChartCellLabel.h"
#include "moses/InputType.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{
SyntaxRHS::SyntaxRHS(const std::string &line)
:StatelessFeatureFunction(1, line)
,m_hardConstraint(true)
{
  ReadParameters();
}

void SyntaxRHS::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void SyntaxRHS::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
	const Phrase *source = targetPhrase.GetRuleSource();
	assert(source);
	assert(stackVec);

	size_t ntInd = 0;
	for (size_t i = 0; i < source->GetSize(); ++i) {
	  const Word &word = source->GetWord(i);

	  if (word.IsNonTerminal()) {
		const ChartCellLabel &cell = *stackVec->at(ntInd);
		const WordsRange &range = cell.GetCoverage();
		const NonTerminalSet &labels = input.GetLabelSet(range.GetStartPos(), range.GetEndPos());
		bool isValid = IsValid(word, labels);

		if (!isValid) {
		  float score = m_hardConstraint ? - std::numeric_limits<float>::infinity() : 1;
		  scoreBreakdown.PlusEquals(this, score);
		  return;
		}

		++ntInd;
	  }
	}
}

void SyntaxRHS::Evaluate(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void SyntaxRHS::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

bool SyntaxRHS::IsValid(const Word &ruleNT, const NonTerminalSet &labels) const
{
  /*
  cerr << "ruleNT=" << ruleNT << " ";

  NonTerminalSet::const_iterator iter;
  for (iter = labels.begin(); iter != labels.end(); ++iter) {
	const Word &word = *iter;
	cerr << word << " ";
  }
  cerr << endl;
  */

  if (ruleNT == StaticData::Instance().GetInputDefaultNonTerminal()) {
	  // hiero non-term. 1 word in labels must be hiero. If more than 1, then the other is syntax
	  bool ret = labels.size() > 1;
	  return ret;
  }
  else {
	  // rule non-term is syntax. Don't bother to check
	  return true;
  }

}

void SyntaxRHS::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "hard-constraint") {
	  m_hardConstraint = Scan<bool>(value);
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

