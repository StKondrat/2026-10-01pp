#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>
#include <unistd.h>

size_t recv_msg(int& err, int rd, char* buffer, size_t size)
{
  err = 0;
  size_t total = 0;

  while (total < size)
  {
    ssize_t count = read(rd, buffer + total, size - total);

    if (count < 0 && errno == EINTR)
    {
      continue;
    }

    if (count <= 0)
    {
      err = -1;
      return total;
    }

    total += static_cast< size_t >(count);
  }

  return total;
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: child <fd>\n";
    return 1;
  }

  char* end = nullptr;
  errno = 0;
  long descriptor = std::strtol(argv[1], &end, 10);

  if (errno != 0 || end == argv[1] || *end != '\0' ||
      descriptor < 0 || descriptor > INT_MAX)
  {
    std::cerr << "Invalid descriptor\n";
    return 1;
  }

  int rd = static_cast< int >(descriptor);
  int err = 0;
  size_t size = 0;

  recv_msg(err, rd, reinterpret_cast< char* >(&size), sizeof(size));

  if (err != 0)
  {
    std::cerr << "Failed to read the message size\n";
    close(rd);
    return 1;
  }

  std::vector< char > message;

  try
  {
    message.resize(size);
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    close(rd);
    return 1;
  }

  recv_msg(err, rd, message.data(), size);
  close(rd);

  if (err != 0)
  {
    std::cerr << "Failed to read the message\n";
    return 1;
  }

  std::cout << "The child process received: ";

  for (char symbol : message)
  {
    std::cout.put(symbol);
  }

  std::cout << '\n';
  std::cout.flush();

  return 0;
}