// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <string>
#ifdef _WIN32
#	include <windows.h>
#endif //_WIN32

//- Common Code For All Addons needed just to ease inclusion as separate files in user code ----------------------
#include <imgui.h>
#undef IMGUI_DEFINE_PLACEMENT_NEW
#define IMGUI_DEFINE_PLACEMENT_NEW
/*#undef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS*/
#include <imgui_internal.h>
//-----------------------------------------------------------------------------------------------------------------


#include "imguifilesystem.h"


#ifdef DIRENT_USES_UTF8_CHARS
#	warning DIRENT_USES_UTF8_CHARS is deprecated and has become the default. (IMGUIFILESYSTEM_USE_ASCII_SHORT_PATHS_ON_WINDOWS can be used to disable it).
#endif //IMGUIBINDINGS_CLEAR_INPUT_DATA_SOON

#ifdef _WIN32
#include <shlobj.h> // Known Directory locations
#   ifndef CSIDL_MYPICTURES
#       define CSIDL_MYPICTURES 0x0027
#   endif //CSIDL_MYPICTURES
#   ifndef CSIDL_MYMUSIC
#       define CSIDL_MYMUSIC 0x000d
#   endif //CSIDL_MYMUSIC
#   ifndef CSIDL_MYVIDEO
#       define CSIDL_MYVIDEO 0x000e
#   endif //CSIDL_MYVIDEO
#else // _WIN32
#   include <unistd.h>     // getpwuid
#   include <pwd.h>        // getenv ?
#endif //#ifdef _WIN32

#ifdef IMGUIFS_NO_EXTRA_METHODS
// We copy the code for FILENAME_MAX and PATH_MAX
#   include <stdint.h>             // this is included by imgui.cpp, and the following headers might redefine incorrectly some types otherwise.
#   include <stdio.h>              // just for FILENAME_MAX
#   include <limits.h>             // just for PATH_MAX
#   if (defined(__linux__) && !defined(PATH_MAX))
#       include <linux/limits.h>
#   endif //(defined(__linux__) && !defined(PATH_MAX))
#   ifndef PATH_MAX
#       define PATH_MAX 1024    // Or 4096 ?
#   endif //IMGUIFS_NO_EXTRA_METHODS
#   ifndef FILENAME_MAX
#       define FILENAME_MAX PATH_MAX
#   endif //FILENAME_MAX
#   ifdef _WIN32
#   include <windef.h> // On Windows we have MAX_PATH too
#   endif //_WIN32
#   if (defined(MAX_PATH) && MAX_PATH>PATH_MAX)
#       define DIRENT_MAX_PATH MAX_PATH
#   else // (defined(MAX_PATH) && MAX_PATH>PATH_MAX)
#       define DIRENT_MAX_PATH PATH_MAX
#   endif // (defined(MAX_PATH) && MAX_PATH>PATH_MAX)

namespace ImGuiFs {

#   if (!defined(IMGUIFS_MEMORY_USES_CHARS_AS_BYTES) || !defined(IMGUIFILESYSTEM_USE_ASCII_SHORT_PATHS_ON_WINDOWS))
const int MAX_FILENAME_BYTES = FILENAME_MAX*4;  // Worst case: 4 bytes per char, but huge waste of memory [we SHOULD have used imguistring.h!]
const int MAX_PATH_BYTES = DIRENT_MAX_PATH*4;
#else //IMGUIFS_MEMORY_USES_CHARS_AS_BYTES
const int MAX_FILENAME_BYTES = FILENAME_MAX+1;
const int MAX_PATH_BYTES = DIRENT_MAX_PATH+1;
#endif //IMGUIFS_MEMORY_USES_CHARS_AS_BYTES
// A bit dangerous typedefs:
typedef char FilenameString[MAX_FILENAME_BYTES];
typedef char PathString[MAX_PATH_BYTES];
// Handy typedefs:
typedef ImVector<FilenameString>	FilenameStringVector;
typedef ImVector<PathString>		PathStringVector;

enum Sorting {
    SORT_ORDER_ALPHABETIC=0,
    SORT_ORDER_ALPHABETIC_INVERSE=1,
    SORT_ORDER_LAST_MODIFICATION=2,
    SORT_ORDER_LAST_MODIFICATION_INVERSE=3,
    SORT_ORDER_SIZE=4,
    SORT_ORDER_SIZE_INVERSE=5,
    SORT_ORDER_TYPE=6,
    SORT_ORDER_TYPE_INVERSE=7,
    SORT_ORDER_COUNT
};

} // namespace ImGuiFs
#endif //IMGUIFS_NO_EXTRA_METHODS



#ifdef IMGUIFILESYSTEM_USE_ASCII_SHORT_PATHS_ON_WINDOWS
#undef DIRENT_USE_ASCII_SHORT_PATHS_ON_WINDOWS
#define DIRENT_USE_ASCII_SHORT_PATHS_ON_WINDOWS
#endif //IMGUIFILESYSTEM_USE_ASCII_SHORT_PATHS_ON_WINDOWS
#include "dirent_portable.h"
#include <sys/stat.h>
#include <ctype.h>  // tolower,...
#include <string.h> // strcmp
#ifndef _WIN32
#include <stdlib.h> // realpath getenv
#endif //_WIN32


#if (defined(_MSC_VER) && !defined(strcasecmp))
#   define strcasecmp _stricmp
/*
    // Never tested (I've just been told that cl.exe does not have strcasecmp: please search the web for other possible alternative implementations)
    inline static int strcasecmp( const char *s1, const char *s2 )  {
        return _stricmp(s1,s2);
        //return lstrcmpiA(s1,s2);  // Not sure this is better
    }
*/
#   endif //(defined(_MSC_VER) && !defined(strcasecmp))


namespace ImGuiFs {

// Definitions of some helper classes (String,Path,SortingHelper,Directory). Better not expose them in the header file----------
/*
// MAIN ISSUE: All these string handling methods work with ASCII strings,
// but the ones returned by dirent are multibyte OS dependant strings.
// That means that there are some limitations:

// LIMITATIONS:
// -> paths with '/','\\','.' bytes (and possibly a few others) inside multibyte codepoints are not supported       (*)
// -> file extensions composed by characters with more than one byte are not supported                              (**)

//(*) That's because when I call something like: mystring.find_last_of('/') or Path::Split('/')
// the result might not be correct if there's some multibyte codepoint that includes that byte(s) (bytes include '/','\\','.').

// (**) String::ToLower() deeply breaks any multibyte char.
// They're currently used only in:
// Path::GetExtension(...)
// Directory::GetFiles(const string& path,const string& wantedExtensions,const string& unwantedExtensions)
// That's because file extensions must be returned lowercase, so that e.g. ".PNG" and ".png" can be string matched (even on case sensitive file systems).
*/
class String    {
protected:
    String() {}
public:
    inline static void PushBack(FilenameStringVector& rv,const char* s)    {	
        if (rv.Size == rv.Capacity) rv.reserve(rv._grow_capacity(rv.Size+1));	// optional optimization from ImVector<>::push_back()
        const size_t sz = rv.size();
        rv.resize(sz+1);
        strcpy(&rv[sz][0], s ? s : "\0");
    }
#   if (FILENAME_MAX!=DIRENT_MAX_PATH)    // Will this work ? (I don't want to use templates)
    inline static void PushBack(PathStringVector& rv,const char* s)    {
        const size_t sz = rv.size();
        rv.resize(sz+1);
        strcpy(&rv[sz][0], s ? s : "\0");
    }
#   endif //#if (FILENAME_MAX!=DIRENT_MAX_PATH)
    inline static void Substr(const char* text,char* rv,int start,int count=-1)    {
        if (!text) count=0;
        if (count<0) count = (int) strlen(text) - start;
        if (count>0) strncpy(rv,&text[start],count);
        rv[count]='\0';
    }
    inline static int Find(const char* text,const char toFind,int beg=0)   {
        if (!text) return -1;
        for (size_t i=beg,len=strlen(text);i<len;i++)    {
            if (text[i]==toFind) return i;
        }
        return -1;
    }
    inline static int FindLastOf(const char* text,const char toFind)   {
        if (!text) return -1;
        for (int i=(int)strlen(text)-1;i>=0;i--)    {
            if (text[i]==toFind) return i;
        }
        return -1;
    }
    inline static void ToLower(const char* text,char* rv)   {
        if (!text) {
            rv[0]='\0';
            return;
        }
        const size_t len = strlen(text);
        for (size_t i=0;i<len;i++)    {
            rv[i]=tolower(text[i]);
        }
        rv[len]='\0';
    }
    inline static void ToLowerInPlace(char* text)  {
        if (!text) return;
        for (size_t i=0,len = strlen(text);i<len;i++)    {
            char& c = text[i];
            c=tolower(c);
        }
    }
    inline static void Split(const char* text,FilenameStringVector& rv,const char c=' ')  {
        rv.clear();
        if (!text) return;
        const int len = (int)strlen(text);
        if (len==0) return;
        int beg = 0;char tmp[MAX_FILENAME_BYTES];
        for (int i=0;i<len;i++)    {
            const char ch = text[i];
            if (ch==c) {
                Substr(text,tmp,beg,i-beg);
                PushBack(rv,tmp);
                beg = i+1;
            }
        }
        if (beg<len) {
            Substr(text,tmp,beg,len-beg);
            PushBack(rv,tmp);
        }
    }
    inline static void Replace(const char*  baseText,const char textToReplace,const char replacement,char* rv)    {
        strcpy(rv,baseText);
        ReplaceInPlace(rv,textToReplace,replacement);
    }
    inline static void ReplaceInPlace(char* text,const char toReplace,const char replacement)  {
        if (!text) return;
        for (size_t i=0,len = strlen(text);i<len;i++)    {
            char& c = text[i];
            if (c==toReplace) c=replacement;
        }
    }

#   ifdef _WIN32
    // Convert a wide Unicode string to an UTF8 string
    inline static void wide_to_utf8(const wchar_t* wstr,char* rv)    {
        rv[0]='\0';
        if (!wstr) return;
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
        WideCharToMultiByte                  (CP_UTF8, 0, wstr, -1, &rv[0], size_needed, NULL, NULL);
        //rv[size_needed]='\0';              // If the parameter after wstr is -1, the function processes the entire input string, including the terminating null character. Therefore, the resulting character string has a terminating null character, and the length returned by the function includes this character.
        return ;
    }
    // Convert an UTF8 string to a wide Unicode String
    inline static void utf8_to_wide(const char* str,wchar_t* rv)    {
        rv[0]=L'\0';
        if (!str) return;
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
        MultiByteToWideChar                  (CP_UTF8, 0, str, -1, &rv[0], size_needed);
        //rv[size_needed]=L'\0';            // // If the parameter after str is -1, the function processes the entire input string, including the terminating null character. Therefore, the resulting character string has a terminating null character, and the length returned by the function includes this character.
        return;
    }
#   endif // _WIN32
};
class Path	{
protected:
    Path() {}
public:

