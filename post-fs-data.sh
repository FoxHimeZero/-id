#!/system/bin/sh
ADB_DIR=/data/adb/all-id-spoof
TMP_DIR=/data/local/tmp/widevine-spoof

[ ! -f "$ADB_DIR/drm_id.txt" ] && exit 0

mkdir -p "$TMP_DIR"
chmod 755 "$TMP_DIR"
cp -f "$ADB_DIR/drm_id.txt" "$TMP_DIR/custom_id"
chmod 644 "$TMP_DIR/custom_id"
