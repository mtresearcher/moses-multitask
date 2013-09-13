//
/************************************************
  Nbest-rescoring : using CS Language Model - Fethi bougares - LIUM Lab, Le mans university - France

  Moses - factored phrase-based language decoder
  CSLM  - Continuous space language Model

*************************************************/


#include <limits>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "Util.h"
#include "moses/TypeDef.h"
#include "CSLM.h"
#include "TrainerNgramSlist.h"  // FROM CSLM
#include "Mach.h" // from cslm toolkit

#include "Vocab.h"
#include "Ngram.h"
#include "moses/StaticData.h"
#include "moses/HypothesisStack.h"
#include "moses/TrellisPathList.h"

using namespace std;

namespace Moses
{

LanguageModelCSLM::LanguageModelCSLM(const std::string &line)
	:LanguageModelSingleFactor("CSLM", line)
	,m_srilmVocab(NULL)
	,m_mach(NULL)
	,m_trainer(NULL),busy(false) {
		ReadParameters();
}

LanguageModelCSLM::~LanguageModelCSLM()
{
	// cerr<<" delete m_trainer "<<endl;
 	delete m_trainer;
	m_trainer=NULL;
}

/******************************
 *  Load CSLM Model
 * ****************************/

void LanguageModelCSLM::Load()
{
  VERBOSE(1," Loading CSLM Model .... "<<m_filePath<<endl);

  ifstream ifs;
  ifs.open(m_filePath.c_str(),ios::binary);
  CHECK_FILE(ifs,m_filePath.c_str());
  m_mach = Mach::Read(ifs);
  ifs.close();

  m_mach->Info();
  nGramOrder = m_mach->GetIdim()+1;

  m_trainer    = new TrainerNgramSlist(m_mach,m_srilmVocab_filePath.c_str(), m_srilmNgram_filePath.c_str());
  m_srilmVocab = m_trainer->GetVocab();

  VERBOSE(1," - using SRILM vocabulary with " << m_srilmVocab->numWords() << " words"<<endl);
}



/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************    SEARCH SPACE RESCORING WITH CSLM Model    *************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/

/***************************************************************************************
 * Run into each Stack and calculate the CSLM score of each hypothesis and it's arc list
 **************************************************************************************/

void LanguageModelCSLM::RescoreLattice( std::vector < HypothesisStack* >& hypoStackColl )
{
	cerr<<" Lock cslm .... "<<endl;
	boost::mutex::scoped_lock lock(cslm_mutex); // Lock the CSLM Model for this Sentence
	std::vector < HypothesisStack* >::iterator iterStack;

	for (iterStack = ++hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack)
	{
		HypothesisStack::iterator iterHypo ;
		for(iterHypo = (*iterStack)->Nocbegin() ; iterHypo != (*iterStack)->Nocend() ; ++iterHypo) // foreach hypothesis
		{
			cerr<<" -------- "<<endl;
			cerr<<"Hypo ID "<<(*iterHypo)->GetId()<<"("<<(*iterHypo)->GetPrevHypo()->GetId()<<")"<<" trans : "<<(*iterHypo)->GetCurrTargetPhrase()<<endl;
			(*iterHypo)->m_nbwords=0 ; //= new float[(*iterHypo)->GetCurrTargetLength()]; // array of nbword of float
			GetCSLMScore(*iterHypo); // Calc Score of each words in this hypothesis

			const ArcList *pAL =   (*iterHypo)->GetArcList();
			if(pAL){
				ArcList::const_iterator iterArc;
				for (iterArc = pAL->begin() ; iterArc != pAL->end() ; ++iterArc)
				{
					Hypothesis *arc = const_cast<Hypothesis*>(*iterArc);
					cerr<<" -------- "<<endl;
					cerr<<" Hypo ID "<<(arc)->GetId()<<"("<<arc->GetPrevHypo()->GetId()<<")"<< " trans : "<<(arc)->GetCurrTargetPhrase()<<endl;
					arc->m_nbwords = 0; //= new float[arc->GetCurrTargetLength() ]; // array to save cslm scores
					GetCSLMScore(arc); // Calc the CSLM for each word in this arc
				}
			}
		}
	}
}


/******************************************
 * Calc the CSLM Score Of one Hypothesis
 *******************************/
void LanguageModelCSLM::GetCSLMScore(Hypothesis* hypo )
{
	if(StaticData::Instance().GetCSLMnGramOrder() <= 1 )
		return;

	VocabString vstr[max_words+1];
	const int max_words=16384;
	const int max_chars=max_words*16;
	char str[max_chars];
	int Hyp_nbw = 0;
	size_t cslmOrder = StaticData::Instance().GetCSLMnGramOrder();
	const int startPos = hypo->GetCurrTargetWordsRange().GetStartPos();
	const size_t currEndPos = hypo->GetCurrTargetWordsRange().GetEndPos();
	int Idx = hypo->GetCurrTargetLength() ;
	std::vector<string> contextFactor ;
	std::string contextCSLM;

	// BPos is where we should start to copy the words from the hypothesis
	size_t BPos = (startPos-cslmOrder+1 >= 0 ) ? startPos-cslmOrder+1 : 0;

	// include BOS word when necessary
	if( startPos < (cslmOrder-1) ) {
		contextFactor.push_back(BOS_);
		startPos++;
	};

	for(size_t Currpos = BPos; Currpos <= currEndPos ; Currpos ++){
		contextFactor.push_back( hypo->GetWord(Currpos).GetString(0).as_string() );
	}

	// Add EOS if hypothesis covers all source sentence
	if( hypo->IsSourceCompleted() ) {
		contextFactor.push_back(EOS_);
		Idx++;
	}

	//	 cerr<<" Hypo ID : "<<hypo->GetId()<<" Start Pos : "<<startPos <<" --> "<<currEndPos<<endl;
	for(size_t i=0 ; i< contextFactor.size()-1;i++){
		contextCSLM.append(contextFactor[i]);
		contextCSLM.append(" ");
	}
	contextCSLM.append(contextFactor[contextFactor.size()-1]);
	strcpy(str,contextCSLM.c_str()); // conversion
//	 cerr<<" idx : "<<Idx<<endl;

	hypo->m_cslmprobs = new float[Idx];
	hypo->m_nbwords = Idx;

	int nb_w = m_srilmVocab->parseWords(str, vstr, max_words + 1);
	int wid[nb_w];
	m_srilmVocab->getIndices(vstr, (VocabIndex*)wid, nb_w + 1, m_srilmVocab->unkIndex() );
//	 cerr<<" nb_w : "<<nb_w<<endl;
//	 cerr<<" WordID To Eval : ";
//	for(size_t j=0;j<nb_w;j++)
//		cerr<<wid[j]<<" ";
//	cerr<<endl;

	// really useful?
	//for(size_t k=0;k<Idx;k++)
	//	hypo->m_cslmprobs[k] = 0.0f; // init scores

	//int n = nb_w - Idx; //
	int n = startPos;
	int *wptr;

	// don't evaluate unigrams -> why ? we just added BOS !!
	/*if( startPos == 1 ) {
		hypo->m_cslmprobs[0] = 0.0f;
		n++;
	}*/

	// Handle short contexts
	while(n<nb_w && n<cslmOrder-1){
		m_trainer->BlockEval(wid, n, hypo->m_cslmprobs + n - startPos );
		n++;
	}
	// Request prob for full context
	wptr = wid;
	while(n<nb_w){
		m_trainer->BlockEval(wptr, cslmOrder, hypo->m_cslmprobs + n - startPos );
		n++;
		wptr++;
	}
}

/*****************************************
 *  Finish Pending -> Get the CSLM probs
 * ***************************************/
void LanguageModelCSLM::FinishPending()
{
	m_trainer->BlockFinish();
}


/* Overriding SetParameter to parse CSLM-specific parameters */
void LanguageModelCSLM::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "wordlist") {
    m_srilmVocab_filePath = value;
  } else if (key == "srilm") {
      m_srilmNgram_filePath = value;
    }else {
    LanguageModelSingleFactor::SetParameter(key, value);
  }
}

