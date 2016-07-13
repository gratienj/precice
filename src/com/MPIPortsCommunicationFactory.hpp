#ifndef PRECICE_NO_MPI

#ifndef PRECICE_COM_MPI_PORTS_COMMUNICATION_FACTORY_HPP_
#define PRECICE_COM_MPI_PORTS_COMMUNICATION_FACTORY_HPP_

#include "CommunicationFactory.hpp"

#include <string>

namespace precice {
namespace com {
class MPIPortsCommunicationFactory : public CommunicationFactory {
public:
  /**
   * @brief Constructor.
   */
  MPIPortsCommunicationFactory(std::string const& addressDirectory = ".");

  Communication::SharedPointer newCommunication();

  std::string addressDirectory();

private:
  std::string _addressDirectory;
};
}
} // namespace precice, com

#endif /* PRECICE_COM_MPI_PORTS_COMMUNICATION_FACTORY_HPP_ */

#endif // not PRECICE_NO_MPI
