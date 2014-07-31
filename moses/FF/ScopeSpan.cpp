#include <vector>
#include "ScopeSpan.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"
#include "moses/ChartTranslationOptionList.h"
#include "moses/ChartTranslationOptions.h"
#include "moses/PP/CountsPhraseProperty.h"

using namespace std;

namespace Moses
{
ScopeSpan::ScopeSpan(const std::string &line)
:StatelessFeatureFunction(1, line)
,m_minCount(20)
,m_scopeRange(3, std::pair<size_t, size_t>(0,100000))
{
  m_tuneable = false;
  ReadParameters();
}

std::vector<float> ScopeSpan::DefaultWeights() const
{
  UTIL_THROW_IF2(m_numScoreComponents != 1,
          "SyntaxRHS must only have 1 score");
  vector<float> ret(1, 1);
  return ret;
}

void ScopeSpan::EvaluateInIsolation(const Phrase &source
                                   , const TargetPhrase &targetPhrase
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void ScopeSpan::EvaluateWithSourceContext(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
}

void ScopeSpan::EvaluateWhenApplied(const Hypothesis& hypo,
                                   ScoreComponentCollection* accumulator) const
{}

void ScopeSpan::EvaluateWhenApplied(const ChartHypothesis &hypo,
                                        ScoreComponentCollection* accumulator) const
{}

void ScopeSpan::Evaluate(ChartTranslationOptionList &transOptList) const
{
	// transopts to be deleted
	typedef set<ChartTranslationOptions*> Coll;
	Coll transOptsToDelete;

	// collect counts
	//cerr << "ChartTranslationOptionList:" << endl;
	for (size_t i = 0; i < transOptList.GetSize(); ++i) {
		ChartTranslationOptions &transOpts = transOptList.Get(i);
		const WordsRange &range = transOpts.GetSourceWordsRange();
		size_t width = range.GetNumWordsCovered();

		//cerr << "ChartTranslationOptions " << i << "=" << transOpts.GetSize() << endl;

		/*
		for (size_t j = 0; j < transOpts.GetSize(); ++j) {
			const ChartTranslationOption &transOpt = transOpts.Get(j);
			cerr << "   " << transOpt << endl;
		}
		*/

		UTIL_THROW_IF2(transOpts.GetSize() == 0, "transOpts can't be empty");

		assert(transOpts.GetSize());
		// get count
		const ChartTranslationOption &transOpt = transOpts.Get(0);
		const TargetPhrase &tp = transOpt.GetPhrase();
		const Phrase *sp = tp.GetRuleSource();
		assert(sp);

		int scope = sp->GetScope();
		UTIL_THROW_IF2(scope > 2, "Max scope = 2. Current scope = " << scope);

		const std::pair<size_t, size_t> &allowableRange = m_scopeRange[scope];

		if (width < allowableRange.first || width > allowableRange.second) {
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

void ScopeSpan::SetParameter(const std::string& key, const std::string& value)
{
  if (key.substr(0, 5) == "scope") {
	  UTIL_THROW_IF2(key.size() < 5, "Incorrect key format:" << key);
	  string scopeStr = key.substr(5, 1);
	  int scope = Scan<int>(scopeStr);
	  UTIL_THROW_IF2(scope > 2, "Max scope = 2. Current scope = " << scope);

	  vector<size_t> range = Tokenize<size_t>(value, ",");
	  UTIL_THROW_IF2(range.size() != 2, "Incorrect value format:" << value);
	  m_scopeRange[scope] = std::pair<size_t, size_t>(range[0], range[1]);
  } else {
	  StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

