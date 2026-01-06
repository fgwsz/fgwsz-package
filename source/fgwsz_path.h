#ifndef FGWSZ_PATH_H
#define FGWSZ_PATH_H

#include<filesystem>//::std::filesystem

#include"fgwsz_except.h"//FGWSZ_THROW_WHAT

//============================================================================
//路径操作相关
//============================================================================
namespace fgwsz{
inline void path_assert_exists(::std::filesystem::path const& path){
    if(!::std::filesystem::exists(path)){
        FGWSZ_THROW_WHAT("path doesn't exist: "+path.generic_string());
    }
}
inline void path_assert_not_exists(::std::filesystem::path const& path){
    if(::std::filesystem::exists(path)){
        FGWSZ_THROW_WHAT("path does exist: "+path.generic_string());
    }
}
inline void path_assert_is_directory(::std::filesystem::path const& path){
    if(!::std::filesystem::is_directory(path)){
        FGWSZ_THROW_WHAT("path isn't directory: "+path.generic_string());
    }
}
inline void path_assert_is_not_directory(::std::filesystem::path const& path){
    if(::std::filesystem::is_directory(path)){
        FGWSZ_THROW_WHAT("path is directory: "+path.generic_string());
    }
}
inline void path_assert_is_symlink(::std::filesystem::path const& path){
    if(!::std::filesystem::is_symlink(path)){
        FGWSZ_THROW_WHAT("path isn't symlink: "+path.generic_string());
    }
}
inline void path_assert_is_not_symlink(::std::filesystem::path const& path){
    if(::std::filesystem::is_symlink(path)){
        FGWSZ_THROW_WHAT("path is symlink: "+path.generic_string());
    }
}
//一个特性(或者是BUG):
//情况一:目录路径:a/b/c    ->父路径:a/b
//情况二:目录路径:a/b/c/   ->父路径:a/b/c
//注意:情况二中的目录路径最后的那个'/',它会改变parent_path的结果
//但是这个特性有一个妙用:
//目录路径a/b/c表示:指向路径为a/b/c的名为c的目录
//目录路径a/b/c/表示:匹配到路径为a/b/c的名为c的目录下的所有子目录/文件
inline ::std::filesystem::path parent_path(
    ::std::filesystem::path const& path
){
    auto absolute_path=::std::filesystem::absolute(path);
    return ::std::filesystem::absolute(absolute_path.parent_path());
}
inline ::std::filesystem::path relative_path(
    ::std::filesystem::path const& path
    ,::std::filesystem::path const& base_path
){
    auto absolute_path=::std::filesystem::absolute(path);
    auto absolute_base_path=::std::filesystem::absolute(base_path);
    return ::std::filesystem::proximate(absolute_path,absolute_base_path);
}
inline bool is_safe_relative_path(
    ::std::filesystem::path const& relative_path
){
    for(auto const& component:relative_path){
        //若包含"..",则存在上级路径攻击的风险
        if(".."==component){
            return false;
        }
    }
    return true;
}
inline void path_assert_is_safe_relative_path(
    ::std::filesystem::path const& relative_path
){
    if(!::fgwsz::is_safe_relative_path(relative_path)){
        FGWSZ_THROW_WHAT(
            "relative path is unsafe: "+relative_path.generic_string()
        );
    }
}
inline void try_create_directories(
    ::std::filesystem::path const& path
){
    if(!::std::filesystem::exists(path)){
        ::std::filesystem::create_directories(path);
    }
}
}//namespace fgwsz

#endif//FGWSZ_PATH_H
