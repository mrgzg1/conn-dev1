#!venv/bin/python
import os
import enum
import sys
import json
import datetime


from flask import Flask, url_for, redirect, render_template, request, abort
from flask_sqlalchemy import SQLAlchemy
from flask_security import Security, SQLAlchemyUserDatastore, \
    UserMixin, RoleMixin, login_required, current_user
from flask_security.utils import encrypt_password
import flask_admin
from flask_admin import helpers as admin_helpers
from flask_admin import BaseView, expose
from flask_admin.contrib import sqla
from flask_security import current_user

import config
import nodes
import db_helper
from phone_caller import make_call
from mqttPublisher import publish_msg
from grafana import helper as grafana_helper

# Create Flask application
app = Flask(__name__)
app.config.from_pyfile('config.py')
db = SQLAlchemy(app)

# Define models
roles_users = db.Table(
    'roles_users',
    db.Column('user_id', db.Integer(), db.ForeignKey('user.id')),
    db.Column('role_id', db.Integer(), db.ForeignKey('role.id'))
)

alert_users = db.Table(
    'alert_users',
    db.Column('user_id', db.Integer(), db.ForeignKey('user.id')),
    db.Column('alert_id', db.Integer(), db.ForeignKey('alert.id'))
)

class Role(db.Model, RoleMixin):
    id = db.Column(db.Integer(), primary_key=True)
    name = db.Column(db.String(80), unique=True)
    description = db.Column(db.String(255))

    def __str__(self):
        return self.name

class Alert(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(255), unique=True, nullable=False)
    message = db.Column(db.String(1023), nullable=False)
    minutes_off = db.Column(db.Integer, nullable=False)
    minutes_out_of = db.Column(db.Integer, nullable=False)
    sensor_id = db.Column(db.Integer, db.ForeignKey('sensor.id'), nullable=False)
    sensor = db.relationship("Sensor", back_populates="alerts")
    users = db.relationship('User', secondary=alert_users,
                            backref=db.backref('alerts', lazy='dynamic'))

    def __str__(self):
        return str(self.name)

class Node(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(255), unique=True, nullable=False)
    misc = db.Column(db.String(2047))
    hw_id = db.Column(db.Integer, nullable=False)
    sensors = db.relationship("Sensor", back_populates="node")

    def machine_dict(self):
        info = {
            'id': self.id,
            'name': self.name,
            'misc': self.misc,
            'hw_id': self.hw_id,
            'sensors': {}
        }
        for each in self.sensors:
            info['sensors'][each.id] = each.machine_dict()
        return info

    def __str__(self):
        return str(self.name)

SensorTypes = ["proxy", "pt", "bat"]
class Sensor(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    type = db.Column(db.Enum(*SensorTypes,
                    name="ValueTypes"), nullable=False)
    name = db.Column(db.String(255), unique=True, nullable=False)
    node_id = db.Column(db.Integer, db.ForeignKey('node.id'), nullable=False)
    node = db.relationship("Node", back_populates="sensors")
    misc = db.Column(db.String(2047), nullable=False)
    hw_id = db.Column(db.Integer, nullable=False)
    interval = db.Column(db.Integer, nullable=False)
    last_enabled = db.Column(db.DateTime)
    alerts = db.relationship("Alert", back_populates="sensor")

    def machine_dict(self):
        return {
            'id': self.id,
            'type': self.type,
            'name': self.name,
            'misc': self.misc,
            'node': self.node,
            'last_enabled': self.last_enabled
        }
    def __str__(self):
        return str(self.name)



class User(db.Model, UserMixin):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(255), nullable=False, unique=True)
    email = db.Column(db.String(255), unique=True)
    number = db.Column(db.Integer(), nullable=False)
    password = db.Column(db.String(255))
    active = db.Column(db.Boolean())
    grafana_notif_id = db.Column(db.Integer())
    roles = db.relationship('Role', secondary=roles_users,
                            backref=db.backref('users', lazy='dynamic'))

    def save(self):
        if self.id == None:
             db.session.add(self)
        return db.session.commit()

    def __str__(self):
        return self.name


# Setup Flask-Security
user_datastore = SQLAlchemyUserDatastore(db, User, Role)
security = Security(app, user_datastore)