    static void GetAbsolutePath(const char *path, char *rv)  {
        rv[0]='\0';
#   ifndef _WIN32
        if (!path || strlen(path)==0) {
            realpath("./", rv);
        }
        else {
            realpath(path, rv);
        }
        //printf("GetAbsolutePath(\"%s\",\"%s\");\n",path,rv);fflush(stdout);
#   else //_WIN32
        //fprintf(stderr,"GetAbsolutePath(\"%s\"); (len:%d)\n",path,(int) strlen(path)); // TO remove!
        static const int bufferSize = DIRENT_MAX_PATH+1;   // 4097 is good (PATH_MAX should be in <limits.h>, or something like that)
        static wchar_t buffer[bufferSize];
        static wchar_t wpath[bufferSize];
        String::utf8_to_wide((path && strlen(path)>0) ? path : "./",wpath);
        ::GetFullPathNameW(&wpath[0],bufferSize,&buffer[0],NULL);
        String::wide_to_utf8(&buffer[0],rv);

        String::ReplaceInPlace(rv,'\\','/');
        size_t len;
        while ( (len=strlen(rv))>0 && rv[len-1]=='/')  rv[len-1]='\0';
        //fprintf(stderr,"AbsolutePath = \"%s\" (len:%d)\n",rv,(int) strlen(rv)); // TO remove!
#   endif // _WIN32
    }
    static void GetDirectoryName(const char *filePath, char *rv) {
        rv[0]='\0';if (!filePath) return;
        const int sz = strlen(filePath);
        if (sz==0 || strcmp(filePath,"/")==0 || strcmp(filePath,"\\")==0) {
            strcpy(rv,filePath);
            return;
        }
        const char c = filePath[sz-1];
        if (c == '/' || c=='\\') {
            char tmp[MAX_PATH_BYTES];
            String::Substr(filePath,tmp,0,sz-1);
            GetDirectoryName(tmp,rv);
            return;
        }

        if (c==':') {
            strcpy(rv,filePath);
            return;
        }
        int beg=String::FindLastOf(filePath,'\\');
        int beg2=String::FindLastOf(filePath,'/');
        beg=(beg>beg2?beg:beg2);
        if (beg==0) {
            String::Substr(filePath,rv,0,1);
            return;
        }
        if (beg!=-1) {
            String::Substr(filePath,rv,0,beg);
            return;
        }
        rv[0]='\0';
        return;
    }
    static void GetFileName(const char *filePath, char *rv)  {
        int beg=String::FindLastOf(filePath,'\\');
        int beg2=String::FindLastOf(filePath,'/');
        beg=(beg>beg2?beg:beg2);
        if (beg!=-1) {
            String::Substr(filePath,rv,beg+1);
            return;
        }
        strcpy(rv,filePath);
        return;
    }
    static void GetFileNameWithoutExtension(const char *filePath, char *rv)  {
        int beg=String::FindLastOf(filePath,'\\');
        int beg2=String::FindLastOf(filePath,'/');
        int beg3=String::FindLastOf(filePath,'.');
        beg=(beg>beg2?beg:beg2);
        if (beg!=-1) {
            if (beg3<beg) {
                String::Substr(filePath,rv,beg+1,(beg3<beg+1)?-1:(beg3-beg+1));
                return;
            }
        }
        if (beg3!=-1) {
            String::Substr(filePath,rv,0,beg3);
            return;
        }
        strcpy(rv,filePath);
        return;
    }
    static void GetExtension(const char* filePath,char *rv) {
        int beg=String::FindLastOf(filePath,'.');
        int beg2=String::FindLastOf(filePath,'/');
        int beg3=String::FindLastOf(filePath,'\\');
        if (beg2!=-1)	{
            if (beg3!=-1) beg2 = beg3;
            else beg2 = beg2 > beg3 ? beg2 : beg3;
        }
        else if (beg3!=-1) beg2 = beg3;
        else {
            if (beg!=-1)  {
                String::Substr(filePath,rv,beg);
                String::ToLowerInPlace(rv);
                return;
            }
        }
        if (beg>beg2)  {
            if (beg!=-1)  {
                String::Substr(filePath,rv,beg);
                String::ToLowerInPlace(rv);
                return;
            }
        }
        rv[0]='\0';
        return;
    }
    static void ChangeExtension(const char* filePath,const char* newExtension,char *rv) {
        strcpy(rv,filePath);
        int beg=String::FindLastOf(rv,'.');
        int beg2=String::FindLastOf(rv,'/');
        int beg3=String::FindLastOf(rv,'\\');
        if (beg2!=-1)	{
            if (beg3!=-1) beg2 = beg3;
            else beg2 = beg2 > beg3 ? beg2 : beg3;
        }
        else if (beg3!=-1) beg2 = beg3;
        else {
            if (beg!=-1)  {
                strcpy(&rv[beg],newExtension);
                return;
            }
        }
        if (beg>beg2)  {
            if (beg!=-1)  {
                strcpy(&rv[beg],newExtension);
                return;
            }
        }
        return;
    }
    static inline bool HasZipExtension(const char* filePath) {
        static const int dot = (int) '.';
        const char * p1 = strrchr(filePath, dot );
        if (!p1) return false;
        const int len = strlen(p1);
        if (len!=4) return false;
        static const char lower[]=".zip";static const char upper[]=".ZIP";
        char c;
        for (int i=1;i<4;i++) {
            c = p1[i];
            if (c!=lower[i] && c!=upper[i]) return false;
        }
        return true;
    }
    static void Combine(const char* directory,const char* fileName,char* rv,bool appendMode=true)  {
        if (!appendMode) rv[0]='\0';
        const size_t size= directory ? strlen(directory) : 0;
        if (size==0) {
            strcat(rv,fileName);
            return;
        }
        strcat(rv,directory);
        if (directory[size-1]!='\\' && directory[size-1]!='/') {
            strcat(rv,"/");
            strcat(rv,fileName);
        }
        else strcat(rv,fileName);
        return;
    }
    static void Append(const char* directory,char* rv)  {
        if (!directory || strlen(directory)==0) return;
        size_t size = strlen(rv);
        if (size>0 && (rv[size-1]!='\\' && rv[size-1]!='/')) {strcat(rv,"/");++size;}
        strcat(rv,directory);
        size = strlen(rv);
        while (size>0 && (rv[size-1]=='\\' || rv[size-1]=='/')) {rv[size-1]='\0';--size;}
        if (size==0 || rv[size-1]==':') strcat(rv,"/");
    }
    static void Split(const char* path,FilenameStringVector& rv,bool leaveIntermediateTrailingSlashes=true) {
        rv.clear();
        static char tex[MAX_PATH_BYTES];
        String::Replace(path,'\\','/',tex);
        size_t len = strlen(tex);
        static char tex2[MAX_PATH_BYTES];
#   ifdef _WIN32
        while ((len = strlen(tex))>0 && tex[len-1]=='/') {
            strncpy(tex2,tex,len+1);
            String::Substr(tex2,tex,0,len-1);
        }
#   endif //_WIN32
        if (len==0) return;
        int beg=-1;
        while ( (beg = String::Find(tex,'/'))!=-1) {
            static char tmp[MAX_FILENAME_BYTES];
            String::Substr(tex,tmp,0,leaveIntermediateTrailingSlashes ? beg+1 : beg);
            String::PushBack(rv,tmp);
            strcpy(tex2,tex);
            String::Substr(tex2,tex,beg+1);
        }
        String::PushBack(rv,tex);
        if (rv.size()>0 && strlen(rv[0])==0) strcpy((char*)&rv[0][0],"/\0");
#   ifdef _WIN32
        if (rv.size()==1 && strlen(rv[0])>0 && rv[0][strlen(rv[0])-1]==':') strcat((char*)&rv[0][0],"/");
#   endif //_WIN32
        return;
    }
    /*
    inline static bool Exists(const char* path) 	{
        struct stat statbuf;
        return (stat(path, &statbuf) != -1);
    }
    */
};
/*
class File {
public:
    inline static bool Exists(const char* path) {
        struct stat statbuf;
        return (stat(path, &statbuf) != -1 && (S_ISREG(statbuf.st_mode)));// || (acceptLinks ? S_ISLNK(statbuf.st_mode) : 1));
    }
protected:
    File() {}
};
*/
class SortingHelper {
public:
    typedef int (*SorterSignature)(const struct dirent **e1,const struct dirent **e2);
    inline static const SorterSignature& SetSorter(Sorting sorting) {
        const int isort =(int) sorting;
        if (isort>=0 && isort<(int)SORT_ORDER_COUNT) return (sorter = Sorters[isort]);
        return (sorter = Sorters[0]);
    }
protected:
    SortingHelper() {}
    const static SorterSignature Sorters[];
    static struct stat stat1;
    static struct stat stat2;
    static SorterSignature sorter;
    // Possible problem: sorting is in ASCII with these methods
    static int Alphasort(const struct dirent **e1,const struct dirent **e2)    {
        return strcasecmp((*e1)->d_name,(*e2)->d_name);
    }
    static int Alphasortinverse (const struct dirent **e1,const struct dirent **e2)    {
        return -strcasecmp((*e1)->d_name,(*e2)->d_name);
    }
    // Please note that calling stat(...) every time inside sorters is a suicide! And that I'm doing right that! (but I guess and hope that on many systems the calls get cached somewhere: otherwise it would take ages to sort)
    static int Lastmodsort (const struct dirent **e1,const struct dirent **e2)    {
        if (stat((*e1)->d_name,&stat1)==-1) return -1;
        if (stat((*e2)->d_name,&stat2)==-1) return  1;
        return (stat1.st_mtime < stat2.st_mtime ? -1 : stat1.st_mtime > stat2.st_mtime ? 1 : 0);
    }
    static int Lastmodsortinverse(const struct dirent **e1,const struct dirent **e2)    {
        if (stat((*e1)->d_name,&stat1)==-1) return  1;
        if (stat((*e2)->d_name,&stat2)==-1) return -1;
        return (stat2.st_mtime < stat1.st_mtime ? -1 : stat2.st_mtime > stat1.st_mtime ? 1 : 0);
    }
    static int Sizesort (const struct dirent **e1,const struct dirent **e2)    {
        if (stat((*e1)->d_name,&stat1)==-1) return -1;
        if (stat((*e2)->d_name,&stat2)==-1) return  1;
        return (stat1.st_size < stat2.st_size ? -1 : stat1.st_size > stat2.st_size ? 1 : 0);
    }
    static int Sizesortinverse(const struct dirent **e1,const struct dirent **e2)    {
        if (stat((*e1)->d_name,&stat1)==-1) return  1;
        if (stat((*e2)->d_name,&stat2)==-1) return -1;
        return (stat2.st_size < stat1.st_size ? -1 : stat2.st_size > stat1.st_size ? 1 : 0);
    }
    // Please note that calculating the file extension every time inside sorters is a suicide (well, much less than before...)!
    static int Typesort(const struct dirent **e1,const struct dirent **e2)    {
        static const int dot = (int) '.';
        const char * p1 = strrchr((const char*) (*e1)->d_name, dot );
        const char * p2 = strrchr((const char*) (*e2)->d_name, dot );
        if (!p1) return (!p2?0:-1);
        else if (!p2)   return 1;
        return strcasecmp(p1,p2);
    }
    static int Typesortinverse (const struct dirent **e1,const struct dirent **e2)    {
        static const int dot = (int) '.';
        const char * p1 = strrchr((const char*) (*e1)->d_name, dot );
        const char * p2 = strrchr((const char*) (*e2)->d_name, dot );
        if (!p1) return (!p2?0:1);
        else if (!p2)   return -1;
        return -strcasecmp(p1,p2);
    }

};
const SortingHelper::SorterSignature SortingHelper::Sorters[] = {&SortingHelper::Alphasort,&SortingHelper::Alphasortinverse,&SortingHelper::Lastmodsort,&SortingHelper::Lastmodsortinverse,&SortingHelper::Sizesort,&SortingHelper::Sizesortinverse,&SortingHelper::Typesort,&SortingHelper::Typesortinverse};
SortingHelper::SorterSignature SortingHelper::sorter;
struct stat SortingHelper::stat1;
struct stat SortingHelper::stat2;
class Directory {
public:
    static void GetDirectories(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL,Sorting sorting= SORT_ORDER_ALPHABETIC)   {
        result.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();
        size_t sz;
        struct dirent **eps = NULL;

        sz = strlen(directoryName);
        static char directoryName2[MAX_PATH_BYTES];
        strcpy(directoryName2,directoryName);
#       ifdef _WIN32
        if (sz>0 && directoryName[sz-1]==':') {directoryName2[sz]='\\';directoryName2[sz+1]='\0';}
#       endif //_WIN32
        const int n = scandir (directoryName2, &eps, DirentGetDirectories, SortingHelper::SetSorter(sorting));

        std::string directoryNameWithoutSlash = directoryName;
        if (sz>0 && directoryName[sz-1] == '/') {
          directoryNameWithoutSlash.substr(0, sz-1);
        }
        if (n >= 0) {
        result.reserve((size_t)n);
        if (pOptionalNamesOut) pOptionalNamesOut->reserve((size_t)n);
            for (int cnt = 0; cnt < n; ++cnt)    {
                const char* pName = &eps[cnt]->d_name[0];
                sz = strlen(pName);
		if (sz>0) {
            if (strcmp(pName,".")!=0 && strcmp(pName,"..")!=0 && pName[0]!='.' && pName[sz-1]!='~'
#               ifdef __EMSCRIPTEN__
                && (!(
                    strcmp(pName,"fd")==0 && directoryNameWithoutSlash == "/proc/self")==0
                ))
#               endif //__EMSCRIPTEN__
            )    {
			String::PushBack(result, (directoryNameWithoutSlash + "/" + pName).c_str());
			if (pOptionalNamesOut) String::PushBack(*pOptionalNamesOut,pName);
		    }
		}
		free(eps[cnt]);
            }
        }
        if (eps) {free(eps);eps=NULL;}
    }
    static void GetFiles(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut=NULL, Sorting sorting= SORT_ORDER_ALPHABETIC)    {
        result.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();
        static char tempString[MAX_PATH_BYTES];size_t sz;
        struct dirent **eps = NULL;

        sz = strlen(directoryName);
        static char directoryName2[MAX_PATH_BYTES];
        strcpy(directoryName2,directoryName);
#       ifdef _WIN32
        if (sz>0 && directoryName[sz-1]==':') {directoryName2[sz]='\\';directoryName2[sz+1]='\0';}
#       endif //_WIN32
        const int n = scandir (directoryName2, &eps, DirentGetFiles, SortingHelper::SetSorter(sorting));

        static char directoryNameWithoutSlash[MAX_PATH_BYTES];
        if (sz>0 && directoryName[sz-1] == '/') String::Substr(directoryName,directoryNameWithoutSlash,0,sz-1);
        else strncpy(directoryNameWithoutSlash,directoryName,MAX_PATH_BYTES-1);

        if (n >= 0) {
        result.reserve((size_t)n);
        if (pOptionalNamesOut) pOptionalNamesOut->reserve((size_t)n);
            for (int cnt = 0; cnt < n; ++cnt)    {
                const char* pName = &eps[cnt]->d_name[0];
		sz = strlen(pName);
		if (sz>0) {
		    if (pName[0]!='.' && pName[sz-1]!='~')    {
			strncpy(tempString,directoryNameWithoutSlash,MAX_PATH_BYTES-1);
			strncat(tempString,"/",MAX_PATH_BYTES-1-strlen(tempString));
			strncat(tempString,pName,MAX_PATH_BYTES-1-strlen(tempString));
			String::PushBack(result,tempString);
			if (pOptionalNamesOut) String::PushBack(*pOptionalNamesOut,pName);
		    }
		}
		free(eps[cnt]);
            }
        }
        if (eps) {free(eps);eps=NULL;}
    }

    // e.g. ".txt;.jpg;.png". To use unwantedExtensions, set wantedExtensions="".
    static void GetFiles(const char* path,PathStringVector& files,const char* wantedExtensions,const char* unwantedExtensions=NULL,FilenameStringVector* pOptionalNamesOut=NULL,Sorting sorting= SORT_ORDER_ALPHABETIC)    {
    PathStringVector filesIn;
	FilenameStringVector namesIn;
        GetFiles(path,filesIn,&namesIn,sorting);
        if ((wantedExtensions==0 || strlen(wantedExtensions)==0) && (unwantedExtensions==0 || strlen(unwantedExtensions)==0)) {files = filesIn;return;}
        files.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();

        char wext[MAX_PATH_BYTES];String::ToLower(wantedExtensions,wext);
        char woext[MAX_PATH_BYTES];String::ToLower(unwantedExtensions,woext);

        char ext[MAX_PATH_BYTES];
        if (wantedExtensions && strlen(wantedExtensions)>0)	{
        files.reserve(filesIn.size());
        if (pOptionalNamesOut) pOptionalNamesOut->reserve(namesIn.size());
	    FilenameStringVector wExts;String::Split(wext,wExts,';');
            const size_t wExtsSize = wExts.size();
            if (wExtsSize>0)	{
                for (size_t i = 0,sz = filesIn.size();i<sz;i++)	{
                    Path::GetExtension(filesIn[i],ext);
                    for (size_t e=0;e<wExtsSize;e++)	{
                        if (strcmp(ext,wExts[e])==0) {
                            String::PushBack(files,filesIn[i]);
                            if (pOptionalNamesOut) String::PushBack(*pOptionalNamesOut,namesIn[i]);
                        }
                    }
                }
            }
            else return;
        }
        else if (unwantedExtensions && strlen(unwantedExtensions)>0) {
	    //files.reserve(filesIn.size());if (pOptionalNamesOut) pOptionalNamesOut->reserve(namesIn.size());
	    FilenameStringVector woExts;String::Split(woext,woExts,';');
            const size_t woExtsSize = woExts.size();
            if (woExts.size()==0) {files = filesIn;return;}
            bool match;
            for (size_t i = 0,sz = filesIn.size();i<sz;i++)	{
                Path::GetExtension(filesIn[i],ext);
                match = false;
                for (size_t e=0;e<woExtsSize;e++)	{
                    if (strcmp(ext,woExts[e])==0) {
                        match = true;
                        break;
                    }
                }
                if (!match) {
                    String::PushBack(files,filesIn[i]);
                    if (pOptionalNamesOut) String::PushBack(*pOptionalNamesOut,namesIn[i]);
                }
            }
        }
        else {
            files = filesIn;
            if (pOptionalNamesOut) *pOptionalNamesOut = namesIn;
        }
    }

    inline static void Create(const char* directoryName)   {
#       ifndef _WIN32
        const mode_t mode = S_IFDIR | S_IREAD | S_IWRITE | S_IRWXU | S_IRWXG | S_IRWXO;
        mkdir(directoryName,mode);
#       else //_WIN32
        static wchar_t name[DIRENT_MAX_PATH+1];
        String::utf8_to_wide(directoryName,name);
        ::CreateDirectoryW(name,NULL);
#       endif //_WIN32
    }

    inline static bool Exists(const char* path) {
        struct stat statbuf;
        return (stat(path, &statbuf) != -1 && S_ISDIR(statbuf.st_mode));
    }
    inline static const PathStringVector &GetUserKnownDirectories(const FilenameStringVector **pOptionalUserKnownDirectoryDisplayNamesOut,const int** pOptionalNumberKnownUserDirectoriesExceptDrives=NULL,bool forceUpdate=false)  {
        static bool init = false;
	static PathStringVector rv;
	static FilenameStringVector dn;
	static PathStringVector mediaFolders;
        static int numberKnownUserDirectoriesExceptDrives = 0;
        if (pOptionalUserKnownDirectoryDisplayNamesOut) *pOptionalUserKnownDirectoryDisplayNamesOut = &dn;
        if (pOptionalNumberKnownUserDirectoriesExceptDrives) *pOptionalNumberKnownUserDirectoriesExceptDrives = &numberKnownUserDirectoriesExceptDrives;
        if (init && !forceUpdate) return rv;
        init = true;
        rv.clear();dn.clear();
#       ifdef _WIN32
        static const int csid[] = {
            CSIDL_DESKTOP,
            CSIDL_PERSONAL, //(Documents)
            CSIDL_FAVORITES,
            CSIDL_MYMUSIC,
            CSIDL_MYPICTURES,
            CSIDL_RECENT,
            CSIDL_MYVIDEO
        };
        static const char* name[] = {
            "Desktop",
            "Documents",
            "Favorites",
            "Music",
            "Pictures",
            "Recent",
            "Video"
        };
        static const int csidSize = sizeof(csid)/sizeof(csid[0]);
        static const int nameSize = sizeof(name)/sizeof(name[0]);
        IM_ASSERT(csidSize==nameSize);
        if (csidSize!=nameSize) fprintf(stderr,"ERROR in file: imguifilesystem.cpp. Directory::GetUserKnownDirectories(...) csidSize!=nameSize.\n");
        char tmp[MAX_PATH_BYTES] = "C:/";while (tmp[0]<='Z') {if (Directory::Exists(tmp)) String::PushBack(mediaFolders,tmp);tmp[0]=(char)((int)tmp[0]+1);}
        rv.reserve(csidSize+mediaFolders.size());
        dn.reserve(csidSize+mediaFolders.size());
        WCHAR path[MAX_PATH+1];
        for (int i=0;i<csidSize;i++)    {
            if (!GetSpecialFolderPathW(csid[i],&path[0],NULL)) continue;
            static char tmp2[MAX_PATH_BYTES];
            String::wide_to_utf8(&path[0],tmp2);
            String::PushBack(rv,tmp2);
            String::PushBack(dn,name[i]);
        }
        numberKnownUserDirectoriesExceptDrives = (int) rv.size();
        static char mediaFolderName[MAX_PATH_BYTES];
        for (int i=0,msz=mediaFolders.size();i<msz;i++)    {
            const char* mediaFolder = mediaFolders[i];
            String::PushBack(rv,mediaFolder);
            String::Substr(mediaFolder,mediaFolderName,0,strlen(mediaFolder)-1);
            String::PushBack(dn,mediaFolderName);
        }
#       else //_WIN32
        const char* homedir = NULL;
        if ((homedir = getenv("HOME")) == NULL) {
            homedir = getpwuid(getuid())->pw_dir;
        }
        if (homedir==NULL) return rv;
        char homeString[MAX_PATH_BYTES];
        strncpy(homeString,homedir,MAX_PATH_BYTES-1);
        homeString[MAX_PATH_BYTES-1] = '\0';
        char userString[MAX_PATH_BYTES];
        Path::GetFileName(homeString,userString);
        // Known folders ---------------------------------------------
        static const char folder[][MAX_FILENAME_BYTES] = {
            "Desktop",
            "Documents",
            "Downloads",
            "Music",
            "Pictures",
            "Videos"
        };
        static const int folderSize = sizeof(folder)/sizeof(folder[0]);
        rv.reserve(folderSize+1);
        dn.reserve(rv.size());
        String::PushBack(rv,homeString);
        char temp[MAX_PATH_BYTES*2];
        strcpy(temp,"Home");
        String::PushBack(dn,temp);
        for (int i=0;i<folderSize;i++)    {
            Path::Combine(homeString,folder[i],temp,false);
            if (Directory::Exists(temp)) {
                String::PushBack(rv,temp);
                String::PushBack(dn,folder[i]);
            }
        }
        numberKnownUserDirectoriesExceptDrives = (int) rv.size();
        // Additional Drives --------------------------------------------
        static const char* mountLocations[] = {"/media","/mnt","/Volumes","/vol","/data"};
        static const int mountLocationSize = sizeof(mountLocations)/sizeof(mountLocations[0]);
        static const bool ifHomeSubfolerIsFoundInMountLocationsForgetThatRootMountLocation = true;  // That means: if "/media/myusername" exists, don't add any other "/media/..." entries.
        char userMediaString[MAX_PATH_BYTES];bool lastGood = false;
        for (int mntLocIndex=0,sz = 2*mountLocationSize;mntLocIndex<sz;mntLocIndex++) {
            const int index = mntLocIndex/2;
            const char* mntLocString = mountLocations[index];
            const bool useUserSuffix = (mntLocIndex%2)==0;
            if (useUserSuffix) {
                Path::Combine(mntLocString,userString,userMediaString,false);
                strcpy(temp,userMediaString);
            }
            else if (lastGood && ifHomeSubfolerIsFoundInMountLocationsForgetThatRootMountLocation) {lastGood = false;continue;} // see "ifHomeSubfolerIsFoundInMountLocationsForgetThatRootMountLocation" above
            else strcpy(userMediaString,mntLocString);
            lastGood = Directory::Exists(userMediaString);
            if (!lastGood) continue;
            Directory::GetDirectories(userMediaString,mediaFolders);
            if (mediaFolders.size()==0) continue;
            rv.reserve(rv.size()+mediaFolders.size());
            dn.reserve(rv.size());
            for (int i=0,msz=mediaFolders.size();i<msz;i++)    {
                if (strcmp(mediaFolders[i],temp)==0) continue;  // I we have processed "/media/myusername" once cycle before, exclude it from processing "/media" subfolders
                String::PushBack(rv,mediaFolders[i]);
                static char tmp[MAX_FILENAME_BYTES];
                Path::GetFileName(mediaFolders[i],tmp);
                String::PushBack(dn,tmp);
            }
        }
#       endif //_WIN32

        return rv;
    }


protected:
    Directory() {}

