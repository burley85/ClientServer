import configparser
import os

defaults = {
    'Server': {
        'host' : '192.168.1.232',
        'port' : '8080',
        'warnings' : 'false',
        'debug' : 'false',
        'logfile' : 'debug.log'
    },
    'API': {
        'host' : '192.168.1.232',
        'port' : '8081'
    }
}

config = configparser.ConfigParser()

config.read_dict(defaults)


config.read('config.cfg')

#Start the API server
cmd = f"start py database/db_server.py {config['API']['host']} {config['API']['port']}"
print(cmd)
os.system(cmd)

#Start the web server
cmd = f"start ./server/bin/server.exe -l {config['Server']['logfile']}"

cmd += f"-ip {config['Server']['host']} -port {config['Server']['port']}"
cmd += f"-apiip {config['API']['host']} -apiport {config['API']['port']}"

if(config['Server'].getboolean('warnings')):
    cmd += " --w"

if(config['Server'].getboolean('debug')):
    cmd += " --d"

print(cmd)
os.system(cmd)
