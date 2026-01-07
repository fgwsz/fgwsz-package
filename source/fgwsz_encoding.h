#ifndef FGWSZ_ENCODING_H
#define FGWSZ_ENCODING_H

#ifdef _WIN32

#include<cstddef>//::std::size_t
#include<cstring>//::std::memcpy

#include<string>//::std::string

#include<windows.h>

//接口声明
namespace fgwsz{
inline ::std::string auto_to_utf8(::std::string const& input);
}//namespace fgwsz

namespace fgwsz::detail{
inline ::std::string auto_to_utf8(char const* input,::std::size_t length);
inline bool is_valid_utf8(char const* str,::std::size_t length);
inline bool is_utf16(char const* input,::std::size_t length);
inline ::std::string utf16_to_utf8(char const* input,::std::size_t length);
inline ::std::string try_common_encodings(
    char const* input
    ,::std::size_t length
);
inline ::std::string wide_to_utf8(wchar_t const* wstr);
inline ::std::string convert_to_utf8(
    char const* input
    ,::std::size_t length
    ,UINT code_page
);
}//namespace fgwsz::detail

//接口实现
namespace fgwsz{

::std::string auto_to_utf8(::std::string const& input){
    if(input.empty()){
        return input;
    }
    return ::fgwsz::detail::auto_to_utf8(input.data(),input.length());
}

}//namespace fgwsz

