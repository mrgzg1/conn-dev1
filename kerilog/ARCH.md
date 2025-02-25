# Kerilog Mesh Network Message Flows
## SUMMARY

### Key Design Principles
- **Resilience**: Multi-layered retry mechanisms prevent data loss
- **Scalability**: Mesh topology allows dynamic network expansion
- **Flexibility**: Configurable sensors and intervals via remote commands
- **Persistence**: Flash storage preserves data during connectivity issues
- **Standardization**: MQTT integration enables use with common IoT tools

### System Capabilities
- Handles intermittent connectivity through store-and-forward
- Auto-configures nodes after power cycles
- Extends network range via repeater nodes
- Maintains accurate time synchronization
- Provides real-time monitoring via Grafana dashboards

## SENSOR DATA FLOW
```ascii
┌─────────┐ @v2:{timestamp}:{sensorID,data;} ┌─────────┐              ┌───────────┐            ┌───────────┐          ┌────────────┐
│ m2nano  │─────────────────────────────────>│ m2node  │─────────────>│m2repeater │───-───────>│ m2bridge  │─────────>│MQTT Broker │
│(sensors)│                                  │(buffer) │              │(relay)    │            │(gateway)  │          │            │
└─────────┘                                  └─────────┘              └───────────┘            └───────────┘          └──────┬─────┘
                                                 │                                                                           │
                                   Stores in SPIFFS flash if                                                                 │
                                   acknowledgment not received                                        "fromNode/{nodeID}/data" topic
                                                                                                                             │
                                                                                                                             v
                                                                                                                    ┌────────────────┐
                                                                                                                    │  Admin Server  │
                                                                                                                    │  (db storage)  │
                                                                                                                    └────────────────┘
```

Sensor data flows from m2nano (Arduino) to the server using a versioned format (@v2). The m2node buffers data and handles retries, storing unacknowledged messages in SPIFFS flash to prevent data loss during connectivity issues. M2repeaters extend network range, while m2bridge translates mesh messages to MQTT topics. The Admin Server subscribes to "fromNode/{nodeID}/data" topics and stores readings in MySQL for Grafana visualization.

## CONFIGURATION AND COMMAND FLOW
```ascii
┌────────────────┐           ┌────────────┐          ┌───────────┐              ┌───────────┐              ┌─────────┐           ┌─────────┐
│  Admin Server  │          "toBridge/{nodeID}"      │ m2bridge  │              │m2repeater │              │  m2node │           │ m2nano  │
│  (dashboard)   │───-──────>│MQTT Broker │─────────>│ (gateway) │─────────────>│  (relay)  │─────────────>│ (buffer)│-─────────>│(sensors)│
└────────────────┘           └────────────┘          └───────────┘              └───────────┘              └─────────┘           └─────────┘
                                                                                   Commands include:
                                                                                   - [enable:proxy:0,60,1]  (enable sensor 0 with interval 60s)
                                                                                   - [disable:proxy:0]      (disable sensor)
                                                                                   - [status]               (request status)
                                                                                   - [reset]                (reset device)
```

Commands flow in the opposite direction, from Admin Server to sensors. The server publishes to "toBridge/{nodeID}" MQTT topics, which m2bridge forwards through the mesh. Commands use a bracketed syntax [command:arg1:arg2] for operations like enabling sensors with specific intervals, requesting status, or triggering resets. This bidirectional control enables remote configuration without physical access to sensor nodes.

## ACKNOWLEDGMENT FLOW
```ascii
┌─────────┐                   ┌───────────┐
│ m2node  │────"data:{msg}"──>│ m2bridge  │
│         │                   │           │
│         │<───"ack:{msg}"────│           │
└─────────┘                   └───────────┘
     │
   If no ack received:
     │
     ▼
┌─────────────────────────┐
│ Add to unack_msg_list   │
│ Retry every 10 seconds  │
│ Up to 16 times          │
└─────────────────────────┘
     │
     ▼
┌─────────────────────────┐
│ If still not ack'd      │
│ Store in SPIFFS flash   │
│ Retry periodically      │
└─────────────────────────┘
```

Reliability is ensured through a multi-stage acknowledgment system. When m2node sends data, it expects an "ack:{msg}" response. Unacknowledged messages enter a retry queue with attempts every 10 seconds, up to 16 times. If still undelivered, messages are persisted to flash storage and retried when connectivity returns. This store-and-forward approach prevents data loss even during extended network outages.

## TIME SYNC FLOW
```ascii
┌─────────┐                   ┌─────────┐                   ┌───────────┐
│ m2nano  │────"[time]"───-──>│ m2node  │───────────────────│ m2bridge  │──(Time from mesh network)
│         │                   │         │                   │           │
│         │<─"time:{unix_ts}"─│         │                   │           │
└─────────┘                   └─────────┘                   └───────────┘
     │
     ▼
┌────────────────────┐
│ Set internal clock │
│ Timestamp data     │
└────────────────────┘
```

Accurate timestamps are critical for sensor data. The m2nano requests time with "[time]" command, and m2node responds with current Unix timestamp. This synchronizes the Arduino's clock, ensuring all sensor readings have consistent timestamps regardless of when they reach the server. Time propagates from m2bridge (connected to WiFi with NTP access) through the mesh network.

## SYSTEM INIT FLOW
```ascii
┌─────────┐                             ┌─────────┐                             ┌───────────┐                       ┌────────────┐                    ┌────────────────┐
│ m2nano  │──────"init:ready"──-───────>│ m2node  │────--──────────────────────>│ m2bridge  │─"fromNode/{id}/init"─>│MQTT Broker │──────--───────────>│  Admin Server  │
│         │                             │         │                             │           │                       │            │                    │                │
│         │<─[enable:proxy:0,60,1;...]──│         │<─────--─────────────────────│           │<"toBridge/{id}" ──────│            │<───Configuration───│                │
└─────────┘                             └─────────┘                             └───────────┘                       └────────────┘                    └────────────────┘
                                              │
                                              ▼
                                    ┌──────────────────────┐
                                    │ Load saved configs   │
                                    │ from flash storage   │
                                    │ (CONFIG_FILE)        │
                                    └──────────────────────┘
```

On startup, m2nano sends "init:ready" to signal it's operational. This triggers a configuration cascade: m2node loads saved settings from flash or requests them from m2bridge, which publishes to MQTT. The Admin Server responds with sensor configurations, which propagate back to m2nano. This auto-configuration ensures nodes resume proper operation after power cycles without manual intervention.

## IMPLEMENTATION NOTES

### Hardware Requirements
- ESP8266 for m2node, m2bridge, m2repeater
- Arduino Nano for m2nano sensor interface
- Server with MQTT broker, MySQL, Grafana

### Network Considerations
- Mesh password: "somethingSneaky"
- Channel: 11
- Max nodes: ~20 per mesh network
- Typical range: 30-100m between nodes (environment dependent)

### Performance
- Message latency: 50-500ms per hop
- Flash storage: ~2MB available for message buffering
- Retry interval: 10s (configurable in code)

### Limitations
- Max sensor reading frequency: 10s
- Max undelivered messages in memory: 64
- Max flash storage messages: ~10,000 (32 bytes each)
- Requires at least one m2bridge with stable WiFi connection