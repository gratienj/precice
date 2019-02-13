#include "Publisher.hpp"

#include "Event.hpp"

#include <boost/filesystem.hpp>

#include <fstream>
#include <sstream>

namespace precice
{
namespace utils
{

std::string Publisher::_pdp;

std::stack<std::string> Publisher::_dps;

Publisher::ScopedPushDirectory::ScopedPushDirectory(std::string const &dp)
{
  Publisher::pushDirectory(dp);
}

Publisher::ScopedPushDirectory::~ScopedPushDirectory()
{
  Publisher::popDirectory();
}

Publisher::ScopedChangePrefixDirectory::ScopedChangePrefixDirectory(
    std::string const &pdp)
    : _pdp(Publisher::prefixDirectoryPath())
{
  Publisher::changePrefixDirectory(pdp);
}

Publisher::ScopedChangePrefixDirectory::~ScopedChangePrefixDirectory()
{
  Publisher::changePrefixDirectory(_pdp);
}

std::string Publisher::parentPath(std::string const &p)
{
  return boost::filesystem::path(p).parent_path().string();
}

bool Publisher::createDirectory(std::string const &dp)
{
  return boost::filesystem::create_directory(dp);
}

bool Publisher::exists(std::string const &p)
{
  return boost::filesystem::exists(p);
}

bool Publisher::remove(std::string const &p)
{
  return boost::filesystem::remove(p);
}

void Publisher::rename(std::string const &op, std::string const &np)
{
  boost::filesystem::rename(op, np);
}

bool Publisher::pushDirectory(std::string const &dp)
{
  using boost::filesystem::path;

  if (not path(dp).empty()) {
    _dps.push(dp);

    return true;
  }

  return false;
}

bool Publisher::popDirectory()
{
  if (not _dps.empty()) {
    _dps.pop();

    return true;
  }

  return false;
}

void Publisher::changePrefixDirectory(std::string const &pdp)
{
  _pdp = boost::filesystem::path(pdp).string();
}

std::string const & Publisher::prefixDirectoryPath()
{
  return _pdp;
}

Publisher::Publisher(std::string const &fp)
    : _fp(buildFilePath(fp))
{
}

std::string Publisher::read() const
{
  std::ifstream ifs;
  std::string   data;

  do {
    ifs.open(filePath(), std::ifstream::in);
  } while (not ifs);

  std::chrono::milliseconds::rep writeTimeStampCount;

  std::string line;
  std::getline(ifs, line);
  std::istringstream iss(line);
  iss >> writeTimeStampCount;
  std::getline(ifs, data);
  return data;
}

void Publisher::write(std::string const &data) const
{
  createDirectory(parentPath(filePath()));

  {
    std::ofstream ofs(filePath() + "~", std::ofstream::out);

    auto writeTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(Event::Clock::now().time_since_epoch());

    ofs << writeTimeStamp.count() << "\n"
        << data;
  }

  rename(filePath() + "~", filePath());
}

std::string const & Publisher::filePath() const
{
  return _fp;
}

std::string Publisher::buildFilePath(std::string const &fp)
{
  using boost::filesystem::path;

  {
    path p(fp);

    if (p.is_absolute())
      return p.string();
  }

  path p(_pdp);

  if (not _dps.empty())
    p /= _dps.top();

  p /= fp;

  return p.string();
}

ScopedPublisher::ScopedPublisher(std::string const &fp)
    : Publisher(fp)
{
}

ScopedPublisher::~ScopedPublisher()
{
  remove(filePath());
}

} // namespace utils
} // namespace precice
