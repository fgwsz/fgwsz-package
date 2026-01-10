#include"fgwsz_unpacker.h"

#include<cstdint>   //::std::uint8_t ::std::uint64_t

#include<string>        //::std::string
#include<filesystem>    //::std::filesystem
#include<ios>           //::std::ios
#include<fstream>       //::std::ofstream ::std::ifstream
#include<vector>        //::std::vector
#include<memory>        //::std::unique_ptr
#include<type_traits>   //::std::remove_cvref_t
#include<format>        //::std::format

#include"fgwsz_endian.hpp"
#include"fgwsz_except.h"
#include"fgwsz_path.h"
#include"fgwsz_fstream.h"
#include"fgwsz_cout.h"

namespace fgwsz{

Unpacker::Unpacker(::std::filesystem::path const& package_path){
    //检查包路径是否存在
    ::fgwsz::path_assert_exists(package_path);
    //检查包路径不为目录路径
    ::fgwsz::path_assert_is_not_directory(package_path);
    //初始化包文件路径字符串(用于抛出异常时的信息显示)
    this->package_path_string_=package_path.generic_string();
    //二进制方式打开包文件
    this->package_.open(package_path,::std::ios::binary);
    //包文件打开失败
    if(!(this->package_.is_open())){
        FGWSZ_THROW_WHAT(
            "failed to open package file: "+this->package_path_string_
        );
    }
    //包文件的大小
    this->package_bytes_=::std::filesystem::file_size(package_path);
}
Unpacker::~Unpacker(void){
    if(this->package_.is_open()){
        this->package_.close();
    }
}
void Unpacker::reset_package(void){
    //重置包文件流位置到文件头
    this->package_.seekg(0);
    if(!this->package_.good()){
        FGWSZ_THROW_WHAT(
            "failed to jump package head: "+this->package_path_string_
        );
    }
    //重置用于记录已读取包内容字节数的计数器
    this->package_count_bytes_=0;
}
void Unpacker::unpack_key(void){
    ::std::uint64_t read_bytes=::fgwsz::std_ifstream_read(
        this->package_
        ,reinterpret_cast<char*>(&(this->header_.key))
        ,sizeof(this->header_.key)
        ,this->package_path_string_
    );
    if(sizeof(this->header_.key)!=read_bytes){
        FGWSZ_THROW_WHAT(
            "failed to read key: "+this->package_path_string_
        );
    }
    this->package_count_bytes_+=read_bytes;
}
void Unpacker::unpack_relative_path_bytes(void){
    ::std::uint64_t read_bytes=::fgwsz::std_ifstream_read(
        this->package_
        ,reinterpret_cast<char*>(&(this->header_.relative_path_bytes))
        ,sizeof(this->header_.relative_path_bytes)
        ,this->package_path_string_
    );
    if(sizeof(this->header_.relative_path_bytes)!=read_bytes){
        FGWSZ_THROW_WHAT(
            "failed to read relative path bytes: "
            +this->package_path_string_
        );
    }
    this->package_count_bytes_+=read_bytes;
    //解码relative path bytes的文件密钥xor混淆
    //字节序辅助结构体
    using number_type=
        ::std::remove_cvref_t<decltype(this->header_.relative_path_bytes)>;
    ::fgwsz::EndianHelper<number_type> net;
    for(::std::uint8_t index=0;index<sizeof(net);++index){
        net.byte_array[index]=static_cast<::std::uint8_t>(
            reinterpret_cast<char*>(&(this->header_.relative_path_bytes))
            [index]
        )^this->header_.key;
    }
    //将网络序转换为主机序,得到relative path bytes
    this->header_.relative_path_bytes=
        ::fgwsz::net_to_host<number_type>(net.data);
}
void Unpacker::unpack_relative_path_string(void){
    this->header_.relative_path_string
        .resize(this->header_.relative_path_bytes);
    ::std::uint64_t read_bytes=::fgwsz::std_ifstream_read(
        this->package_
        ,this->header_.relative_path_string.data()
        ,this->header_.relative_path_bytes
        ,this->package_path_string_
    );
    if(this->header_.relative_path_bytes!=read_bytes){
        FGWSZ_THROW_WHAT(
            "failed to read relative path: "+this->package_path_string_
        );
    }
    this->package_count_bytes_+=read_bytes;
    //解码relative path的文件密钥xor混淆
    for(char& ch:this->header_.relative_path_string){
        ch=static_cast<char>(
            static_cast<::std::uint8_t>(ch)^this->header_.key
        );
    }
}
void Unpacker::unpack_content_bytes(void){
    ::std::uint64_t read_bytes=::fgwsz::std_ifstream_read(
        this->package_
        ,reinterpret_cast<char*>(&(this->header_.content_bytes))
        ,sizeof(this->header_.content_bytes)
        ,this->package_path_string_
    );
    if(sizeof(this->header_.content_bytes)!=read_bytes){
        FGWSZ_THROW_WHAT(
            "failed to read content bytes: "+this->package_path_string_
        );
    }
    this->package_count_bytes_+=read_bytes;
    //解码content bytes的文件密钥xor混淆
    //字节序辅助结构体
    using number_type=
        ::std::remove_cvref_t<decltype(this->header_.content_bytes)>;
    ::fgwsz::EndianHelper<number_type> net;
    for(::std::uint64_t index=0;index<sizeof(net);++index){
        net.byte_array[index]=static_cast<::std::uint8_t>(
            reinterpret_cast<char*>(&(this->header_.content_bytes))[index]
        )^this->header_.key;
    }
    //将网络序转换为主机序,得到content bytes
    this->header_.content_bytes=::fgwsz::net_to_host<number_type>(net.data);
}
void Unpacker::unpack_header(void){
    this->unpack_key();
    this->unpack_relative_path_bytes();
    this->unpack_relative_path_string();
    this->unpack_content_bytes();
}
void Unpacker::unpack_content(
    ::std::filesystem::path const& output_dir_path
    ,char* block
    ,::std::uint64_t block_bytes
){
    //判断相对路径是否是安全路径
    ::fgwsz::path_assert_is_safe_relative_path(
        this->header_.relative_path_string
    );
    //创建文件父目录
    ::std::filesystem::path file_path=::std::filesystem::absolute(
        output_dir_path/this->header_.relative_path_string
    );
    ::fgwsz::try_create_directories(
        ::fgwsz::parent_path(file_path)
    );
    //文件输出流关联文件路径
    ::std::ofstream file(file_path,::std::ios::binary|::std::ios::trunc);
    ::std::string file_path_string=file_path.generic_string();
    if(!file.is_open()){
        FGWSZ_THROW_WHAT("file isn't open: "+file_path_string);
    }
    //分块读取content
    ::std::uint64_t file_count_bytes=0;
    ::std::uint64_t read_bytes=0;
    while(file_count_bytes<this->header_.content_bytes){
        read_bytes=::fgwsz::std_ifstream_read(
            this->package_
            ,&(block[0])
            ,((block_bytes>(this->header_.content_bytes-file_count_bytes)
                ?(this->header_.content_bytes-file_count_bytes)
                :block_bytes))
            ,this->package_path_string_
        );
        this->package_count_bytes_+=read_bytes;
        //解码content的文件密钥xor混淆
        for(::std::uint64_t index=0;index<read_bytes;++index){
            block[index]=static_cast<char>(
                static_cast<::std::uint8_t>(block[index])^this->header_.key
            );
        }
        //将分块读取的content写入文件
        ::fgwsz::std_ofstream_write(
            file
            ,&(block[0])
            ,static_cast<::std::streamsize>(read_bytes)
            ,file_path_string
        );
        file_count_bytes+=read_bytes;
    }
    if(file_count_bytes!=this->header_.content_bytes){
        FGWSZ_THROW_WHAT("file write incomplete: "+file_path_string);
    }
}
void Unpacker::unpack_package(::std::filesystem::path const& output_dir_path){
    //输入参数检查阶段
    ::fgwsz::try_create_directories(output_dir_path);
    ::fgwsz::path_assert_is_directory(output_dir_path);
    //重置包文件流到头部和重置包读取字节计数器为0
    this->reset_package();
    //用于读取文件内容信息的内存块
    //MSVC中栈全部内存默认1MB,直接分配在栈上会导致栈溢出,改为分配在堆上
    constexpr ::std::uint64_t block_bytes=1024*1024;//1MB
    auto block=::std::make_unique<char[]>(block_bytes);
    //文件头信息变量
    while(this->package_count_bytes_<this->package_bytes_){
        //文件头信息处理阶段
        this->unpack_header();
        //文件内容信息处理阶段
        this->unpack_content(
            output_dir_path
            ,block.get()
            ,block_bytes
        );
    }
    if(this->package_count_bytes_!=this->package_bytes_){
        FGWSZ_THROW_WHAT(
            "package read incomplete: "+this->package_path_string_
        );
    }
}
void Unpacker::list_package(void){
    //重置包文件流到头部和重置包读取字节计数器为0
    this->reset_package();
    //文件id
    ::std::uint64_t file_id=0;
    while(this->package_count_bytes_<this->package_bytes_){
        //文件头信息处理阶段
        this->unpack_header();
        ::fgwsz::cout<<::std::format(
            "file[{}]: {{\n"
            "\tkey: {}\n"
            "\trelative path bytes: {}\n"
            "\trelative path string: {}\n"
            "\tcontent bytes: {}\n"
            "}}\n"
            ,file_id
            ,static_cast<unsigned>(this->header_.key)
            ,this->header_.relative_path_bytes
            ,this->header_.relative_path_string
            ,this->header_.content_bytes
        );
        //文件内容信息跳过阶段
        this->package_.seekg(this->header_.content_bytes,::std::ios::cur);
        if(!this->package_.good()){
            FGWSZ_THROW_WHAT(
                "failed to skip content bytes: "
                +this->header_.relative_path_string
            );
        }
        this->package_count_bytes_+=this->header_.content_bytes;
        //更新文件id
        ++file_id;
    }
    if(this->package_count_bytes_!=this->package_bytes_){
        FGWSZ_THROW_WHAT(
            "package read incomplete: "+this->package_path_string_
        );
    }
}

}//namespace fgwsz
