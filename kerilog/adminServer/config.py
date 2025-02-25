import os

# Create dummy secrey key so we can use sessions
SECRET_KEY = '123456790'

# Create in-memory database
DATABASE_FILE = 'sample_db.sqlite'
SQLALCHEMY_DATABASE_URI = 'sqlite:///' + DATABASE_FILE
# SQLALCHEMY_ECHO = True

# Flask-Security config
SECURITY_URL_PREFIX = "/admin"
SECURITY_PASSWORD_HASH = "pbkdf2_sha512"
SECURITY_PASSWORD_SALT = ""

# Flask-Security URLs, overridden because they don't put a / at the end
SECURITY_LOGIN_URL = "/login/"
SECURITY_LOGOUT_URL = "/logout/"
SECURITY_REGISTER_URL = "/register/"

SECURITY_POST_LOGIN_VIEW = "/admin/"
SECURITY_POST_LOGOUT_VIEW = "/admin/"
SECURITY_POST_REGISTER_VIEW = "/admin/"

# Flask-Security features
SECURITY_REGISTERABLE = True
SECURITY_SEND_REGISTER_EMAIL = False
SQLALCHEMY_TRACK_MODIFICATIONS = False


MQTT_BROKER = "localhost"
BRIDGE_TOPIC = "fromBridge/#"
NODE_TOPIC = "fromNode/#"

ADMIN_SERVER_PORT = 3001
ADMIN_SERVER_HOST = "0.0.0.0"

# BASE_GRAFANA = "219.91.138.148"
# BASE_GRAFANA = "192.168.1.56"
BASE_GRAFANA = "127.0.0.1"

# RPi KEY
if "DEBUG" in os.environ:
    GRAFANA_ADMIN_API = "="
else:
    GRAFANA_ADMIN_API = "="

GRAFANA_ADMIN = "admin"
GRAFANA_ADMIN_PASS = "admin"
GRAFANA_ALERT_DB_UID = "alertdb"
GRAFANA_BASE_URL = "http://%s:3000" % BASE_GRAFANA
GRAFANA_ADMIN_BASE_URL = "http://%s:%s@%s:3000" % (
                                GRAFANA_ADMIN,
                                GRAFANA_ADMIN_PASS,
                                BASE_GRAFANA
                        )
