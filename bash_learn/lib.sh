### 测试多文件 库函数
echo "In file lib.sh"
echo "接收传递的参数"$@
lib_var="libvar_test001"

libFunc()
{
    echo "In libFunc"
}