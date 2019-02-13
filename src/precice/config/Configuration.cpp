#include "Configuration.hpp"
#include "xml/XMLAttribute.hpp"


namespace precice {

extern bool syncMode;

namespace config {

Configuration:: Configuration()
:
  _tag(*this, "precice-configuration", xml::XMLTag::OCCUR_ONCE),
  _logConfig(_tag),
  _solverInterfaceConfig(_tag)
{
  _tag.setDocumentation("Main tag containing preCICE configuration.");
  _tag.addNamespace("data");
  _tag.addNamespace("communication");
  _tag.addNamespace("mapping");
  _tag.addNamespace("export");
  _tag.addNamespace("action");
  _tag.addNamespace("server");
  _tag.addNamespace("coupling-scheme");
  _tag.addNamespace("post-processing");

  auto attrSyncMode = xml::makeXMLAttribute("sync-mode", false)
      .setDocumentation("sync-mode enabled additional inter- and intra-participant synchronizations");
  _tag.addAttribute(attrSyncMode);

}

xml::XMLTag& Configuration:: getXMLTag()
{
  return _tag;
}

void Configuration::xmlTagCallback(xml::XMLTag& tag)
{
  TRACE(tag.getName());
  if (tag.getName() == "precice-configuration") {
    precice::syncMode = tag.getBooleanAttributeValue("sync-mode");
  }
}

void Configuration:: xmlEndTagCallback
(
  xml::XMLTag& tag )
{
  TRACE(tag.getName());
}

const SolverInterfaceConfiguration&
Configuration:: getSolverInterfaceConfiguration() const
{
  return _solverInterfaceConfig;
}

}} // namespace precice, config
