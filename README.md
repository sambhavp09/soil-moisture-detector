# soil-moisture-detector

Soil moisture dashboard for NodeMCU/Arduino serial telemetry.

## Run locally

1. Open `index.html` in Chrome or Edge.
2. Click **CONNECT ARDUINO**.
3. Select the NodeMCU serial port and keep baud at `115200`.

## Expected NodeMCU payload

The dashboard now supports JSON lines like:

```json
{"uptimeMs":1000,"sensorDry":0,"moisturePctApprox":100,"pump":0}
```

Supported fields:
- `moisturePctApprox` or `moisturePct` (0-100)
- `sensorDry` (`0` wet, `1` dry)
- `pump` (`0` off, `1` on)
- `uptimeMs`

It also still accepts plain numeric/pct serial values for compatibility.
