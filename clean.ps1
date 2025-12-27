$root_path=Split-Path -Parent $MyInvocation.MyCommand.Definition
$build_path=Join-Path $root_path "build/windows"
$project_name=Split-Path -Leaf $root_path
$binary_path=Join-Path $root_path "$project_name.exe"

if(Test-Path -Path $build_path){
    Remove-Item $build_path -Recurse -Force
}
if(Test-Path -Path $binary_path){
    Remove-Item $binary_path -Recurse -Force
}
