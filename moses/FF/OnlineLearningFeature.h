/*
 * OnlineLearner.h
 *
 */

/*
 * Online feature keeps track of recent source and target phrases in hope / fear hypothesis.
*/

#include "moses/Util.h"
#include "moses/TypeDef.h"
#include "moses/FF/FeatureFunction.h"
#include "moses/Hypothesis.h"
#include "moses/Factor.h"
#include "moses/TrellisPath.h"
#include "moses/TrellisPathList.h"
#include "moses/Manager.h"
#include "math.h"
#include "moses/StaticData.h"
#include "moses/FF/OnlineLearning/Optimiser.h"
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"

#ifndef ONLINELEARNINGFEATURE_H_
#define ONLINELEARNINGFEATURE_H_

using namespace std;
using namespace Optimizer;

typedef boost::unordered_map<std::string, boost::unordered_map<std::string, float> > pp_feature;
typedef boost::unordered_map<std::string, boost::unordered_map<std::string, int> > pp_list;

typedef float learningrate;

namespace Moses {

class Optimiser;

class OnlineLearningFeature : public StatelessFeatureFunction {

public:
	OnlineLearningFeature(const std::string&);
	virtual ~OnlineLearningFeature();

	const bool OnlineLearningActivated() const {return m_learn;}
	void DeactivateOnlineLearning() {m_learn=false;}
	void ActivateOnlineLearning() {m_learn=true;}

	bool SetSourceSentence(std::string);
	const std::string GetSourceSentence() const {return m_source;}
	bool SetPostEditedSentence(std::string s);
	void RunOnlineLearning(Manager& manager);
	void RunOnlineMultiTaskLearning(Manager& manager, uint8_t task);
	void RemoveJunk();

//	inline size_t GetNumScoreComponents() const { return 1; };

	inline std::string GetScoreProducerWeightShortName(unsigned) const { return "ol"; };

	// new functions - implement these .. include chart decoding too this time
/*	void Evaluate(const Phrase &source, const TargetPhrase &targetPhrase,
			ScoreComponentCollection &scoreBreakdown
			, ScoreComponentCollection &estimatedFutureScore) const {};
	void Evaluate(const InputType &input, const InputPath &inputPath, const TargetPhrase &targetPhrase,
			ScoreComponentCollection &scoreBreakdown,
			ScoreComponentCollection *estimatedFutureScore = NULL) const{};
	void Evaluate(const InputType &input, const InputPath &inputPath, const TargetPhrase &targetPhrase,
			const StackVec *stackVec , ScoreComponentCollection &scoreBreakdown,
			ScoreComponentCollection *estimatedFutureScore = NULL) const{};
	void Evaluate(const Hypothesis& hypo, ScoreComponentCollection* accumulator) const {};
	void EvaluateChart(const ChartHypothesis &hypo,	ScoreComponentCollection* accumulator) const {};
	void Evaluate(const InputType &input, const InputPath &inputPath,
			ScoreComponentCollection &scoreBreakdown) const{};
*/

	virtual void EvaluateInIsolation(const Moses::Phrase&, const Moses::TargetPhrase&,
			Moses::ScoreComponentCollection&, Moses::ScoreComponentCollection&) const ;
	void EvaluateWithSourceContext(const InputType &input
	                    , const InputPath &inputPath
	                    , const TargetPhrase &targetPhrase
	                    , const StackVec *stackVec
	                    , ScoreComponentCollection &scoreBreakdown
	                    , ScoreComponentCollection *estimatedFutureScore = NULL) const  {}
	void EvaluateWhenApplied(const Hypothesis& hypo,
                        ScoreComponentCollection* accumulator) const {}
	void EvaluateWhenApplied(const ChartHypothesis &hypo,
                             ScoreComponentCollection* accumulator) const {assert(false);}

	void EvaluateWhenApplied(const Syntax::SHyperedge &,
                             ScoreComponentCollection*) const { assert(false); }

    void SetParameter(const std::string& key, const std::string& value);

	static const OnlineLearningFeature& Instance() {
		return *s_instance;
	}
	static OnlineLearningFeature& InstanceNonConst() {
		return *s_instance;
	}

	bool IsUseable(const FactorMask &mask) const {
		return true;
	}

	void Load();
	void Load(const std::string filename);


	enum Algorithm {
		SparseFeatures = 0
		, FOnlyPerceptron = 1
		, FPercepWMira = 2
		, Mira = 3
	};

	enum UpdateStep {
		vonNeumann = 0
		, logDet = 1
	};

private:
	static OnlineLearningFeature *s_instance;
	Algorithm implementation;
	UpdateStep m_updateType;
	boost::unordered_set<std::string> m_vocab;
	pp_feature m_feature;
	pp_list m_featureIdx;
	pp_list PP_ORACLE, PP_BEST;
	learningrate flr, wlr;
	float m_decayValue;
	size_t m_nbestSize;
	std::string m_source, m_postedited;
	std::string m_sctype;
	bool m_normaliseScore, m_sigmoidParam, m_normaliseMargin, m_learn, m_triggerTargetWords, m_l1, m_l2, m_updateFeatures;
	bool scale_margin, scale_margin_precision, scale_update, scale_update_precision;
	MiraOptimiser* optimiser;
	std::string m_filename;
	float w_init, w_initTargetWords,slack;
	std::string m_algorithm;

	void Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const;

	void ShootUp(std::string, std::string, float);
	void ShootDown(std::string, std::string, float);

	float calcMargin(Hypothesis* oracle, Hypothesis* bestHyp);
	void PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream);
	bool has_only_spaces(const std::string& str);

	float GetBleu(std::string hypothesis, std::string reference);
	float GetTer(std::string hypothesis, std::string reference);

	void compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams);
	int getNGrams(std::string str, map<string, int>& ngrams);
	int split_marker_perl(string str, string marker, vector<string> &array);
	void updateFeatureValues();
	void chop(string &str);
	void Decay();

	void DumpFeatures(string);
	void ReadFeatures(string);
	void InsertTargetWords();

	void updateIntMatrix();

};
}
#endif /* ONLINELEARNINGFEATURE_H_ */
