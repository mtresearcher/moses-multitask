
#pragma once

#include "PhraseDictionary.h"

namespace Moses
{
class ChartParser;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

class UnknownWordPenalty2 : public PhraseDictionary
{
  friend std::ostream& operator<<(std::ostream&, const UnknownWordPenalty2&);

public:
  UnknownWordPenalty2(const std::string &line);

  void Load();

  void InitializeForInput(InputType const& source);

  // for phrase-based model
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  // for syntax/hiero model (CKY+ decoding)
  ChartRuleLookupManager* CreateRuleLookupManager(const ChartParser&, const ChartCellCollectionBase&, std::size_t);

  void SetParameter(const std::string& key, const std::string& value);

  TO_STRING();


protected:
  TargetPhrase *CreateTargetPhrase(const Phrase &sourcePhrase) const;

  size_t m_maxPhraseLength;
};

}  // namespace Moses
