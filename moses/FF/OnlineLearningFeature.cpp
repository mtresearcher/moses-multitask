/*
 * OnlineLearner.cpp
 *
 */

#include "OnlineLearningFeature.h"
#include "moses/Util.h"
#include "util/exception.hh"
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace Optimizer;
using namespace std;

namespace MatrixOps {
//    ------------------- Kronecker Product code ---------------------------- //

	bool KroneckerProduct (const boost::numeric::ublas::matrix<double>& A, const boost::numeric::ublas::matrix<double>& B, boost::numeric::ublas::matrix<double>& C) {
		int rowA=-1,colA=-1,rowB=0,colB=0,prowB=1,pcolB=1;
		for(int i=0; i<C.size1(); i++){
			for(int j=0; j<C.size2(); j++){
				rowB=i%B.size1();
				colB=j%B.size2();
				if(pcolB!=0 && colB == 0) colA++;
				if(prowB!=0 && rowB==0) rowA++;
				prowB=rowB;
				pcolB=colB;
				if(colA >= A.size2()){colA=0; colB=0;pcolB=1;}
				C(i, j) = A(rowA, colA) * B(rowB, colB) ;
			}
		}
		return true;
	}
	//    ------------------------------- ends here ---------------------------- //
    //    ------------------- matrix inversion code ---------------------------- //
    template<class T>
    bool InvertMatrix (const boost::numeric::ublas::matrix<T>& input, boost::numeric::ublas::matrix<T>& inverse) {
    	typedef boost::numeric::ublas::permutation_matrix<std::size_t> pmatrix;

    	// create a working copy of the input
    	boost::numeric::ublas::matrix<T> A(input);

    	// create a permutation matrix for the LU-factorization
    	pmatrix pm(A.size1());

    	// perform LU-factorization
    	int res = boost::numeric::ublas::lu_factorize(A, pm);
    	if (res != 0)
    		return false;

    	// create identity matrix of "inverse"
    	inverse.assign(boost::numeric::ublas::identity_matrix<T> (A.size1()));

    	// backsubstitute to get the inverse
    	boost::numeric::ublas::lu_substitute(A, pm, inverse);

    	return true;
    }
}

namespace Moses {

	OnlineLearningFeature *OnlineLearningFeature::s_instance = NULL;

	OnlineLearningFeature::OnlineLearningFeature(const std::string &line) : StatelessFeatureFunction(0, line) {
		s_instance=this;
		m_learn=false;
		ReadParameters();
		if(implementation!=FOnlyPerceptron){
			optimiser = new Optimizer::MiraOptimiser(slack, scale_margin, scale_margin_precision, scale_update,
					scale_update_precision, m_normaliseMargin, m_sigmoidParam);
		}
	}

	void OnlineLearningFeature::SetParameter(const std::string& key, const std::string& value)
	{
		VERBOSE(2, "OnlineLearningFeature::SetParameter key:|" << key << "| value:|" << value << "|" << std::endl);
		if (key == "path") {
			m_filename = Scan<std::string>(value);
		} else if (key == "normaliseScore") {
			m_normaliseScore = Scan<bool>(value);
		} else if (key == "normaliseMargin") {
			m_normaliseMargin = Scan<bool>(value);
		} else if (key == "f_learningrate") {
			flr = Scan<float>(value);
		} else if (key == "w_learningrate") {
			wlr = Scan<float>(value);
		} else if (key == "slack") {
			slack = Scan<float>(value);
		} else if (key == "scale_update_precision") {
			scale_update_precision = Scan<float>(value);
		} else if (key == "scale_update") {
			scale_update = Scan<float>(value);
		} else if (key == "scale_margin_precision") {
			scale_margin_precision = Scan<float>(value);
		} else if (key == "scale_margin") {
			scale_margin = Scan<float>(value);
		} else if (key == "w_algorithm") {
			m_algorithm = Scan<std::string>(value);
			// set algorithm
			if(m_algorithm.compare("perceptron")==0){
				implementation=FOnlyPerceptron;
				cerr<<"Online Algorithm : Perceptron\n";
			}
			else if(m_algorithm.compare("alsoMira")==0){
				implementation=FPercepWMira;
				cerr<<"Online Algorithm : Perceptron + Mira\n";
			}
			else if(m_algorithm.compare("onlyMira")==0){
				implementation=Mira;
				cerr<<"Online Algorithm : Mira\n";
			}
		} else {
			StatelessFeatureFunction::SetParameter(key, value);
		}
	}