    static int DirentGetDirectories(const struct dirent *de) {
        if (de->d_type==DT_DIR) return 1;
        return 0;
    }
    static int DirentGetFiles(const struct dirent *de) {
        if (de->d_type==DT_REG) return 1;
        return 0;
    }

#   ifdef _WIN32
    static bool GetSpecialFolderPathW(int specialFolderCSIDL,WCHAR* pathOutWithSizeMaxPathPlusOne,HWND parent)  {
        //	CSIDL_DESKTOP,CSIDL_BITBUCKET,CSIDL_CONTROLS,CSIDL_DESKTOP,CSIDL_DESKTOPDIRECTORY,
        //	CSIDL_DRIVES,CSIDL_FONTS,CSIDL_NETHOOD,CSIDL_NETWORK,CSIDL_PERSONAL (Documents)
        //	CSIDL_PRINTERS,CSIDL_PROGRAMS,CSIDL_RECENT,CSIDL_SENDTO,CSIDL_STARTMENU,
        //	CSIDL_STARTUP,CSIDL_TEMPLATES

        //	CSIDL_INTERNET_CACHE,CSIDL_COOKIES,CSIDL_HISTORY,CSIDL_COMMON_APPDATA,
        //	CSIDL_WINDOWS,CSIDL_SYSTEM,CSIDL_PROGRAM_FILES,CSIDL_MYPICTURES,...

        WCHAR* temp_path = pathOutWithSizeMaxPathPlusOne;//[MAX_PATH+1];
        temp_path[0]=L'\0';
        LPITEMIDLIST pidl=NULL;
        if (!SUCCEEDED(::SHGetSpecialFolderLocation(parent,specialFolderCSIDL, &pidl)))
        {
            temp_path[0]=L'\0';return false;
        }
        bool ok=SUCCEEDED(::SHGetPathFromIDListW(pidl,&temp_path[0]));

        LPMALLOC mal = NULL;
        if ( ::SHGetMalloc( & mal ) == E_FAIL || !mal )	::free( pidl );
        else
        {
            mal->Free( pidl );
            mal->Release();
        }
        if (!ok)
        {
            temp_path[0]=L'\0';return false;
        }
        return true;
    }
#   endif //_WIN32

};
#ifndef IMGUIFS_NO_EXTRA_METHODS
#ifdef IMGUI_USE_MINIZIP
struct UnZipFileImpl {
    unzFile uf;
    char zipFilePath[MAX_PATH_BYTES];

    struct unz_file_info64_plus {
        unz_file_info64 info;
        char path[MAX_PATH_BYTES];
        char name[MAX_PATH_BYTES];
        void assign(const char* _path,const char* _name,const unz_file_info64& _info) {
            strcpy(path,_path);strcpy(name,_name);
            memcpy(&info,&_info,sizeof(info));
        }
        static void Sort(ImVector<unz_file_info64_plus>& fileInfos,Sorting sorting) {
            if (fileInfos.size()==0) return;
            typedef int (*Sorter)(const void *, const void *);
            Sorter sorter = NULL;
            switch (sorting)    {
            case SORT_ORDER_ALPHABETIC:                 sorter = &unz_file_info64_plus::Alphasort;break;
            case SORT_ORDER_ALPHABETIC_INVERSE:         sorter = &unz_file_info64_plus::Alphasortinverse;break;
            case SORT_ORDER_LAST_MODIFICATION:          sorter = &unz_file_info64_plus::LastModsort;break;
            case SORT_ORDER_LAST_MODIFICATION_INVERSE:  sorter = &unz_file_info64_plus::LastModsortinverse;break;
            case SORT_ORDER_SIZE:                       sorter = &unz_file_info64_plus::Sizesort;break;
            case SORT_ORDER_SIZE_INVERSE:               sorter = &unz_file_info64_plus::Sizesortinverse;break;
            case SORT_ORDER_TYPE:                       sorter = &unz_file_info64_plus::Typesort;break;
            case SORT_ORDER_TYPE_INVERSE:               sorter = &unz_file_info64_plus::Typesortinverse;break;
            default: break;
            }
            if (sorter) qsort(&fileInfos[0],fileInfos.size(),sizeof(unz_file_info64_plus),sorter);
        }
        //void qsort( void *ptr, size_t count, size_t size,int (*comp)(const void *, const void *) );
        inline static int Alphasort(const void* e1,const void* e2) {
            return strcasecmp(static_cast<const unz_file_info64_plus*>(e1)->name,static_cast<const unz_file_info64_plus*>(e2)->name);
        }
        inline static int Alphasortinverse(const void* e1,const void* e2) {
            return -strcasecmp(static_cast<const unz_file_info64_plus*>(e1)->name,static_cast<const unz_file_info64_plus*>(e2)->name);
        }
        inline static int LastModsort(const void* e1,const void* e2) {  // or tmu_date ?
            const uLong& dosDate1 = static_cast<const unz_file_info64_plus*>(e1)->info.dosDate;
            const uLong& dosDate2 = static_cast<const unz_file_info64_plus*>(e2)->info.dosDate;
            return (dosDate1<dosDate2) ? -1 : (dosDate1>dosDate2) ? 1 : 0;
        }
        inline static int LastModsortinverse(const void* e1,const void* e2) {
            const uLong& dosDate1 = static_cast<const unz_file_info64_plus*>(e1)->info.dosDate;
            const uLong& dosDate2 = static_cast<const unz_file_info64_plus*>(e2)->info.dosDate;
            return (dosDate1<dosDate2) ? 1 : (dosDate1>dosDate2) ? -1 : 0;
        }
        inline static int  Sizesort(const void* e1,const void* e2) {
            const ZPOS64_T& size1 = static_cast<const unz_file_info64_plus*>(e1)->info.uncompressed_size;
            const ZPOS64_T& size2 = static_cast<const unz_file_info64_plus*>(e2)->info.uncompressed_size;
            return (size1<size2) ? -1 : (size1>size2) ? 1 : 0;
        }
        inline static int Sizesortinverse(const void* e1,const void* e2) {
            const ZPOS64_T& size1 = static_cast<const unz_file_info64_plus*>(e1)->info.uncompressed_size;
            const ZPOS64_T& size2 = static_cast<const unz_file_info64_plus*>(e2)->info.uncompressed_size;
            return (size1<size2) ? 1 : (size1>size2) ? -1 : 0;
        }
        inline static int Typesort(const void* e1,const void* e2) {
            static const int dot = (int) '.';
            const char * p1 = strrchr((const char*) static_cast<const unz_file_info64_plus*>(e1)->name, dot );
            const char * p2 = strrchr((const char*) static_cast<const unz_file_info64_plus*>(e2)->name, dot );
            if (!p1) return (!p2?0:-1);
            else if (!p2)   return 1;
            return strcasecmp(p1,p2);
        }
        inline static int Typesortinverse(const void* e1,const void* e2) {
            static const int dot = (int) '.';
            const char * p1 = strrchr((const char*) static_cast<const unz_file_info64_plus*>(e1)->name, dot );
            const char * p2 = strrchr((const char*) static_cast<const unz_file_info64_plus*>(e2)->name, dot );
            if (!p1) return (!p2?0:-1);
            else if (!p2)   return -1;
            return -strcasecmp(p1,p2);
        }
    };

