#!/bin/sh
echo "Content-Type: text/plain"
echo ""
tail -10 /tmp/telemetry.log 2>/dev/null || echo "NO_DATA"