	void OnlineLearningFeature::Load() {
		VERBOSE(2,"OnlineLearningFeature::Load()" << std::endl);
		ReadFeatures(m_filename);
		PrintUserTime("Loaded Online Learning Feature ");
	}


    void OnlineLearningFeature::chop(string &str) {
        int i = 0;
        while (isspace(str[i]) != 0) {
            str.replace(i, 1, "");
        }
        while (isspace(str[str.length() - 1]) != 0) {
            str.replace(str.length() - 1, 1, "");
        }
        return;
    }

    int OnlineLearningFeature::split_marker_perl(string str, string marker, vector<string> &array) {
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

    int OnlineLearningFeature::getNGrams(std::string str, map<string, int>& ngrams) {
        vector<string> words;
        int numWords = split_marker_perl(str, " ", words);
        for (int i = 1; i <= 4; i++) {
            for (int start = 0; start < numWords - i + 1; start++) {
                string str = "";
                for (int pos = 0; pos < i; pos++) {
                    str += words[start + pos] + " ";
                }
                ngrams[str]++;
            }
        }
        return str.size();
    }

    void OnlineLearningFeature::compareNGrams(map<string, int>& hyp, map<string, int>& ref, map<int, float>& countNgrams, map<int, float>& TotalNgrams) {
        for (map<string, int>::const_iterator itr = hyp.begin(); itr != hyp.end(); itr++) {
            vector<string> temp;
            int ngrams = split_marker_perl((*itr).first, " ", temp);
            TotalNgrams[ngrams] += hyp[(*itr).first];
            if (ref.find((*itr).first) != ref.end()) {
                if (hyp[(*itr).first] >= ref[(*itr).first]) {
                    countNgrams[ngrams] += hyp[(*itr).first];
                } else {
                    countNgrams[ngrams] += ref[(*itr).first];
                }
            }
        }
        for (int i = 1; i <= 4; i++) {
            TotalNgrams[i] += 0.1;
            countNgrams[i] += 0.1;
        }
        return;
    }

    float OnlineLearningFeature::GetBleu(std::string hypothesis, std::string reference) {
        double bp = 1;
        map<string, int> hypNgrams, refNgrams;
        map<int, float> countNgrams, TotalNgrams;
        map<int, double> BLEU;
        int length_translation = getNGrams(hypothesis, hypNgrams);
        int length_reference = getNGrams(reference, refNgrams);
        compareNGrams(hypNgrams, refNgrams, countNgrams, TotalNgrams);

        for (int i = 1; i <= 4; i++) {
            BLEU[i] = (countNgrams[i]*1.0) / (TotalNgrams[i]*1.0);
        }
        double ratio = ((length_reference * 1.0 + 1.0) / (length_translation * 1.0 + 1.0));
        if (length_translation < length_reference)
            bp = exp(1 - ratio);
        return ((bp * exp((log(BLEU[1]) + log(BLEU[2]) + log(BLEU[3]) + log(BLEU[4])) / 4))*100);
    }

    bool OnlineLearningFeature::has_only_spaces(const std::string& str) {
        return (str.find_first_not_of(' ') == str.npos);
    }

    bool OnlineLearningFeature::SetPostEditedSentence(std::string s) {
        if (m_postedited.empty()) {
            m_postedited = s;
            return true;
        } else {
            cerr << "post edited already exists.. " << m_postedited << endl;
            return false;
        }
    }

    bool OnlineLearningFeature::SetSourceSentence(std::string s) {
    	cerr<<"Setting Source Sentence\n";
    	m_source = s;
    	return true;
    }

    void OnlineLearningFeature::DumpFeatures(std::string filename)
    {
    	ofstream file;
    	file.open(filename.c_str(), ios::out);
    	if(file.is_open())
    	{
    		pp_feature::iterator itr1=m_feature.begin();
    		while(itr1!=m_feature.end()){
    			boost::unordered_map<std::string, float>::iterator itr2=(*itr1).second.begin();
    			while(itr2!=(*itr1).second.end()){
    				file << itr1->first <<"|||"<<itr2->first<<"|||"<<itr2->second<<std::endl;
    				itr2++;
    			}
    			itr1++;
    		}
    	}
    	file.close();
    }
    void OnlineLearningFeature::ReadFeatures(std::string filename)
    {
    	ifstream file;
    	file.open(filename.c_str(), ios::in);
    	std::string line;
    	if(file.is_open())
    	{
    		while(getline(file, line)){
    			chop(line);
    			std::vector<string> splits;
    			split_marker_perl(line, "|||", splits);		// line:string1|||string2|||score
    			if(splits.size()==3){
    				float score;
    				stringstream(splits[2])>>score;
    				m_feature[splits[0]][splits[1]] = score;
    			}
    			else{
    				TRACE_ERR("The format of feature file does not comply!");
    			}
    		}
    	}
    	file.close();
    }

    void OnlineLearningFeature::ShootUp(std::string sp, std::string tp, float margin) {
    	if (m_feature.find(sp) != m_feature.end()) {
    		if (m_feature[sp].find(tp) != m_feature[sp].end()) {
    			float val = m_feature[sp][tp];
    			val += flr * margin;
    			m_feature[sp][tp] = val;
    		} else {
    			m_feature[sp][tp] = flr*margin;
    			std::string featureName(sp+"|||"+tp);
    			StaticData::InstanceNonConst().SetSparseWeight(this, featureName, 1.0);
    		}
    	} else {
    		m_feature[sp][tp] = flr*margin;
    		std::string featureName(sp+"|||"+tp);
    		StaticData::InstanceNonConst().SetSparseWeight(this, featureName, 1.0);
    	}
    }

    void OnlineLearningFeature::ShootDown(std::string sp, std::string tp, float margin) {
    	if (m_feature.find(sp) != m_feature.end()) {
    		if (m_feature[sp].find(tp) != m_feature[sp].end()) {
    			float val = m_feature[sp][tp];
    			val -= flr * margin;
    			m_feature[sp][tp] = val;
    		} else {
    			m_feature[sp][tp] = 0;
    			std::string featureName(sp+"|||"+tp);
    			StaticData::InstanceNonConst().SetSparseWeight(this, featureName, 1.0);
    		}
    	} else {
    		m_feature[sp][tp] = 0;
    		std::string featureName(sp+"|||"+tp);
    		StaticData::InstanceNonConst().SetSparseWeight(this, featureName, 1.0);
    	}
    }

    float OnlineLearningFeature::calcMargin(Hypothesis* oracle, Hypothesis* bestHyp) {
        return (oracle->GetScore() - bestHyp->GetScore());
    }
    // clears history

    void OnlineLearningFeature::RemoveJunk() {
        m_postedited.clear();
        PP_ORACLE.clear();
        PP_BEST.clear();
    }

    OnlineLearningFeature::~OnlineLearningFeature() {
        pp_feature::iterator iter;
        for (iter = m_feature.begin(); iter != m_feature.end(); iter++) {
            (*iter).second.clear();
        }
    }

    void OnlineLearningFeature::PrintHypo(const Hypothesis* hypo, ostream& HypothesisStringStream) {
        if (hypo->GetPrevHypo() != NULL) {
            PrintHypo(hypo->GetPrevHypo(), HypothesisStringStream);
            Phrase p = hypo->GetCurrTargetPhrase();
            for (size_t pos = 0; pos < p.GetSize(); pos++) {
                const Factor *factor = p.GetFactor(pos, 0);
                HypothesisStringStream << *factor << " ";
            }
            std::string sourceP = hypo->GetSourcePhraseStringRep();
            std::string targetP = hypo->GetTargetPhraseStringRep();
            PP_BEST[sourceP][targetP] = 1;
        }
    }

    void OnlineLearningFeature::Decay(int lineNum) {
        float decay_value = 1.0 / (exp(lineNum)*1.0);
        pp_feature::iterator itr1 = m_feature.begin();
        while (itr1 != m_feature.end()) {
            boost::unordered_map<std::string, float>::iterator itr2 = (*itr1).second.begin();
            while (itr2 != (*itr1).second.end()) {
                float score = m_feature[itr1->first][itr2->first];
                score *= decay_value;
                m_feature[itr1->first][itr2->first] = score;
                itr2++;
            }
            itr1++;
        }
    }

    void OnlineLearningFeature::Evaluate(const Hypothesis& hypo, ScoreComponentCollection* accumulator) const{
    	const TargetPhrase& target = hypo.GetCurrTargetPhrase();
    	Evaluate(target, accumulator);
    }

    void OnlineLearningFeature::Evaluate(const TargetPhrase& tp, ScoreComponentCollection* out) const {
    	float score = 0.0;
    	std::string s = "", t = "";
    	size_t endpos = tp.GetSize();
    	for (size_t pos = 0; pos < endpos; ++pos) {
    		if (pos > 0) {
    			t += " ";
    		}
    		t += tp.GetWord(pos)[0]->GetString().as_string();
    	}
    	endpos = tp.GetRuleSource()->GetSize();
    	for (size_t pos = 0; pos < endpos; ++pos) {
    		if (pos > 0) {
    			s += " ";
    		}
    		s += tp.GetRuleSource()->GetWord(pos)[0]->GetString().as_string();
    	}
    	pp_feature::const_iterator it;
    	it=m_feature.find(s);
    	if(it!=m_feature.end())
    	{
    		boost::unordered_map<std::string, float>::const_iterator it2;
    		it2=it->second.find(t);
    		if(it2!=it->second.end())
    		{
    			score=it2->second;
    		}
    	}
    	if (m_normaliseScore)
    		score = (2 / (1 + exp(-score))) - 1; // normalising score!
//    	cerr<<"OL : "<<s<<" ||| "<<t<<" : "<<score<<"\n";
    	StringPiece featureName(s+"|||"+t);
    	out->PlusEquals(this, featureName, score);
    }

    void OnlineLearningFeature::RunOnlineLearning(Manager& manager) {

        const StaticData& staticData = StaticData::Instance();
        const std::vector<Moses::FactorType>& outputFactorOrder = staticData.GetOutputFactorOrder();
        ScoreComponentCollection weightUpdate = staticData.GetAllWeights();

        const Hypothesis* hypo = manager.GetBestHypothesis();
        //	Decay(manager.m_lineNumber);
        stringstream bestHypothesis;
        PP_BEST.clear();
        PrintHypo(hypo, bestHypothesis);
        float bestbleu = GetBleu(bestHypothesis.str(), m_postedited);
        float bestScore = hypo->GetScore();
        cerr << "Best Hypothesis : " << bestHypothesis.str() << endl;
        cerr << "Post Edit       : " << m_postedited << endl;
        TrellisPathList nBestList;
        int nBestSize = 100;
        manager.CalcNBest(nBestSize, nBestList, true);

        std::string bestOracle;
        std::vector<string> HypothesisList;
        std::vector<float> loss, BleuScore, oracleBleuScores, modelScore, oracleModelScores;
        std::vector<std::vector<float> > losses, BleuScores, modelScores;
        std::vector<ScoreComponentCollection> featureValue, oraclefeatureScore;
        std::vector<std::vector<ScoreComponentCollection> > featureValues;
        std::map<int, map<string, map<string, int> > > OracleList;
        TrellisPathList::const_iterator iter;
        pp_list BestOracle, Visited;
        float maxBleu = 0.0, maxScore = 0.0, oracleScore = 0.0;
        int whichoracle = -1;
        for (iter = nBestList.begin(); iter != nBestList.end(); ++iter) {
            whichoracle++;
            const TrellisPath &path = **iter;
            PP_ORACLE.clear();
            const std::vector<const Hypothesis *> &edges = path.GetEdges();
            stringstream oracle;
            for (int currEdge = (int) edges.size() - 1; currEdge >= 0; currEdge--) {
                const Hypothesis &edge = *edges[currEdge];
//                UTIL_THROW_IF2(outputFactorOrder.size() > 0, "output factor size cannot be 0\n");
                size_t size = edge.GetCurrTargetPhrase().GetSize();
                for (size_t pos = 0; pos < size; pos++) {
                    const Factor *factor = edge.GetCurrTargetPhrase().GetFactor(pos, outputFactorOrder[0]);
                    oracle << *factor;
                    oracle << " ";
                }
                std::string sourceP = edge.GetSourcePhraseStringRep(); // Source Phrase
                std::string targetP = edge.GetTargetPhraseStringRep(); // Target Phrase
                if (!has_only_spaces(sourceP) && !has_only_spaces(targetP)) {
                    PP_ORACLE[sourceP][targetP] = 1; // phrase pairs in the current nbest_i
                    OracleList[whichoracle][sourceP][targetP] = 1; // list of all phrase pairs given the nbest_i
                }
            }
            oracleScore = path.GetTotalScore();
            float oraclebleu = GetBleu(oracle.str(), m_postedited);
            if (implementation != FOnlyPerceptron) {
                HypothesisList.push_back(oracle.str());
                BleuScore.push_back(oraclebleu);
                featureValue.push_back(path.GetScoreBreakdown());
                modelScore.push_back(oracleScore);
            }
            if (oraclebleu > maxBleu) {
                cerr << "NBEST : " << oracle.str() << "\t|||\tBLEU : " << oraclebleu << endl;
                maxBleu = oraclebleu;
                maxScore = oracleScore;
                bestOracle = oracle.str();
                pp_list::const_iterator it1;

                oracleBleuScores.clear();
                oraclefeatureScore.clear();
                BestOracle = PP_ORACLE;
                oracleBleuScores.push_back(oraclebleu);
                oraclefeatureScore.push_back(path.GetScoreBreakdown());
            }
            // ------------------------trial--------------------------------//
            if (implementation == FPercepWMira) {
                if (oraclebleu > bestbleu) {
                    pp_list::const_iterator it1;
                    for (it1 = PP_ORACLE.begin(); it1 != PP_ORACLE.end(); it1++) {
                        boost::unordered_map<std::string, int>::const_iterator itr1;
                        for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                            if (PP_BEST[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                                ShootUp(it1->first, itr1->first, abs(oracleScore - bestScore));
                                Visited[it1->first][itr1->first] = 1;
                            }
                        }
                    }
                    for (it1 = PP_BEST.begin(); it1 != PP_BEST.end(); it1++) {
                    	boost::unordered_map<std::string, int>::const_iterator itr1;
                        for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                            if (PP_ORACLE[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                                ShootDown(it1->first, itr1->first, abs(oracleScore - bestScore));
                                Visited[it1->first][itr1->first] = 1;
                            }
                        }
                    }
                }
                if (oraclebleu < bestbleu) {
                    pp_list::const_iterator it1;
                    for (it1 = PP_ORACLE.begin(); it1 != PP_ORACLE.end(); it1++) {
                    	boost::unordered_map<std::string, int>::const_iterator itr1;
                        for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                            if (PP_BEST[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                                ShootDown(it1->first, itr1->first, abs(oracleScore - bestScore));
                                Visited[it1->first][itr1->first] = 1;
                            }
                        }
                    }
                    for (it1 = PP_BEST.begin(); it1 != PP_BEST.end(); it1++) {
                    	boost::unordered_map<std::string, int>::const_iterator itr1;
                        for (itr1 = (it1->second).begin(); itr1 != (it1->second).end(); itr1++) {
                            if (PP_ORACLE[it1->first][itr1->first] != 1 && Visited[it1->first][itr1->first] != 1) {
                                ShootUp(it1->first, itr1->first, abs(oracleScore - bestScore));
                                Visited[it1->first][itr1->first] = 1;
                            }
                        }
                    }
                }
            }
            // ------------------------trial--------------------------------//
        }
        cerr << "Read all the oracles in the list!\n";

        //	Update the weights
        if (implementation == FPercepWMira || implementation == Mira) {
            for (int i = 0; i < HypothesisList.size(); i++) // same loop used for feature values, modelscores
            {
                float bleuscore = BleuScore[i];
                loss.push_back(maxBleu - bleuscore);
            }
            modelScores.push_back(modelScore);
            featureValues.push_back(featureValue);
            BleuScores.push_back(BleuScore);
            losses.push_back(loss);
            oracleModelScores.push_back(maxScore);
            cerr << "Updating the Weights...\n";
            size_t update_status = optimiser->updateWeights(weightUpdate, featureValues, losses,
                    BleuScores, modelScores, oraclefeatureScore, oracleBleuScores, oracleModelScores, wlr);
            if(update_status > 0)
            	StaticData::InstanceNonConst().SetAllWeights(weightUpdate);
            cerr<<"Total number of features are : "<<weightUpdate.Size()<<endl;

        }
        return;
    }

}
