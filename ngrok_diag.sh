#!/bin/bash

NGROK_DOMAIN="6792-73-238-75-136.ngrok-free.app"
NGROK_IP=$(dig +short $NGROK_DOMAIN | head -n1)

echo "🔧 Testing Connectivity to $NGROK_DOMAIN ($NGROK_IP)"

# Test DNS Resolution
if [ -z "$NGROK_IP" ]; then
    echo "❌ DNS Resolution Failed for $NGROK_DOMAIN"
    exit 1
else
    echo "✅ DNS Resolved: $NGROK_IP"
fi

# Test Common Ports Ngrok Uses
for PORT in 80 443 4040; do
    echo -n "🔍 Testing TCP Connection to $NGROK_IP on Port $PORT... "
    timeout 3 bash -c "cat < /dev/null > /dev/tcp/$NGROK_IP/$PORT" 2>/dev/null && \
    echo "✅ Open" || echo "❌ Blocked"
done

# Final CURL Test
echo "📡 Final CURL Test over HTTPS:"
curl -Iv --connect-timeout 5 https://$NGROK_DOMAIN 2>&1 | grep "Connected\|HTTP"

echo -e "\n✅ Diagnostic Complete."
