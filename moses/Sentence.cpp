// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include "Sentence.h"
#include "TranslationOptionCollectionText.h"
#include "StaticData.h"
#include "moses/FF/DynamicCacheBasedLanguageModel.h"
#include "moses/TranslationModel/PhraseDictionaryDynamicCacheBased.h"
#include "ChartTranslationOptions.h"
#include "Util.h"
#include "XmlOption.h"
#include "FactorCollection.h"
#include "moses/FF/OnlineLearningFeature.h"
#include "moses/FF/MultiTaskLearning.h"

using namespace std;

namespace Moses
{

Sentence::
Sentence() : Phrase(0) , InputType()
{
  const StaticData& SD = StaticData::Instance();
  if (SD.IsSyntax()) 
    m_defaultLabelSet.insert(SD.GetInputDefaultNonTerminal());
}

Sentence::
~Sentence()
{
  RemoveAllInColl(m_xmlOptions);
}

void
Sentence::
aux_init_partial_translation(string& line)
{
  //get covered words - if continual-partial-translation is switched on, parse input

  string sourceCompletedStr;
  int loc1 = line.find( "|||", 0 );
  int loc2 = line.find( "|||", loc1 + 3 );
  if (loc1 > -1 && loc2 > -1) 
    {
      m_initialTargetPhrase = Trim(line.substr(0, loc1));
      string scov = Trim(line.substr(loc1 + 3, loc2 - loc1 - 3));
      line = line.substr(loc2 + 3);

      m_sourceCompleted.resize(scov.size());
      int contiguous = 1;
      for (size_t i = 0; i < scov.size(); ++i) 
	{
	  if (sourceCompletedStr.at(i) == '1') 
	    {
	      m_sourceCompleted[i] = true;
	      if (contiguous) m_frontSpanCoveredLength++;
	    } 
	  else 
	    {
	      m_sourceCompleted[i] = false;
	      contiguous = 0;
	    }
	}
    }
}

void
Sentence::
aux_interpret_sgml_markup(string& line)
{
  // if sentences is specified as "<seg id=1> ... </seg>", extract id
  typedef std::map<std::string, std::string> metamap;
  metamap meta = ProcessAndStripSGML(line);
  metamap::const_iterator i;
  if ((i = meta.find("id")) != meta.end()) 
    this->SetTranslationId(atol(i->second.c_str()));
  if ((i = meta.find("docid")) != meta.end()) 
    {
      this->SetDocumentId(atol(i->second.c_str()));
      this->SetUseTopicId(false);
      this->SetUseTopicIdAndProb(false);
    }
  if ((i = meta.find("topic")) != meta.end()) 
    {
      vector<string> topic_params;
      boost::split(topic_params, i->second, boost::is_any_of("\t "));
      if (topic_params.size() == 1) 
	{
	  this->SetTopicId(atol(topic_params[0].c_str()));
	  this->SetUseTopicId(true);
	  this->SetUseTopicIdAndProb(false);
	} 
      else 
	{
	  this->SetTopicIdAndProb(topic_params);
	  this->SetUseTopicId(false);
	  this->SetUseTopicIdAndProb(true);
	}
    }
  if ((i = meta.find("weight-setting")) != meta.end()) 
    {
      this->SetWeightSetting(i->second);
      this->SetSpecifiesWeightSetting(true);
      StaticData::Instance().SetWeightSetting(i->second); 
      // oh this is so horrible! Why does this have to be propagated globally?
      // --- UG
    } 
  else this->SetSpecifiesWeightSetting(false);
}

void 
Sentence::
aux_interpret_dlt(string& line) // whatever DLT means ... --- UG
{
  using namespace std;
  typedef map<string, string> str2str_map; 
  vector<str2str_map> meta = ProcessAndStripDLT(line);
  BOOST_FOREACH(str2str_map const& M, meta)
    {
      str2str_map::const_iterator i,j;
      if ((i = M.find("type")) != M.end())
	{
	  j = M.find("id");
	  string id = j == M.end() ? "default" : j->second;
	  if (i->second == "cbtm")
	    {
	      PhraseDictionaryDynamicCacheBased* cbtm;
	      cbtm = PhraseDictionaryDynamicCacheBased::InstanceNonConst(id);
	      if (cbtm) cbtm->ExecuteDlt(M);
	    }
	  if (i->second == "cblm") 
	    {
	      DynamicCacheBasedLanguageModel* cblm;
	      cblm = DynamicCacheBasedLanguageModel::InstanceNonConst(id);
	      if (cblm) cblm->ExecuteDlt(M);
	    }
	}
    }
}

void
Sentence::
aux_interpret_xml(std::string& line, std::vector<size_t> & xmlWalls,
		  std::vector<std::pair<size_t, std::string> >& placeholders)
{ // parse XML markup in translation line

  const StaticData &SD = StaticData::Instance();

  using namespace std;
  if (SD.GetXmlInputType() != XmlPassThrough) 
    {
      int offset = SD.IsSyntax() ? 1 : 0;
      bool OK = ProcessAndStripXMLTags(line, m_xmlOptions, 
				       m_reorderingConstraint, 
				       xmlWalls, placeholders, offset,
				       SD.GetXmlBrackets().first,
				       SD.GetXmlBrackets().second);
      UTIL_THROW_IF2(!OK, "Unable to parse XML in line: " << line);
    }
}

void 
Sentence::
init(string line, std::vector<FactorType> const& factorOrder)
{
  using namespace std;
  const StaticData &SD = StaticData::Instance();

  m_frontSpanCoveredLength = 0;
  m_sourceCompleted.resize(0);

  OnlineLearningFeature *ol = &OnlineLearningFeature::InstanceNonConst();
  MultiTaskLearning *mtl = &MultiTaskLearning::InstanceNonConst();
  if(ol!=NULL && mtl==NULL)
  {
	  cerr<<"In Sentence:: "<<line<<endl;
	  std::vector<string> strs;
	  std::string marker="_#_";
	  size_t found = line.find(marker), prev=0;
	  while (found != string::npos)
	  {
		  strs.push_back(line.substr(prev, found - prev));
		  prev = found + marker.length();
		  found = line.find(marker, found + marker.length());
	  }
	  strs.push_back(line.substr(prev));
	  if(strs.size()>=2){
		  ol->ActivateOnlineLearning();
		  if(!ol->SetPostEditedSentence(strs[1])) exit(1);
		  if(strs.size()>=3 && mtl==NULL){
			  // set the alignments
			  if(!ol->SetAlignments(strs[2])) exit(1);
			  VERBOSE(1, "Setting Alignments\n");
		  }
		  /*else if(strs.size()>=3 && mtl!=NULL && mtl->IfMultiTask()){
			  // set task id
			  int task = atoi(strs[2].c_str());
			  mtl->SetCurrentTask(task);
			  StaticData::InstanceNonConst().SetAllWeights(mtl->GetWeightsVector(task));
		  }*/
	  }
	  else{
		  ol->DeactivateOnlineLearning();
		  VERBOSE(1, "Deactivating Online Learning\n");
	  }
	  line=strs[0];
	  ol->SetSourceSentence(line);
	  VERBOSE(1, "Setting source sentence : "<<line<<endl);
  }
  if(ol!=NULL && mtl!=NULL)	// online and multi task learning are active
  {
	cerr<<"In Sentence:: "<<line<<endl;
    std::vector<string> strs;
    std::string marker="_#_";
    size_t found = line.find(marker), prev=0;
    while (found != string::npos)
    {
      strs.push_back(line.substr(prev, found - prev));
      prev = found + marker.length();
      found = line.find(marker, found + marker.length());
    }
    strs.push_back(line.substr(prev));
    if(strs.size()==3){
    	if(ol!=NULL){
    		ol->ActivateOnlineLearning();
    		ol->SetPostEditedSentence(strs[1]);
    		int task = atoi(strs[2].c_str());
    		mtl->SetCurrentTask(task);
    		StaticData::InstanceNonConst().SetAllWeights(mtl->GetWeightsVector(task));
    		cerr<<"Using weights for task id "<<task<<"\t";
    		ScoreComponentCollection weightUpdate = StaticData::Instance().GetAllWeights();
    	}
    	else{
    		VERBOSE(1, "online learning module not activated!!");
    	}
    }
    else if(strs.size()==2){		// even while decoding normal sentence we need task id
    	ol->DeactivateOnlineLearning();
    	int task = atoi(strs[1].c_str());
    	mtl->SetCurrentTask(task);
    	StaticData::InstanceNonConst().SetAllWeights(mtl->GetWeightsVector(task));
    	ScoreComponentCollection weightUpdate = StaticData::Instance().GetAllWeights();
    }
    line=strs[0];
  }


  if (SD.ContinuePartialTranslation()) 
    aux_init_partial_translation(line);

  line = Trim(line);
  aux_interpret_sgml_markup(line); // for "<seg id=..." markup
  aux_interpret_dlt(line); // some poorly documented cache-based stuff
  
  // if sentences is specified as "<passthrough tag1=""/>"
  if (SD.IsPassthroughEnabled() || SD.IsPassthroughInNBestEnabled()) 
    {
      string pthru = PassthroughSGML(line,"passthrough");
      this->SetPassthroughInformation(pthru);
    }

  vector<size_t> xmlWalls;
  vector<pair<size_t, string> >placeholders;
  aux_interpret_xml(line, xmlWalls, placeholders);

  Phrase::CreateFromString(Input, factorOrder, line, NULL);

  ProcessPlaceholders(placeholders);

  if (SD.IsSyntax()) InitStartEndWord();
  
  // now that we have final word positions in phrase (from
  // CreateFromString), we can make input phrase objects to go with
  // our XmlOptions and create TranslationOptions

  // only fill the vector if we are parsing XML
  if (SD.GetXmlInputType() != XmlPassThrough) 
    {
      m_xmlCoverageMap.assign(GetSize(), false);
      BOOST_FOREACH(XmlOption* o, m_xmlOptions)
	{
	  WordsRange const& r = o->range;
	  for(size_t j = r.GetStartPos(); j <= r.GetEndPos(); ++j) 
	    m_xmlCoverageMap[j]=true;
	}
    }

  // reordering walls and zones
  m_reorderingConstraint.InitializeWalls(GetSize());

  // set reordering walls, if "-monotone-at-punction" is set
  if (SD.UseReorderingConstraint() && GetSize()) 
    {
      WordsRange r(0, GetSize()-1);
      m_reorderingConstraint.SetMonotoneAtPunctuation(GetSubString(r));
    }

  // set walls obtained from xml
  for(size_t i=0; i<xmlWalls.size(); i++)
    if(xmlWalls[i] < GetSize()) // no buggy walls, please
      m_reorderingConstraint.SetWall(xmlWalls[i], true);
  m_reorderingConstraint.FinalizeWalls();
  
}

int 
Sentence::
Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
  std::string line;
  if (getline(in, line, '\n').eof())
    return 0;
  init(line, factorOrder);
  return 1;
}

