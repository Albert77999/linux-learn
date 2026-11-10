#!/usr/bin/env bash
set -euo pipefail

# 安装到当前用户的家目录，不需要 sudo；不会创建系统服务。
VERSION="0.58.1"
INSTALL_DIR="$HOME/.local/opt/frp/$VERSION"
BIN_DIR="$HOME/.local/bin"
RELEASE_URL="https://github.com/fatedier/frp/releases/download/v$VERSION"

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "错误：此脚本仅支持 Linux。" >&2
    exit 1
fi

case "$(uname -m)" in
    x86_64|amd64) ARCH="amd64" ;;
    aarch64|arm64) ARCH="arm64" ;;
    *) echo "错误：暂不支持当前架构：$(uname -m)" >&2; exit 1 ;;
esac

for cmd in curl tar sha256sum awk; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "错误：缺少命令 $cmd，请先安装后重试。" >&2
        exit 1
    fi
done

# 不覆盖已有版本或已有命令，包括失效的符号链接。
for target in "$INSTALL_DIR" "$BIN_DIR/frps" "$BIN_DIR/frpc"; do
    if [[ -e "$target" || -L "$target" ]]; then
        echo "错误：目标已存在，未覆盖：$target" >&2
        exit 1
    fi
done

ARCHIVE="frp_${VERSION}_linux_${ARCH}.tar.gz"
TMP_DIR="$(mktemp -d)"
trap 'rm -rf -- "$TMP_DIR"' EXIT

echo "正在下载 frp $VERSION（Linux $ARCH）……"
curl -fLsS --retry 2 --connect-timeout 15 --max-time 180 \
    "$RELEASE_URL/$ARCHIVE" -o "$TMP_DIR/$ARCHIVE"
curl -fLsS --retry 2 --connect-timeout 15 --max-time 180 \
    "$RELEASE_URL/frp_sha256_checksums.txt" -o "$TMP_DIR/checksums.txt"

# 校验文件和压缩包都来自指定版本的官方发布页。
EXPECTED="$(awk -v name="$ARCHIVE" '$2 == name || $2 == "*" name {print $1}' "$TMP_DIR/checksums.txt")"
if [[ ! "$EXPECTED" =~ ^[[:xdigit:]]{64}$ ]]; then
    echo "错误：未在校验文件中找到唯一的 SHA-256。" >&2
    exit 1
fi
if ! (cd "$TMP_DIR" && printf '%s  %s\n' "$EXPECTED" "$ARCHIVE" | sha256sum -c --status); then
    echo "错误：SHA-256 校验失败，停止安装。" >&2
    exit 1
fi
echo "SHA-256 校验通过。"

tar -xzf "$TMP_DIR/$ARCHIVE" -C "$TMP_DIR"
SOURCE_DIR="$TMP_DIR/frp_${VERSION}_linux_${ARCH}"
for file in frps frpc frps.toml frpc.toml LICENSE; do
    if [[ ! -f "$SOURCE_DIR/$file" ]]; then
        echo "错误：压缩包中缺少 $file。" >&2
        exit 1
    fi
done

mkdir -p "$(dirname "$INSTALL_DIR")" "$BIN_DIR"
mkdir "$INSTALL_DIR" # 原子创建，避免覆盖已存在的目录。
cp "$SOURCE_DIR"/{frps,frpc,frps.toml,frpc.toml,LICENSE} "$INSTALL_DIR/"
chmod 755 "$INSTALL_DIR/frps" "$INSTALL_DIR/frpc"
ln -s "$INSTALL_DIR/frps" "$BIN_DIR/frps"
ln -s "$INSTALL_DIR/frpc" "$BIN_DIR/frpc"

echo "安装完成：$INSTALL_DIR"
"$BIN_DIR/frps" --version
"$BIN_DIR/frpc" --version
echo '如果命令不在 PATH 中，请使用 ~/.local/bin/frps 和 ~/.local/bin/frpc。'
