from flask import request
import json
import requests
from requests.auth import HTTPBasicAuth

PHLO_URL = ""
AUTH_TOKEN = ""
AUTH_ID = ""
def make_call(fr, to, text):
    print "CALLLLING______"
    print "from:", fr
    print "to:", to
    print "text:", text
    # convert to list
    if type(to) is list:
        if len(to) > 1:
            to_s = "<".join([str(each) for each in to])
            to = to_s
        else:
            to = str(to[0])
    URL = "https://" + PHLO_URL + "?to=" + to + "&text=" + text + "&from=" + fr
    resp = requests.get(
        URL,
        auth=HTTPBasicAuth(AUTH_ID, AUTH_TOKEN),
        headers={'Cache-Control': 'no-cache'}
        )
    print resp.text
    return resp
