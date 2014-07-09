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
  m_tuneable = false;
  ReadParameters();
}

std::vector<float> SyntaxRHS::DefaultWeights() const
{
  UTIL_THROW_IF2(m_numScoreComponents != 1,
          "SyntaxRHS must only have 1 score");
  vector<float> ret(1, 1);
  return ret;
}

void SyntaxRHS::EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void SyntaxRHS::EvaluateWithSourceContext(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
	if (IsGlueRule(targetPhrase)) {
		return;
	}

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

		  if (m_hardConstraint) {
			  return;
		  }
		}

		++ntInd;
	  }
	}
}

void SyntaxRHS::EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void SyntaxRHS::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

bool SyntaxRHS::IsValid(const Word &ruleNT, const NonTerminalSet &labels) const
{
  /*
  cerr << "ruleNT=" << ruleNT << " " << labels.size() << " ";

  NonTerminalSet::const_iterator iter;
  for (iter = labels.begin(); iter != labels.end(); ++iter) {
	const Word &word = *iter;
	cerr << word << " ";
  }
  cerr << endl;
  */

  if (ruleNT == StaticData::Instance().GetInputDefaultNonTerminal()) {
	  // hiero non-term. 1 word in labels must be hiero. If more than 1, then the other is syntax
	  bool ret = labels.size() == 1;
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

bool SyntaxRHS::IsGlueRule(const TargetPhrase &targetPhrase) const
{
	const Phrase *source = targetPhrase.GetRuleSource();
	assert(source);

	string sourceStr = source->ToString();
	if (sourceStr == "<s> " || sourceStr == "X </s> " || sourceStr == "X X ") {
	  // don't score glue rule
	  //cerr << "sourceStr=" << sourceStr << endl;
	  return true;
	}
	else {
	  return false;
	}

}

}

