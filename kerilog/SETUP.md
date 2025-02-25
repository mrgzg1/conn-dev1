## Setup a new node:
1. Get Node Id by flashing the m2nodeDebug and seeing the serial log
2. Add config in pyServer/nodes/<anyname>.py based on what sensors you want to use
3. Push commit to master
4. Pull commit on RaspberryPi:
  - cd Code/kerilog
  - git pull
5. `sudo reboot` RPi
6. Reboot the new node





## View log on RaspberryPi
1. SSH to raspberry pi
2. Run this to view log `tail -f ~/mqtt.log`
3. Press Ctrl+C to exit

## Get status of the mesh
1. Flash m2nodeDebug and type [status] to get detailed status of the network
2. Make sure the debug node is within the network

alternative:
1. Publish empty message to queue "toBridge/status"
2. You should get reply in the RPi log with size of the mesh

## Get status of a remote node
1. Get node id of the remote
2. Publish "[status]" message to queue "toBridge/<node_id>"
3. You should get reply in the RPi log with status of the node