LMResult LanguageModelCSLM::GetValue(const std::vector<const Word*> &contextFactor, State* finalState) const
{

	LMResult ret;
	FactorType    factorType = GetFactorType();
	size_t count = contextFactor.size();

	if(count <= 0){
		ret.score = 0.0;
		ret.unknown = false;
		return ret;
	}

	VocabString vstr[max_words+1];
	const int max_words=16384;
	const int max_chars=max_words*16;
	char str[max_chars];
	int nbw = 0;

	std::string phrases;

	size_t CSModelOrder = 7 ; // CSLM order need to be parsed from the config file!!!!

	// Get the words to be evaluated

	for(size_t i=0; count ;i++){
		phrases.append(contextFactor[i]->GetString(0,false));
		phrases.append(" ");
	}

	strcpy(str,phrases.c_str());
	
	int nb_w = m_srilmVocab->parseWords(str, vstr, max_words + 1); // Get the numb of words of this hypothesis  
	int wid[nb_w];

	m_srilmVocab->getIndices(vstr, (VocabIndex*) wid, nb_w + 1, m_srilmVocab->unkIndex() ); // Get indices in wid

	if(nb_w > m_trainer->BlockGetFree() )
		m_trainer->BlockFinish();

	
	size_t n = 2; // evaluate From bigram 
	float res[nb_w-2]={ 0.0f };
	
	while( n < nb_w  ){ //   

		if(n<CSModelOrder)
			m_trainer->BlockEval(wid  , n   ,  res + n-2  );	
		else
			m_trainer->BlockEval(wid+n ,CSModelOrder,res + n-2)
		
		n++;
	}


	m_trainer->BlockFinish();

	for(size_t j=0;j < nb_w;j++)
	{
		ret.score += res[j];
	}



	return ret;
}


}
