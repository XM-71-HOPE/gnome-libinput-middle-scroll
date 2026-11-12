#!/bin/sh
# 编译并安装 libinput 中键滚动钩子，写入 GNOME Shell 的 systemd drop-in
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
DEST="$HOME/.local/lib/libinput-scroll-emulation.so"

command -v gcc >/dev/null || { echo "需要 gcc"; exit 1; }
[ -f /usr/include/libinput.h ] || {
	echo "需要 libinput-devel：dnf install -y libinput-devel"
	exit 1
}

echo ">>> 编译"
gcc -shared -ldl -linput -fPIC "$HERE/hook.c" -o "$DEST"

echo ">>> 写入 drop-in"
mkdir -p "$HOME/.config/systemd/user/org.gnome.Shell@user.service.d"
cat > "$HOME/.config/systemd/user/org.gnome.Shell@user.service.d/preload.conf" <<CONF
[Service]
Environment=LD_PRELOAD=$DEST
CONF

systemctl --user daemon-reload || true

echo ">>> 完成，注销重登生效"
