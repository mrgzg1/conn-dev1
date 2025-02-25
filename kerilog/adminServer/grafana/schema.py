def db_schema(alerts):
    DB_SCHEMA = {
      "dashboard": {
        "title": 'alert',
        "timezone": "browser",
        "schemaVersion": 16,
        "version": 0,
        "uid": 'alert',
        "panels": [panel_schema(index, alert) for index, alert in enumerate(alerts)]
      },
      "overwrite": True
    }
    return DB_SCHEMA

def alert_schema(alert):
    if 'no data' in alert.name.lower():
        return _no_data_alert(alert)
    else:
        return _alert(alert)

def _no_data_alert(alert):
    ALERT_SCHEMA = \
        {u'conditions': [{u'evaluator': {u'params': [], u'type': u'no_value'},
        u'operator': {u'type': u'and'},
        u'query': {u'params': [u'A', u'%dm' % alert.minutes_out_of, u'now']},
        u'reducer': {u'params': [], u'type': u'sum'},
        u'type': u'query'}],
        u'executionErrorState': u'alerting',
        u'frequency': u'60s',
        u'handler': 1,
        u'message': alert.message,
        u'name': alert.name,
        u'noDataState': u'alerting',
        u'notifications': [{u'id': user.grafana_notif_id} for user in alert.users]}
    return ALERT_SCHEMA

def _alert(alert):
    ALERT_SCHEMA = \
        {u'conditions': [{u'evaluator': {u'params': [alert.minutes_out_of - alert.minutes_off +1], u'type': u'lt'},
        u'operator': {u'type': u'and'},
        u'query': {u'params': [u'A', u'%dm' % alert.minutes_out_of, u'now']},
        u'reducer': {u'params': [], u'type': u'sum'},
        u'type': u'query'}],
        u'executionErrorState': u'keep_state',
        u'frequency': u'60s',
        u'handler': 1,
        u'message': alert.message,
        u'name': alert.name,
        u'noDataState': u'keep_state',
        u'notifications': [{u'id': user.grafana_notif_id} for user in alert.users]}
    return ALERT_SCHEMA


def panel_schema(index, alert):
    assert alert.minutes_out_of >= alert.minutes_off
    PANEL_SCHEMA = \
    {u'alert': alert_schema(alert),
     u'aliasColors': {},
     u'bars': False,
     u'dashLength': 10,
     u'dashes': False,
     u'datasource': None,
     u'fill': 1,
     u'gridPos': {u'h': 4, u'w': 24, u'x': 0, u'y': 4*index},
     u'id': 1*(index+1),
     u'legend': {u'avg': True,
                 u'current': False,
                 u'max': False,
                 u'min': False,
                 u'show': True,
                 u'total': True,
                 u'values': True},
     u'lines': True,
     u'linewidth': 1,
     u'links': [],
     u'nullPointMode': u'null',
     u'percentage': False,
     u'pointradius': 5,
     u'points': False,
     u'renderer': u'flot',
     u'seriesOverrides': [],
     u'spaceLength': 10,
     u'stack': False,
     u'steppedLine': False,
     u'targets': [{u'alias': u'',
                   u'format': u'time_series',
                   u'hide': False,
                   u'rawSql': u"SELECT\n  UNIX_TIMESTAMP(time) as time_sec,\n  IF(convert(value, decimal) > 0, 1, 0),\n  substring(id, 1, 9) as metric\nFROM sense_data\nWHERE $__timeFilter(time) AND id = '%s'\nORDER BY time ASC" % alert.sensor.name,
                   u'refId': u'A'}],
     u'thresholds': [],
     u'timeFrom': None,
     u'timeShift': None,
     u'title': alert.name,
     u'tooltip': {u'shared': True, u'sort': 0, u'value_type': u'individual'},
     u'type': u'graph',
     u'xaxis': {u'buckets': None,
                u'mode': u'time',
                u'name': None,
                u'show': True,
                u'values': []},
     u'yaxes': [{u'format': u'short',
                 u'label': None,
                 u'logBase': 1,
                 u'max': u'1',
                 u'min': None,
                 u'show': True},
                {u'format': u'short',
                 u'label': None,
                 u'logBase': 1,
                 u'max': None,
                 u'min': None,
                 u'show': True}],
     u'yaxis': {u'align': False, u'alignLevel': None}
     }
    return PANEL_SCHEMA
