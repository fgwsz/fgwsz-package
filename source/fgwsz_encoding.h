#ifndef FGWSZ_ENCODING_H
#define FGWSZ_ENCODING_H

#include<string>    //::std::string
#include<memory>    //::std::unique_ptr

#ifdef _WIN32
    #include<Windows.h>
#endif

#include"fgwsz_except.h"

//======================= 跨平台编码转换API =======================
namespace fgwsz{
/**
 * @brief 将本地编码字符串转换为UTF-8编码
 * @param local_str 本地编码的字符串(在Windows上通常是系统代码页,如GBK)
 * @return UTF-8编码的字符串
 */
inline ::std::string local_to_utf8(::std::string const& local_str){
    //空字符串直接返回
    if(local_str.empty()){
        return "";
    }
#ifdef _WIN32
    //=============== Windows实现 ===============
    //获取当前系统的ANSI代码页(如CP_ACP在中文Windows上是936/GBK)
    int ansi_cp=CP_ACP;
    //第一步:本地ANSI编码->UTF-16
    int wlen=MultiByteToWideChar(ansi_cp,0,local_str.c_str(),-1,nullptr,0);
    if(wlen<=0){
        FGWSZ_THROW_WHAT("local to utf8 error");
    }
    ::std::unique_ptr<wchar_t[]> wbuf(new wchar_t[wlen]);
    if(MultiByteToWideChar(ansi_cp,0,local_str.c_str(),-1,wbuf.get(),wlen)==0){
        FGWSZ_THROW_WHAT("local to utf8 error");
    }
    //第二步:UTF-16->UTF-8
    int ulen=
        WideCharToMultiByte(CP_UTF8,0,wbuf.get(),-1,nullptr,0,nullptr,nullptr);
    if(ulen<=0){
        FGWSZ_THROW_WHAT("local to utf8 error");
    }
    ::std::unique_ptr<char[]> ubuf(new char[ulen]);
    if(
        WideCharToMultiByte(
            CP_UTF8,0,wbuf.get(),-1,ubuf.get(),ulen,nullptr,nullptr
        )==0
    ){
        FGWSZ_THROW_WHAT("local to utf8 error");
    }
    return ::std::string(ubuf.get());
#else
    //=============== Linux/macOS实现 ===============
    //假设Linux/macOS系统默认使用UTF-8
    //如果本地编码不是UTF-8,需要根据实际情况修改
    //这里简单返回原字符串,实际可能需要使用iconv等库
    return local_str;
#endif
}
/**
 * @brief 将UTF-8编码字符串转换为本地编码
 * @param utf8_str UTF-8编码的字符串
 * @return 本地编码的字符串
 */
inline ::std::string utf8_to_locale(::std::string const& utf8_str){
    //空字符串直接返回
    if(utf8_str.empty()){
        return "";
    }
#ifdef _WIN32
    //=============== Windows实现 ===============
    //第一步:UTF-8->UTF-16
    int wlen=MultiByteToWideChar(CP_UTF8,0,utf8_str.c_str(),-1,nullptr,0);
    if(wlen<=0){
        FGWSZ_THROW_WHAT("utf8 to local error");
    }
    ::std::unique_ptr<wchar_t[]> wbuf(new wchar_t[wlen]);
    if(MultiByteToWideChar(CP_UTF8,0,utf8_str.c_str(),-1,wbuf.get(),wlen)==0){
        FGWSZ_THROW_WHAT("utf8 to local error");
    }
    //第二步:UTF-16->本地ANSI编码
    int ansi_cp=CP_ACP;
    int alen=
        WideCharToMultiByte(ansi_cp,0,wbuf.get(),-1,nullptr,0,nullptr,nullptr);
    if(alen<=0){
        FGWSZ_THROW_WHAT("utf8 to local error");
    }
    ::std::unique_ptr<char[]> abuf(new char[alen]);
    if(
        WideCharToMultiByte(
            ansi_cp,0,wbuf.get(),-1,abuf.get(),alen,nullptr,nullptr
        )==0
    ){
        FGWSZ_THROW_WHAT("utf8 to local error");
    }
    return ::std::string(abuf.get());
#else
    //=============== Linux/macOS实现 ===============
    //假设Linux/macOS系统默认使用UTF-8
    //如果本地编码不是UTF-8,需要根据实际情况修改
    //这里简单返回原字符串,实际可能需要使用iconv等库
    return utf8_str;
#endif
}

}//namespace fgwsz

#endif//FGWSZ_ENCODING_H
