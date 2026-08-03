### shell数组
arr1=(11 22 33 "第四" 55)
echo  $arr1
arr1[0]=111
echo "arr1[0]=${arr1[0]}"
echo "arr1[1]=${arr1[1]}"

### shell for
for var in {0..10}
do
    echo -n "var=${var} "
done
echo

### 循环数组
for var in ${arr1[@]}
do
    echo -n "var=${var} "
done
echo

### 遍历文件列表
$tmp=`ls`
echo "tmp=${tmp}"
for var in `ls`
do
    echo "filename=${var}"
done

### C语言风格的for循环
for ((i=0;i<10;i++))
do
    echo -n "i=${i} "
done
echo

# 无限循环
for ((;;))
do
    echo "无限循环"
if [ -e "./arr_for.sh"]; then
    break
fi
done