void 
Sentence::
ProcessPlaceholders(const std::vector< std::pair<size_t, std::string> > &placeholders)
{
  FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();
  if (placeholderFactor == NOT_FOUND) {
    return;
  }

  for (size_t i = 0; i < placeholders.size(); ++i) {
    size_t pos = placeholders[i].first;
    const string &str = placeholders[i].second;
    const Factor *factor = FactorCollection::Instance().AddFactor(str);
    Word &word = Phrase::GetWord(pos);
    word[placeholderFactor] = factor;
  }
}

TranslationOptionCollection*
Sentence::
CreateTranslationOptionCollection() const
{
  size_t maxNoTransOptPerCoverage = StaticData::Instance().GetMaxNoTransOptPerCoverage();
  float transOptThreshold = StaticData::Instance().GetTranslationOptionThreshold();
  TranslationOptionCollection *rv= new TranslationOptionCollectionText(*this, maxNoTransOptPerCoverage, transOptThreshold);
  assert(rv);
  return rv;
}
void Sentence::Print(std::ostream& out) const
{
  out<<*static_cast<Phrase const*>(this);
}


bool Sentence::XmlOverlap(size_t startPos, size_t endPos) const
{
  for (size_t pos = startPos; pos <=  endPos ; pos++) {
    if (pos < m_xmlCoverageMap.size() && m_xmlCoverageMap[pos]) {
      return true;
    }
  }
  return false;
}

