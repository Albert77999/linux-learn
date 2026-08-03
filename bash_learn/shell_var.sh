#!/bin/bash
### 变量处理

echo "shell变量处理"
var1="字符串变量001" # = 号两边不能有空格
var2=100
echo $var1
echo $var2
var3="var3"$var1$var2
echo $var3
echo "var1=$var1 var2=$var2"
echo 'var1=$var1 var2=$var2' # 单引号不会解析变量
echo "var1=${var1} var2=${var2}" # 变量名用大括号括起来，避免歧义
echo "var1 size is ${#var1}" # 变量长度
echo "var1 sub string 3,4 is ${var1:3:4}" # 变量截取

### shell脚本运行传递参数 ./shell_var.sh 192.168.0.206 80
echo "shell脚本运行传递参数"
echo $0 # 脚本名
echo $1 # 第一个参数
echo $2 # 第二个参数
echo $@ # 所有参数