    bool getFilesOrDirectories(bool fileMode,const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting,bool prefixResultWithTheFullPathOfTheZipFile) const  {
        result.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();
        if (!uf) return false;
        char dirName[MAX_PATH_BYTES];

        // copy a clean directoryName into dirName
        size_t dirNameLen = 0;
        if (directoryName)  {
            dirNameLen = strlen(directoryName);
            if (dirNameLen>1 && directoryName[0]=='.' && (directoryName[1]=='/' || directoryName[1]=='\\')) {
                strcpy(dirName,&directoryName[2]);
            }
            else strcpy(dirName,directoryName);
        }
        else strcpy(dirName,directoryName ? directoryName : "");
        dirNameLen = strlen(dirName);
        while (dirNameLen>0 && (dirName[dirNameLen-1]=='/' || dirName[dirNameLen-1]=='\\')) {dirName[dirNameLen-1]='\0';--dirNameLen;}

        // get gi
        uLong i;unz_global_info64 gi;int err;
        err = unzGetGlobalInfo64(uf,&gi);
        if (err!=UNZ_OK) {
            fprintf(stderr,"error %d with zipfile in unzGetGlobalInfo \n",err);
            return false;
        }

        ImVector<unz_file_info64_plus> fileInfos;

        // Fill fileInfos
        char tmp[MAX_PATH_BYTES]="";char filename_inzip[MAX_PATH_BYTES]="";const char charCrypt[2]="*";
        unz_file_info64 file_info;bool ok=false;
        err = unzGoToFirstFile(uf);
        if (err!=UNZ_OK)  {
            fprintf(stderr,"error %d with zipfile in unzGoToFirstFile",err);
            return false;
        }

        for (i=0;i<gi.number_entry;i++)	{
            err = unzGetCurrentFileInfo64(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
            if (err!=UNZ_OK) {
                fprintf(stderr,"Error %d with zipfile in unzGetCurrentFileInfo\n",err);
                return false;
            }

            ok = false;
            size_t filename_inzip_len = strlen(filename_inzip);
            const bool hasZeroSize = file_info.compressed_size == 0 && file_info.uncompressed_size == 0;
            const bool isDirectory = filename_inzip_len>0 && (filename_inzip[filename_inzip_len-1]=='/' || filename_inzip[filename_inzip_len-1]=='\\');
            ok = fileMode ? (!isDirectory && !hasZeroSize) : (isDirectory && hasZeroSize) ? true : false;

            if (ok) {
                if (dirNameLen>0)   {
                    if (dirNameLen>=filename_inzip_len) ok = false;
                    if (strncmp(dirName,filename_inzip,dirNameLen)!=0) ok = false;
                    if (filename_inzip[dirNameLen]!='/' && filename_inzip[dirNameLen]!='\\') ok = false;
                    if (!fileMode && dirNameLen+1 == filename_inzip_len) ok = false;
                }
                if (ok) {
                    // remove trailing dash from filename_inzip
                    if (filename_inzip_len>0 && (filename_inzip[filename_inzip_len-1]=='/' || filename_inzip[filename_inzip_len-1]=='\\')) {filename_inzip[filename_inzip_len-1]='\0';--filename_inzip_len;}   // remove trailing dash

                    strcpy(tmp,dirNameLen>0 ? &filename_inzip[dirNameLen+1] : filename_inzip);  // Here "tmp" if the plain name, and "filename_inzip" the full name
                    size_t tmpLen = strlen(tmp);
                    for (size_t k=0;k<tmpLen;k++) {
                        if (tmp[k]=='/' || tmp[k]=='\\') {
                            if (fileMode || k!=tmpLen-1)
                            {ok = false;break;}
                        }
                    }
                    if (ok) {
                        if ((file_info.flag & 1) != 0) strcat(tmp,charCrypt); // Optional display a '*' in the file name only if it is crypted

                        size_t fileInfosSize = fileInfos.size();
                        fileInfos.resize(fileInfosSize+1);
                        fileInfos[fileInfosSize].assign(filename_inzip,tmp,file_info);
                        //fprintf(stderr,"%s\t%s\t%u\t%u\n",filename_inzip,tmp,(unsigned)file_info.compressed_size,(unsigned)file_info.uncompressed_size);
                    }
                }
            }

           if (i<gi.number_entry-1)  {
                err = unzGoToNextFile(uf);
                if (err!=UNZ_OK)  {
                    fprintf(stderr,"error %d with zipfile in unzGoToNextFile\n",err);
                    return false;
                }
            }

        } // for loop

if (fileInfos.size()>0) {
    //sorting:
    if (!fileMode && sorting >= SORT_ORDER_SIZE) sorting = (Sorting)(sorting%2);
    unz_file_info64_plus::Sort(fileInfos,sorting);

    for (size_t j=0,jsz=fileInfos.size();j<jsz;j++) {
        const unz_file_info64_plus& fip = fileInfos[j];
        //const unz_file_info64& fi = fip.info;

        const size_t resultSize = result.size();
        result.resize(resultSize+1);
        char* pResult = &result[resultSize][0];
        pResult[0]='\0';
        if (prefixResultWithTheFullPathOfTheZipFile) {
            strcpy(pResult,zipFilePath);
            strcat(pResult,"/");
        }
        strcat(pResult,fip.path);


        if (pOptionalNamesOut)  {
            pOptionalNamesOut->resize(resultSize+1);
            char* pName = &(*pOptionalNamesOut)[resultSize][0];
            strcpy(pName,fip.name);

            //const unz_file_info64& file_info = fip.info;
            //fprintf(stderr,"%s\t%s\t%u\t%u\n",pResult,pName,(unsigned)file_info.compressed_size,(unsigned)file_info.uncompressed_size);
        }

    }

}
/*
 if ((file_info.flag & 1) != 0) strcat(tmp,charCrypt);
                    const size_t resultSize = result.size();
                    result.resize(resultSize+1);
                    char* pResult = &result[resultSize][0];
                    strcpy(pResult,filename_inzip);

                    if (pOptionalNamesOut)  {
                        pOptionalNamesOut->resize(resultSize+1);
                        char* pName = &(*pOptionalNamesOut)[resultSize][0];
                        strcpy(pName,tmp);
                        if ((file_info.flag & 1) != 0) strcat(pName,charCrypt); // display a '*' if the file is crypted

                        //fprintf(stderr,"%s\t%s\t%u\t%u\t%d\n",pResult,pName,(unsigned)file_info.compressed_size,(unsigned)file_info.uncompressed_size,isDirectory);
                    }
*/
        err = unzGoToFirstFile(uf);
        if (err!=UNZ_OK)  {
            fprintf(stderr,"error %d with zipfile in unzGoToFirstFile",err);
            return false;
        }
        unzCloseCurrentFile(uf);
        return true;

    }

    static bool PathSplitFirstZipFolder(const char* path, char* rv1,char* rv2,bool rv1IsAbsolutePath=true)  {
        rv1[0]='\0';rv2[0]='\0';
        if (path) {
            int gc = 0;
            const char* lowerZip = ".zip";const char* upperZip = ".ZIP";const int numCharsToMath = (int)strlen(lowerZip);
            //fprintf(stderr,"%s %s %s %d %d\n",path,rv1,rv2,(int)strlen(path)-numCharsToMath,numCharsToMath);
            for (int i=0,isz=(int)strlen(path)-numCharsToMath;i<=isz;i++) {
                char c = path[i];
                gc=0;
                while (c==lowerZip[gc] || c==upperZip[gc]) {
                    if (++gc==numCharsToMath) {
                        gc=i+numCharsToMath;
                        while (gc<isz && (path[gc]=='/' || path[gc]=='\\')) ++gc;
                        strcpy(rv2,&path[gc]);

                        if (rv1IsAbsolutePath)  {
                            char rv3[MAX_PATH_BYTES];
                            strncpy(rv3,path,i+numCharsToMath);
                            rv3[i+numCharsToMath]='\0';
                            //fprintf(stderr,"rv3 %s\n",rv3);
                            Path::GetAbsolutePath(rv3,rv1);
                        }
                        else {
                            strncpy(rv1,path,i+numCharsToMath);
                            rv1[i+numCharsToMath]='\0';
                        }

                        //fprintf(stderr,"MATCH \"%s\": \"%s\"+\"%s\"\n",path,rv1,rv2);

                        return true;
                    }
                    c = path[i+gc];
                }
            }
            // No zip found:
            if (rv1IsAbsolutePath) Path::GetAbsolutePath(path,rv1);
            else strcpy(rv1,path);
        }
        return false;
    }
    static bool PathExistsWithZipSupport(const char* path, bool reportOnlyFiles, bool reportOnlyDirectories, bool checkAbsolutePath=true, bool* isInsideAZipFile=NULL) {
        char rv1[MAX_PATH_BYTES];char rv2[MAX_PATH_BYTES];rv1[0]='\0';rv2[0]='\0';
        if (PathSplitFirstZipFolder(path,rv1,rv2,checkAbsolutePath))    {
            if (isInsideAZipFile) *isInsideAZipFile = true;
            UnZipFile unz(rv1);
            return unz.exists(rv2,reportOnlyFiles,reportOnlyDirectories);
        }
        else {
            if (isInsideAZipFile) *isInsideAZipFile = false;
            return PathExists(rv1);
        }
    }
};
bool UnZipFile::load(const char* zipFilePath,bool reloadIfAlreadyLoaded) {
    if (!reloadIfAlreadyLoaded && isValid() && strcmp(zipFilePath,im->zipFilePath)==0) return true;
    close();
    if (zipFilePath) {
        Path::GetAbsolutePath(zipFilePath,im->zipFilePath);
        im->uf = unzOpen64(im->zipFilePath);
    }
    //else if (im->zipFilePath) im->uf = unzOpen64(im->zipFilePath);
    return im->uf!=NULL;
}
const char* UnZipFile::getZipFilePath() const {
    return im->zipFilePath;
}
bool UnZipFile::isValid() const {
    return im->uf!=NULL;
}
void UnZipFile::close() {
    if (im->uf) {unzClose(im->uf);im->uf=NULL;}
}
bool UnZipFile::getDirectories(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting,bool prefixResultWithTheFullPathOfTheZipFile) const  {
    return im->getFilesOrDirectories(false,directoryName,result,pOptionalNamesOut,sorting,prefixResultWithTheFullPathOfTheZipFile);
}
bool UnZipFile::getFiles(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting,bool prefixResultWithTheFullPathOfTheZipFile) const  {
    return im->getFilesOrDirectories(true,directoryName,result,pOptionalNamesOut,sorting,prefixResultWithTheFullPathOfTheZipFile);
}
unsigned int UnZipFile::getFileSize(const char* filePath) const   {
    if (!im->uf || !filePath) return 0;
    if (unzLocateFile(im->uf,filePath,0)!=UNZ_OK) return 0;
    unz_file_info64 file_info;char filename_inzip[2048];
    int err = unzGetCurrentFileInfo64(im->uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
    if (err!=UNZ_OK)  return 0;
    unsigned int sz = file_info.uncompressed_size;
    if (sz==file_info.uncompressed_size) return sz;
    return 0;
}
template<typename CharType> static bool UnZipFileGetFileContentBase(ImGuiFs::UnZipFileImpl* im,const char* filePath,ImVector<CharType>& bufferOut,const char* password)   {
    bufferOut.clear();
    if (!im->uf || !filePath) return false;
    if (unzLocateFile(im->uf,filePath,0)!=UNZ_OK) return false;
    unz_file_info64 file_info;char filename_inzip[2048];    // it's the filename without path
    int err = unzGetCurrentFileInfo64(im->uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
    if (err<0)  {
        fprintf(stderr,"Error while unzipping: \"%s\": %d with zipfile in unzGetCurrentFileInfo\n",filePath,err);
        return false;
    }
    //if (file_info.uncompressed_size>MAX_UNSIGNED_INT) return false; // where is MAX_UNSIGNED_INT ???
    err = unzOpenCurrentFilePassword(im->uf,password);
    if (err<0)  {
        fprintf(stderr,"Error while unzipping: \"%s\": %d with zipfile in unzOpenCurrentFilePassword\n",filePath,err);
        return false;
    }
    bufferOut.resize(file_info.uncompressed_size);
    if ((unsigned)bufferOut.size()<file_info.uncompressed_size) {
        fprintf(stderr,"Error while unzipping: \"%s\": file is too big.\n",filePath);
        ImVector<CharType> tmp;bufferOut.swap(tmp);
        unzCloseCurrentFile (im->uf);
        return false;
    }
    err = unzReadCurrentFile(im->uf,&bufferOut[0],bufferOut.size());
    if (err<0) {
        fprintf(stderr,"Error while unzipping: \"%s\": %d with zipfile in unzReadCurrentFile\n",filePath,err);
        ImVector<CharType> tmp;bufferOut.swap(tmp);
        unzCloseCurrentFile (im->uf);
        return false;
    }
    err = unzCloseCurrentFile (im->uf);
    if (err<0) {
        fprintf(stderr,"Error while unzipping: \"%s\": %d with zipfile in unzCloseCurrentFile\n",filePath,err);
    }
    return true;
}
bool UnZipFile::getFileContent(const char* filePath,ImVector<unsigned char>& bufferOut,const char* password) const    {return UnZipFileGetFileContentBase<unsigned char>(im,filePath,bufferOut,password);}
bool UnZipFile::getFileContent(const char* filePath,ImVector<char>& bufferOut,const char* password) const    {return UnZipFileGetFileContentBase<char>(im,filePath,bufferOut,password);}
bool UnZipFile::exists(const char* pathInsideZip,bool reportOnlyFiles,bool reportOnlyDirectories) const {
    if (!im->uf || !pathInsideZip) return false;
    char path[MAX_PATH_BYTES];

    // copy a clean pathInsideZip into path
    size_t dirNameLen = 0;
    if (pathInsideZip)  {
        dirNameLen = strlen(pathInsideZip);
        if (dirNameLen>1 && pathInsideZip[0]=='.' && (pathInsideZip[1]=='/' || pathInsideZip[1]=='\\')) {
            strcpy(path,&pathInsideZip[2]);
        }
        else strcpy(path,pathInsideZip);
    }
    else strcpy(path,pathInsideZip ? pathInsideZip : "");

    dirNameLen = strlen(path);
    while (dirNameLen>0 && (path[dirNameLen-1]=='/' || path[dirNameLen-1]=='\\')) {path[dirNameLen-1]='\0';--dirNameLen;}

    bool onlyDirs = false,onlyFiles=false;
    if ((reportOnlyFiles || reportOnlyDirectories) && !(reportOnlyFiles && reportOnlyDirectories)) {
        onlyDirs = reportOnlyDirectories;
        onlyFiles = reportOnlyFiles;
    }

    if (strlen(path)==0 && !onlyFiles) return true;   // base zip folder

    if (!onlyFiles) {path[dirNameLen++]='/';path[dirNameLen]='\0';}

    bool found = (unzLocateFile(im->uf,path,0)==UNZ_OK);    // it becames the current file

    //fprintf(stderr,"%s %s %s\n",pathInsideZip,path,found?"true":"false");

    if (found && (onlyDirs || onlyFiles)) {
        unz_file_info64 file_info;
        char filename_inzip[2048];    // it's the filename without path
        int err = unzGetCurrentFileInfo64(im->uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);
        if (err!=UNZ_OK)  {
            fprintf(stderr,"Error while checking: \"%s\": %d in unzGetCurrentFileInfo\n",pathInsideZip,err);
            found = false;
        }
        else    {
            size_t filename_inzip_len = strlen(filename_inzip);
            const bool hasZeroSize = file_info.compressed_size == 0 && file_info.uncompressed_size == 0;
            const bool isDirectory = filename_inzip_len>0 && (filename_inzip[filename_inzip_len-1]=='/' || filename_inzip[filename_inzip_len-1]=='\\');
            found = onlyFiles ? (!isDirectory && !hasZeroSize) : onlyDirs ? (isDirectory && hasZeroSize) : false;
        }

    }
    unzCloseCurrentFile(im->uf);

    return found;
}
bool UnZipFile::fileExists(const char* pathInsideZip) const {
    return exists(pathInsideZip,true,false);
}
bool UnZipFile::directoryExists(const char* pathInsideZip) const    {
    return exists(pathInsideZip,false,true);
}
UnZipFile::UnZipFile(const char* zipFilePath)  {
    im = (UnZipFileImpl*) ImGui::MemAlloc(sizeof(UnZipFileImpl));
    im->uf = NULL;im->zipFilePath[0]='\0';
    load(zipFilePath);
}
UnZipFile::~UnZipFile() {
    if (im) {
        close();
        ImGui::MemFree(im);
        im=NULL;
    }
}

bool PathSplitFirstZipFolder(const char* path, char* rv1,char* rv2,bool rv1IsAbsolutePath)  {
    return UnZipFileImpl::PathSplitFirstZipFolder(path,rv1,rv2,rv1IsAbsolutePath);
}
bool PathExistsWithZipSupport(const char* path, bool reportOnlyFiles, bool reportOnlyDirectories,bool checkAbsolutePath,bool* isInsideAZipFile) {
    return UnZipFileImpl::PathExistsWithZipSupport(path,reportOnlyFiles,reportOnlyDirectories,checkAbsolutePath,isInsideAZipFile);
}
bool PathIsInsideAZipFile(const char* path) {
    bool isInsideAZipFile = false;
    UnZipFileImpl::PathExistsWithZipSupport(path,true,true,true,&isInsideAZipFile);
    return isInsideAZipFile;
}
bool DirectoryGetDirectoriesWithZipSupport(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting,bool prefixResultWithTheFullPathOfTheZipFile)   {
    result.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();
    char zipRoot[ImGuiFs::MAX_PATH_BYTES]="";
    char zipFolder[ImGuiFs::MAX_PATH_BYTES]="";
    if (PathSplitFirstZipFolder(directoryName,zipRoot,zipFolder)) {
        UnZipFile uzf(zipRoot);
        if (!uzf.isValid()) return false;
        return uzf.getDirectories(zipFolder,result,pOptionalNamesOut,sorting,prefixResultWithTheFullPathOfTheZipFile);
    }
    ImGuiFs::DirectoryGetDirectories(directoryName,result,pOptionalNamesOut,sorting);
    return true;
}
bool DirectoryGetFilesWithZipSupport(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting,bool prefixResultWithTheFullPathOfTheZipFile) {
    result.clear();if (pOptionalNamesOut) pOptionalNamesOut->clear();
    char zipRoot[ImGuiFs::MAX_PATH_BYTES]="";
    char zipFolder[ImGuiFs::MAX_PATH_BYTES]="";
    if (PathSplitFirstZipFolder(directoryName,zipRoot,zipFolder)) {
        UnZipFile uzf(zipRoot);
        if (!uzf.isValid()) return false;
        return uzf.getFiles(zipFolder,result,pOptionalNamesOut,sorting,prefixResultWithTheFullPathOfTheZipFile);
    }
    ImGuiFs::DirectoryGetFiles(directoryName,result,pOptionalNamesOut,sorting);
    return true;
}
void PathGetDirectoryNameWithZipSupport(const char* path,char* rv,bool prefixResultWithTheFullPathOfTheZipFile)  {
    char zipRoot[ImGuiFs::MAX_PATH_BYTES]="";
    char zipRelativePath[ImGuiFs::MAX_PATH_BYTES]="";
    rv[0]='\0';
    if (PathSplitFirstZipFolder(path,zipRoot,zipRelativePath)) {
        char tmp[ImGuiFs::MAX_PATH_BYTES]="";
        ImGuiFs::PathGetDirectoryName(zipRelativePath,tmp);
        if (prefixResultWithTheFullPathOfTheZipFile)    {
            strcpy(rv,zipRoot);
            if (strlen(tmp)>0 && tmp[0]!='/' && tmp[0]!='\\') strcat(rv,"/");
        }
        strcat(rv,tmp);
    }
    else ImGuiFs::PathGetDirectoryName(path,rv);
}
void PathGetAbsoluteWithZipSupport(const char* path,char* rv)   {
    char zipRoot[ImGuiFs::MAX_PATH_BYTES]="";
    char zipRelativePath[ImGuiFs::MAX_PATH_BYTES]="";
    rv[0]='\0';
    if (PathSplitFirstZipFolder(path,zipRoot,zipRelativePath)) {
        char tmp[ImGuiFs::MAX_PATH_BYTES]="";
        ImGuiFs::PathGetAbsolute(zipRoot,tmp);
        strcpy(rv,tmp);
        if (strlen(zipRelativePath)>0 && zipRelativePath[0]!='/' && zipRelativePath[0]!='\\') strcat(rv,"/");
        strcat(rv,zipRelativePath);
    }
    else ImGuiFs::PathGetAbsolute(path,rv);
}

#endif //IMGUI_USE_MINIZIP
template <typename CharType> bool FileGetContentBase(const char* path,ImVector<CharType>& bufferOut,const char* password) {
    bufferOut.clear();
    char mainPath[MAX_PATH_BYTES];
#   ifdef IMGUI_USE_MINIZIP
    char zipPath[MAX_PATH_BYTES];
    PathSplitFirstZipFolder(path,mainPath,zipPath,true);
    if (!FileExists(mainPath)) return false;
    if (strlen(zipPath)>0) {
        UnZipFile unz(mainPath);
        return unz.getFileContent(zipPath,bufferOut,password);
    }
#   else //IMGUI_USE_MINIZIP
    strcpy(mainPath,path);
#   endif //IMGUI_USE_MINIZIP
    //FILE* fin = ImFileOpen(mainPath,"rb"); //Carson: undefined function... using fopen instead
    FILE* fin = fopen(mainPath,"rb");
    if (!fin) return false;
    fseek(fin,0,SEEK_END);
    const long szl = ftell(fin);
    const size_t sz = (size_t) szl;
    if ((sizeof(long)>sizeof(size_t) && szl!=(long)sz) || (sizeof(long)<sizeof(size_t) && (size_t)szl!=sz))    {
        fprintf(stderr,"Error in: FileGetContent(\"%s\"): file too big.\n",mainPath);
        fclose(fin);fin=NULL;
        return false;
    }
    fseek(fin,0,SEEK_SET);
    if (sz>0)   {
        bufferOut.resize(sz);
        if (bufferOut.size()==(int)sz) fread(&bufferOut[0],(size_t)sz,1,fin);
        else {
            fprintf(stderr,"Error in: FileGetContent(\"%s\"): file too big.\n",mainPath);
            bufferOut.clear();
            fclose(fin);fin=NULL;
            return false;
        }
    }
    fclose(fin);fin=NULL;
    return true;
}
bool FileGetContent(const char* path,ImVector<unsigned char>& bufferOut,const char* password) {return FileGetContentBase<unsigned char>(path,bufferOut,password);}
bool FileGetContent(const char* path,ImVector<char>& bufferOut,const char* password) {return FileGetContentBase<char>(path,bufferOut,password);}
#endif // IMGUIFS_NO_EXTRA_METHODS
// End definitions of some helper classes----------------------------------------------------------------------------------------

// Internal Usage----------------------------------------------------------------------------------------
struct ImGuiFsDrawIconStruct {
    ImVector<char> v;
    ImVector<int> vStarters;              // size = number of strings in v
    ImVector<int> vStartersLengths;       // same size as vStarters
    ImVector<int> vStartersCounts;        // same size as vStarters (map to ICON_FA_XXX)
    int add(const char* exts) {
    IM_ASSERT(exts && strlen(exts)>1);
    int cnt = 0;
    const int vStartersCountValue = vStartersCounts.size()>0 ? (vStartersCounts[vStartersCounts.size()-1]+1) : 0;
    static char tmp[512];
    IM_ASSERT(strlen(exts)<511);
    strcpy(tmp,exts);
    char* token = strtok(tmp,";");
    int sz = 0,vSz=0;
    while(token) {
        sz=strlen(token);
        vSz=v.size();
        vStarters.push_back(vSz);
        vStartersLengths.push_back(sz);
        vStartersCounts.push_back(vStartersCountValue);
        v.resize(vSz+sz+1);
        strcpy(&v[vSz],token);

        token = strtok(NULL,";");
        ++cnt;
    }
    IM_ASSERT(cnt>0);

    return cnt;
    }
    ImGuiFsDrawIconStruct() {
    v.reserve(1024);
    vStarters.reserve(400);vStartersLengths.reserve(400);
    vStartersCounts.reserve(400);

    add("bin");                                     // ICON_FA_FILE_O
    add("h;hpp;hh;hxx;inl");                        // ICON_FA_H_SQUARE
    add("cpp;c;cxx;cc");                            // ICON_FA_PLUS_SQUARE
    add("jpg;jpeg;png;bmp;ico;gif;tif;tiff;tga");   // ICON_FA_FILE_IMAGE_O
    add("pdf");                                     // ICON_FA_FILE_PDF_O
    add("doc;docx;odt;ott;uot");                    // ICON_FA_FILE_WORD_O
    add("txt;setting;settings;layout;ini;md;sh;bat");// ICON_FA_FILE_TEXT_O
    add("db;sql;sqlite");                           // ICON_FA_DATABASE
    add("ods;ots;uos;xlsx;xls");                    // ICON_FA_FILE_EXCEL_O
    add("odp;otp;uop;pptx;ppt");                    // ICON_FA_FILE_POWERPOINT_O
    add("7z;zip;bz2;gz;lz;lzma;ar;rar");            // ICON_FA_FILE_ARCHIVE_O
    add("mp3;wav;ogg;spx;opus;mid;mod;flac");       // ICON_FA_FILE_AUDIO_O
    add("mp4;flv;avi;ogv;theora;mkv;webm;mpg");     // ICON_FA_FILE_VIDEO_O
    add("xml");                                     // ICON_FA_FILE_CODE_O
    add("htm;html");                                // ICON_FA_FILE_CODE_O

    }
    int getExtensionType(const char* ext,bool caseSensitiveMatch=false) const {
        if (!ext) return -1;
        if (ext[0]=='.') ext+=1;
        const int extLen = strlen(ext);
        if (extLen==0) return -1;
        typedef int (*strcmpdelegate) (const char*, const char*);
        const strcmpdelegate myStrCmp = caseSensitiveMatch ? &strcmp : &strcasecmp;
        int start=0,startLen=0;
        for (int si = 0,siSz=vStarters.size();si<siSz;si++) {
            startLen = vStartersLengths[si];
            if (extLen!=startLen) continue;
            start = vStarters[si];
            if (myStrCmp(&v[start],ext)==0) return vStartersCounts[si];
        }
        return -1;
    }
    inline void fillExtensionTypesFromFilenames(ImVector<int>& fileExtensionTypes,const FilenameStringVector& fileNames)  {
    fileExtensionTypes.resize(fileNames.size());
    for (int i=0,isz=fileNames.size();i<isz;i++) fileExtensionTypes[i] = getExtensionType(strrchr(fileNames[i],'.'));
    }

    inline bool drawIcon(int extensionType,const ImVec4* pOptionalColorOverride=NULL) const {
    return ImGuiFs::Dialog::DrawFileIconCallback ? ImGuiFs::Dialog::DrawFileIconCallback(extensionType,pOptionalColorOverride) : false;
    }
    inline bool drawIcon(const char* ext,bool caseSensitiveMatch=false,const ImVec4* pOptionalColorOverride=NULL) const {
    const int extType = getExtensionType(ext,caseSensitiveMatch);
    return drawIcon(extType,pOptionalColorOverride);
    }
};
ImGuiFsDrawIconStruct& MyImGuiFsDrawIconStruct()
{
  static ImGuiFsDrawIconStruct singleton;
  return singleton;
}
#ifndef IMGUIFS_NO_EXTRA_METHODS
int FileGetExtensionType(const char* path) {
    return MyImGuiFsDrawIconStruct().getExtensionType(strrchr(path,'.'));
}
void FileGetExtensionTypesFromFilenames(ImVector<int>& fileExtensionTypesOut,const FilenameStringVector& fileNames)  {
    MyImGuiFsDrawIconStruct().fillExtensionTypesFromFilenames(fileExtensionTypesOut,fileNames);
}
#if (defined(__EMSCRIPTEN__) && defined(EMSCRIPTEN_SAVE_SHELL))
bool FileDownload(const char* path,const char* optionalSaveFileName)    {
    if (!path || !FileExists(path)) return false;
    ImGuiTextBuffer buffer;
    if (optionalSaveFileName) buffer.append("saveFileFromMemoryFSToDisk(\"%s\",\"%s\")",path,optionalSaveFileName);
    else {
        char fileName[ImGuiFs::MAX_FILENAME_BYTES]="";
        ImGuiFs::PathGetFileName(path,fileName);
        buffer.append("saveFileFromMemoryFSToDisk(\"%s\",\"%s\")",path,fileName);
    }
    emscripten_run_script(&buffer.Buf[0]);
    return true;
}
#endif // (defined(__EMSCRIPTEN__) && defined(EMSCRIPTEN_SAVE_SHELL))
#endif // IMGUIFS_NO_EXTRA_METHODS
//-------------------------------------------------------------------------------------------------------

// Internal usage----------------------------------------------------------------------------------------
struct FolderInfo    {
    char fullFolder[MAX_PATH_BYTES];
    char currentFolder[MAX_PATH_BYTES];
    int splitPathIndex;
    static FilenameStringVector SplitPath;    // tmp field used internally
    int splitPathIndexOfZipFile;

    void display() const {
        fprintf(stderr,"fullFolder=\"%s\" currentFolder=\"%s\" splitPathIndex=%d splitPathIndexOfZipFile=%d\n",fullFolder,currentFolder,splitPathIndex,splitPathIndexOfZipFile);
    }
    void getSplitPath(FilenameStringVector& splitPath) const   {
        Path::Split(fullFolder,splitPath);
    }
    const FolderInfo& operator=(const FolderInfo& o) {
        strcpy(currentFolder,o.currentFolder);
        strcpy(fullFolder,o.fullFolder);
        splitPathIndex = o.splitPathIndex;
        splitPathIndexOfZipFile = o.splitPathIndexOfZipFile;
        return *this;
    }
    inline void reset() {
        currentFolder[0]='\0';fullFolder[0]='\0';splitPathIndex=-1;splitPathIndexOfZipFile=-1;
    }
    FolderInfo() {reset();}
    FolderInfo(const FolderInfo& o) {*this=o;}

    void fromCurrentFolder(const char* path)   {
        if (!path || strlen(path)==0) reset();
        else {
            strcpy(currentFolder,path);
            strcpy(fullFolder,path);
            Path::Split(fullFolder,SplitPath);
            splitPathIndex = (int) SplitPath.size()-1;
            splitPathIndexOfZipFile = GetSplitPathIndexOfZipFile(SplitPath);
        }
    }
    inline static int GetSplitPathIndexOfZipFile(const FilenameStringVector& SplitPath) {
            int splitPathIndexOfZipFile = -1;
#           ifdef IMGUI_USE_MINIZIP
            const char* lower = ".zip";const char* upper = ".ZIP";const int numCharsToMath = (int)strlen(lower);int gc=0;
            for (int j=0,jsz=(int) SplitPath.size();j<jsz;j++)  {
                const char* path = SplitPath[j];
                //fprintf(stderr,"%d) %s\n",j,path);
                const int sz = (int) strlen(path);
                const int startCharToMatch = (j==jsz-1) ? numCharsToMath : (numCharsToMath+1);
                if (sz<=startCharToMatch) continue;
                const int i = sz-startCharToMatch;

                gc=0;
                char c = path[i];
                while (c==lower[gc] || c==upper[gc]) {
                    //fprintf(stderr,"match: %c\n",c);
                    if (++gc==numCharsToMath) {
                        splitPathIndexOfZipFile = j;
                        //fprintf(stderr,"splitPathIndexOfZipFile=%d (%s)\n",splitPathIndexOfZipFile,path);
                        return splitPathIndexOfZipFile;// Warning: I should double break here!
                    }
                    c = path[i+gc];
                }

            }
#           endif //IMGUI_USE_MINIZIP
            return splitPathIndexOfZipFile;
    }
    bool isEqual(const FolderInfo& fi) const {
        return strcmp(fullFolder,fi.fullFolder)==0 && strcmp(currentFolder,fi.currentFolder)==0;
    }
    bool isEqual(const char* path) const {
        return strcmp(fullFolder,path)==0 && strcmp(currentFolder,path)==0;
    }
    int getSplitPathIndexFor(const char* path/*,int* pOptionalSplitPathIndexOfZipFile=NULL*/) const {
        if (!path || strncmp(path,fullFolder,strlen(path))!=0) return -1;

        int spi = -1;
        Path::Split(fullFolder,SplitPath);
        char tmp[MAX_PATH_BYTES];tmp[0]='\0';
        for (int i=0,sz=(int)SplitPath.size();i<sz;i++)    {
            Path::Append(SplitPath[i],tmp);
            //fprintf(stderr,"%d) \"%s\" <-> \"%s\"\n",i,tmp,path);
            if (strcmp(tmp,path)==0) {spi=i;break;}
        }
        //if (pOptionalSplitPathIndexOfZipFile) *pOptionalSplitPathIndexOfZipFile = GetSplitPathIndexOfZipFile(SplitPath);

        return spi;
    }
    bool getFolderInfoForSplitPathIndex(int _splitPathIndex,FolderInfo& rv) const {
        Path::Split(fullFolder,SplitPath);
        const int splitPathSize = (int)SplitPath.size();
        if (_splitPathIndex<0 || _splitPathIndex>=splitPathSize) return false;
        rv = *this;
        rv.splitPathIndex = _splitPathIndex;     
        rv.splitPathIndexOfZipFile = GetSplitPathIndexOfZipFile(SplitPath);

        rv.currentFolder[0]='\0';
        if (_splitPathIndex>=0 && _splitPathIndex<splitPathSize) {
            for (int i=0;i<=_splitPathIndex;i++)    {
                Path::Append(SplitPath[i],rv.currentFolder);
                //fprintf(stderr,"%d) \"%s\" (\"%s\")\n",i,rv.currentFolder,SplitPath[i]);
            }
        }
        /*fprintf(stderr,"getFolderInfoForSplitPathIndex(%d):\nSource:    ",_splitPathIndex);
        this->display();
        fprintf(stderr,"Result:   ");
        rv.display();*/
        return true;
    }
};
FilenameStringVector FolderInfo::SplitPath;   // tmp field used internally
struct History {
protected:
    ImVector<FolderInfo> info;
    int currentInfoIndex;  // into info
public:
    inline bool canGoBack()    {
        return currentInfoIndex>0;
    }
    inline bool canGoForward()   {
        return currentInfoIndex>=0 && currentInfoIndex<(int)info.size()-1;
    }
    void reset() {info.clear();currentInfoIndex=-1;}
    History() {reset();}
    ~History() {info.clear();}
    // -------------------------------------------------------------------------------------------------
    void goBack()   {
        if (canGoBack()) --currentInfoIndex;
    }
    void goForward()    {
        if (canGoForward()) ++currentInfoIndex;
    }
    bool switchTo(const char* currentFolder)    {
        if (!currentFolder || strlen(currentFolder)==0) return false;
        if (currentInfoIndex<0) {
            ++currentInfoIndex;
        info.resize(currentInfoIndex+1);
	    FolderInfo& fi = info[currentInfoIndex];
            fi.fromCurrentFolder(currentFolder);
            //fprintf(stderr,"switchTo 1 %d\n",fi.splitPathIndexOfZipFile);
            return true;
        }
        else {
            const FolderInfo& lastInfo = info[currentInfoIndex];
            if (lastInfo.isEqual(currentFolder)) return false;
            const int splitPathIndexInsideLastInfo = lastInfo.getSplitPathIndexFor(currentFolder);
            ++currentInfoIndex;
            info.resize(currentInfoIndex+1);
	    FolderInfo& fi = info[currentInfoIndex];
            if (splitPathIndexInsideLastInfo==-1)   {
                fi.fromCurrentFolder(currentFolder);
                //fprintf(stderr,"switchTo 2a: %d (%s)\n",fi.splitPathIndexOfZipFile,currentFolder);
            }
            else {
		fi = lastInfo;
                fi.splitPathIndex = splitPathIndexInsideLastInfo;
                strcpy(fi.currentFolder,currentFolder);

                //fprintf(stderr,"switchTo 2b: %d\n",fi.splitPathIndexOfZipFile);
            }                        
            return true;
        }
    }
    bool switchTo(const FolderInfo& fi) {
        if (/*!fi.currentFolder ||*/ strlen(fi.currentFolder)==0) return false;
        if (currentInfoIndex>=0) {
            const FolderInfo& lastInfo = info[currentInfoIndex];
            if (lastInfo.isEqual(fi)) return false;
        }
        ++currentInfoIndex;
        info.resize(currentInfoIndex+1);
	info[currentInfoIndex] = fi;
        //fprintf(stderr,"switchTo 3 %d\n",fi.splitPathIndexOfZipFile);
        return true;
    }
    //-----------------------------------------------------------------------------------------------------

    inline bool isValid() const {return (currentInfoIndex>=0 && currentInfoIndex<(int)info.size());}
    const FolderInfo* getCurrentFolderInfo() const {return isValid() ? &info[currentInfoIndex] : NULL;}
    const char* getCurrentFolder() const {return isValid() ? &info[currentInfoIndex].currentFolder[0] : NULL;}
    bool getCurrentSplitPath(FilenameStringVector& rv) const {
        if (isValid()) {
            info[currentInfoIndex].getSplitPath(rv);
            return true;
        }
        else return false;
    }
    const int* getCurrentSplitPathIndex() const {return isValid() ? &info[currentInfoIndex].splitPathIndex : NULL;}
    size_t getInfoSize() const {return info.size();}
};

struct Internal {
    PathStringVector dirs,files;
    FilenameStringVector dirNames,fileNames,currentSplitPath;
    ImVector<int> fileExtensionTypes;
    char currentFolder[MAX_PATH_BYTES];
    bool forceRescan;
    bool open;
    ImVec2 wndPos;
    ImVec2 wndSize;
    char wndTitle[MAX_PATH_BYTES];
    int sortingMode;
    bool userHasJustCancelledDialog;

    History history;
#   ifdef IMGUI_USE_MINIZIP
    UnZipFile unz;
#   endif //IMGUI_USE_MINIZIP
    //-----------------------------------------------------
    bool isSelectFolderDialog;
    bool isSaveFileDialog;

    bool allowDirectoryCreation,forbidDirectoryCreation;
    bool allowKnownDirectoriesSection;
    char newDirectoryName[MAX_FILENAME_BYTES];
    char saveFileName[MAX_FILENAME_BYTES];
    //----------------------------------------------------

    char chosenPath[MAX_PATH_BYTES];
    bool rescan;
    int uniqueNumber;

    ImGuiTextFilter filter;
    bool allowFiltering;

    int totalNumBrowsingEntries;
    int numBrowsingColumns;
    int numBrowsingEntriesPerColumn;
    static bool BrowsingPerRow;
    bool allowDisplayByOption;

    bool detectKnownDirectoriesAtEveryOpening;
    bool mustFilterSaveFilePathWithFileFilterExtensionString;

    bool editLocationCheckButtonPressed;
    char editLocationInputText[MAX_PATH_BYTES];
    bool forceSetWindowPositionAndSize;


    ~Internal() {
	dirs.clear();files.clear();
	dirNames.clear();fileNames.clear();
	currentSplitPath.clear();
    }

    inline static void FreeMemory(PathStringVector& v) {PathStringVector o;v.swap(o);}
#   if FILENAME_MAX!=DIRENT_MAX_PATH  // otherwise PathStringVector == FilenameStringVector
    inline static void FreeMemory(FilenameStringVector& v) {FilenameStringVector o;v.swap(o);}
#   endif //FILENAME_MAX!=DIRENT_MAX_PATH
    void freeMemory() {
        FreeMemory(dirs);FreeMemory(files);
        FreeMemory(dirNames);FreeMemory(fileNames);FreeMemory(currentSplitPath);
        FreeMemory(FolderInfo::SplitPath);  // Not too sure about this...
    }

    void resetVariables() {
        strcpy(currentFolder,"./");
        forceRescan = false;
        open = true;

        wndTitle[0] = '\0';
        sortingMode = 0;

        history.reset();

        isSelectFolderDialog = false;
        isSaveFileDialog = false;

        allowDirectoryCreation = true;
        forbidDirectoryCreation = false;
        strcpy(newDirectoryName,"New Folder");
        saveFileName[0] = '\0';

        uniqueNumber = 0;

        rescan = true;
        chosenPath[0] = '\0';

        filter.Clear();
        allowFiltering = false;

        totalNumBrowsingEntries = 0;
        numBrowsingColumns = 1;
        numBrowsingEntriesPerColumn = 1000;

        detectKnownDirectoriesAtEveryOpening = false;
        allowDisplayByOption = false;
        allowKnownDirectoriesSection = true;

        mustFilterSaveFilePathWithFileFilterExtensionString = true;

        editLocationCheckButtonPressed = false;
        strcpy(editLocationInputText,"\0");

        forceSetWindowPositionAndSize = true;
        userHasJustCancelledDialog = false;

#       ifdef IMGUI_USE_MINIZIP
        unz.close();
#       endif //IMGUI_USE_MINIZIP
    }

    inline void calculateBrowsingDataTableSizes(const ImVec2& childWindowSize=ImVec2(-1,-1))    {
        int approxNumEntriesPerColumn = 20;//(int) (20.f / browseSectionFontScale);// tweakable
        if (childWindowSize.y>0) {
            int numLinesThatFit = childWindowSize.y/ImGui::GetTextLineHeightWithSpacing();
            if (numLinesThatFit<=0) numLinesThatFit=1;
            approxNumEntriesPerColumn = numLinesThatFit;
            //static int tmp = 0;if (tmp!=numLinesThatFit) {tmp=numLinesThatFit;fprintf(stderr,"childWindowSize.y = %f numLinesThatFit=%d\n",childWindowSize.y,numLinesThatFit);}
        }
        numBrowsingColumns = totalNumBrowsingEntries/approxNumEntriesPerColumn;
        if (numBrowsingColumns<=0) {
            numBrowsingColumns = 1;numBrowsingEntriesPerColumn = approxNumEntriesPerColumn;
            return;
        }
        if (totalNumBrowsingEntries%approxNumEntriesPerColumn>(approxNumEntriesPerColumn/2)) ++numBrowsingColumns;
        int maxNumBrowsingColumns = (childWindowSize.x>0) ? (childWindowSize.x/100) : 6;
        if (maxNumBrowsingColumns<1) maxNumBrowsingColumns=1;
        if (numBrowsingColumns>maxNumBrowsingColumns) numBrowsingColumns = maxNumBrowsingColumns;
        numBrowsingEntriesPerColumn = totalNumBrowsingEntries/numBrowsingColumns;
        if (totalNumBrowsingEntries%numBrowsingColumns!=0) ++numBrowsingEntriesPerColumn;
    }

    // Just a convenience enum used internally
    enum Color {
        ImGuiCol_Dialog_Directory_Background,
        ImGuiCol_Dialog_Directory_Hover,
        ImGuiCol_Dialog_Directory_Pressed,
        ImGuiCol_Dialog_Directory_Text,

        ImGuiCol_Dialog_File_Background,
        ImGuiCol_Dialog_File_Hover,
        ImGuiCol_Dialog_File_Pressed,
        ImGuiCol_Dialog_File_Text,

        ImGuiCol_Dialog_SelectedFolder_Text,

        ImGuiCol_Dialog_ZipDirectory_Background,
        ImGuiCol_Dialog_ZipDirectory_Hover,
        ImGuiCol_Dialog_ZipDirectory_Pressed,
        ImGuiCol_Dialog_ZipDirectory_Text,

        ImGuiCol_Dialog_Size

    };
    inline static void ColorCombine(ImVec4& c,const ImVec4& r,const ImVec4& factor)   {
        const float rr = (r.x+r.y+r.z)*0.3334f;
        c.x = rr * factor.x;c.y = rr * factor.y;c.z = rr * factor.z;c.w = r.w;
    }
};
bool Internal::BrowsingPerRow = false;
// End Internal Usage-------------------------------------------------------------------------------------

bool Dialog::WrapMode = true;
ImVec2 Dialog::WindowSize(600,400);
ImVec4 Dialog::WindowLTRBOffsets(0,0,0,0);
ImGuiWindowFlags Dialog::ExtraWindowFlags = 0;
Dialog::DrawFileIconDelegate Dialog::DrawFileIconCallback=NULL;
Dialog::DrawFolderIconDelegate Dialog::DrawFolderIconCallback=NULL;


Dialog::Dialog(bool noKnownDirectoriesSection,bool noCreateDirectorySection,bool noFilteringSection,bool detectKnownDirectoriesAtEachOpening,bool addDisplayByOption,bool dontFilterSaveFilePathsEnteredByTheUser)    {
    internal = (Internal*) ImGui::MemAlloc(sizeof(Internal));
    IM_PLACEMENT_NEW(internal) Internal();

    internal->resetVariables();
    static int un = 0;
    internal->uniqueNumber = un++;

    internal->detectKnownDirectoriesAtEveryOpening = detectKnownDirectoriesAtEachOpening;
    internal->allowDisplayByOption = addDisplayByOption;
    internal->forbidDirectoryCreation = noCreateDirectorySection;
    internal->allowKnownDirectoriesSection = !noKnownDirectoriesSection;
    internal->allowFiltering = !noFilteringSection;
    internal->mustFilterSaveFilePathWithFileFilterExtensionString = !dontFilterSaveFilePathsEnteredByTheUser;
}
Dialog::~Dialog()   {
    if (internal) {
	internal->~Internal();
	ImGui::MemFree(internal);
	internal = NULL;
    }
}
const char* Dialog::getChosenPath() const {return internal->chosenPath;}
const char* Dialog::getLastDirectory() const {return internal->currentFolder;}
bool Dialog::hasUserJustCancelledDialog() const {return internal->userHasJustCancelledDialog;}

// -- from imgui.cpp --
static size_t ImFormatString(char* buf, size_t buf_size, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int w = vsnprintf(buf, buf_size, fmt, args);
    va_end(args);
    buf[buf_size-1] = 0;
    return (w == -1) ? buf_size : (size_t)w;
}
// ---------------------

// 90% of the functionality of the whole imguifilesystem.cpp is inside this single method
const char* ChooseFileMainMethod(Dialog& ist,const char* directory,const bool _isFolderChooserDialog,const bool _isSaveFileDialog,const char* _saveFileName,const char* fileFilterExtensionString,const char* windowTitle,const ImVec2& windowSize,const ImVec2& windowPos,const float windowAlpha) {
    //-----------------------------------------------------------------------------
    Internal& I = *ist.internal;
    char* rv = I.chosenPath;rv[0] = '\0';
    //-----------------------------------------------------
    bool& isSelectFolderDialog = I.isSelectFolderDialog = _isFolderChooserDialog;
    bool& isSaveFileDialog = I.isSaveFileDialog = _isSaveFileDialog;

    bool& allowDirectoryCreation = I.allowDirectoryCreation = I.forbidDirectoryCreation ? false : (isSelectFolderDialog || isSaveFileDialog);
    //----------------------------------------------------------
    static const int* pNumberKnownUserDirectoriesExceptDrives=NULL;
    static const FilenameStringVector* pUserKnownDirectoryDisplayNames=NULL;
    static const PathStringVector* pUserKnownDirectories = &Directory::GetUserKnownDirectories(&pUserKnownDirectoryDisplayNames,&pNumberKnownUserDirectoriesExceptDrives);
    //----------------------------------------------------------
    const ImGuiStyle& style = ImGui::GetStyle();
    ImVec4 dummyButtonColor(0.0f,0.0f,0.0f,0.5f);     // Only the alpha is twickable from here
    ImVec4 dummyZipButtonColor(0.0f,0.0f,0.0f,0.5f);     // Only the alpha is twickable from here
    static ImVec4 ColorSet[Internal::ImGuiCol_Dialog_Size];
    // Fill ColorSet above and fix dummyButtonColor here
    {
        static const ImVec4 df(0.9,0.9,0.3,0.65);          // directory color factor
        static const ImVec4 ff(0.7,0.7,0.7,0.65);          // file color factor
        static const ImVec4 zdf(1.5,0.8,0.8,0.65);          // zip directory color factor

        for (int i=0,sz=(int)Internal::ImGuiCol_Dialog_Directory_Text;i<=sz;i++)    {
            ImVec4& c = ColorSet[i];
            const ImVec4& r = style.Colors[i<sz ? ((int)ImGuiCol_Button + i) : ImGuiCol_Text];
            Internal::ColorCombine(c,r,df);
            if (i<sz) c.w *= df.w;
        }
        for (int i=(int)Internal::ImGuiCol_Dialog_File_Background,sz=(int)Internal::ImGuiCol_Dialog_File_Text;i<=sz;i++)    {
            ImVec4& c = ColorSet[i];
            const ImVec4& r = style.Colors[i<sz ? ((int)ImGuiCol_Button-(int)Internal::ImGuiCol_Dialog_File_Background + i) : ImGuiCol_Text];
            Internal::ColorCombine(c,r,ff);
            if (i<sz) c.w *= ff.w;
        }
        for (int i=(int)Internal::ImGuiCol_Dialog_ZipDirectory_Background,sz=(int)Internal::ImGuiCol_Dialog_ZipDirectory_Text;i<=sz;i++)    {
            ImVec4& c = ColorSet[i];
            const ImVec4& r = style.Colors[i<sz ? ((int)ImGuiCol_Button + i) : ImGuiCol_Text];
            Internal::ColorCombine(c,r,zdf);
            if (c.x>1.f) c.x=1.f;if (c.y>1.f) c.y=1.f;if (c.z>1.f) c.z=1.f;if (c.w>1.f) c.w=1.f;
            if (i<sz) c.w *= zdf.w;
        }
        if (dummyButtonColor.w>0)   {
            const ImVec4& bbc = style.Colors[ImGuiCol_Button];
            dummyButtonColor.x = bbc.x;dummyButtonColor.y = bbc.y;dummyButtonColor.z = bbc.z;dummyButtonColor.w *= bbc.w;
        }
        if (dummyZipButtonColor.w>0)   {
            const ImVec4& bbc = ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Background];
            dummyZipButtonColor.x = bbc.x;dummyZipButtonColor.y = bbc.y;dummyZipButtonColor.z = bbc.z;dummyZipButtonColor.w *= bbc.w;
        }
    }

    if (I.rescan)   {
        char validDirectory[MAX_PATH_BYTES];validDirectory[0]='\0';   // for robustness
#       ifndef IMGUI_USE_MINIZIP
        if (directory && strlen(directory)>0) {
            if (Directory::Exists(directory)) strcpy(validDirectory,directory);
            else {
                Path::GetDirectoryName(directory,validDirectory);
                if (!Directory::Exists(validDirectory)) validDirectory[0]='\0';
            }
        }
        Path::GetAbsolutePath(validDirectory,I.currentFolder);
#       else //IMGUI_USE_MINIZIP
        char basePath[MAX_PATH_BYTES];basePath[0]='\0';char zipPath[MAX_PATH_BYTES];zipPath[0]='\0';
        bool isInsideZipFile = false;
        if (directory && strlen(directory)>0) isInsideZipFile = PathSplitFirstZipFolder(directory,basePath,zipPath);
        else isInsideZipFile = PathSplitFirstZipFolder(I.currentFolder,basePath,zipPath);
        if (isInsideZipFile ? FileExists(basePath) : Directory::Exists(basePath)) {
            strcpy(validDirectory,basePath);
            if (zipPath[0]!='\0') {
                char zipPathDir[MAX_PATH_BYTES];zipPathDir[0]='\0';
                Path::GetDirectoryName(zipPath,zipPathDir);
                Path::Append(zipPathDir,validDirectory);
            }
            //fprintf(stderr,"basePath=%s zipPath=%s validDirectory=%s\n",basePath,zipPath,validDirectory);
        }
        else {
            Path::GetDirectoryName(directory,validDirectory);
            if (!Directory::Exists(validDirectory)) validDirectory[0]='\0';
        }
        strcpy(I.currentFolder,validDirectory);
        //fprintf(stderr,"%s %d\n",I.currentFolder,isInsideZipFile);
#       endif //IMGUI_USE_MINIZIP

        I.editLocationCheckButtonPressed = false;
        I.history.reset(); // reset history
        I.history.switchTo(I.currentFolder);    // init history
    I.dirs.clear();I.files.clear();I.dirNames.clear();I.fileNames.clear();I.currentSplitPath.clear();I.fileExtensionTypes.clear();
        strcpy(&I.newDirectoryName[0],"New Folder");
        if (_saveFileName) {
            //strcpy(&I.saveFileName[0],_saveFileName);
            Path::GetFileName(_saveFileName,I.saveFileName);    // Better!
        }
        else I.saveFileName[0]='\0';
        isSelectFolderDialog = _isFolderChooserDialog;
        isSaveFileDialog = _isSaveFileDialog;
        allowDirectoryCreation =  I.forbidDirectoryCreation ? false : (isSelectFolderDialog || isSaveFileDialog);
        if (isSelectFolderDialog && I.sortingMode>SORT_ORDER_LAST_MODIFICATION_INVERSE) I.sortingMode = 0;
        I.forceRescan = true;
        I.open = true;
        I.filter.Clear();
        if (!windowTitle || strlen(windowTitle)==0)  {
            if (isSelectFolderDialog)   strcpy(I.wndTitle,"Please select a folder");
            else if (isSaveFileDialog)  strcpy(I.wndTitle,"Please choose/create a file for saving");
            else                        strcpy(I.wndTitle,"Please choose a file");
        }
        else                            strcpy(I.wndTitle,windowTitle);
	strcat(I.wndTitle,"##");
        char tmpWndTitleNumber[12];
        ImFormatString(tmpWndTitleNumber,11,"%d",I.uniqueNumber);
	strcat(I.wndTitle,tmpWndTitleNumber);
        I.wndPos = windowPos;
        I.wndSize = windowSize;
	if (I.wndSize.x<=0) I.wndSize.x = Dialog::WindowSize.x;
	if (I.wndSize.y<=0) I.wndSize.y = Dialog::WindowSize.y;
        const ImVec2 mousePos = ImGui::GetMousePos();//
        ImGui::GetCursorPos();
        if (I.wndPos.x<=0)  I.wndPos.x = mousePos.x - I.wndSize.x*0.5f;
        if (I.wndPos.y<=0)  I.wndPos.y = mousePos.y - I.wndSize.y*0.5f;
        const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    if (I.wndPos.x>screenSize.x-I.wndSize.x-Dialog::WindowLTRBOffsets.z) I.wndPos.x = screenSize.x-I.wndSize.x-Dialog::WindowLTRBOffsets.z;
    if (I.wndPos.y>screenSize.y-I.wndSize.y-Dialog::WindowLTRBOffsets.w) I.wndPos.y = screenSize.y-I.wndSize.y-Dialog::WindowLTRBOffsets.w;
	if (I.wndPos.x < Dialog::WindowLTRBOffsets.x) I.wndPos.x = Dialog::WindowLTRBOffsets.x;
	if (I.wndPos.y < Dialog::WindowLTRBOffsets.y) I.wndPos.y = Dialog::WindowLTRBOffsets.y;
        //fprintf(stderr,"screenSize = %f,%f mousePos = %f,%f wndPos = %f,%f wndSize = %f,%f\n",screenSize.x,screenSize.y,mousePos.x,mousePos.y,wndPos.x,wndPos.y,wndSize.x,wndSize.y);
        if (I.detectKnownDirectoriesAtEveryOpening) pUserKnownDirectories = &Directory::GetUserKnownDirectories(&pUserKnownDirectoryDisplayNames,&pNumberKnownUserDirectoriesExceptDrives,true);

        //fprintf(stderr,"Pos: %1.0f %1.0f Size: %1.0f %1.0f\n",I.wndPos.x,I.wndPos.y,I.wndSize.x,I.wndSize.y);
    }
    if (!I.open) {
        // This case (user clicks the close button) does not seem to be triggered here...
        // No matter, I just handle it in the 3 caller methods, where it's triggered.
        return rv;
    }


    if (I.forceRescan)    {
        I.forceRescan = false;
#       ifndef IMGUI_USE_MINIZIP
        const int sortingModeForDirectories = (I.sortingMode <= (int)SORT_ORDER_LAST_MODIFICATION_INVERSE) ? I.sortingMode : (I.sortingMode%2);
        Directory::GetDirectories(I.currentFolder,I.dirs,&I.dirNames,(Sorting)sortingModeForDirectories);  // this is because directories don't return their size or their file extensions (so if needed we sort them alphabetically)

        if (!isSelectFolderDialog)  {
            if (!fileFilterExtensionString || strlen(fileFilterExtensionString)==0) Directory::GetFiles(I.currentFolder,I.files,&I.fileNames,(Sorting)I.sortingMode);
            else                                        Directory::GetFiles(I.currentFolder,I.files,fileFilterExtensionString,NULL,&I.fileNames,(Sorting)I.sortingMode);
            if (Dialog::DrawFileIconCallback) MyImGuiFsDrawIconStruct().fillExtensionTypesFromFilenames(I.fileExtensionTypes,I.fileNames);
        }
        else {
            I.files.clear();I.fileNames.clear();I.fileExtensionTypes.clear();
            I.saveFileName[0]='\0';
            char currentFolderName[MAX_FILENAME_BYTES];
            Path::GetFileName(I.currentFolder,currentFolderName);
            const size_t currentFolderNameSize = strlen(currentFolderName);
            if (currentFolderNameSize==0 || currentFolderName[currentFolderNameSize-1]==':') strcat(currentFolderName,"/");
            strcat(I.saveFileName,currentFolderName);
        }
#       else //IMGUI_USE_MINIZIP
        const int sortingModeForDirectories = (I.sortingMode <= (int)SORT_ORDER_LAST_MODIFICATION_INVERSE) ? I.sortingMode : (I.sortingMode%2);
        char basePath[MAX_PATH_BYTES];basePath[0]='\0';char zipPath[MAX_PATH_BYTES];zipPath[0]='\0';
        const bool isInsideZipFile = PathSplitFirstZipFolder(I.currentFolder,basePath,zipPath);
        if (!isInsideZipFile) Directory::GetDirectories(basePath,I.dirs,&I.dirNames,(Sorting)sortingModeForDirectories);  // this is because directories don't return their size or their file extensions (so if needed we sort them alphabetically)
        else {
            if (I.unz.load(basePath,false))  I.unz.getDirectories(zipPath,I.dirs,&I.dirNames,(Sorting)sortingModeForDirectories,true);
            else I.unz.close();
        }

        if (!isSelectFolderDialog)  {
            if (!isInsideZipFile)   {
                if (!fileFilterExtensionString || strlen(fileFilterExtensionString)==0) Directory::GetFiles(I.currentFolder,I.files,&I.fileNames,(Sorting)I.sortingMode);
                else                                        Directory::GetFiles(I.currentFolder,I.files,fileFilterExtensionString,NULL,&I.fileNames,(Sorting)I.sortingMode);
                if (Dialog::DrawFileIconCallback) MyImGuiFsDrawIconStruct.fillExtensionTypesFromFilenames(I.fileExtensionTypes,I.fileNames);
            }
            else if (I.unz.isValid()) {
                /*if (!fileFilterExtensionString || strlen(fileFilterExtensionString)==0)*/ I.unz.getFiles(zipPath,I.files,&I.fileNames,(Sorting)I.sortingMode,true);
                //else                                        I.unz.getFiles(zipPath,I.files,fileFilterExtensionString,NULL,&I.fileNames,(Sorting)I.sortingMode,true);
            }
       }
        else {
            I.files.clear();I.fileNames.clear();
            I.saveFileName[0]='\0';
            char currentFolderName[MAX_FILENAME_BYTES];
            Path::GetFileName(I.currentFolder,currentFolderName);
            const size_t currentFolderNameSize = strlen(currentFolderName);
            if (currentFolderNameSize==0 || currentFolderName[currentFolderNameSize-1]==':') strcat(currentFolderName,"/");
            strcat(I.saveFileName,currentFolderName);
        }
#       endif //IMGUI_USE_MINIZIP

        I.history.getCurrentSplitPath(I.currentSplitPath);
        I.totalNumBrowsingEntries = (int)(I.dirs.size()+I.files.size());

        //I.calculateBrowsingDataTableSizes();    // can we move it down ?

        //#       define DEBUG_HISTORY
#       ifdef DEBUG_HISTORY
        if (I.history.getInfoSize()>0) fprintf(stderr,"\nHISTORY: currentFolder:\"%s\" history.canGoBack=%s history.canGoForward=%s currentHistory:\n",I.currentFolder,I.history.canGoBack()?"true":"false",I.history.canGoForward()?"true":"false");
        if (I.history.getCurrentFolderInfo()) I.history.getCurrentFolderInfo()->display();
#       endif //DEBUG_HISTORY
    }

    if (I.rescan) {
        I.rescan = false; // Mandatory
        ImGui::OpenPopup(I.wndTitle);
        ImGui::SetNextWindowPos(I.wndPos);ImGui::SetNextWindowSize(I.wndSize);
        const bool popupOk = ImGui::BeginPopupModal(I.wndTitle, &I.open,Dialog::ExtraWindowFlags);
        if (!popupOk) return rv;
    }
    else {
        if (I.forceSetWindowPositionAndSize) {
            I.forceSetWindowPositionAndSize = false;
            ImGui::SetNextWindowPos(I.wndPos);
            ImGui::SetNextWindowSize(I.wndSize);
        }
        const bool popupOk = ImGui::BeginPopupModal(I.wndTitle, &I.open,Dialog::ExtraWindowFlags);
        if (!popupOk) return rv;
    }
    ImGui::Separator();

    //------------------------------------------------------------------------------------
    // History (=buttons: < and >)
    {
        bool historyBackClicked = false;
        bool historyForwardClicked = false;

        // history -----------------------------------------------
        ImGui::PushID("historyDirectoriesID");

        const bool historyCanGoBack = I.history.canGoBack();
        const bool historyCanGoForward = I.history.canGoForward();

        if (!historyCanGoBack) {
            ImGui::PushStyleColor(ImGuiCol_Button,dummyButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,dummyButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,dummyButtonColor);
        }
        historyBackClicked = ImGui::Button("<")&historyCanGoBack;
        ImGui::SameLine();
        if (!historyCanGoBack) {
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }

        if (!historyCanGoForward) {
            ImGui::PushStyleColor(ImGuiCol_Button,dummyButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,dummyButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,dummyButtonColor);
        }
        historyForwardClicked = ImGui::Button(">")&historyCanGoForward;
        ImGui::SameLine();
        if (!historyCanGoForward) {
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }

        ImGui::PopID();
        // -------------------------------------------------------

        if (historyBackClicked || historyForwardClicked)    {
            ImGui::EndPopup();

            if (historyBackClicked)         I.history.goBack();
            else if (historyForwardClicked) I.history.goForward();

            I.forceRescan = true;

            strcpy(I.currentFolder,I.history.getCurrentFolder());
            strcpy(I.editLocationInputText,I.currentFolder);

#       ifdef DEBUG_HISTORY
            if (historyBackClicked) fprintf(stderr,"\nPressed BACK to\t");
            else fprintf(stderr,"\nPressed FORWARD to\t");
            fprintf(stderr,"\"%s\" (%d)\n",I.currentFolder,(int)*I.history.getCurrentSplitPathIndex());
#       undef DEBUG_HISTOTY
#       endif //DEBUG_HISTORY
            return rv;
        }
    }
    //------------------------------------------------------------------------------------
    const FolderInfo* fi = I.history.getCurrentFolderInfo();
    const bool isBrowsingInsideZipFile = (fi && (!(fi->splitPathIndexOfZipFile<0 || fi->splitPathIndexOfZipFile>fi->splitPathIndex)));

    // Edit Location CheckButton
    bool editLocationInputTextReturnPressed = false;
    {

        bool mustValidateInputPath = false;
        ImGui::PushStyleColor(ImGuiCol_Button,I.editLocationCheckButtonPressed ? dummyButtonColor : style.Colors[ImGuiCol_Button]);

        if (ImGui::Button("L##EditLocationCheckButton"))    {
            I.editLocationCheckButtonPressed = !I.editLocationCheckButtonPressed;
            //fprintf(stderr,"I.editLocationCheckButtonPressed=%s\n",I.editLocationCheckButtonPressed ? "true" : "false");
            if (I.editLocationCheckButtonPressed) {
                strcpy(I.editLocationInputText,I.currentFolder);
                ImGui::SetKeyboardFocusHere();
            }
            //if (!I.editLocationCheckButtonPressed) mustValidateInputPath = true;   // or not ? I mean: the user wants to quit or to validate in this case ?
            }

        ImGui::PopStyleColor();

        if (I.editLocationCheckButtonPressed) {
            ImGui::SameLine();
            editLocationInputTextReturnPressed = ImGui::InputText("##EditLocationInputText",I.editLocationInputText,MAX_PATH_BYTES,ImGuiInputTextFlags_AutoSelectAll|ImGuiInputTextFlags_EnterReturnsTrue);
            if (editLocationInputTextReturnPressed)  mustValidateInputPath = true;
            else ImGui::Separator();
        }

        if (mustValidateInputPath)  {
            // it's better to clean the input path here from trailing slashes:
            char cleanEnteredPath[MAX_PATH_BYTES];
            strcpy(cleanEnteredPath,I.editLocationInputText);
            size_t len = strlen(cleanEnteredPath);
            while (len>0 && (cleanEnteredPath[len-1]=='/' || cleanEnteredPath[len-1]=='\\')) {cleanEnteredPath[len-1]='\0';len = strlen(cleanEnteredPath);}

            if (len==0 || strcmp(I.currentFolder,cleanEnteredPath)==0)    I.editLocationCheckButtonPressed = false;
#           ifndef IMGUI_USE_MINIZIP
            else if (Directory::Exists(cleanEnteredPath))   {
                I.editLocationCheckButtonPressed = false; // Optional (return to split-path buttons)
                //----------------------------------------------------------------------------------
                I.history.switchTo(cleanEnteredPath);
                strcpy(I.currentFolder,cleanEnteredPath);
                I.forceRescan = true;
            }
#           else //IMGUI_USE_MINIZIP
            else {
                bool isInsideZipFile = false;
                char basePath[MAX_PATH_BYTES];basePath[0]='\0';char zipPath[MAX_PATH_BYTES];zipPath[0]='\0';
                isInsideZipFile = PathSplitFirstZipFolder(cleanEnteredPath,basePath,zipPath,false);
                if (!isInsideZipFile)   {
                    if (Directory::Exists(cleanEnteredPath))   {
                        I.editLocationCheckButtonPressed = false; // Optional (return to split-path buttons)
                        //----------------------------------------------------------------------------------
                        I.history.switchTo(cleanEnteredPath);
                        strcpy(I.currentFolder,cleanEnteredPath);
                        I.forceRescan = true;
                    }
                }
                else {
                    bool dirExists = false;
                    if (strcmp(I.unz.getZipFilePath(),basePath)==0) dirExists = I.unz.directoryExists(zipPath);
                    else {
                        UnZipFile unz(basePath);
                        dirExists = unz.directoryExists(zipPath);
                    }
                    if (dirExists) {
                        I.editLocationCheckButtonPressed = false; // Optional (return to split-path buttons)
                        //----------------------------------------------------------------------------------
                        I.history.switchTo(cleanEnteredPath);
                        strcpy(I.currentFolder,cleanEnteredPath);
                        I.forceRescan = true;
                    }
                }
            }
#           endif //IMGUI_USE_MINIZIP
            //else fprintf(stderr,"mustValidateInputPath NOOP: \"%s\" \"%s\"\n",I.currentFolder,cleanEnteredPath);
        }
        else ImGui::SameLine();

    }
    //------------------------------------------------------------------------------------
    // Split Path control
    if (!I.editLocationCheckButtonPressed && !editLocationInputTextReturnPressed) {
        bool mustSwitchSplitPath = false;
        const FolderInfo& fi = *I.history.getCurrentFolderInfo();

        ImVec2& framePadding = ImGui::GetStyle().FramePadding;
        const float originalFramePaddingX = framePadding.x;
        framePadding.x = 0;

        // Split Path
        // Tab:
        {
            //-----------------------------------------------------
            // TAB LABELS
            //-----------------------------------------------------
            {
                const int numTabs=(int) I.currentSplitPath.size();
                int newSelectedTab = fi.splitPathIndex;

                /* // DEBUG--To remove--
                static int tmpzfi = -2;
                if (fi.splitPathIndexOfZipFile!=tmpzfi) {
                    tmpzfi = fi.splitPathIndexOfZipFile;
                    if (fi.splitPathIndexOfZipFile!=-1) fprintf(stderr,"%d %s\n",fi.splitPathIndexOfZipFile,I.currentSplitPath[fi.splitPathIndexOfZipFile]);
                    else   fprintf(stderr,"%d\n",fi.splitPathIndexOfZipFile);
                }*/
                //-------------------
		const bool wrapMode = Dialog::WrapMode;
                float windowWidth = -1;float sumX = 0;
                if (wrapMode) {
                    sumX+=ImGui::GetCursorPosX();
                    windowWidth=ImGui::GetWindowWidth()-ImGui::GetStyle().WindowPadding.x;
                }
                for (int t=0;t<numTabs;t++)	{
                    if (t==fi.splitPathIndex) {
                        const ImVec4* pDummyButtonColor = &dummyButtonColor;
#                       ifdef IMGUI_USE_MINIZIP
                        if (t==fi.splitPathIndexOfZipFile) pDummyButtonColor = &dummyZipButtonColor;
#                       endif // IMGUI_USE_MINIZIP
                        ImGui::PushStyleColor(ImGuiCol_Button,*pDummyButtonColor);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,*pDummyButtonColor);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,*pDummyButtonColor);
                    }
#                   ifdef IMGUI_USE_MINIZIP
                    else if (t==fi.splitPathIndexOfZipFile) {
                        ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Background]);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Hover]);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Pressed]);
                    }
