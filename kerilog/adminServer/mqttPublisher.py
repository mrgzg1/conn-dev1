import paho.mqtt.client as mqtt
import sys

from config import MQTT_BROKER

def publish_msg(topic, msg):
    client = mqtt.Client()
    client.connect(MQTT_BROKER, 1883, 60)
    r = client.publish(topic, msg)
    print r
    return r

if __name__ == "__main__":
    publish_msg(sys.argv[1], sys.argv[2])
