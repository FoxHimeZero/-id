#!/system/bin/sh
ADB_DIR=/data/adb/all-id-spoof
TMP_DIR=/data/local/tmp/widevine-spoof

mkdir -p "$ADB_DIR" "$TMP_DIR"

echo "旧 ID: $(cat "$ADB_DIR/drm_id.txt" 2>/dev/null)"

NEW=$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')
echo "$NEW" > "$ADB_DIR/drm_id.txt"
chmod 600 "$ADB_DIR/drm_id.txt"

cp -f "$ADB_DIR/drm_id.txt" "$TMP_DIR/custom_id"
chmod 644 "$TMP_DIR/custom_id"

echo "新 ID: $NEW"

killall android.hardware.drm-service.widevine 2>/dev/null
killall android.hardware.drm@1.3-service.widevine 2>/dev/null
killall android.hardware.drm@1.4-service.widevine 2>/dev/null
sleep 1

echo "完成。重启后 ID 不变，App 重开即可生效。"
