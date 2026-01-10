#ifndef FGWSZ_HEADER_H
#define FGWSZ_HEADER_H

#include<cstdint>   //::std::uint8_t ::std::uint64_t

#include<string>    //::std::string

namespace fgwsz{

struct Header{
    ::std::uint8_t key;
    ::std::uint64_t relative_path_bytes;
    ::std::string relative_path_string;
    ::std::uint64_t content_bytes;
};

}//namespace fgwsz

#endif//FGWSZ_HEADER_H