# Flask views
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/sensor/set_status/<sensor_id>')
def set_sensor_enabled(sensor_id):
    sensor = db.session.query(Sensor).get(sensor_id)
    sensor.last_enabled = datetime.datetime.utcnow()
    db.session.commit()
    return "DONE", 200

@app.route('/create_alerts')
def create_alerts():
    alerts = db.session.query(Alert).all()
    # make sure each user has alert created for this
    for alert in alerts:
        for user in alert.users:
            grafana_helper.user_notification(user)
    # create db
    r = grafana_helper.create_alert_db(alerts)
    return r.text, r.status_code

@app.route('/node_mqtt/<node_id>', methods=["POST"])
def node_mqtt(node_id):
    topic = "toBridge/" + str(node_id)
    msg = request.form.get("msg")
    publish_msg(topic, msg)
    return "DONE", 200

@app.route('/node/enable/<node_id>')
def enable_sensor(node_id):
    topic = "toBridge/" + str(node_id)
    msgs = db_helper.get_enable_cmds(node_id)
    for msg in msgs:
        print "Publishing->", topic, msg
        publish_msg(topic, msg)
    return "DONE", 200

@app.route('/node/fenable/<node_id>')
def force_enable_sensor(node_id):
    topic = "toBridge/" + str(node_id)
    msgs = db_helper.get_enable_cmds(node_id)
    for msg in msgs:
        print "Publishing->", topic, msg
        publish_msg(topic, "f"+msg)
    return "DONE", 200

@app.route('/calling', methods=["GET", "POST"])
def caller():
    try:
        data = json.loads(request.get_data())
        print data
    except:
        print "FAILED TO PARSE DATA, calling anyways"
        data = {}

    # get phones
    phones = request.args.get("ph")
    if phones is None:
        #call ghanshyam bhai instead
        phones = "919327310113"
    else:
        phones = phones.split(",")
        if len(phones) == 0:
            phones = phones[0]
        else:
            phones = "<".join(phones)

    if "message" not in data:
        msg = "Bandh hai kuch"
    else:
        msg = data["message"]
    r = make_call("16626267492", phones, msg)
    print r.text
    return "HI", r.status_code


# Create admin
admin = flask_admin.Admin(
    app,
    'Mesh Monitoring',
    base_template='my_master.html',
    template_mode='bootstrap3',
)

# define a context processor for merging flask-admin's template context into the
# flask-security views.
@security.context_processor
def security_context_processor():

    return dict(
        admin_base_template=admin.base_template,
        admin_view=admin.index_view,
        h=admin_helpers,
        get_url=url_for
    )

def build_sample_db():
    """
    Populate a small db with some example entries.
    """

    import string
    import random
    with app.app_context():
        db.drop_all()
        db.create_all()

        user_role = Role(name='user')
        super_user_role = Role(name='superuser')
        db.session.add(user_role)
        db.session.add(super_user_role)
        db.session.commit()

        test_user = user_datastore.create_user(
            name='Admin',
            email='admin',
            password=encrypt_password('admin'),
            roles=[user_role, super_user_role],
            number=919925324418
        )

        names = [
            'Manan',
            'Ashish',
            'Darshesh',
            'Ganshyam',
        ]

        for i in range(len(names)):
            tmp_email = names[i].lower() + ""
            tmp_pass = ''.join(random.choice(string.ascii_lowercase + string.digits) for i in range(10))
            user_datastore.create_user(
                name=names[i],
                email=tmp_email,
                password=encrypt_password(tmp_pass),
                roles=[user_role, ],
                number=111
            )

        for n_id, val in nodes.CONFIG.items():
            node = Node(name=val['name'], hw_id=n_id)
            sensors = []
            for p_id, p_val in val['proxy'].items():
                proxy = Sensor(
                            name=p_val["name"],
                            interval=p_val["interval"],
                            hw_id=p_id,
                            misc=json.dumps({"circum": p_val["circum"]}),
                            type="proxy"
                        )
                sensors.append(proxy)
            for p_id, p_val in val['pt'].items():
                pt = Sensor(
                            name=p_val["name"],
                            interval=p_val["interval"],
                            hw_id=p_id,
                            misc="{}",
                            type="pt"
                        )
                sensors.append(pt)
            node.sensors = sensors
            db.session.add(node)

        db.session.commit()
    return

############ VIEWS

