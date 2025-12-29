#include<cstddef>   //::std::size_t
#include<cstdint>   //::std::uint8_t ::std::uint16_t ::std::uint64_t
                    //::std::uint64_t
#include<ios>       //::std::ios ::std::streamsize
#include<iostream>  //::std::cout ::std::cin ::std::cerr
#include<iterator>  //::std::make_move_iterator
#include<string>    //::std::string
#include<vector>    //::std::vector
#include<fstream>   //::std::ifstream ::std::ofstream
#include<exception> //::std::exception
#include<stdexcept> //::std::runtime_error
#include<filesystem>//::std::filesystem
#include<random>    //::std::uniform_int_distribution ::std::random_device
                    //::std::mt19937
#include<source_location>//::std::source_location
#include<format>         //::std::format
#include<bit>            //::std::endian

//作者制定了归档一个或多个(文件/目录)的包标准:
//    包的二进制的文件结构如下:
//        [file item 1]...[file item N]
//    每个文件的二进制信息[file item]的文件结构是[K|A|B|C|D]:
//        K部分是[key(1字节)]
//        A部分是[relative path bytes(8字节)]
//        B部分是[relative path]
//        C部分是[content bytes(8字节)]
//        D部分是[content(binary)]

//============================================================================
//路径安全相关
//============================================================================
//检查相对路径是否安全
bool is_safe_relative_path(::std::filesystem::path const& relative_path){
    //空路径一定存在逻辑错误
    if(relative_path.empty()){
        return false;
    }
    for(auto const& component:relative_path){
        //若包含"..",则存在文件溢出输出路径的风险
        if(".."==component){
            return false;
        }
    }
    return true;
}
//============================================================================
//异常处理相关
//============================================================================
//用于错误处理的字符串信息
::std::string what(
    ::std::string const& message
    ,::std::source_location location=::std::source_location::current()
){
    return ::std::format(
        "file: {}({}:{}) `{}`: {}"
        ,location.file_name()
        ,location.line()
        ,location.column()
        ,location.function_name()
        ,message
    );
}
//抛出运行时异常(简化宏定义)
#define THROW_RUNTIME_ERROR(...) do{\
    throw ::std::runtime_error(::what(__VA_ARGS__)); \
}while(0) \
//
//============================================================================
//字节序相关
//============================================================================
//字节序辅助类
template<typename UnsignedType>
union EndianHelper{
    ::std::uint8_t byte_array[sizeof(UnsignedType)];
    UnsignedType number;
};
//将主机字节序无符号整数转换为同类型的网络字节序
template<typename UnsignedType>
constexpr UnsignedType host_to_net_uint(UnsignedType host_number){
    if constexpr(::std::endian::native==::std::endian::big){
        return host_number;
    }else{
        ::EndianHelper<UnsignedType> host;
        host.number=host_number;
        ::EndianHelper<UnsignedType> net;
        for(::std::size_t index=0;index<sizeof(UnsignedType);++index){
            net.byte_array[sizeof(UnsignedType)-1-index]=
                host.byte_array[index];
        }
        return net.number;
    }
}
//将网络字节序无符号整数转换为同类型的主机字节序
template<typename UnsignedType>
constexpr UnsignedType net_to_host_uint(UnsignedType net_number){
    if constexpr(::std::endian::native==::std::endian::big){
        return net_number;
    }else{
        ::EndianHelper<UnsignedType> net;
        net.number=net_number;
        ::EndianHelper<UnsignedType> host;
        for(::std::size_t index=0;index<sizeof(UnsignedType);++index){
            host.byte_array[sizeof(UnsignedType)-1-index]=
                net.byte_array[index];
        }
        return host.number;
    }
}
//============================================================================
//打包模式相关
//============================================================================
//打包模式初始化
//若要使用打包模式请先调用该函数进行初始化
inline void pack_mode_init(::std::filesystem::path const& package_path){
    //检查包路径的父路径是否存在,若不存在则创建父路径
    auto package_absolute_path=::std::filesystem::absolute(package_path);
    auto package_parent_path=package_absolute_path.parent_path();
    if(!::std::filesystem::exists(package_parent_path)){
        ::std::filesystem::create_directories(package_parent_path);
    }
    //二进制覆盖方式打开包输出文件路径
    ::std::ofstream package(
        package_path
        ,::std::ios::binary|::std::ios::trunc
    );//确保包文件内容为空
    //包输出文件路径打开失败
    if(!package.is_open()){
        THROW_RUNTIME_ERROR(
            "Failed to open package file:"+package_path.string()
        );
    }
}
//打包单个文件内容到包
inline void pack_file(
    ::std::filesystem::path const& file_path
    ,::std::filesystem::path const& base_dir_path
    ,::std::filesystem::path const& package_path
){
    //========================================================================
    //输入参数检查阶段
    //========================================================================
    //检查文件路径是否存在
    if(!::std::filesystem::exists(file_path)){
        THROW_RUNTIME_ERROR(
            "File path doesn't exist:"+file_path.string()
        );
    }
    //检查文件路径是否为目录
    if(::std::filesystem::is_directory(file_path)){
        THROW_RUNTIME_ERROR(
            "File path is directory:"+file_path.string()
        );
    }
    //检查文件路径是否为符号链接
    if(::std::filesystem::is_symlink(file_path)){
        //读取符号链接内容设置为未定义行为
        THROW_RUNTIME_ERROR(
            "File path is symlink(UB):"+file_path.string()
        );
    }
    //检查基准目录路径是否存在
    if(!::std::filesystem::exists(base_dir_path)){
        THROW_RUNTIME_ERROR(
            "Base dir path doesn't exist:"+base_dir_path.string()
        );
    }
    //检查基准目录路径是否为目录
    if(!::std::filesystem::is_directory(base_dir_path)){
        THROW_RUNTIME_ERROR(
            "Base path isn't directory:"+base_dir_path.string()
        );
    }
    //检查包路径是否存在
    if(!::std::filesystem::exists(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path doesn't exist:"+package_path.string()
        );
    }
    //检查包路径是否为目录
    if(::std::filesystem::is_directory(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path is directory:"+package_path.string()
        );
    }
    //========================================================================
    //文件头信息处理阶段
    //========================================================================
    //文件头信息包含4部分:
    //[key|relative path bytes|relative path|content bytes]
    //得到文件的密钥
    //使用静态变量,确保只初始化一次随机设备
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<::std::uint8_t> distrib(1,255);
        //[1,255]
    ::std::uint8_t key=distrib(gen);
    //根据基准目录路径和文件路径得到归档之后的相对路径
    auto relative_path=::std::filesystem::proximate(file_path,base_dir_path);
    //检查文件的相对路径是否安全(不安全情况,存在溢出输出目录的风险)
    if(!::is_safe_relative_path(relative_path)){
        THROW_RUNTIME_ERROR(
            "File relative path is unsafe:"+relative_path.string()
        );
    }
    //得到相对路径和相对路径的所占用的字节数
    ::std::string relative_path_string=relative_path.string();
    ::std::uint64_t relative_path_bytes=relative_path_string.size();
    //得到文件内容字节数
    ::std::uint64_t content_bytes=::std::filesystem::file_size(file_path);
    //构建文件头信息缓冲区
    ::std::vector<char> buffer={};
    buffer.resize(
        sizeof(key)
        +sizeof(relative_path_bytes)
        +relative_path_bytes
        +sizeof(content_bytes)
    );
    //将文件头信息写入缓冲区
    ::std::uint64_t buffer_index=0;
    buffer[0]=key;
    ++buffer_index;
    ::EndianHelper<::std::uint64_t> net_endian;
    net_endian.number=
        ::host_to_net_uint<::std::uint64_t>(relative_path_bytes);
    for(::std::uint8_t byte:net_endian.byte_array){
        buffer[buffer_index]=static_cast<char>(byte);
        ++buffer_index;
    }
    for(char byte:relative_path_string){
        buffer[buffer_index]=byte;
        ++buffer_index;
    }
    net_endian.number=
        ::host_to_net_uint<::std::uint64_t>(content_bytes);
    for(::std::uint8_t byte:net_endian.byte_array){
        buffer[buffer_index]=static_cast<char>(byte);
        ++buffer_index;
    }
    //将缓冲区的文件头信息编码混淆
    for(::std::uint64_t index=1;index<buffer.size();++index){
        buffer[index]=
            static_cast<char>(key^static<std::uint8_t>(buffer[index]));
    }
    //二进制拼接方式打开包输出文件路径
    ::std::ofstream package(
        package_path
        ,::std::ios::binary|::std::ios::app
    );
    //包输出文件路径打开失败
    if(!package.is_open()){
        THROW_RUNTIME_ERROR(
            "Failed to open package:"+package_path.string()
        );
    }
    //将缓冲区的文件头信息写入包
    package.write(
        buffer.data()
        ,static_cast<::std::streamsize>(buffer.size())
    );
    package.flush();
    //写入失败时
    if(!package.good()){
        THROW_RUNTIME_ERROR(
            "Package write error:"+package_path.string()
        );
    }
    //========================================================================
    //文件内容信息处理阶段
    //========================================================================
    //二进制方式打开文件
    ::std::ifstream file(file_path,::std::ios::binary);
    //文件打不开时
    if(!file.is_open()){
        THROW_RUNTIME_ERROR("Failed to open file:"+file_path.string());
    }
    //设置内存块
    constexpr ::std::uint64_t block_bytes=1024*1024;//1MB
    char block[block_bytes];
    //分块读取文件内容
    ::std::uint64_t count_bytes=0;
    ::std::uint64_t read_bytes=0;
    while(count_bytes<content_bytes){
        //读取文件内容到块中
        file.read(
            &(block[0])
            ,static_cast<::std::streamsize>(block_bytes)
        );
        read_bytes=static_cast<::std::uint64_t>(file.gcount());
        //将内存块中的文件内容信息编码混淆
        for(::std::uint64_t index=0;index<read_bytes;++index){
            block[index]=static_cast<char>(
                key^static<std::uint8_t>(block[index])
            );
        }
        //将内存块中的文件内容信息写入包
        package.write(
            &(block[0])
            ,static_cast<::std::streamsize>(read_bytes)
        );
        package.flush();
        //写入失败时
        if(!package.good()){
            THROW_RUNTIME_ERROR(
                "Package write error:"+package_path.string()
            );
        }
        //更新字节计数器
        count_bytes+=read_bytes;
    }
    //文件内容读取不完整
    if(count_bytes!=content_bytes){
        THROW_RUNTIME_ERROR(
            "File read incomplete:"+file_path.string()
        );
    }
}
//打包单个目录到包
inline void pack_dir(
    ::std::filesystem::path const& dir_path
    ,::std::filesystem::path const& package_path
){
    //========================================================================
    //输入参数检查阶段
    //========================================================================
    //检查目录路径是否存在
    if(!::std::filesystem::exists(dir_path)){
        THROW_RUNTIME_ERROR(
            "Dir path doesn't exist:"+dir_path.string()
        );
    }
    //检查目录路径是否指向目录
    if(!::std::filesystem::is_directory(dir_path)){
        THROW_RUNTIME_ERROR(
            "Dir path isn't directory:"+dir_path.string()
        );
    }
    //检查包路径是否存在
    if(!::std::filesystem::exists(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path doesn't exist:"+package_path.string()
        );
    }
    //检查包路径是否为目录
    if(::std::filesystem::is_directory(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path is directory:"+package_path.string()
        );
    }
    //========================================================================
    //打包子文件阶段
    //========================================================================
    //递归遍历所有的子文件进行打包
    try{
        for(::std::filesystem::directory_entry const& dir_entry
            : ::std::filesystem::recursive_directory_iterator(dir_path)
        ){
            ::pack_file(dir_entry.path(),dir_path,package_path);
        }
    }catch(::std::exception const& e){
        THROW_RUNTIME_ERROR(e.what());
    }
}
//打包单个路径(目录/文件)到包
inline void pack_path(
    ::std::filesystem::path const& path
    ,::std::filesystem::path const& package_path
){
    //========================================================================
    //输入参数检查阶段
    //========================================================================
    //检查路径是否存在
    if(!::std::filesystem::exists(path)){
        THROW_RUNTIME_ERROR(
            "Path doesn't exist:"+path.string()
        );
    }
    //检查包路径是否存在
    if(!::std::filesystem::exists(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path doesn't exist:"+package_path.string()
        );
    }
    //检查包路径是否为目录
    if(::std::filesystem::is_directory(package_path)){
        THROW_RUNTIME_ERROR(
            "Package path is directory:"+package_path.string()
        );
    }
    //========================================================================
    //打包目录/文件阶段
    //========================================================================
    //检查路径类型
    if(::std::filesystem::is_directory(path)){//目录路径
        ::pack_dir(path,package_path);
    }else{//文件路径
        //为了避免出现如下情况,扩展path为绝对路径:
        //path:"file.txt"
        //  parent_path:"" 
        //  is_directory(parent_path) is false
        //path:"./file.txt"
        //  parent_path:"./"
        //  is_directory(parent_path) is true
        auto absolute_path=::std::filesystem::absolute(path);
        ::pack_file(absolute_path,absolute_path.parent_path(),package_path);
    }
}
//打包多个路径(目录/文件)到包
inline void pack_paths(
    ::std::vector<::std::filesystem::path> const& paths
    ,::std::filesystem::path const& package_path
){
    for(auto const& path:paths){
        ::pack_path(path,package_path);
    }
}
//============================================================================
//解包模式相关
//============================================================================
void unpack_package(
    ::std::filesystem::path const& package_path
    ,::std::filesystem::path const& output_dir_path
){
    //TODO
}
//============================================================================
//终端打印相关
//============================================================================
static bool const std_cout_init=[](void){
    //关闭与C语言的输入输出流同步
    ::std::ios_base::sync_with_stdio(false);
    //解除cin和cout的绑定
    ::std::cin.tie(nullptr);
    ::std::cout.tie(nullptr);
    return true;
}();
void help(void){
    ::std::cout<<
R"(Usages:
    Pack  : -c <output package path> <input path 1> ... <input path N>
    Unpack: -x <input package path> <output directory path>
Examples:
    Pack a file and directory: -c mypkg.fgwsz README.md source
    Unpack                   : -x mypkg.fgwsz output
)";
}
int main(int argc,char* argv[]){
    //输入参数太少
    if(argc<4){
        ::help();
        return 1;
    }
    //检查功能选项
    ::std::string option=argv[1];
    if(option!="-c"&&option!="-x"){
        ::help();
        return 1;
    }
    try{
        if('x'==option[1]){//解包模式
            //排除解包模式输入参数过多的情况
            if(argc>4){
                ::help();
                return 1;
            }
            ::std::filesystem::path package_path=argv[2];
            ::std::filesystem::path output_dir_path=argv[3];
            ::unpack_package(package_path,output_dir_path);
            //TODO
        }else{//打包模式
            ::std::filesystem::path package_path=argv[2];
            ::std::vector<::std::filesystem::path> input_paths={};
            input_paths.reserve(argc-3);
            for(int index=3;index<argc;++index){
                input_paths.emplace_back(argv[index]);
            }
            ::pack_mode_init();
            ::pack_paths(input_paths,package_path);
        }
    }catch(::std::exception const& e){
        ::std::cerr<<"Error:"<<e.what()<<'\n';
        return 1;
    }
    return 0;
}
