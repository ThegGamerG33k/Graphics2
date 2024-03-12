#include "consoleoutput.h"
#include <assert.h>
#include "platform.h"
#include "mischelpers.h"

#if not defined WINDOWS_PLATFORM && not defined LINUX_PLATFORM
    #if defined _MSC_VER
        #define WIN32_LEAN_AND_MEAN 1
        #define VC_EXTRALEAN 1
        #define WINDOWS_PLATFORM 1
        #include <windows.h>
    #elif defined __linux
        #define LINUX_PLATFORM 1
    #endif
#endif

#define MAX_WIDTH 75

bool _useColors=true;
bool _printErrors=true;
bool _printWarnings=true;
bool _printInfo=true;
bool _printVerbose=false;


void setOutputOptions( bool printErrors, bool printWarnings, bool printInfo,
    bool printVerbose, bool useColor )
{
    _printErrors = printErrors;
    _printWarnings = printWarnings;
    _printInfo = printInfo;
    _printVerbose = printVerbose;
    _useColors = useColor;
}



void newline(const std::string& s, std::size_t& col, std::ostringstream& os, std::size_t& i, const std::string& prefix)
{
    col=0;
    os << '\n';
    i++;
    if( i < s.length() ){
        for(std::size_t ss=0;ss<prefix.length();++ss){
            os << ' ';
            col++;
        }
    }

    //consume leading spaces
    while(i < s.length() && s[i] == ' ')
        i++;
}

static
std::string nextWord(std::string& line){
    std::string w;
    while(true){
        if( line.length() == 0 )
            return w;
        if( line[0] == '-' ){
            w += line[0];
            line.erase(0,1);
            return w;
        }
        if( std::isspace(line[0]) ){
            return w;
        }
        w += line[0];
        line.erase(0,1);
    }
}

static
std::string getSpaces(std::string& line)
{
    std::string w;
    while(true){
        if( line.length() == 0 )
            return w;
        if( !std::isspace(line[0])){
            return w;
        }
        w += line[0];
        line.erase(0,1);
    }
}


void printWrapHelper(PrintStyle pstyle, const std::string& prefix, const std::string& st)
{

    std::size_t maxlen = MAX_WIDTH - prefix.length();


    auto lines = split(st,'\n');
    std::vector<std::string> L;
    for(std::string line : lines ){
        L.push_back("");
        while(line.length() > 0 ){
            auto w = nextWord(line);
            if( L.back().length() + w.length() > maxlen ){
                L.push_back("");
            }
            L.back() += w;
            auto sp = getSpaces(line);

            unsigned numNewlines=0;
            for(unsigned ii=0;ii<sp.length();++ii){
                if( sp[ii] == '\n' )
                    ++numNewlines;
            }

            if( numNewlines > 0 ){
                //ignore spaces before final \n
                for(unsigned ii=0;ii<numNewlines;++ii){
                    L.push_back("");
                }
                //keep spaces after final \n
                sp = sp.substr( sp.rfind("\n")+1 );
            }
            L.back() += sp;
        }
    }

    std::string alltext="";
    for(unsigned ii=0;ii<L.size();++ii){
        if( ii == 0 ){
            alltext += prefix;
        } else {
            for(unsigned j=0;j<prefix.length();++j){
                alltext += ' ';
            }
        }
        alltext += trim(L[ii]);
        alltext += "\n";
    }

    outputConsoleString(pstyle,alltext);
}


