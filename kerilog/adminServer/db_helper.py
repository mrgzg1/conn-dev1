from flask_sqlalchemy import SQLAlchemy
from flask_security.utils import encrypt_password

from app import app, SensorTypes
from app import Node, Sensor, Alert, User, SensorTypes, Role
import datetime
import json
import requests
import config

db = SQLAlchemy()

app_ctxt = app.app_context

def get_nodes():
    with app_ctxt():
        nodes = db.session.query(Node).all()
        return nodes

def get_node_hw_ids():
    nodes = get_nodes()
    return [each.hw_id for each in nodes]

def get_sensor_misc(sensor, key):
    #get the misc parsed as dict from the sensor
    misc = json.loads(sensor.misc)
    return misc[key]

def get_enable_cmds(node_hw_id):
    node = get_node_by_hw_id(node_hw_id)
    cmds = {each:[] for each in SensorTypes}
    # add to each type
    for each in node.sensors:
        cmds[each.type].append(get_enable_cmd(each))
    # consolidate each type now
    final_cmds = []
    for key, value in cmds.iteritems():
        type_cmd = "[enable:%s:%s]" % (
            key,
            ';'.join(value)
        )
        final_cmds.append(type_cmd)
    return final_cmds

def get_enable_cmd(sensor):
    """
    [enable:type:hw_id:interval:id]
    """
    return "%s,%s,%s" % (
        sensor.hw_id,
        sensor.interval,
        sensor.id
    )

def set_last_enabled_now(sensor):
    url = "http://localhost:%s/sensor/set_status/%s" %(
        config.ADMIN_SERVER_PORT,
        sensor.id
    )
    requests.get(url)

def session_commit():
    with app_ctxt():
        db.session.commit()

def get_node_by_hw_id(hw_id):
    with app_ctxt():
        return db.session.query(Node).filter(Node.hw_id == hw_id).first()

def get_sensor_by_id(id):
    id = int(id)
    with app_ctxt():
        return db.session.query(Sensor).get(id)

def get_sensor_by_hw_id(node_id, hw_id, type):
    with app_ctxt():
        node = get_node_by_hw_id(node_id)
        for each in node.sensors:
            if each.type == type and each.hw_id == hw_id:
                return each

def create_admin():
    with app_ctxt():
        user_role = Role(name='user')
        super_user_role = Role(name='superuser')
        test_user = User(
            name='Admin',
            email='adminpass',
            password=encrypt_password('admin'),
            roles=[user_role, super_user_role],
            number=919925324418
        )
        test_user.save()

def get_alerts_dict():
    """
    Returns all the alerts in a dict as follows
    {
        alert_id: {
            users: [ids of users],
            sensor: name of the sensor,
            minutes_off: time threshold for alert
        }
    }
    """
    with app_ctxt():
        alerts = db.session.query(Alert).all()
        master_dict = {}
        for alert in alerts:
            master_dict[alert.id] = {
                "users": [each.id for each in alert.users],
                "sensor": alert.sensor.name,
                "minutes_off": alert.minutes_off
            }
        return master_dict

def get_user_id_number():
    with app_ctxt():
        alerts = db.session.query(Alert).all()
