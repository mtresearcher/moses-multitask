/*
 * TriggerModel.cpp
 *
 *  Created on: Jul 25, 2014
 *      Author: prashant
 */

#include <utility>
#include "TriggerModel.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"

namespace Moses {
TriggerModel *TriggerModel::s_instance = NULL;

    int split_marker_perl(const std::string& str, string marker, vector<string> &array) {
        int found = str.find(marker), prev = 0;
        while (found != string::npos) // warning!
        {
            array.push_back(str.substr(prev, found - prev));
            prev = found + marker.length();
            found = str.find(marker, found + marker.length());
        }
        array.push_back(str.substr(prev));
        return array.size() - 1;
    }

    TriggerModel::TriggerModel(const std::string line) : StatelessFeatureFunction(3, line) {
    	VERBOSE(2,"Initializing TriggerModel feature..." << std::endl);
        m_source.resize(0);
        m_stm.empty();
        s_instance = this;
        StaticData::InstanceNonConst().SetTriggerModel();
        ReadParameters();
    }

    TriggerModel::~TriggerModel() {
        m_source.resize(0);
        m_stm.empty();
    }

    void TriggerModel::Clear() {
#ifdef WITH_THREADS
    	boost::shared_lock<boost::shared_mutex> lock(m_threadLock);
#endif
    	m_stm.clear();
    }

    void TriggerModel::SetParameter(const std::string& key, const std::string& value)
    {
      VERBOSE(2, "TriggerModel::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);
      if (key == "path") {
        m_filename = Scan<std::string>(value);
      } else if (key == "stm-normalize") {
        m_sigmoidParam = Scan<bool>(value);
      } else {
        StatelessFeatureFunction::SetParameter(key, value);
      }
    }

    void TriggerModel::SetSentence(std::string& sent){
#ifdef WITH_THREADS
    	boost::shared_lock<boost::shared_mutex> lock(m_threadLock);
#endif
    	m_source = sent;
    	return;
    }

    void TriggerModel::Load() {
    	VERBOSE(2,"TriggerModel::Load()" << std::endl);
        InputFileStream inFile(m_filename);
        std::string line;
        while (getline(inFile, line)) {
          std::vector<std::string> vecStr = TokenizeMultiCharSeparator( line , "|||" );
          if (vecStr.size() > 4) {
        	std::vector<float> scoreVec;
        	for (int i=2; i < 5; i++){
        		float score = Scan<float>(vecStr[i]);
            	scoreVec.push_back(score);
        	}
            m_stm[vecStr[0]][vecStr[1]] = scoreVec;
          } else {
            UTIL_THROW_IF2(false, "The format of the loaded file is wrong: " << line);
          }
        }
        PrintUserTime("Loaded Trigger Model...");
    }

    void TriggerModel::EvaluateScore(const TargetPhrase& tp, ScoreComponentCollection* out) const {
        vector<float> score(3);
        std::vector<string> str;
        split_marker_perl(m_source, " ", str);
        size_t endpos = tp.GetSize();
        for (size_t pos = 0; pos < endpos; ++pos) {
        	const std::string t = tp.GetWord(pos).GetFactor(0)->GetString().as_string();
            for (int i = 0; i < str.size(); i++) {
            	if(m_stm.find(str[i]) != m_stm.end()){
            		if(m_stm.at(str[i]).find(t) != m_stm.at(str[i]).end()){
            			vector<float> tempscore = m_stm.at(str[i]).at(t);
            			for(int k=0;k<score.size();k++){
            				score[k]+=tempscore[k];
            			}
            		}
            	}
            }
        }
        if(m_sigmoidParam)
        	for(int k=0;k<score.size();k++)
        		score[k]=2*(score[k]/(1+abs(score[k])));
        out->PlusEquals(this, score);
    }

    void TriggerModel::Evaluate(const Hypothesis& hypo, ScoreComponentCollection* accumulator) const{
    	const TargetPhrase& target = hypo.GetCurrTargetPhrase();
    	EvaluateScore(target, accumulator);
    }
}
