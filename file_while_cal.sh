### while循环和文件按行读取
# while true;
# do
#     echo -n "@"
#     sleep 1
# done

while read line;
do
echo ${line}
echo "------------"
done < file_while_cal.sh #重定向到while

while true;
do
    echo -n "输入命令(exit退出):"
    read cmd
    if [ ${cmd} == "exit" ]; then
        break
    fi
    echo "cmd is ${cmd}"
done

### 算术运算和while循环
i=0
((i++))
echo "i=${i}"
