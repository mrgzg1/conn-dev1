# M2 - Mesh Monitoring
This system easily enables one to monitor variety of sensors using esp8266 mesh + mqtt + grafana

################################## INFRASTRUCTURE ##################################

[ Grafana <-> MySQL <-> PyMQTT <-> MQTT BROKER ]      Server
                                        ^
                                        |
                                        |
                                        v
                                  [ M2Bridge ]        Middleman
                                        ^
                                        |
                                        |
                                        v
                                [ M2Node + M2Nano ]   Sense Nodes

################################## COMMANDS ##################################
NOTES:
  - indexes start from 0 NOT 1

m2nano:
 Enable sensor
   // enable a given sensor type
   CMD: [enable:<sensor_type>:<sensor_number start from 0>:<sensor_interval, min 10s>]
   REPLY: [ack:enable:<sensor_type>:<sensor_number>:<sensor_interval>]
 Disable sensor
   CMD: [disable:<sensor_type>:<sensor_number>]
   REPLY: [ack:<sensor_type>/<sensor_number>]
 Status cmd:
   CMD: [status]
   REPLY: <TODO>
 Ready cmd:
   CMD: [ready]
   REPLY: init:success
 MISC:
  Init
   CMD:[init]
   CAUSE: Printed when initialization of the device has been completed
  Sensor Reading:
   CMD: [<sensor_type>/<sensor_number>:<sensor_value>:<sense_time>]
   CAUSE: Sensor has reached its reading interval thus it reads

 m2node:
 INPUT:
  >> PASS THROUGH TO m2bridge via to mesh
OUTPUT:
  >> PASS THROUGH to m2nano via from mesh

 m2bridge (INCOMING from m2nano):
  "BOOT:<timestamp>"
  "error:<string>"
  "ack:<enable/disable>:<sensor_type>:<sensor_number>"
  "<sensor_type>/<sensor_number>:<value>:<timestamp>"
