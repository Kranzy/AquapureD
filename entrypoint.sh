#!/bin/bash
# Entrypoint script for AquapureD
# Substitutes environment variables into config file at startup

CONFIG_SRC="/app/release/aquapured.conf"
CONFIG_OUT="/config/aquapured.conf"

# If no config exists in /config yet, copy the template from the image
if [ ! -f "$CONFIG_OUT" ]; then
    echo "No config found at $CONFIG_OUT, using built-in template..."
    cp "$CONFIG_SRC" "$CONFIG_OUT"
fi

# Substitute environment variables into the config
# Uses a temp file to avoid partial writes
TEMP_CONFIG=$(mktemp)

sed \
    -e "s|\${MQTT_ADDRESS}|${MQTT_ADDRESS:-localhost:1883}|g" \
    -e "s|\${MQTT_USER}|${MQTT_USER:-}|g" \
    -e "s|\${MQTT_PASSWORD}|${MQTT_PASSWORD:-}|g" \
    -e "s|\${MQTT_TOPIC}|${MQTT_TOPIC:-}|g" \
    "$CONFIG_OUT" > "$TEMP_CONFIG"

mv "$TEMP_CONFIG" "$CONFIG_OUT"

echo "Config written to $CONFIG_OUT"

# Start AquapureD in foreground (not daemonized)
exec /app/release/aquapured -d -c "$CONFIG_OUT"