#                   endif // IMGUI_USE_MINIZIP
#                   ifndef _MSC_VER
                    ImGui::PushID(&I.currentSplitPath[t]);    // old cl.exe versions don't like this
#                   else //_MSC_VER
                    ImGui::PushID(t);                         // will this work ?
#                   endif //_MSC_VER
                    if (wrapMode) {
                        sumX+=ImGui::CalcTextSize(I.currentSplitPath[t]).x;
                        sumX+= 2.*ImGui::GetStyle().FramePadding.x;    // needed ?
                        if (sumX >= windowWidth) sumX=0;
                        if (t!=0 && sumX>0) ImGui::SameLine(0,0);
                    }
                    else if (t>0) ImGui::SameLine(0,0);
                    const bool pressed = ImGui::Button(I.currentSplitPath[t]);
                    if (wrapMode && sumX==0)    {sumX = ImGui::GetStyle().WindowPadding.x + ImGui::GetItemRectSize().x;}
                    ImGui::PopID();
                    if (pressed) {
                        if (fi.splitPathIndex!=t && !mustSwitchSplitPath) mustSwitchSplitPath = true;
                        newSelectedTab = t;
                    }
                    if (t==fi.splitPathIndex) {
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                    }
#                   ifdef IMGUI_USE_MINIZIP
                    else if (t==fi.splitPathIndexOfZipFile) {
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleColor();
                    }
