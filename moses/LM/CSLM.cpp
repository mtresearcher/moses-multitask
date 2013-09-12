//
/************************************************
  Nbest-rescoring : using CS Language Model - Fethi bougares - LIUM Lab, Le mans university - France

  Moses - factored phrase-based language decoder
  CSLM  - Continuous space language Model

*************************************************/


#include <limits>
#include <iostream>
#include <fstream>

#include "Util.h"
#include "moses/TypeDef.h"
#include "CSLM.h"
#include "TrainerNgramSlist.h"  // FROM CSLM

#include "Vocab.h"
#include "Ngram.h"
#include "moses/StaticData.h"
#include "moses/HypothesisStack.h"
#include "moses/TrellisPathList.h"

using namespace std;

namespace Moses
{

LanguageModelCSLM::LanguageModelCSLM(const std::string &line) : LanguageModelSingleFactor("CSLM", line)
	:m_srilmVocab(NULL)
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
  ifs.open(m_filePath,ios::binary);
  CHECK_FILE(ifs,m_filePath);
  m_mach = Mach::Read(ifs);
  ifs.close();

  m_mach->Info();
  nGramOrder = m_mach->GetIdim()+1;

  m_trainer    = new TrainerNgramSlist(m_mach,m_srilmVocab_filePath, m_srilmNgram_filePath);
  m_srilmVocab = m_trainer->GetVocab();

  VERBOSE(1," - using SRILM vocabulary with " << m_srilmVocab->numWords() << " words"<<endl);
}



/***************************************
 *  Rescore Nbest List using CSLM Model
 ***************************************/
/*
void LanguageModelCSLM::RescoreNbest(TrellisPathList & nBestList)
{
	boost::mutex::scoped_lock lock(cslm_mutex);

	TrellisPathList::iterator iter, iter1;
	float cslm_score=0.0f,TotalScore=0.0f;

	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		TrellisPath * path =  const_cast<TrellisPath *>(*iter);
		RescorePath(path);
	}

	m_trainer->BlockFinish();
	IFVERBOSE(1)
	m_trainer->BlockStats();

	for(iter1 = nBestList.begin() ; iter1 != nBestList.end() ; ++iter1 )
	{
		TrellisPath * path =  const_cast<TrellisPath *>(*iter1);
		cslm_score=0.0f ;
		for(size_t i =0; i< path->m_nbwords - 1 ;i++)
		{
			cslm_score += path->m_cslmScores[i] ;
		}

		path->SetCslmScore(cslm_score);
		TotalScore= UpdatePathTotalScore(path);
		path->SetTotalScore(TotalScore);
	}
	nBestList.SortAfterResco();

	VERBOSE(1," Free CSLM by "<<(unsigned int)pthread_self()<<endl);
}


*/

/********************************
 * Calculate the CSLM Score	*
 * of a Path			*
 ********************************/
/*
void LanguageModelCSLM::RescorePath(TrellisPath *path )
{
	const int max_words=16384;
	const int max_chars=max_words*16;
	const vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
	char str[max_chars];
	VocabString vstr[max_words+1];
	int lm_order = m_mach->GetIdim()+1;
	std::string nbest;
	nbest.append(path->GetTargetPhrase().GetStringRep(outputFactorOrder));

	strcpy(str,nbest.c_str());
	int nb_w = m_srilmVocab->parseWords(str, vstr, max_words + 1);

	path->m_nbwords = nb_w +2 ;
	path->m_cslmScores = new float[nb_w +2];

	if (nb_w == max_words+1) cerr<<"too many words in one hypothesis "<<endl;
	int wid[path->m_nbwords +2 ];
	int b=0;

	wid[b++] = m_srilmVocab->ssIndex();//put <s> in first case
	m_srilmVocab->getIndices(vstr, (VocabIndex*) (wid+b), nb_w + 1, m_srilmVocab->unkIndex() );
	nb_w += b;
	wid[nb_w++]=  m_srilmVocab->seIndex(); //end of Sentence
	int n=2;

	while (n<=nb_w && n<lm_order) {
		//request one n-gram probability "of wid", n is the order, last argument is the address to stock the ngram prob
		m_trainer->BlockEval(wid, n, path->m_cslmScores+ n-2  );
		n++;
	}

	int *wptr=wid;
	while (n<= nb_w ) {
		 m_trainer->BlockEval(wptr, lm_order, path->m_cslmScores+n-2 );
		 n++; wptr++;
	}

	m_trainer->BlockFinish();
}

*/
/****************************************
 * Update the Total Score of Actual Path
 * Add CSLMScore * CSLM_Weight
 ****************************************/

/*float LanguageModelCSLM::UpdatePathTotalScore(TrellisPath * path )
{
	float TotalScore=0.0;
	std::vector<float > Weights  = StaticData::Instance().GetRescoWeights();
	std::valarray<float > Scores = path->GetScoreBreakdown().getCoreFeatures();

	if(Scores.size() != Weights.size()){
		std::cerr<<" Problem of size Scores vs Weights "<<Scores.size()<<" <?> "<<Weights.size()<<endl;
	}

	for(size_t i=0;i<Scores.size();i++)
		TotalScore += Scores[i]*Weights[i];

	TotalScore += path->GetCslmScore() * StaticData::Instance().GetLMRescoringWeight();


   return TotalScore;
}*/



