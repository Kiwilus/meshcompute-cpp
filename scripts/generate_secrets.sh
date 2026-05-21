#!/bin/bash
CONFIG_FILE="${1:-config/config.json}"
ADMIN_TOKEN=$(openssl rand -hex 32)
REG_TOKEN=$(openssl rand -hex 32)
ADMIN_HASH=$(echo -n "$ADMIN_TOKEN" | openssl dgst -sha256 | awk '{print $2}')
REG_HASH=$(echo -n "$REG_TOKEN" | openssl dgst -sha256 | awk '{print $2}')

echo "Admin token (save): $ADMIN_TOKEN"
echo "Reg token (save):  $REG_TOKEN"

if command -v jq &> /dev/null; then
    jq --arg ah "$ADMIN_HASH" --arg rh "$REG_HASH" \
       '.server.admin_token_hash = $ah | .server.registration_token_hash = $rh' \
       "$CONFIG_FILE" > tmp.$$.json && mv tmp.$$.json "$CONFIG_FILE"
    echo "Config updated."
else
    echo "jq not found. Add manually:"
    echo "  admin_token_hash: $ADMIN_HASH"
    echo "  registration_token_hash: $REG_HASH"
fi