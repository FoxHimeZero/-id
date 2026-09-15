#!/system/bin/sh
SKIPUNZIP=1
[ "$BOOTMODE" != true ] && abort "请在 KernelSU/Magisk 里安装"

unzip -o "$ZIPFILE" -x 'META-INF/*' -d "$MODPATH" >&2
set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/action.sh" 0 0 0755
set_perm "$MODPATH/service.sh" 0 0 0755
set_perm "$MODPATH/post-fs-data.sh" 0 0 0755

# 关键路径：必须可以被 system 读
TMP_DIR=/data/local/tmp/widevine-spoof
ADB_DIR=/data/adb/all-id-spoof

mkdir -p "$TMP_DIR" "$ADB_DIR"
chmod 755 "$TMP_DIR" "$ADB_DIR"

# 只在第一次安装生成 ID，不覆盖已有的
if [ ! -f "$ADB_DIR/drm_id.txt" ]; then
    od -An -N16 -tx1 /dev/urandom | tr -d ' \n' > "$ADB_DIR/drm_id.txt"
    ui_print "- 已生成初始 ID"
else
    ui_print "- 保留已有 ID"
fi
chmod 600 "$ADB_DIR/drm_id.txt"

# 复制一份到 tmp 目录（Hook 读这个）
cp -f "$ADB_DIR/drm_id.txt" "$TMP_DIR/custom_id"
chmod 644 "$TMP_DIR/custom_id"

ui_print "- 安装完成，请重启手机"
