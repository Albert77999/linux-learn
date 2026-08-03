### 函数递归遍历目录和子目录文件
LSDir()
{
    for file in `ls $1 `
    do
        path="$1/$file"
        if [ -d "$path" ]; then
            # echo -n "-"
            echo "[$path]"
            LSDir "$path"
        else 
            echo $path
        fi
    done
}

LSDir $1