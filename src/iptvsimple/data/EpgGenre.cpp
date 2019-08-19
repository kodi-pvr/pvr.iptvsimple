#include "EpgGenre.h"

#include "../utilities/XMLUtils.h"
#include "p8-platform/util/StringUtils.h"

using namespace iptvsimple;
using namespace iptvsimple::data;
using namespace rapidxml;

bool EpgGenre::UpdateFrom(rapidxml::xml_node<>* genreNode)
{
  std::string buffer;
  if (!GetAttributeValue(genreNode, "type", buffer))
    return false;

  if (!StringUtils::IsNaturalNumber(buffer))
    return false;

  m_genreString = genreNode->value();
  m_genreType = atoi(buffer.c_str());
  m_genreSubType = 0;

  if (GetAttributeValue(genreNode, "subtype", buffer) && StringUtils::IsNaturalNumber(buffer))
    m_genreSubType = atoi(buffer.c_str());

  return true;
}