#                   endif // IMGUI_USE_MINIZIP
                }
                if (mustSwitchSplitPath) {
		    FolderInfo mfi;
                    fi.getFolderInfoForSplitPathIndex(newSelectedTab,mfi);
                    I.history.switchTo(mfi);
                    I.forceRescan = true;
                    strcpy(I.currentFolder,I.history.getCurrentFolder());
                    strcpy(I.editLocationInputText,I.currentFolder);
                    //fprintf(stderr,"%s\n",I.currentFolder);
                }
            }
        }

        framePadding.x = originalFramePaddingX;
    }
    //------------------------------------------------------------------------------------


    // Start collapsable regions----------------------------------------------------------
    // User Known directories-------------------------------------------------------------
    if (I.allowKnownDirectoriesSection && pUserKnownDirectories->size()>0)  {
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Known Directories##imguifs_UserKnownDirectories"))  {
            static int id;
            ImGui::PushID(&id);

            ImGui::PushStyleColor(ImGuiCol_Text,ColorSet[Internal::ImGuiCol_Dialog_Directory_Text]);
            ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_Directory_Background]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_Directory_Hover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_Directory_Pressed]);
            //const ImVec4& iconsColor = ColorSet[Internal::ImGuiCol_Dialog_Directory_Text];

            for (int i=0,sz=(int)pUserKnownDirectories->size();i<sz;i++)  {
                const char* userKnownFolder = (*pUserKnownDirectories)[i];
                const char* userKnownFolderDisplayName = (*pUserKnownDirectoryDisplayNames)[i];
                //if (Dialog::DrawFolderIconCallback && Dialog::DrawFolderIconCallback(false,&iconsColor)) ImGui::SameLine();
                if (ImGui::SmallButton(userKnownFolderDisplayName) && strcmp(userKnownFolder,I.currentFolder)!=0) {
                    strcpy(I.currentFolder,userKnownFolder);
                    strcpy(I.editLocationInputText,I.currentFolder);
                    I.history.switchTo(I.currentFolder);
                    I.forceRescan = true;
                    //------------------------------------------------------------------------------------------------------------------------------
                }
                if (i!=sz-1 && (i>=*pNumberKnownUserDirectoriesExceptDrives || i%7!=6)) ImGui::SameLine();
            }

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            ImGui::PopID();
        }

    }
    // End User Known directories---------------------------------------------------------
    // Allow directory creation ----------------------------------------------------------
    if (allowDirectoryCreation && !isBrowsingInsideZipFile) {
        ImGui::Separator();
        bool mustCreate = false;

        if (ImGui::CollapsingHeader("New Directory##imguifs_allowDirectoryCreation"))  {
            static int id;
            ImGui::PushID(&id);

            ImGui::InputText("##createNewFolderName",&I.newDirectoryName[0],MAX_FILENAME_BYTES);
            ImGui::SameLine();
            mustCreate = ImGui::Button("CREATE");

            ImGui::PopID();
        }

        if (mustCreate && strlen(I.newDirectoryName)>0)    {
            char newDirPath[MAX_PATH_BYTES];
            Path::Combine(I.currentFolder,I.newDirectoryName,newDirPath,false);
            if (!Directory::Exists(newDirPath)) {
                //#           define SIMULATING_ONLY
#           ifdef SIMULATING_ONLY
                fprintf(stderr,"creating: \"%s\"\n",newDirPath);
#           undef SIMULATING_ONLY
#           else //SIMULATING_ONLY
                Directory::Create(newDirPath);
                if (!Directory::Exists(newDirPath)) fprintf(stderr,"Error creating new folder: \"%s\"\n",newDirPath);
                else I.forceRescan = true;   // Just update
#           endif //SIMULATING_ONLY
            }
        }
    }
    // End allow directory creation ------------------------------------------------------
    // Filtering entries -----------------------------------------------------------------
    if (I.allowFiltering)  {
        ImGui::Separator();
        if (ImGui::CollapsingHeader("Filtering##imguifs_fileNameFiltering"))  {
            static int id;
            ImGui::PushID(&id);
            I.filter.Draw();
            ImGui::PopID();
        }

    }
    // End filtering entries -------------------------------------------------------------
    // End collapsable regions------------------------------------------------------------

    // Reserve space for selection field------------------------------------------
    //----------------------------------------------------------------------------

    ImGui::Separator();
    // sorting --------------------------------------------------------------------
    ImGui::Text("Sorting by: ");ImGui::SameLine();
    {
        const int oldSortingMode = I.sortingMode;
        const int oldSelectedTab = I.sortingMode/2;
        //-----------------------------------------------------
        // TAB LABELS
        //-----------------------------------------------------
        {
            static const int numTabs=(int)SORT_ORDER_COUNT/2;
            int newSortingMode = oldSortingMode;
            static const char* names[numTabs] = {"Name","Modified","Size","Type"};
            const int numUsedTabs = isSelectFolderDialog ? 2 : numTabs;
            for (int t=0;t<numUsedTabs;t++)	{
                if (t>0) ImGui::SameLine();
                if (t==oldSelectedTab) {
                    ImGui::PushStyleColor(ImGuiCol_Button,dummyButtonColor);
                }
                ImGui::PushID(&names[t]);
                const bool pressed = ImGui::SmallButton(names[t]);
                ImGui::PopID();
                if (pressed) {
                    if (oldSelectedTab==t) {
                        newSortingMode = oldSortingMode;
                        if (newSortingMode%2==0) ++newSortingMode;// 0,2,4
                        else --newSortingMode;
                    }
                    else newSortingMode = t*2;
                }
                if (t==oldSelectedTab) {
                    ImGui::PopStyleColor();
                }
            }

            if (newSortingMode!=oldSortingMode) {
                I.sortingMode = newSortingMode;
                //printf("sortingMode = %d\n",sortingMode);
                I.forceRescan = true;
            }

            //-- Browsing per row -----------------------------------
            if (I.allowDisplayByOption && I.numBrowsingColumns>1)   {
                ImGui::SameLine();
                ImGui::Text("   Display by:");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button,dummyButtonColor);
                if (ImGui::SmallButton(!Internal::BrowsingPerRow ? "Column##browsingPerRow" : "Row##browsingPerRow"))   {
                    Internal::BrowsingPerRow = !Internal::BrowsingPerRow;
                }
                ImGui::PopStyleColor();
            }
            //-- End browsing per row -------------------------------
        }
    }
    //-----------------------------------------------------------------------------
    ImGui::Separator();

    //-----------------------------------------------------------------------------
    // MAIN BROWSING FRAME:
    //-----------------------------------------------------------------------------
    {
        ImGui::BeginChild("BrowsingFrame",ImVec2(0,(isSaveFileDialog || isSelectFolderDialog)?(ImGui::GetContentRegionAvail().y-1.2f*ImGui::GetTextLineHeightWithSpacing()-ImGui::GetStyle().WindowPadding.y):0));

        I.calculateBrowsingDataTableSizes(ImGui::GetWindowSize());  // (Actually we could save this call every frame, but it's difficult to detect whn we need it)

        // ImGui::SetScrollPosHere();   // possible future ref: while drawing to place the scroll bar making a certain entry visible
        // Also we don't do any maual clipping here (for long directories using a clipper might be useful)

        ImGui::Columns(I.numBrowsingColumns);

        static int id;
        ImGui::PushID(&id);
        int cntEntries = 0;
        // Directories --------------------------------------------------------------
        if (I.dirs.size()>0)   {
            ImGui::PushStyleColor(ImGuiCol_Text,ColorSet[Internal::ImGuiCol_Dialog_Directory_Text]);
            ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_Directory_Background]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_Directory_Hover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_Directory_Pressed]);
            const ImVec4& iconsColor = ColorSet[Internal::ImGuiCol_Dialog_Directory_Text];

            for (int i=0,sz=(int)I.dirs.size();i<sz;i++) {
                const char* dirName = &I.dirNames[i][0];
                if (I.filter.PassFilter(dirName)) {
                    if (Dialog::DrawFolderIconCallback && Dialog::DrawFolderIconCallback(false,&iconsColor)) ImGui::SameLine();
                    if (ImGui::SmallButton(dirName)) {
                        strcpy(I.currentFolder,I.dirs[i]);
                        strcpy(I.editLocationInputText,I.currentFolder);
                        I.history.switchTo(I.currentFolder);
                        I.forceRescan = true;
                        //------------------------------------------------------------------------------------------------------------------------------
                    }
                    ++cntEntries;
                    if (Internal::BrowsingPerRow) ImGui::NextColumn();
                    else if (cntEntries==I.numBrowsingEntriesPerColumn) {
                        cntEntries = 0;
                        ImGui::NextColumn();
                    }
                }
            }

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }
        // Files ----------------------------------------------------------------------
        if (!isSelectFolderDialog && I.files.size()>0)   {
            ImGui::PushStyleColor(ImGuiCol_Text,ColorSet[Internal::ImGuiCol_Dialog_File_Text]);
            ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_File_Background]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_File_Hover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_File_Pressed]);
            const ImVec4* pIconsColor = &ColorSet[Internal::ImGuiCol_Dialog_File_Text];