/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************    SEARCH SPACE RESCORING  WITH CSLM Model    *************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/
/*************************************************************************************************************/

/***************************************************************************************
 * Run into each Stack and calculate the CSLM score of each hypothesis and it's arc list
 **************************************************************************************/

void LanguageModelCSLM::RescoreLAT( std::vector < HypothesisStack* >& hypoStackColl )
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

/*************************
 **************************************/
// RescoreLAT From begin
// And Backward Pass : Minus Previous Breakdown Score To clean the Scores (Merci Loïc ;) )
//
void LanguageModelCSLM::RescoreLATV1( std::vector < HypothesisStack* >& hypoStackColl )
{

	boost::mutex::scoped_lock lock(cslm_mutex);
	std::vector < HypothesisStack* >::iterator iterStack;

	for (iterStack = ++hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack)
	{
		HypothesisStack::iterator iterHypo;
		for(iterHypo = (*iterStack)->Nocbegin() ; iterHypo != (*iterStack)->Nocend() ; ++iterHypo)
		{
		  GetCSLMScoreOpt(*iterHypo);
		  const ArcList *pAL =   (*iterHypo)->GetArcList();
			if(pAL){
				ArcList::const_iterator iterArc;
				for (iterArc = pAL->begin() ; iterArc != pAL->end() ; ++iterArc)
				{
					Hypothesis *arc = const_cast<Hypothesis*>(*iterArc);
					GetCSLMScoreOpt(arc);
				}
			}
		FinishPending();
		}



	}
}

/******************************************************
 *     		Calc CSLM Score optimize	      *
 ******************************************************/

