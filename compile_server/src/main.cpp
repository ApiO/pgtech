#include <Windows.h>
#include <iostream>
#include <string>

#include "application.h"

using namespace std;

BOOL WINAPI console_handler(DWORD CEvent)
{
  switch (CEvent)
  {
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      pge::application::stop();
      break;
  }
  return TRUE;
}

int main(int argc, char * argv[])
{
  if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_handler, TRUE) == FALSE)
  {
    printf("Unable to install console handler!\n");
    return -1;
  }

  int r = pge::application::start(argc, argv);
  if (r) return r;
  
  bool exit = false;
  string str;

  while (!exit)
  {
    getline(cin, str);
    r = str.length()>0 ? (int)((str)._Bx._Buf)[0] : 0;

    switch (r)
    {
      default:cout << "Unhandled command\n"; break;
      case 'b':
      case 'B': 
        pge::application::build();  
        break;
      case 'r':
      case 'R': 
        pge::application::rebuid();
        break;
      case 's':
      case 'S': 
        exit = true; 
        break;
      case '0':
        pge::application::start_watch();
        break;
      case '1':
        pge::application::stop_watch();
        break;
    }
  }
  pge::application::stop();

  return 0;
}