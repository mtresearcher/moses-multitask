#include <vector>
#include "RuleBackoff.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/PP/CountsPhraseProperty.h"

using namespace std;

namespace Moses
{
RuleBackoff::RuleBackoff(const std::string &line)
:StatelessFeatureFunction(1, line)
,m_minCount(20)
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
	// transopts to be deleted
	typedef set<ChartTranslationOptions*> Coll;
	Coll transOptsToDelete;

	// collect counts
	cerr << "ChartTranslationOptionList:" << endl;
	for (size_t i = 0; i < transOptList.GetSize(); ++i) {
		ChartTranslationOptions &transOpts = transOptList.Get(i);
		cerr << "ChartTranslationOptions " << i << "=" << transOpts.GetSize() << endl;

		/*
		for (size_t j = 0; j < transOpts.GetSize(); ++j) {
			const ChartTranslationOption &transOpt = transOpts.Get(j);
			cerr << "   " << transOpt << endl;
		}
		*/

		assert(transOpts.GetSize());
		// get count
		const ChartTranslationOption &transOpt = transOpts.Get(0);
		const TargetPhrase &tp = transOpt.GetPhrase();
		size_t numNT = tp.GetNumNonTerminals();

		if (numNT == 0) {
			const PhraseProperty *pp = tp.GetProperty("Counts");
			assert(pp);
			const CountsPhraseProperty *countProp = static_cast<const CountsPhraseProperty *>(pp);

			float counts = countProp->GetSourceMarginal();
			if (counts < m_minCount) {
				// count too small. keep everything
				return;
			}
		}
		else {
			transOptsToDelete.insert(&transOpts);
		}
	}

	// delete
	Coll::iterator iter;
	for (iter = transOptsToDelete.begin(); iter != transOptsToDelete.end(); ++iter) {
		ChartTranslationOptions *transOpts = *iter;
		transOpts->Clear();
	}
}

void RuleBackoff::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "min-count") {
	  m_minCount = Scan<float>(value);
  } else {
	  StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

