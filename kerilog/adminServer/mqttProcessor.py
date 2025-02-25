#!/usr/bin/env python
import signal
import sys
import time
import logging
import math
import datetime

import paho.mqtt.client as mqtt

import logger
from mysql import coredb
from config import MQTT_BROKER, BRIDGE_TOPIC, NODE_TOPIC
import db_helper

LOG = logging.getLogger(__name__)
BRIDGE_MQTT_LOG = logging.getLogger("bridge_mqtt")
NODE_MQTT_LOG = logging.getLogger("node_mqtt")


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    LOG.info("Connected with result code "+str(rc))
    client.subscribe(BRIDGE_TOPIC)
    client.subscribe(NODE_TOPIC)

def unknwon_message(client, userdata, msg):
    LOG.warning("RECVD unknown")
    LOG.warning(msg.topic)
    LOG.warning(msg.payload)

def node_message(client, userdata, og_msg):
    topic = str(og_msg.topic).rstrip().lower()
    topic = topic[len(NODE_TOPIC)-1:]
    msg = str(og_msg.payload).rstrip().lower()
    topics = topic.split('/')
    node_id = int(topics[0])
    try:
        assert(node_id in db_helper.get_node_hw_ids())
        if topics[1] == "init" and msg.startswith("ready"):
            # todo: Add the thing to db
            node_is_ready(client, node_id)
        elif topics[1] == "ack":
            # ensure got ack for all the sensors that were enabled
            handle_ack_msg(client, node_id, msg)
        elif topics[1] == "status":
            handle_status_msg(node_id, topics, msg)
        elif topics[1] == "data":
            handle_data_point(node_id, topics, msg)
            # if we are here without exception, then ack the data point
            client.publish("toBridge/"+str(node_id), "ackd:" + msg)
        else:
            NODE_MQTT_LOG.error("Ignored")

        # log the critical values to db
        if topics[1] in ["error", "info", "status", "init"]:
            node = db_helper.get_node_by_hw_id(node_id)
            coredb.insert_data_points([
                [node.name + "-node", int(time.time()), str("-".join(topics[1:]) + ":" + msg)]
            ])
    except Exception as e:
        LOG.exception(e)
        LOG.error("Failed to process node message, ignoring.\nTOPIC:%s|MSG:%s",
                og_msg.topic, og_msg.payload)
    finally:
        if topics[1] != "data":
            NODE_MQTT_LOG.error("=====\ntopic:%s\nmsg:%s\n=====", topic, msg)

def handle_status_msg(node_id, topics, msg):
    sensor_type, msg = msg.split(':')
    hw_ids = proc_status_msg(msg)
    for each in hw_ids:
        s = db_helper.get_sensor_by_hw_id(int(node_id), int(each), sensor_type)
        db_helper.set_last_enabled_now(s)

def proc_status_msg(m):
    m = m[1:-1]
    m = m.split('[')[1:]
    h_ids = []
    for each in m:
        h_ids.append(each.split(',')[0])
    return h_ids

def handle_data_point(node_id, topics, msg):
    if msg.startswith("v2"):
        return handle_data_point_v2(node_id, topics, msg)
    # store proxy data
    try:
        id, value, time = msg.split(":")
        sensor = db_helper.get_sensor_by_id(id)
        LOG.debug("Got sensor data, Type:%s | Value:%s | Time:%s | node:%d" % (sensor.type, value, time, node_id))
        # little hack for proxy calc
        if sensor.type == "proxy":
            circum = int(db_helper.get_sensor_misc(sensor, "circum"))
            value = float(value) * circum / 1000

        #get the id based on the name
        sensor_id = sensor.name
        coredb.insert_data_points([
            [sensor_id, int(time), value]
        ])
        #send ack

    except Exception as e:
        LOG.error("node_id:%d. Failed to process data msg:%s" % (node_id, msg))
        LOG.exception(e)

def handle_data_point_v2(node_id, topics, msg):
    # store proxy data
    try:
        ignore, timestamp, values = msg.split(":")
        timestamp = int(timestamp) # cast this to time
        values = values.split(";")[:-1] # ignore the last one
        for value in values:
            id, value = value.split(",") # split this
            sensor = db_helper.get_sensor_by_id(id)
            # little hack for proxy calc
            if sensor.type == "proxy":
                circum = int(db_helper.get_sensor_misc(sensor, "circum"))
                value = float(value) * circum / 1000
            LOG.debug("Got sensor data, Type:%s | Value:%s | Time:%s | node:%d" % (sensor.type, value, time, node_id))
            #get the id based on the name
            sensor_id = sensor.name
            coredb.insert_data_points([
                [sensor_id, timestamp, value]
            ])
    except Exception as e:
        LOG.error("node_id:%d. Failed to process v2 data msg:%s" % (node_id, msg))
        LOG.exception(e)

def handle_ack_msg(client, node_id, msg):
    # todo: add checking in place to make sure we have all the sensors enabled
    # also to make sure that all the sensors are sending data religiously
    msgs = msg.split(":")
    if msgs[0] == "enable":
        # thing is enabled
        sensor_type, sensor_num = msgs[1].split('/')
        sensor = db.helper.get_sensor_by_hw_id(node_id, hw_id, sensor_type)
        db_helper.set_last_enabled_now(sensor)
        LOG.debug("enabled. %s %d" % (sensor_type, int(sensor_num)))
    else:
        LOG.warning("not sure what ack we got!")

def node_is_ready(client, node_id):
    # we are ready to send over config
    topic = "toBridge/%d" % (node_id)

    # set the time on the device
    date = datetime.datetime.utcnow()
    date_str = date.strftime('%Y:%-m:%-d:%-H:%-M:%-S')
    client.publish(topic, "[settime:%s]"%date_str);
    time.sleep(0.5)

    # this enables all the sensors
    for each in db_helper.get_enable_cmds(node_id):
        LOG.debug("sending to node:%d msg:%s" % (node_id, each))
        client.publish(topic, each)
        time.sleep(0.5)

    # this sets the client to be enabled
    client.publish(topic, "[config]");

def bridge_message(client, userdata, msg):
    topic = str(msg.topic).rstrip().lower()
    topic = topic[len(BRIDGE_TOPIC)-1:]
    msg = str(msg.payload).rstrip().lower()
    BRIDGE_MQTT_LOG.error("=====\ntopic:%s\nmsg:%s\n=====", topic, msg)

    # if we got the meshsize, write it to DB so we can stay live on it
    if(topic == "meshsize"):
        value = int(msg)
        sensor_id = "meshsize"
        coredb.insert_data_points([
            [sensor_id, int(time.time()), value]
        ])

def signal_handler(sig, frame):
        coredb.flush_all()

def main():
    #halt handler
    signal.signal(signal.SIGINT, signal_handler)
    print('Press Ctrl+C')

    client = mqtt.Client()
    client.on_connect = on_connect
    client.message_callback_add("fromBridge/#", bridge_message)
    client.message_callback_add("fromNode/#", node_message)
    # client.on_message = unknwon_message
    client.connect(MQTT_BROKER, 1883, 60)
    client.loop_forever()
    LOG.error("Loop failed!")

if __name__ == "__main__":
    main()