namespace fgwsz::detail{

::std::string auto_to_utf8(char const* input,::std::size_t length){
    if(!input||length==0){
        return "";
    }
    //1.首先检查是否是有效的UTF-8
    if(::fgwsz::detail::is_valid_utf8(input,length)){
        return ::std::string(input,length);
    }
    //2.检查是否是 UTF-16
    if(::fgwsz::detail::is_utf16(input,length)){
        return ::fgwsz::detail::utf16_to_utf8(input,length);
    }
    //3.尝试常见编码
    return ::fgwsz::detail::try_common_encodings(input,length);
}
bool is_valid_utf8(char const* str,::std::size_t length){
    unsigned char const* bytes=(unsigned char const*)str;
    ::std::size_t i=0;
    while(i<length){
        if((bytes[i]&0x80)==0x00){
            i++;
        }else if(i+1<length&&(bytes[i]&0xE0)==0xC0){
            if((bytes[i+1]&0xC0)!=0x80){
                return false;
            }
            i+=2;
        }else if(i+2<length&&(bytes[i]&0xF0)==0xE0){
            if((bytes[i+1]&0xC0)!=0x80||
                (bytes[i+2]&0xC0)!=0x80){
                return false;
            }
            i+=3;
        }else if(i+3<length&&(bytes[i]&0xF8)==0xF0){
            if((bytes[i+1]&0xC0)!=0x80||
                (bytes[i+2]&0xC0)!=0x80||
                (bytes[i+3]&0xC0)!=0x80){
                return false;
            }
            i+=4;
        }else{
            return false;
        }
    }
    return true;
}
bool is_utf16(char const* input,::std::size_t length){
    if(length<2){
        return false;
    }
    //检查BOM
    unsigned char const* bytes=(unsigned char const*)input;
    if(bytes[0]==0xFF&&bytes[1]==0xFE){
        return true;  //UTF-16 LE BOM
    }
    if(bytes[0]==0xFE&&bytes[1]==0xFF){
        return true;  //UTF-16 BE BOM
    }
    //使用IsTextUnicode API检测
    int result=IsTextUnicode(input,(int)length,nullptr);
    return (result!=0);
}
::std::string utf16_to_utf8(char const* input,::std::size_t length){
    // 确定字节顺序
    bool is_big_endian=false;
    unsigned char const* bytes=(unsigned char const*)input;
    if(length>=2){
        if(bytes[0]==0xFE&&bytes[1]==0xFF){
            is_big_endian=true;
            input+=2;  //跳过 BOM
            length-=2;
        }else if(bytes[0]==0xFF&&bytes[1]==0xFE){
            is_big_endian=false;
            input+=2;  //跳过 BOM
            length-=2;
        }
    }
    //转换为UTF-16 wchar_t数组
    ::std::wstring wstr;
    if(length%2!=0){
        //无效的 UTF-16
        return "";
    }
    ::std::size_t char_count=length/2;
    wstr.resize(char_count);
    if(is_big_endian){
        //大端序转换为小端序(Windows 使用小端序)
        unsigned char const* src=(unsigned char const*)input;
        wchar_t* dest=&wstr[0];
        for(::std::size_t i=0;i<char_count;i++){
            dest[i]=(src[i*2]<<8)|src[i*2+1];
        }
    }else{
        //小端序,直接复制
        ::std::memcpy(&wstr[0],input,length);
    }
    //转换到UTF-8
    return ::fgwsz::detail::wide_to_utf8(wstr.c_str());
}
::std::string try_common_encodings(char const* input,::std::size_t length){
    struct EncodingTest{
        UINT code_page;
        char const* name;
    };
    EncodingTest encodings[]={
        {CP_ACP,"System ANSI"},
        {932,"Shift-JIS"},
        {936,"GBK"},
        {949,"EUC-KR"},
        {950,"Big5"},
        {1250,"Windows-1250 (Eastern Europe)"},
        {1251,"Windows-1251 (Cyrillic)"},
        {1252,"Windows-1252 (Latin)"},
        {1253,"Windows-1253 (Greek)"},
        {1254,"Windows-1254 (Turkish)"},
        {1255,"Windows-1255 (Hebrew)"},
        {1256,"Windows-1256 (Arabic)"},
        {1257,"Windows-1257 (Baltic)"},
        {1258,"Windows-1258 (Vietnamese)"},
        {28591,"ISO-8859-1 (Latin-1)"},
        {28592,"ISO-8859-2 (Latin-2)"},
        {28595,"ISO-8859-5 (Cyrillic)"},
        {28596,"ISO-8859-6 (Arabic)"},
        {28597,"ISO-8859-7 (Greek)"},
        {28598,"ISO-8859-8 (Hebrew)"},
        {28599,"ISO-8859-9 (Latin-5)"},
    };
    //尝试每个编码
    for(auto const& enc:encodings){
        try{
            ::std::string result=
                ::fgwsz::detail::convert_to_utf8(input,length,enc.code_page);
            if(
                    !result.empty()
                    &&::fgwsz::detail::is_valid_utf8(
                        result.data(),result.length()
                    )
            ){
                return result;
            }
        }catch(...){
            continue;
        }
    }
    //如果都失败,原样返回(假设是UTF-8)
    return ::std::string(input,length);
}
::std::string wide_to_utf8(wchar_t const* wstr){
    int utf8_len=WideCharToMultiByte(
        CP_UTF8,0,wstr,-1,nullptr,0,nullptr,nullptr
    );
    if(utf8_len<=0){
        return "";
    }
    ::std::string utf8_str(utf8_len-1,'\0');
    WideCharToMultiByte(
        CP_UTF8,0,wstr,-1,&utf8_str[0],utf8_len,nullptr,nullptr
    );
    return utf8_str;
}
::std::string convert_to_utf8(
    char const* input
    ,::std::size_t length
    ,UINT code_page
){
    //转换到UTF-16
    int wide_len=MultiByteToWideChar(
        code_page,MB_ERR_INVALID_CHARS,input,(int)length,nullptr,0
    );
    if(wide_len<=0){
        return "";
    }
    ::std::wstring wide_str(wide_len,L'\0');
    MultiByteToWideChar(
        code_page,MB_ERR_INVALID_CHARS,input,(int)length,&wide_str[0],wide_len
    );
    //转换到UTF-8
    return ::fgwsz::detail::wide_to_utf8(wide_str.c_str());
}

}//namespace fgwsz::detail

#endif//_WIN32

#endif//FGWSZ_ENCODING_H