void Sentence::GetXmlTranslationOptions(std::vector <TranslationOption*> &list) const
{
  for (std::vector<XmlOption*>::const_iterator iterXMLOpts = m_xmlOptions.begin();
       iterXMLOpts != m_xmlOptions.end(); ++iterXMLOpts) {
    const XmlOption &xmlOption = **iterXMLOpts;
    const WordsRange &range = xmlOption.range;
    const TargetPhrase &targetPhrase = xmlOption.targetPhrase;
    TranslationOption *transOpt = new TranslationOption(range, targetPhrase);
    list.push_back(transOpt);
  }
}

void Sentence::GetXmlTranslationOptions(std::vector <TranslationOption*> &list, size_t startPos, size_t endPos) const
{
  //iterate over XmlOptions list, find exact source/target matches

  for (std::vector<XmlOption*>::const_iterator iterXMLOpts = m_xmlOptions.begin();
       iterXMLOpts != m_xmlOptions.end(); ++iterXMLOpts) {
    const XmlOption &xmlOption = **iterXMLOpts;
    const WordsRange &range = xmlOption.range;

    if (startPos == range.GetStartPos()
        && endPos == range.GetEndPos()) {
      const TargetPhrase &targetPhrase = xmlOption.targetPhrase;

      TranslationOption *transOpt = new TranslationOption(range, targetPhrase);
      list.push_back(transOpt);
    }
  }
}

