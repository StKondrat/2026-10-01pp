#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

size_t send_msg(int& err, int wr, const char* buffer, size_t size)
{
  err = 0;
  size_t total = 0;

  while (total < size)
  {
    ssize_t count = write(wr, buffer + total, size - total);

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

int main()
{
  if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    std::perror("signal");
    return 1;
  }

  int pps[2];

  if (pipe(pps) == -1)
  {
    std::perror("pipe");
    return 1;
  }

  int rd = pps[0];
  int wr = pps[1];
  pid_t pid = fork();

  if (pid == -1)
  {
    std::perror("fork");
    close(rd);
    close(wr);
    return 1;
  }

  if (pid == 0)
  {
    close(wr);

    char argument[32];
    std::snprintf(argument, sizeof(argument), "%d", rd);

    execl("./child", "child", argument, static_cast< char* >(nullptr));

    std::perror("execl");
    _exit(1);
  }

  close(rd);

  std::string message;
  std::cout << "Enter the message: ";

  int result = 0;
  int err = 0;

  if (!std::getline(std::cin, message))
  {
    std::cerr << "Failed to read the message\n";
    result = 1;
  }
  else
  {
    size_t size = message.size();

    send_msg(err, wr, reinterpret_cast< const char* >(&size), sizeof(size));

    if (err == 0)
    {
      send_msg(err, wr, message.data(), size);
    }

    if (err != 0)
    {
      std::cerr << "Failed to send the message\n";
      result = 1;
    }
  }

  close(wr);

  int status = 0;
  pid_t waited;

  do
  {
    waited = waitpid(pid, &status, 0);
  }
  while (waited == -1 && errno == EINTR);

  if (waited == -1)
  {
    std::perror("waitpid");
    return 1;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
  {
    result = 1;
  }

  return result;
}