#           ifdef IMGUI_USE_MINIZIP
            //const FolderInfo* fi = I.history.getCurrentFolderInfo();
            const bool acceptZipFilesForBrowsing = !isBrowsingInsideZipFile;//(fi && (fi->splitPathIndexOfZipFile<0 || fi->splitPathIndexOfZipFile>fi->splitPathIndex));
            bool isZipFile = false;bool hasZipExtension = false;

            /* // DEBUG-- TO DELETE --
            static bool old = true;
            if (old!=acceptZipFilesForBrowsing) {
                old = acceptZipFilesForBrowsing;
                fprintf(stderr,"acceptZipFilesForBrowsing=%s %d %d\n",acceptZipFilesForBrowsing?"true":"false",fi->splitPathIndexOfZipFile,fi->splitPathIndex);
            }*/        
#           endif //IMGUI_USE_MINIZIP	    

            for (int i=0,sz=(int)I.files.size();i<sz;i++) {
                const char* fileName = &I.fileNames[i][0];
                if (I.filter.PassFilter(fileName)) {
#                   ifdef IMGUI_USE_MINIZIP
                    if (acceptZipFilesForBrowsing)  {
                        hasZipExtension = Path::HasZipExtension(fileName);
                        if (hasZipExtension && !isZipFile)  {
                            ImGui::PopStyleColor(4);
                            ImGui::PushStyleColor(ImGuiCol_Text,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Text]);
                            ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Background]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Hover]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Pressed]);
                            pIconsColor = &ColorSet[Internal::ImGuiCol_Dialog_ZipDirectory_Text];
                        }
                        else if (!hasZipExtension && isZipFile) {
                            ImGui::PopStyleColor(4);
                            ImGui::PushStyleColor(ImGuiCol_Text,ColorSet[Internal::ImGuiCol_Dialog_File_Text]);
                            ImGui::PushStyleColor(ImGuiCol_Button,ColorSet[Internal::ImGuiCol_Dialog_File_Background]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ColorSet[Internal::ImGuiCol_Dialog_File_Hover]);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ColorSet[Internal::ImGuiCol_Dialog_File_Pressed]);
                            pIconsColor = &ColorSet[Internal::ImGuiCol_Dialog_File_Text];
                        }
                        isZipFile = hasZipExtension;
                    }
