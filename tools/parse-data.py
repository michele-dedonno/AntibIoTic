#!/bin/python3

# Helper scipts used to parse evaluation data

import os

SAMPLES = 120

# Compute stats on a list of float
def process_data(f, float_list):
    diffv=len(float_list)-SAMPLES
    if(diffv >= 0):
        f.write("\t {} values where not considered in the stats, since they exceed the number of samples chosen ({}).\n".format(diffv, SAMPLES))
        minv=min(float_list[:SAMPLES]); maxv=max(float_list[:SAMPLES]); avgv=sum(float_list[:SAMPLES])/len(float_list[:SAMPLES])
        f.write("\t Based on the first {} elements: Min = {}, Max = {}, Avg = {}\n".format(SAMPLES, minv, maxv, avgv))
    else:
        f.write("\t Not enough data to compute the stats.\n")

# Process a file
def process_file(fname):
    # Read file
    fp = open(fname, 'r') 
    Lines = fp.readlines() 
    fp.close()

    # Initialization
    count = 0
    pid = -1
    pidstring = ">[PID]:"
    #memstring = "VmSize:"
    memstring = ">Memory"
    netstring = "Refreshing:"
    netln = False
    cpu_percs = list() # CPU %
    mem_percs = list() # MEM %
    mem_kb = list()    # MEM (KB)
    etime = list()     # Elapsed time
    sent_kb = list()   # Network KB sent
    rcvd_kb = list()   # Network KB rcvd

    # Loop for each line. Strips the newline character.
    for line in Lines: 
        count+=1
        strpLine = line.strip()
        # Check if we are at the beginning of the file
        if(pid == -1):
            # Look for PID
            if(strpLine.startswith(pidstring)):
                #print("> Line {}: {}".format(count, strpLine))
                pid = strpLine[strpLine.find(pidstring)+len(pidstring):].strip()
                #print("PID: {}".format(pid))
                continue
        else:
            if (netln):
                # processing NET data
                if(strpLine.startswith("unknown")):
                    # finished NET data
                    netln = False
                else:
                    netstat=strpLine.split() # CMD Total-KB-Sent Total-KB-Received
                    if (netstat[0].find(pid)!=-1):
                        #print("> Line {}: {}".format(count, strpLine))
                        #print("CMD: {}".format(netstat[0]))
                        #print("Total sent: {} KB".format(netstat[1]))
                        sent_kb.append(float(netstat[1]))
                        #print("Total rcvd: {} KB".format(netstat[2]))
                        rcvd_kb.append(float(netstat[2]))
                continue
            if(strpLine.startswith(pid)):
                # CPU Data
                #print("> Line {}: {}".format(count, strpLine))
                cpustat = strpLine.split() # PID %CPU %MEM TIME_ELAPSED CMD PARAMS
                #print("PID: {}".format(cpustat[0]))
                #print("CPU: {}%".format(cpustat[1]))
                cpu_percs.append(float(cpustat[1]))
                #print("MEM: {}%".format(cpustat[2]))
                mem_percs.append(float(cpustat[2]))
                #print("CPU Time: {}".format(cpustat[3]))
                #print("Elapsed Time (MM:SS): {}".format(cpustat[4]))
                etime.append(cpustat[4])
                #print("CMD: {}".format(' '.join(cpustat[5:])))
            elif(strpLine.startswith(memstring)):
                # MEME Data
                #print("> Line {}: {}".format(count, strpLine))
                #memstat=strpLine.split()[1:] # <Kbytes> KB
                memstat=strpLine.split()[3:] # Kbytes RSS Anon
                #print("Total MEM: {} KB".format(memstat[0]))
                mem_kb.append(float(memstat[0]))
            elif(strpLine.startswith(netstring)):
                # NET Data will follow
                #print("> Line {}: {}".format(count, strpLine))
                #print("The following lines will have net data")
                netln = True
        # if line is empty, end of file is reached 
        #if not line:
        #    break

#    print("'{}'".format(fname))
#    print("-------------------------------------------------------------------------------------")
#    print(">PID: {}".format(pid))
#    print(">Elapsed Time [{}].".format(len(etime),' '.join(etime)))    
#    print(">CPU usage (%) [{}]: {}".format(len(cpu_percs),' '.join([str(i) for i in cpu_percs])))
#    print(">MEM usage (%) [{}]: {}".format(len(mem_percs),' '.join([str(i) for i in mem_percs])))
#    print(">MEM used (KB) [{}]: {}".format(len(mem_kb),' '.join([str(i) for i in mem_kb])))
#    print(">Total data sent (KB) [{}]: {}".format(len(sent_kb),' '.join([str(i) for i in sent_kb])))
#    print(">Total data received (KB) [{}]: {}".format(len(rcvd_kb),' '.join([str(i) for i in rcvd_kb])))
#    print("=======================================================================================\n")
    
    f = open("./output/results.txt", "a")
    f.write("'{}', Samples considered: {}\n".format(fname, SAMPLES))    
    f.write("-------------------------------------------------------------------------------------\n")
    f.write(">PID: {}\n".format(pid))
    f.write(">Elapsed Time [{}]:  {}\n".format(len(etime),' '.join(etime)))
    
    f.write(">CPU usage (%) [{}]: {}\n".format(len(cpu_percs),' '.join([str(i) for i in cpu_percs])))
    process_data(f, cpu_percs)
    f.write(">MEM usage (%) [{}]: {}\n".format(len(mem_percs),' '.join([str(i) for i in mem_percs])))
    process_data(f, mem_percs)
    f.write(">Vm Size (KB) [{}]: {}\n".format(len(mem_kb),' '.join([str(i) for i in mem_kb])))
    #process_data(f, mem_kb)
    f.write(">Total data sent (KB) [{}]: {}\n".format(len(sent_kb),' '.join([str(i) for i in sent_kb])))
    #process_data(f, sent_kb)
    f.write(">Total data received (KB) [{}]: {}\n".format(len(rcvd_kb),' '.join([str(i) for i in rcvd_kb])))
    #process_data(f, rcvd_kb)
    f.write("=======================================================================================\n\n\n")
    f.close()
    
if __name__ == '__main__':
    
    # Erase output file
    f = open("./output/results.txt", "w")
    f.close()
    
    # Process each file in the current folder, in order
    directory = r'.'
    for entry in sorted(os.scandir(directory), key=lambda x: (x.is_dir(), x.name)):
        if entry.is_file() and entry.path.endswith(".txt"):
            process_file(entry.path)