void outputConsoleString(PrintStyle pstyle, const std::string& s)
{
    if( !_useColors ){
        std::cout << s;
        return;
    }

    #if defined WINDOWS_PLATFORM
        static HANDLE stdoutHandle;
        static bool initialized = false;

        if (!initialized) {
            initialized = true;
            stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        std::cout.flush();
        WORD attr=0;
        switch (pstyle) {
            case PrintStyle::RED:       attr = FOREGROUND_RED ; break;
            case PrintStyle::PINK:      attr = FOREGROUND_RED | FOREGROUND_INTENSITY ; break;
            case PrintStyle::RED_BOLD:  attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case PrintStyle::YELLOW:    attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY ; break;
            case PrintStyle::OLIVE:     attr = FOREGROUND_RED | FOREGROUND_GREEN ; break;
            case PrintStyle::GREEN:     attr = FOREGROUND_GREEN ; break;
            case PrintStyle::LIME:      attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY ; break;
            case PrintStyle::CYAN:      attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY ; break;
            case PrintStyle::BLUE:      attr = FOREGROUND_BLUE; break;
            case PrintStyle::GREY:      attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE ; break;
            case PrintStyle::DARKGREY:  attr = FOREGROUND_INTENSITY; break;
            case PrintStyle::PURPLE:    attr = FOREGROUND_RED | FOREGROUND_BLUE; break;
            case PrintStyle::PERIWINKLE:attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case PrintStyle::MAGENTA:   attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY ; break;
            case PrintStyle::LIGHTBLUE: attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case PrintStyle::WHITE:     attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            case PrintStyle::NONE:      attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE ; break;
        }
        if( pstyle != PrintStyle::NONE )
            SetConsoleTextAttribute(stdoutHandle, attr);

        WriteConsoleA(stdoutHandle, s.c_str(), (DWORD)s.length(), nullptr, nullptr);

        if (pstyle != PrintStyle::NONE)
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    #elif defined LINUX_PLATFORM
        static const char* _BOLD="\033[1m"         ;
        static const char* _NORMAL="\033[0m"       ;
        //~ static const char* _UNDERLINE="\033[4m"    ;
        //~ static const char* _INVERSE="\033[7m"      ;
        static const char* _RED="\033[31m"         ;
        static const char* _GREEN="\033[32m"       ;
        static const char* _OLIVE="\033[33m"       ;
        static const char* _BLUE="\033[34m"        ;
        static const char* _PURPLE="\033[35m"      ;
        static const char* _CYAN="\033[36m"        ;
        static const char* _GREY="\033[37m"        ;
        static const char* _DARKGREY="\033[90m"    ;
        static const char* _PINK="\033[91m"        ;
        static const char* _LIME="\033[92m"        ;
        static const char* _YELLOW="\033[93m"      ;
        static const char* _PERIWINKLE="\033[94m"  ;
        static const char* _MAGENTA="\033[95m"     ;
        static const char* _LIGHTBLUE="\033[96m"   ;
        static const char* _WHITE="\033[97m"       ;

        switch(pstyle){
            case PrintStyle::RED:       std::cout << _RED           << s << _NORMAL ; break;
            case PrintStyle::PINK:      std::cout << _PINK          << s << _NORMAL ; break;
            case PrintStyle::RED_BOLD:  std::cout << _BOLD << _RED  << s << _NORMAL ; break;
            case PrintStyle::YELLOW:    std::cout << _YELLOW        << s << _NORMAL ; break;
            case PrintStyle::OLIVE:     std::cout << _OLIVE         << s << _NORMAL ; break;
            case PrintStyle::GREEN:     std::cout << _GREEN         << s << _NORMAL ; break;
            case PrintStyle::LIME:      std::cout << _LIME         << s << _NORMAL ; break;
            case PrintStyle::CYAN:      std::cout << _CYAN          << s << _NORMAL ; break;
            case PrintStyle::BLUE:      std::cout << _BLUE          << s << _NORMAL ; break;
            case PrintStyle::GREY:      std::cout << _GREY          << s << _NORMAL ; break;
            case PrintStyle::DARKGREY:  std::cout << _DARKGREY      << s << _NORMAL ; break;
            case PrintStyle::PURPLE:    std::cout << _PURPLE        << s << _NORMAL ; break;
            case PrintStyle::PERIWINKLE:std::cout << _PERIWINKLE    << s << _NORMAL ; break;
            case PrintStyle::MAGENTA:   std::cout << _MAGENTA       << s << _NORMAL ; break;
            case PrintStyle::LIGHTBLUE: std::cout << _LIGHTBLUE     << s << _NORMAL ; break;
            case PrintStyle::WHITE:     std::cout << _WHITE         << s << _NORMAL ; break;
            case PrintStyle::NONE:      std::cout <<                   s            ; break;
        }
        std::cout.flush();
    #else
        #error No platform defined
    #endif
}

void consoleColorTest()
{
    #if defined WINDOWS_PLATFORM
        static HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    for (int m = 0; m < 2; ++m) {
                        WORD attr = 0;
                        if (i) attr |= FOREGROUND_RED;
                        if (j) attr |= FOREGROUND_GREEN;
                        if (k) attr |= FOREGROUND_BLUE;
                        if (m) attr |= FOREGROUND_INTENSITY;
                        SetConsoleTextAttribute(stdoutHandle, attr);
                        std::ostringstream oss;
                        oss << "R=" << i << " G=" << j << " B=" << k <<  " I=" << m << ": Pack my box with five dozen liquor jugs\n";
                        std::string s = oss.str();
                        WriteConsoleA(stdoutHandle, s.c_str(), (DWORD)s.length(), nullptr, nullptr);
                        SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    }
                }
            }
        }
        std::cout << "\n\n";
    #endif
    for(int i=0;i<int(PrintStyle::WHITE);i++){
        outputConsoleString( PrintStyle(i), "Color number "+ std::to_string(i)+": Pack my box with five dozen liquor jugs.\n");
    }
};
