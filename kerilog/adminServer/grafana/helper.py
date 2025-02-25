import requests
import json
import time
import logging

import config
import schema

LOG = logging.getLogger(__name__)

def make_grafana_req(url, method="get", add_header=None, data=None, admin=False):
    if admin:
        url = config.GRAFANA_ADMIN_BASE_URL + url
    else:
        url = config.GRAFANA_BASE_URL + url

    method = method.lower()
    headers = {
        "Authorization": "Bearer %s" % config.GRAFANA_ADMIN_API,
        "Content-Type": "application/json"
        }
    data = json.dumps(data)
    if type(add_header) == dict:
        headers.update(add_header)
    LOG.info("Making request:\nurl:%s", url)
    if method == "get":
        return requests.get(url, headers=headers)
    elif method == "post":
        return requests.post(url, data=data, headers=headers)
    elif method == "put":
        return requests.put(url, data=data, headers=headers)
    elif method == "delete":
        return requests.delete(url, headers=headers)

def update_alert(alert_id, sensor, time, users):
    pass

def create_alert(sensor, time, users):
    pass

def get_dashboard(uid):
    url = "/api/dashboards/uid/" + str(uid)
    return make_grafana_req(url)

def create_alert_db(alerts):
    dic = schema.db_schema(alerts)
    url = "/api/dashboards/db"
    return make_grafana_req(url, method="post", data=dic)

def get_notifications():
    return make_grafana_req("/api/alert-notifications")

def user_notification(user):
    if (user.grafana_notif_id is None):
        # create
        resp = create_notitification(user.name, user.number).json()
        print resp
        user.grafana_notif_id = resp["id"]
        user.save()
    else:
        update_notification(user.grafana_notif_id, user.name, user.number)
    current_notifications = get_notifications().json()

def update_notification(id, name, number):
    base_dict = {
        'id': id,
        'isDefault': False,
        'name': name,
        'settings': {
            'httpMethod': 'POST',
            'uploadImage': False,
            'url': 'http://localhost:3001/calling?ph=%d' % number
            },
        'type': 'webhook'
        }
    url = '/api/alert-notifications/' + str(id)
    return make_grafana_req(
        url,
        method="put",
        data=base_dict
    )

def create_notitification(name, number):
    base_dict = {
        'isDefault': False,
        'name': name,
        'settings': {
            'httpMethod': 'POST',
            'uploadImage': False,
            'url': 'http://localhost:3001/calling?ph=%d' % number
            },
        'type': 'webhook'
        }
    url = '/api/alert-notifications'
    return make_grafana_req(
        url,
        method="post",
        data=base_dict
    )

def pause_all_alerts():
    url = "/api/admin/pause-all-alerts"
    return make_grafana_req(url, method="post", data={"paused": True}, admin=True)

def unpause_all_alerts():
    url = "/api/admin/pause-all-alerts"
    return make_grafana_req(url, method="post", data={"paused": False}, admin=True)

def get_org():
    url = "/api/org"
    return make_grafana_req(url)

def get_annotations(start=0, end=time.time(), limit=100, type="annotations"):
    """
    start -> int in unix seconds
    end -> int in unix seconds
    limit -> limit
    """
    start = int(start)
    end = int(end)
    url = "/api/annotations?from=%s&to=%s" % (start, end)
    return make_grafana_req(url)
