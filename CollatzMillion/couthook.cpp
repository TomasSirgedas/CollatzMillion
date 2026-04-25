#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <vector>
#include <regex>
#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

class CustomStreamBuf : public std::streambuf
{
public:
   CustomStreamBuf( const std::function<void( const std::string& )>& onLineFunc )
      : m_onLineFunc( onLineFunc )
   {
   }
private:
   virtual int overflow( int c )
   {
      m_ss.rdbuf()->sputc( c );
      if ( c == 10 )
      {
         m_onLineFunc( m_ss.str() );
         m_ss.str( "" ); // clear buffer 
         m_ss.clear();   // clear error flags
      }
      return c;
   }

   // Sync both teed buffers.
   virtual int sync()
   {
      m_ss.rdbuf()->pubsync();
      return 0;
   }
private:
   std::stringstream m_ss;
   std::function<void( const std::string& )> m_onLineFunc;
};

class CoutHook
{
public:
   CoutHook()
   {
      m_customStreamBuf = std::make_unique<CustomStreamBuf>( [&]( const std::string& line )
      {
         m_coutOriginalBuf->sputn( line.data(), line.length() ); // emulate `std::cout << line;`
         for ( const auto& func : m_hookFuncs )
            func( line );
      } );

      m_coutOriginalBuf = std::cout.rdbuf();
      std::cout.set_rdbuf( m_customStreamBuf.get() );
   }
   ~CoutHook()
   {
      std::cout.rdbuf( m_coutOriginalBuf );
   }
   static CoutHook& instance()
   {
      static CoutHook g_redirectCout;
      return g_redirectCout;
   }
   void addHook( const std::function<void( const std::string& )> & func )
   {
      m_hookFuncs.push_back( func );
   }

private:
   std::unique_ptr<CustomStreamBuf> m_customStreamBuf;
   std::streambuf* m_coutOriginalBuf;
   std::vector< std::function<void( const std::string& )>> m_hookFuncs;
};

namespace
{
   struct InitRedirectCout
   {
      std::ofstream m_logFile;

      InitRedirectCout()
      {
         CoutHook::instance(); // instantiate singleton
#ifdef _WIN32
         // echo to Visual Studio Output console (or DebugView.exe)
         CoutHook::instance().addHook( []( const std::string& line ) {
            ::OutputDebugStringA( line.c_str() );
         } );

         // echo to log file
         {
            char exePath[MAX_PATH];
            GetModuleFileNameA( NULL, exePath, MAX_PATH );
            std::regex re( R"(.*\\(.*)\.exe)" );
            std::smatch matches;
            std::string path = exePath;
            std::string logFilename = "out.txt";
            if ( std::regex_match( path, matches, re ) )
               logFilename = matches[1].str() + ".txt";
            m_logFile.open( logFilename );
            CoutHook::instance().addHook( [&]( const std::string& line ) {
               m_logFile << line;
            } );
         }
#endif
      }
   } g_init;
}