void LanguageModelCSLM::GetCSLMScoreOpt(Hypothesis* hypo )
{
	if(StaticData::Instance().GetCSLMnGramOrder() <= 1 )
		return;

	VocabString vstr[max_words+1];
	const int max_words=16384;
	const int max_chars=max_words*16;
	char str[max_chars];
	int Hyp_nbw = 0;
	size_t CSModelOrder = StaticData::Instance().GetCSLMnGramOrder();
	const int startPos   = hypo->GetCurrTargetWordsRange().GetStartPos();
	const size_t currEndPos = hypo->GetCurrTargetWordsRange().GetEndPos();
	int Idx = hypo->GetCurrTargetLength() ;
	std::vector<string> contextFactor ;
	std::string contextCSLM;

	size_t BPos = (startPos - 6 >= 0 ) ? startPos - 6 : 0 ;

	if( startPos < 6 ) { contextFactor.push_back(BOS_); }; //add BOS_


	for(size_t Currpos = BPos; Currpos <= currEndPos ; Currpos ++){
		contextFactor.push_back( hypo->GetWord(Currpos).GetString(0)  );
	}

	if( hypo->IsSourceCompleted() ) { contextFactor.push_back(EOS_); Idx++;  };

//	 cerr<<" Hypo ID : "<<hypo->GetId()<<" Start Pos : "<<startPos <<" --> "<<currEndPos<<endl;
	for(size_t i =0 ; i< contextFactor.size()-1;i++)
	{
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
	m_srilmVocab->getIndices(vstr, (VocabIndex*) (wid), nb_w + 1, m_srilmVocab->unkIndex() );
//	 cerr<<" nb_w : "<<nb_w<<endl;
//	 cerr<<" WordID To Eval : ";
//	for(size_t j=0;j<nb_w;j++)
//		cerr<<wid[j]<<" ";
//	cerr<<endl;
	for(size_t k=0;k<Idx;k++)
		hypo->m_cslmprobs[k] =0.0f; // Init Scores to zero

	int n=  nb_w - Idx ;
	int from = n;
	int *wptr;

	while( n < nb_w){
//		cerr<<" n : "<<n<<endl;
		if( n < 6 && n >= 1){ //SRILM
//			cerr<<" Eval SRILM : O ("<<n+1<<") ";
//			for(size_t fe=0; fe <= n; fe++)
//				cerr<<" "<<wid[fe];
//			cerr<<"(in "<<n-from <<" )"<<endl;

		 m_trainer->BlockEval(wid,n+1, hypo->m_cslmprobs + (n - from )  );

		}else if (n >= 6) {
//		cerr<<" Eval CSLM  : ";
		wptr = &wid[n-6];
//		for(size_t fe=0; fe < 7; fe++)
//			cerr<<" "<<wptr[fe];
//		cerr<<"(in "<<n-from <<" )"<<endl;
		m_trainer->BlockEval(wptr, CSModelOrder, hypo->m_cslmprobs + (n - from )  );
		}
	     n++;
	}
}

/******************************************
 * Calc the CSLM Score Of one Hypothesis
 *******************************/
void LanguageModelCSLM::GetCSLMScore(Hypothesis* hypo )
{
	if(StaticData::Instance().GetCSLMnGramOrder() <= 1 )
		return ;
	if (hypo->GetCurrTargetLength() == 0)
		return ;

	VocabString vstr[max_words+1];
	const int max_words=16384;
	const int max_chars=max_words*16;
	char str[max_chars];
	int Hyp_nbw = 0;

	//const Hypothesis* PrevHyp = hypo->GetPrevHypo() ;
	size_t CSModelOrder = StaticData::Instance().GetCSLMnGramOrder(); // CSLM Model Order

	Hyp_nbw = hypo->GetCurrTargetLength();
	const size_t currEndPos = hypo->GetCurrTargetWordsRange().GetEndPos();
	const int startPos   = hypo->GetCurrTargetWordsRange().GetStartPos();

	std::string contextFactor ;

	contextFactor.append(BOS_); // Set the Begin of Sentence
//	Hyp_nbw++; // nbword + start of Sentence
	contextFactor.append(" ");
	 // GET The Context (look for all previous strings )

	for (int currPos = 0 ; currPos <= (int) currEndPos ; currPos++) {
			contextFactor.append( hypo->GetWord(currPos).GetString(0) );
			contextFactor.append(" ");
			cerr<<hypo->GetWord(currPos).GetString(0)<<" ";
	}

	cerr<<endl;

	if( hypo->IsSourceCompleted() ){ contextFactor.append(EOS_); /*contextFactor.append(" "); */ Hyp_nbw++; } // Add the EndofSentence </s> if source compeleted

	cerr<<" Hyp "<<hypo->GetId()<<" nbW "<<Hyp_nbw<<" currEndPos :"<<currEndPos<<" ,  Output : "<<contextFactor;
	cerr<<endl;

	strcpy(str,contextFactor.c_str());

	// Prepare the space to stock the cslm prob
	hypo->m_nbwords = Hyp_nbw ;
	hypo->m_cslmprobs = new float[Hyp_nbw];
	// init the probs to zero

	for(size_t i=0;i<Hyp_nbw;i++)
		hypo->m_cslmprobs[i]=0;

	int nb_w = m_srilmVocab->parseWords(str, vstr, max_words + 1); // Get the numb of words from <s> to the final word of this hypothesis

	int wid[nb_w];

	m_srilmVocab->getIndices(vstr, (VocabIndex*) (wid), nb_w + 1, m_srilmVocab->unkIndex() ); // Get indices in wid

	cerr<<" Word cumulated : ";
	for(size_t j=0;j<nb_w;j++)
		cerr<<wid[j]<<" ";
	cerr<<endl;

	size_t begin = startPos + 1 ;
	cerr<<" Word to eval   : ";
	if( begin < 2  ) begin =2 ;

	for(size_t j= begin ; j<nb_w ;j++)
		cerr<<wid[j]<<" ";
	cerr<<endl;

	if(nb_w > m_trainer->BlockGetFree() )
		m_trainer->BlockFinish();

	int from = std::max( 2,startPos +1 ) ;
	// int to   = nb_w; // ( currEndPos < 3 ) ? currEndPos : 3 ;  // LM prob for words under the CSLM Order



	int n = from ;

	cerr<<"Start Pos = "<<startPos<<", from = "<<from<<endl<<endl;
	cerr<<"nbre word from begin "<<nb_w<<endl;

	cerr<<" Word to evaluate SRI =  (n="<<n<<", nb="<<nb_w<<", currEndPos="<<currEndPos<<", startPos="<<startPos<<")  ";
	while ( nb_w > 3 &&  n < nb_w && n < CSModelOrder) {
		cerr<<" "<<wid[n]<<"(o="<<n<<" -- idx="<<n - from <<")" ;
	        m_trainer->BlockEval(wid, n, hypo->m_cslmprobs + (n - from)  );
		n++;
		}
	cerr<<endl;

	// Prob when hypothesis has history lenght geq to CSLM Order
	cerr<<" n =  "<<n<<endl;
	if( nb_w >  CSModelOrder ){

	int *wptr; // =&wid[n-7];
	int fe = 0;
	// n = startPos ;
	cerr<<" Word to evaluate CSL =  (n="<<n<<", nb="<<nb_w<<", currEndPos="<<currEndPos<<", startPos="<<startPos<<")  ";
	while( n < nb_w && n >= CSModelOrder  ) {
		wptr=&wid[n-7];
		cerr<<endl<<" "<<wid[n]<<" (n="<<n<<" ,"<<" -- idx="<<n - from <<")" ;
		cerr<<" Ctxt = ";
		for(fe=0; fe < 7; fe++)
			cerr<<" "<<wptr[fe];

		m_trainer->BlockEval(wptr, CSModelOrder, hypo->m_cslmprobs + (n - from ) );
		n++; //wptr++;
	}

	cerr<<endl<<endl;
	}

	m_trainer->BlockFinish();

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



}
