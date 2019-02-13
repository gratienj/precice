#include "CommunicationConfiguration.hpp"
#include "com/MPIDirectCommunication.hpp"
#include "com/MPIPortsCommunication.hpp"
#include "com/SocketCommunication.hpp"
#include "utils/Helpers.hpp"

namespace precice
{
namespace com
{
PtrCommunication CommunicationConfiguration::createCommunication(
    const xml::XMLTag &tag) const
{
  com::PtrCommunication com;
  if (tag.getName() == "sockets") {
    std::string network = tag.getStringAttributeValue("network");
    int         port    = tag.getIntAttributeValue("port");

    CHECK(not utils::isTruncated<unsigned short>(port),
          "The value given for the \"port\" attribute is not a 16-bit unsigned integer: " << port);

    std::string dir = tag.getStringAttributeValue("exchange-directory");
    com             = std::make_shared<com::SocketCommunication>(port, false, network, dir);
  }
  else if (tag.getName() == "mpi") {
    std::string dir = tag.getStringAttributeValue("exchange-directory");
#ifdef PRECICE_NO_MPI
    std::ostringstream error;
    error << "Communication type \"mpi\" can only be used "
          << "when preCICE is compiled with argument \"mpi=on\"";
    throw error.str();
#else
    com = std::make_shared<com::MPIPortsCommunication>(dir);
#endif
  }
  else if (tag.getName() == "mpi-single") {
#ifdef PRECICE_NO_MPI
    std::ostringstream error;
    error << "Communication type \"mpi-single\" can only be used "
          << "when preCICE is compiled with argument \"mpi=on\"";
    throw error.str();
#else
    com = std::make_shared<com::MPIDirectCommunication>();

#endif
  }
  assertion(com.get() != nullptr);
  return com;
}

} // namespace com
} // namespace precice
