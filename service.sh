#!/system/bin/sh
ADB_DIR=/data/adb/all-id-spoof
TMP_DIR=/data/local/tmp/widevine-spoof

[ ! -f "$ADB_DIR/drm_id.txt" ] && exit 0

while [ "$(getprop sys.boot_completed)" != "1" ]; do sleep 1; done
sleep 3

mkdir -p "$TMP_DIR"
cp -f "$ADB_DIR/drm_id.txt" "$TMP_DIR/custom_id"
chmod 644 "$TMP_DIR/custom_id"

# 重启 DRM 服务，让它重新加载
killall android.hardware.drm-service.widevine 2>/dev/null
killall android.hardware.drm@1.3-service.widevine 2>/dev/null
killall android.hardware.drm@1.4-service.widevine 2>/dev/null
