### 用户输入和文件读写

# 用户输入
echo -n "输入用户名:"
read user_name
echo "username is ${user_name}"
echo "ip port:"
read ip port
echo "ip=${ip} port=${port}"
# 用户输入和文件读写
echo -e "test write\n" > test.txt
echo "test 001" >> test.txt
