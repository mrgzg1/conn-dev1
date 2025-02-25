# Kerilog Mesh Network Message Flows
                                              SENSOR DATA FLOW
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

                                           CONFIGURATION AND COMMAND FLOW
┌────────────────┐           ┌────────────┐          ┌───────────┐              ┌───────────┐              ┌─────────┐           ┌─────────┐
│  Admin Server  │          "toBridge/{nodeID}"      │ m2bridge  │              │m2repeater │              │  m2node │           │ m2nano  │
│  (dashboard)   │───-──────>│MQTT Broker │─────────>│ (gateway) │─────────────>│  (relay)  │─────────────>│ (buffer)│-─────────>│(sensors)│
└────────────────┘           └────────────┘          └───────────┘              └───────────┘              └─────────┘           └─────────┘
                                                                                   Commands include:
                                                                                   - [enable:proxy:0,60,1]  (enable sensor 0 with interval 60s)
                                                                                   - [disable:proxy:0]      (disable sensor)
                                                                                   - [status]               (request status)
                                                                                   - [reset]                (reset device)

                                              ACKNOWLEDGMENT FLOW
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

                                              TIME SYNC FLOW
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

                                              SYSTEM INIT FLOW
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