# Create customized model view class
class MyModelView(sqla.ModelView):
    def is_accessible(self):
        if type(self).__name__ == "AlertView" and current_user is not None:
            return True
        if not current_user.is_active or not current_user.is_authenticated:
            return False

        if current_user.has_role('superuser'):
            return True

        return False

    def _handle_view(self, name, **kwargs):
        """
        Override builtin _handle_view in order to redirect users when a view is not accessible.
        """
        if not self.is_accessible():
            if current_user.is_authenticated:
                # permission denied
                abort(403)
            else:
                # login
                return redirect(url_for('security.login', next=request.url))


    # can_edit = True
    edit_modal = True
    create_modal = True
    can_export = True
    can_view_details = True
    details_modal = True

class NodeView(MyModelView):
    column_display_pk = True
    # column_list = ['id', 'name', 'hw_id', 'sensors']
    # form_column = ('id', 'name', 'hw_id', 'sensors')
    # column_editable_list = ['misc']
    # column_searchable_list = column_editable_list
    # # column_exclude_list = ['password']
    # # form_excluded_columns = column_exclude_list
    # # column_details_exclude_list = column_exclude_list
    # column_filters = column_editable_list

class AlertView(MyModelView):
    low_security = True
    column_display_pk = True
    # column_searchable_list = column_editable_list
    # column_editable_list = ['sensors']
    # column_exclude_list = ['password']
    # form_excluded_columns = column_exclude_list
    # column_details_exclude_list = column_exclude_list
    # column_filters = column_editable_list

class SensorView(MyModelView):
    column_display_pk = True
    # column_list=['id', 'type', 'name', 'node', 'misc']
    # column_editable_list = ['name', 'node_id']
    # column_searchable_list = column_editable_list
    # # column_exclude_list = ['password']
    # # form_excluded_columns = column_exclude_list
    # # column_details_exclude_list = column_exclude_list
    # column_filters = column_editable_list


class UserView(MyModelView, BaseView):
    column_editable_list = ['email', 'name', 'number']
    column_searchable_list = column_editable_list
    column_exclude_list = ['password']
    # form_excluded_columns = column_exclude_list
    column_details_exclude_list = column_exclude_list
    column_filters = column_editable_list

class TopologyView(BaseView):
    def get_data(self):
        nodes = db.session.query(Node).all()
        return_dict = {}
        for each in nodes:
            return_dict[each.id] = each.machine_dict()
        return return_dict

    @expose('/')
    def index(self):
        return self.render(
                'admin/topology_overview.html',
                nodes=self.get_data(),
                now = datetime.datetime.utcnow())


# Add model views
admin.add_view(TopologyView(name="Topology", endpoint='topology', menu_icon_type='fa', menu_icon_value='fa-connectdevelop',))
admin.add_view(MyModelView(Role, db.session, menu_icon_type='fa', menu_icon_value='fa-server', name="Roles"))
admin.add_view(UserView(User, db.session, menu_icon_type='fa', menu_icon_value='fa-users', name="Users"))
admin.add_view(SensorView(Sensor, db.session, menu_icon_type='fa', menu_icon_value='fa-thermometer-empty', name="Sensors"))
admin.add_view(NodeView(Node, db.session, menu_icon_type='fa', menu_icon_value='fa-code-fork', name="Nodes"))
admin.add_view(AlertView(Alert, db.session, menu_icon_type='fa', menu_icon_value='fa-exclamation-circle', name="Alerts"))


# filter
import babel
def format_datetime(value, format='medium'):
    if format == 'full':
        format="EEEE, d. MMMM y 'at' HH:mm"
    elif format == 'medium':
        format="EE dd.MM.y HH:mm"
    if value == None:
        return "N/A"
    return babel.dates.format_datetime(value, format)

app.jinja_env.filters['datetime'] = format_datetime

if __name__ == '__main__':
    # Build a sample db on the fly, if one does not exist yet.
    app_dir = os.path.realpath(os.path.dirname(__file__))
    database_path = os.path.join(app_dir, app.config['DATABASE_FILE'])
    if not os.path.exists(database_path):
        build_sample_db()

    if "DEBUG" in os.environ:
        debug = True
    else:
        debug = False

    # Start app
    app.run(
        debug=debug,
        port=config.ADMIN_SERVER_PORT,
        host=config.ADMIN_SERVER_HOST
    )