#                   endif //IMGUI_USE_MINIZIP
                    if (Dialog::DrawFileIconCallback && Dialog::DrawFileIconCallback(I.fileExtensionTypes[i],pIconsColor)) ImGui::SameLine();
                    if (ImGui::SmallButton(fileName)) {
                        if (!isSaveFileDialog)  {
                            strcpy(rv,I.files[i]);
                            I.open = true;
                        }
                        else {
                            Path::GetFileName(I.files[i],I.saveFileName);
                        }
                    }
#                   if (defined(__EMSCRIPTEN__) && defined(EMSCRIPTEN_SAVE_SHELL))
                    static char tmpString[ImGuiFs::MAX_PATH_BYTES*2]="";
#                       ifndef IMGUI_USE_MINIZIP
                        if (ImGui::IsItemHovered() && ImGui::GetIO().KeyCtrl)   {
                            ImGui::SetTooltip("%s","right click to download this file locally");
                            if (ImGui::GetIO().MouseClicked[1]) {
                                strcpy(tmpString,"saveFileFromMemoryFSToDisk('");
                                strcat(tmpString,I.files[i]);strcat(tmpString,"','");
                                strcat(tmpString,fileName);strcat(tmpString,"')");

                                emscripten_run_script(tmpString);
                            }
                        }
#                       else // IMGUI_USE_MINIZIP
                        if (ImGui::IsItemHovered())   {
                            if (!isBrowsingInsideZipFile && ImGui::GetIO().KeyCtrl) {
                                ImGui::SetTooltip("%s","right click to download this file locally");
                                if (ImGui::GetIO().MouseClicked[1]) {
                                    strcpy(tmpString,"saveFileFromMemoryFSToDisk('");
                                    strcat(tmpString,I.files[i]);strcat(tmpString,"','");
                                    strcat(tmpString,fileName);strcat(tmpString,"')");

                                    emscripten_run_script(tmpString);
                                }
                            }
                            else if (isZipFile)  {
                                ImGui::SetTooltip("right click to browse it");
                                if (ImGui::GetIO().MouseClicked[1])  {
                                    strcpy(I.currentFolder,I.files[i]);
                                    strcpy(I.editLocationInputText,I.currentFolder);
                                    I.history.switchTo(I.currentFolder);
                                    I.forceRescan = true;
                                    //------------------------------------------------------------------------------------------------------------------------------
                                }
                            }
                        }
#                       endif // IMGUI_USE_MINIZIP
#                   elif IMGUI_USE_MINIZIP // (defined(__EMSCRIPTEN__) && defined(EMSCRIPTEN_SAVE_SHELL))
                    if (isZipFile)  {
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("right click to browse it");
                        if (ImGui::GetIO().MouseClicked[1])  {
                            strcpy(I.currentFolder,I.files[i]);
                            strcpy(I.editLocationInputText,I.currentFolder);
                            I.history.switchTo(I.currentFolder);
                            I.forceRescan = true;
                            //------------------------------------------------------------------------------------------------------------------------------
                        }
                    }
#                   endif //(defined(__EMSCRIPTEN__) && defined(EMSCRIPTEN_SAVE_SHELL))
                    ++cntEntries;
                    if (Internal::BrowsingPerRow) ImGui::NextColumn();
                    else if (cntEntries==I.numBrowsingEntriesPerColumn) {
                        cntEntries = 0;
                        ImGui::NextColumn();
                    }
                }
            }

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }
        //-----------------------------------------------------------------------------
        ImGui::PopID();
        ImGui::EndChild();

    }
    //-----------------------------------------------------------------------------
    // Selection field -------------------------------------------------------------------
    if (isSaveFileDialog || isSelectFolderDialog)   {
        ImGui::Separator();
        bool selectionButtonPressed = false;

        static int id;
        float lastTwoButtonsWidth = 0;
        ImGui::PushID(&id);
        if (isSaveFileDialog)   {
            ImGui::AlignFirstTextHeightToWidgets();
            ImGui::Text("File:");ImGui::SameLine();
            lastTwoButtonsWidth = ImGui::CalcTextSize("Save Cancel").x+2.0f*(style.FramePadding.x+style.ItemSpacing.x)+style.WindowPadding.x;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()-lastTwoButtonsWidth);
            ImGui::InputText("##saveFileNameDialog",&I.saveFileName[0],MAX_FILENAME_BYTES);
            if (ImGui::IsItemHovered() && I.mustFilterSaveFilePathWithFileFilterExtensionString && fileFilterExtensionString && strlen(fileFilterExtensionString)>0) ImGui::SetTooltip("%s",fileFilterExtensionString);
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }
        else {
            ImGui::AlignFirstTextHeightToWidgets();

            static const ImVec4 sf(1.0,0.8,0.5,1);      // selected folder color factor
            ImVec4& c = ColorSet[Internal::ImGuiCol_Dialog_SelectedFolder_Text];
            const ImVec4& r = style.Colors[ImGuiCol_Text];
            Internal::ColorCombine(c,r,sf);
            ImGui::TextColored(ColorSet[Internal::ImGuiCol_Dialog_SelectedFolder_Text],"Folder:");

            //ImGui::Text("Folder:");	// Faster alternative to the 5 lines above

            ImGui::SameLine();
            lastTwoButtonsWidth = ImGui::CalcTextSize("Select Cancel").x+2.0f*(style.FramePadding.x+style.ItemSpacing.x)+style.WindowPadding.x;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()-lastTwoButtonsWidth);
            ImGui::InputText("##selectFileNameDialog",&I.saveFileName[0],MAX_FILENAME_BYTES,ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        bool mustCancel = false;
        if (ImGui::Button("Cancel")) mustCancel = true;ImGui::SameLine();   // Optional line

        if (isSelectFolderDialog)  selectionButtonPressed = ImGui::Button("Select");
        else selectionButtonPressed = ImGui::Button("Save");


        ImGui::PopID();

        if (selectionButtonPressed) {
            if (isSelectFolderDialog) {
                strcpy(rv,I.currentFolder);
                I.open = true;
            }
            else if (isSaveFileDialog)  {
                if (strlen(I.saveFileName)>0)  {
                    bool pathOk = true;
                    if (I.mustFilterSaveFilePathWithFileFilterExtensionString && fileFilterExtensionString && strlen(fileFilterExtensionString)>0)    {
                        pathOk = false;
                        char saveFileNameExtension[MAX_FILENAME_BYTES];Path::GetExtension(I.saveFileName,saveFileNameExtension);
                        const bool saveFileNameHasExtension = strlen(saveFileNameExtension)>0;
                        //-------------------------------------------------------------------
                        FilenameStringVector wExts;String::Split(fileFilterExtensionString,wExts,';');
                        const size_t wExtsSize = wExts.size();
                        if (!saveFileNameHasExtension)   {
                            if (wExtsSize==0) pathOk = true;    // Bad situation, better allow this case
                            else strcat(I.saveFileName,wExts[0]);
                        }
                        else    {
                            // saveFileNameHasExtension
                            for (size_t i = 0;i<wExtsSize;i++)	{
                                const char* ext = wExts[i];
                                if (strcmp(ext,saveFileNameExtension)==0)   {
                                    pathOk = true;
                                    break;
                                }
                            }
                            if (!pathOk && wExtsSize>0) strcat(I.saveFileName,wExts[0]);
                        }
                    }
                    if (pathOk) {
                        char savePath[MAX_PATH_BYTES];
                        Path::Combine(I.currentFolder,I.saveFileName,savePath,false);
                        strcpy(rv,savePath);
                        I.open = true;
                    }
                }
            }
        }
        else if (mustCancel && rv[0]==0) {
            ImGui::CloseCurrentPopup();
#           ifdef IMGUI_USE_MINIZIP
            I.unz.close();
#           endif // IMGUI_USE_MINIZIP
            I.freeMemory();

            I.userHasJustCancelledDialog = true;
        }
        //ImGui::Separator();
        //ImGui::Spacing();
    }
    // End selection field----------------------------------------------------------------

    if (rv[0]!=0 || !I.open) {
        ImGui::CloseCurrentPopup();
#       ifdef IMGUI_USE_MINIZIP
        I.unz.close();
#       endif // IMGUI_USE_MINIZIP
        I.freeMemory();
    }
    ImGui::EndPopup();
    return rv;
}

const char* Dialog::chooseFileDialog(bool dialogTriggerButton,const char* directory,const char* fileFilterExtensionString,const char* windowTitle,const ImVec2& windowSize,const ImVec2& windowPos,const float windowAlpha) {
    if (dialogTriggerButton)    {internal->open = internal->rescan = internal->forceSetWindowPositionAndSize = true;internal->chosenPath[0]='\0';internal->userHasJustCancelledDialog = false;}
    if (dialogTriggerButton || (!internal->rescan && internal->open && strlen(getChosenPath())==0)) {
	if (this->internal->open) ImGui::SetNextWindowFocus();  // Not too sure about this line (it seems to just keep the window on the top, but it does not prevent other windows to be used...)
        const char* cp = ChooseFileMainMethod(*this,directory,false,false,"",fileFilterExtensionString,windowTitle,windowSize,windowPos,windowAlpha);
        if (!internal->open) {  // AFAIK this should happen only when user clicks the close button (but not when he clicks cancel)
            internal->userHasJustCancelledDialog = true;
#           ifdef IMGUI_USE_MINIZIP
            internal->unz.close();
#           endif // IMGUI_USE_MINIZIP
            internal->freeMemory();
        }
        return cp;
    }
    return "";
}
const char* Dialog::chooseFolderDialog(bool dialogTriggerButton,const char* directory,const char* windowTitle,const ImVec2& windowSize,const ImVec2& windowPos,const float windowAlpha)  {
    if (dialogTriggerButton) {internal->open = internal->rescan = internal->forceSetWindowPositionAndSize = true;internal->chosenPath[0]='\0';internal->userHasJustCancelledDialog = false;}
    if (dialogTriggerButton || (!internal->rescan && internal->open && strlen(getChosenPath())==0)) {
	if (this->internal->open) ImGui::SetNextWindowFocus();  // Not too sure about this line (it seems to just keep the window on the top, but it does not prevent other windows to be used...)
        const char* cp = ChooseFileMainMethod(*this,directory,true,false,"","",windowTitle,windowSize,windowPos,windowAlpha);
        if (!internal->open) {
            internal->userHasJustCancelledDialog = true;
#           ifdef IMGUI_USE_MINIZIP
            internal->unz.close();
#           endif // IMGUI_USE_MINIZIP
            internal->freeMemory();
        }
        return cp;
    }
    return "";
}
const char* Dialog::saveFileDialog(bool dialogTriggerButton,const char* directory,const char* startingFileNameEntry,const char* fileFilterExtensionString,const char* windowTitle,const ImVec2& windowSize,const ImVec2& windowPos,const float windowAlpha)    {
    if (dialogTriggerButton) {internal->open = internal->rescan = internal->forceSetWindowPositionAndSize = true;internal->chosenPath[0]='\0';internal->userHasJustCancelledDialog = false;}
    if (dialogTriggerButton || (!internal->rescan && internal->open && strlen(getChosenPath())==0)) {
	if (this->internal->open) ImGui::SetNextWindowFocus();  // Not too sure about this line (it seems to just keep the window on the top, but it does not prevent other windows to be used...)
        const char* cp = ChooseFileMainMethod(*this,directory,false,true,startingFileNameEntry,fileFilterExtensionString,windowTitle,windowSize,windowPos,windowAlpha);
        if (!internal->open) {
            internal->userHasJustCancelledDialog = true;
#           ifdef IMGUI_USE_MINIZIP
            internal->unz.close();
#           endif // IMGUI_USE_MINIZIP
            internal->freeMemory();
        }
        return cp;
    }
    return "";
}



#ifndef IMGUIFS_NO_EXTRA_METHODS
// just expose some methods

void PathGetAbsolute(const char *path, char *rv) {Path::GetAbsolutePath(path,rv);}
void PathGetDirectoryName(const char *filePath, char *rv)    {Path::GetDirectoryName(filePath,rv);}
void PathGetFileName(const char *filePath, char *rv) {Path::GetFileName(filePath,rv);}
void PathGetFileNameWithoutExtension(const char *filePath, char *rv) {Path::GetFileNameWithoutExtension(filePath,rv);}
void PathGetExtension(const char* filePath,char *rv) {Path::GetExtension(filePath,rv);}
void PathChangeExtension(const char* filePath,const char* newExtension,char *rv) {Path::ChangeExtension(filePath,newExtension,rv);}
void PathAppend(const char* directory,char* rv) {Path::Append(directory,rv);}
void PathSplit(const char* path,FilenameStringVector& rv,bool leaveIntermediateTrailingSlashes) {Path::Split(path,rv,leaveIntermediateTrailingSlashes);}
void DirectoryGetDirectories(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut,Sorting sorting) {Directory::GetDirectories(directoryName,result,pOptionalNamesOut,sorting);}
void DirectoryGetFiles(const char* directoryName,PathStringVector& result,FilenameStringVector* pOptionalNamesOut, Sorting sorting) {Directory::GetFiles(directoryName,result,pOptionalNamesOut,sorting);}
void DirectoryCreate(const char* directoryName) {Directory::Create(directoryName);}
bool PathExists(const char* path) {
    struct stat statbuf;return (stat(path, &statbuf) != -1 && (S_ISDIR(statbuf.st_mode) || S_ISREG(statbuf.st_mode)));
}
bool DirectoryExists(const char* path) {return Directory::Exists(path);}
bool FileExists(const char* path) {
        struct stat statbuf;return (stat(path, &statbuf) != -1 && S_ISREG(statbuf.st_mode));
}
#endif //IMGUIFS_NO_EXTRA_METHODS


} // namespace ImGuiFs


