### 函数和多脚本文件调用

### 参数说明
MyFunction()
{
    echo "call my function"
    echo "参数个数: $#"
    echo "参数列表: $@"
    # for循环遍历所有参数
    for arg in $@;
    do
        echo -n "参数:${arg} "
    done
    echo
}
MyFunction
MyFunction "arg1" "arg2" "arg3"

### 多文件访问
# . 文件路径 运行参数
. lib.sh 001 002
echo "get lib_var = ${lib_var}" #引用文件的变量
libFunc #引用文件函数