std::vector <ChartTranslationOptions*> Sentence::GetXmlChartTranslationOptions() const
{
  const StaticData &staticData = StaticData::Instance();
  std::vector <ChartTranslationOptions*> ret;

  // XML Options
  // this code is a copy of the 1 in Sentence.

  //only fill the vector if we are parsing XML
  if (staticData.GetXmlInputType() != XmlPassThrough ) {
    //TODO: needed to handle exclusive
    //for (size_t i=0; i<GetSize(); i++) {
    //  m_xmlCoverageMap.push_back(false);
    //}

    //iterXMLOpts will be empty for XmlIgnore
    //look at each column
    for(std::vector<XmlOption*>::const_iterator iterXmlOpts = m_xmlOptions.begin();
        iterXmlOpts != m_xmlOptions.end(); iterXmlOpts++) {

      const XmlOption &xmlOption = **iterXmlOpts;
      TargetPhrase *targetPhrase = new TargetPhrase(xmlOption.targetPhrase);

      WordsRange *range = new WordsRange(xmlOption.range);
      StackVec emptyStackVec; // hmmm... maybe dangerous, but it is never consulted

      TargetPhraseCollection *tpc = new TargetPhraseCollection;
      tpc->Add(targetPhrase);

      ChartTranslationOptions *transOpt = new ChartTranslationOptions(*tpc, emptyStackVec, *range, 0.0f);
      ret.push_back(transOpt);

      //TODO: needed to handle exclusive
      //for(size_t j=transOpt->GetSourceWordsRange().GetStartPos(); j<=transOpt->GetSourceWordsRange().GetEndPos(); j++) {
      //  m_xmlCoverageMap[j]=true;
      //}
    }
  }

  return ret;
}

void
Sentence::
CreateFromString(vector<FactorType> const& FOrder, string const& phraseString)
{
  Phrase::CreateFromString(Input, FOrder, phraseString, NULL);
}

Sentence::
Sentence(size_t const transId, string const& stext) : InputType(transId)
{
  vector<FactorType> const& IFO = StaticData::Instance().GetInputFactorOrder();
  init(stext, IFO);
}

}

