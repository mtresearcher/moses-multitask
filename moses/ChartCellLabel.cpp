#include "ChartCellLabel.h"
#include "ChartParser.h"

namespace Moses
{

 ChartCellLabel::ChartCellLabel(const ChartParser &parser, const WordsRange &coverage, const Word &label,
                 Stack stack)
 : m_inputPath(parser.GetInputPath(coverage))
 , m_coverage(coverage)
 , m_label(label)
 , m_stack(stack)
 , m_bestScore(0)
 {

 }

}

