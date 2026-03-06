import os

def run_ping(host):
    # OS Command Injection: host is passed directly to os.system
    command = "ping -c 1 " + host
    os.system(command)

user_host = "8.8.8.8; rm -rf /"
run_ping(user_host)
