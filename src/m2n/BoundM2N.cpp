#include "m2n/BoundM2N.hpp"
#include "m2n/M2N.hpp"
#include "com/Communication.hpp"

namespace precice {
namespace m2n {

void BoundM2N::prepareEstablishment()
{
    m2n->prepareEstablishment();
}

void BoundM2N::connectMasters()
{
    std::string fullLocalName = localName;
    if (localServer) fullLocalName += "Server";

    if (isRequesting){
        m2n->requestMasterConnection(remoteName, fullLocalName);
    }
    else {
        m2n->acceptMasterConnection(fullLocalName, remoteName);
    }
}

void BoundM2N::connectSlaves()
{
    if (isRequesting){
        m2n->requestSlavesConnection(remoteName, localName);
    }
    else {
        m2n->acceptSlavesConnection(localName, remoteName);
    }
}

void BoundM2N::cleanupEstablishment()
{
    m2n->prepareEstablishment();
}

} // namespace m2n
} // namespace precice
