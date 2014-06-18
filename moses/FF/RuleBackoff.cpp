#include <vector>
#include "RuleBackoff.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/ChartTranslationOptions.h"

using namespace std;

namespace Moses
{
RuleBackoff::RuleBackoff(const std::string &line)
  :StatelessFeatureFunction(1, line)
{
  ReadParameters();
}

void RuleBackoff::Evaluate(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void RuleBackoff::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
}

void RuleBackoff::Evaluate(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void RuleBackoff::EvaluateChart(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void RuleBackoff::Evaluate(ChartTranslationOptionList &transOptList) const
{
	cerr << "ChartTranslationOptionList:" << endl;
	for (size_t i = 0; i < transOptList.GetSize(); ++i) {
		cerr << "ChartTranslationOptions " << i << endl;

		const ChartTranslationOptions &transOpts = transOptList.Get(i);
		for (size_t j = 0; j < transOpts.GetSize(); ++j) {
			const ChartTranslationOption &transOpt = transOpts.Get(j);
			cerr << "   " << transOpt << endl;
		}
	}

}

}

