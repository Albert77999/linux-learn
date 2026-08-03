#!/bin/bash
### shell条件判断if文件_字符串和运算符判断

if [ 1 -eq 1 ]; then
    echo "1 = 1 true"
fi

age=10
if [ "$age" -gt 18 ]; then
    echo "age > 18 true"
else
    echo "age > 18 false"
fi

if [ "$age" -lt 100 ]; then
    echo "age < 100 true"
else
    echo "age < 100 false"
fi

if [ "$age" -ge 10 ]; then
    echo "age >= 10 true"
else
    echo "age >= 10 false"
fi

if [ $age -le 10 ]; then
    echo "age <= 10 true"
else
    echo "age <= 10 false"
fi

if [ $age -ne 10 ]; then
    echo "age != 10 true"
else
    echo "age != 10 false"
fi

### 字符串比较
name="http"
if [ "$name" == "http" ]; then
    echo "name == http true"
else
    echo "name == http false"
fi

if [ "$name" != "http" ]; then
    echo "name != http true"
else
    echo "name != http false"
fi
name=""
if [ -z "$name" ]; then
    echo "name is null"
else
    echo "name is not null"
fi
if [ -n "$name" ]; then
    echo "name is not null"
else
    echo "name is null"
fi

### 文件判断
file="/home/whb/linux_learn/bash_learn/first.sh"
if [ -r $file ]; then. # -r可读,-w可写,-x可执行,-f普通文件,-d目录文件,-e文件存在
    echo "$file is readable"
else
    echo "$file is not readable"
fi

## 逻辑运算判断
if [ "$age" -gt 18 ] && [ "$age" -lt 100 ]; then
    echo "age > 18 and age < 100 true"
else
    echo "age > 18 and age < 100 false"
fi