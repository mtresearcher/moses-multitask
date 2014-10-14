/*
 * TriggerModel.h
 *
 *  Created on: Jul 25, 2014
 *      Author: prashant
 */

#ifndef TRIGGERMODEL_H_
#define TRIGGERMODEL_H_

#include "moses/UserMessage.h"
#include "moses/Util.h"
#include "moses/FF/StatelessFeatureFunction.h"

#ifdef WITH_THREADS
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#endif

using namespace std;

namespace Moses {

class TriggerModel : public StatelessFeatureFunction {

public:
    TriggerModel(const std::string line);

    virtual ~TriggerModel();

    // this function should be called before decoding of a sentence starts
    void SetSentence(std::string& sent);

    void Clear();

    void SetParameter(const std::string& key, const std::string& value);

    int SizeofSTM() const { return m_stm.size(); };

    void Evaluate(const Phrase &source, const TargetPhrase &targetPhrase,ScoreComponentCollection &scoreBreakdown, ScoreComponentCollection &estimatedFutureScore) const{};

    void Evaluate(const InputType &input, const InputPath &inputPath, const TargetPhrase &targetPhrase, ScoreComponentCollection &scoreBreakdown
    		, ScoreComponentCollection *estimatedFutureScore = NULL) const{};

    void Evaluate(const InputType &input, const InputPath &inputPath, const TargetPhrase &targetPhrase,
    		const StackVec *stackVec , ScoreComponentCollection &scoreBreakdown, ScoreComponentCollection *estimatedFutureScore = NULL) const{};

    void Evaluate(const Hypothesis& hypo, ScoreComponentCollection* accumulator) const;

    void EvaluateChart(const ChartHypothesis &hypo,	ScoreComponentCollection* accumulator) const {};

    void Evaluate(const InputType &input, const InputPath &inputPath, ScoreComponentCollection &scoreBreakdown) const{};

    static const TriggerModel& Instance() {
    	return *s_instance;
    }
    static TriggerModel& InstanceNonConst() {
    	return *s_instance;
    }

    bool IsUseable(const FactorMask &mask) const {
    	return true;
    }

    void Load();
    void Load(const std::string filename);

#ifdef WITH_THREADS
  //multiple readers - single writer lock
    mutable boost::shared_mutex m_threadLock;
#endif

private:
    static TriggerModel *s_instance;
    void EvaluateScore(const TargetPhrase& tp, ScoreComponentCollection* out) const;
    std::string m_filename;
    std::map<std::string, std::map<std::string, vector<float> > > m_stm;
    bool m_sigmoidParam;
    std::string m_source;
};

}

#endif /* TRIGGERMODEL_H_ */
