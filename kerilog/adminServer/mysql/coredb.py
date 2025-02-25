"""
Creates user for interacting with SQL, mainly just inserting no deleting
user: m2writer
pass: poeticjustice
------ Reader too for grafana
user: m2reader
pass: poeticinjustice
"""
import argparse
import MySQLdb
import logging

WRITER = ["m2writer", "poeticjustice"]
READER = ["m2reader", "poeticinjustice"]
DATABASE = "m2core"
DATA_TABLE = "sense_data"
LOG = logging.getLogger(__name__)

def create_user_table(args):
    db = MySQLdb.connect("localhost", args.user, args.password)
    cursor = db.cursor()
    create_user_if_not_exist(cursor, WRITER[0], WRITER[1])
    create_user_if_not_exist(cursor, READER[0], READER[1])
    grant_all_read_only(cursor, READER[0], READER[1])
    grant_all_readwrite(cursor, WRITER[0], WRITER[1])
    create_sensor_table(cursor)
    # create tables as set in schemas

    db.close()

def create_sensor_table(cursor):
    database = "CREATE DATABASE IF NOT EXISTS %s" % DATABASE
    # ensure db exits
    echo_cursor_execute(cursor,database)
    echo_cursor_execute(cursor, "use %s" % DATABASE)
    table_create = """ CREATE TABLE IF NOT EXISTS {table} (
        id VARCHAR(63),
        time DATETIME,
        value VARCHAR(63),
        PRIMARY KEY (id, time)
    )
    """.format(**{
        'table': DATA_TABLE
    })

    echo_cursor_execute(cursor,table_create)


def clear_data_table(args):
    raw_input("Sure you want to dump and clean the tables?")
    db = MySQLdb.connect("localhost", args.user, args.password)
    db.cursor().execute("TRUNCATE TABLE %s" % DATA_TABLE)
    db.close()


def create_user_if_not_exist(cursor, user, passwd):
    create_user_string = "CREATE USER IF NOT EXISTS '{user}'@'%' IDENTIFIED BY '{pass}'"
    cmd =  create_user_string.format(**{
        "user": user,
        "pass": passwd
    })
    # create writer user
    echo_cursor_execute(cursor, cmd)

def grant_all_read_only(cursor, user, passwd):
    grant_string = "GRANT SELECT ON *.* TO '{user}'@'%' IDENTIFIED BY '{pass}'"
    cmd = grant_string.format(**{
        "user": user,
        "pass": passwd
    })
    echo_cursor_execute(cursor, cmd)

def insert_data_points(data_list):
    """
    data list
    [sensor_id, time, data]
    """
    db = MySQLdb.connect("localhost", WRITER[0], WRITER[1])
    cursor = db.cursor()
    cursor.execute("USE %s" % DATABASE)
    for each in data_list:
        sensor_id, time, data = None, None, None
        try:
            sensor_id = str(each[0])
            time = int(each[1])
            data = str(each[2])
        except Exception as e:
            LOG.exception(e)
            LOG.error("Failed to parse when inserting data point:", each)
            break
        cmd = 'insert into {table} (id, time, value) values("{sensor_id}", FROM_UNIXTIME({time}), "{data}")'
        cmd = cmd.format(**{
            'table': DATA_TABLE,
            'sensor_id': sensor_id,
            'time': time,
            'data': data
        })
        try:
            echo_cursor_execute(cursor, cmd)
        except MySQLdb.IntegrityError as err:
            LOG.error("Duplicate Entry, ignoring:%s", each)

    cursor.close()
    db.commit()
    db.close()

def flush_all():
    """
    Handles whatever needs to be done when the connection closes
    """
    pass

def grant_all_readwrite(cursor, user, passwd):
    grant_string = "GRANT SELECT, INSERT, UPDATE, DELETE ON *.* TO '{user}'@'%' IDENTIFIED BY '{pass}'"
    cmd = grant_string.format(**{
        "user": user,
        "pass": passwd
    })
    echo_cursor_execute(cursor, cmd)

def arg_parse():
    parser = argparse.ArgumentParser()
    parser.add_argument("-u" , "--user", help="user to execute command with")
    parser.add_argument("-p" , "--password", help="user to execute command with")
    parser.add_argument("-f", "--flow", type=int, help="1:create user / table 2.clear data table", required=True)
    args = parser.parse_args()
    if args.password == None:
        args.password = ""
    return args

def echo_cursor_execute(cursor, cmd):
    LOG.debug(cmd)
    cursor.execute(cmd)
    result = cursor.fetchall()
    return result

if __name__ == "__main__":
    args = arg_parse();
    if(args.flow == 1):
        create_user_table(args)
    elif(args.flow == 2):
        clear_data_table(args)
