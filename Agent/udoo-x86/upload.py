#!/usr/bin/env python3

import sys
import argparse
import os.path
import getpass                                                                       
import paramiko

"""
This script can be used to compile and run the AntibIoTic Agent on a remote Linux system via SSH.
NOTE: paramiko is required for this script: "pip3 install paramiko" (https://github.com/paramiko/paramiko)
"""

"""
Upload an entire folder via SSH to emulate an SCP command
"""
def put_all(sftp,localpath,remotepath):
    # recursively upload a full directory
    head = os.path.split(localpath)[0]
    remote_head = os.path.split(remotepath)[0]
    tail = os.path.split(localpath)[1]
    os.chdir(head)
    for root,dirs,files in os.walk(tail):
        #print("\t root: {}".format(root))
        try:
            sftp.mkdir(os.path.join(remote_head,root))
            print("\t Created remote folder '{}'".format(os.path.join(remote_head,root)))
        except:
            pass
        for name in files:
            #print("\t file: {}".format(name))
            print("\t Transfer {} > {}".format(os.path.join(head,root,name),os.path.join(remote_head, root,name)))
            sftp.put(os.path.join(head,root,name),os.path.join(remote_head,root,name))

def exec_commands(host, port, username, password, path, execute, gateway):
    cmd = ""
    client = paramiko.client.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
            # Connect
            # Equivalent to 'ssh <username>@<host> -p <port>'
            print("[Local] > ssh {}@{} -p {}".format(username, host, port))
            client.connect(host, port=port, username=username, password=password)

            # Create destination directories if don't exist
            cmd = "mkdir -p AntibIoTic/src"
            print("[{}] > {}".format(host,cmd))
            stdin, stdout, stderr = client.exec_command(cmd)
            return_code = stdout.channel.recv_exit_status()
#            # For Debug
#            print("[{}] > return code: '{}'".format(host,return_code))
#            stdin_str = stdin.read().decode('utf-8')
#            print("[{}] > stdin: '{}'".format(host,stdin_str))
#            stdout_str = stdout.read().decode('utf-8')
#            print("[{}] > stdout: '{}'".format(host,stdout_str))
#            stderr_str = stderr.read().decode('utf-8')
#            print("[{}] > stderr: '{}'".format(host,stderr_str))
#            if return_code==0:
#                # Success
#               print("\t[{}] > {}".format(host,stdout_str))
#            else:
#                raise ValueError("Error executing command '[{}]> {}'.\t {}".format(host,cmd,stderr_str))
            stderr_str = stderr.read().decode('utf-8')
            if return_code!=0:
                raise ValueError("Error executing command '[{}]> {}'.\t {}".format(host,cmd,stderr_str))
           
            # Upload files
            # Equivalent to 'scp -r <local_path> <username>@<host>: AntibIoTic/src'
            sftp = client.open_sftp()
            print("[Local] > Coping files from localhost:{} to {}:AntibIoTic/src".format(path,host))
            put_all(sftp,path,"AntibIoTic/src")
            if sftp: sftp.close()
            print("\t Finished")
            
            # Compile the code
            cmd = "mkdir -p AntibIoTic/bin && gcc AntibIoTic/src/*.c -o AntibIoTic/bin/antibiotic_udoo_debug -std=c99 -pthread -g -DDEBUG"
            print("[{}] > {}".format(host,cmd))
            stdin, stdout, stderr = client.exec_command(cmd)
            return_code = stdout.channel.recv_exit_status()
            stderr_str = stderr.read().decode('utf-8')
            if return_code!=0:
                raise ValueError("Error executing command '[{}]> {}'.\t {}".format(host,cmd,stderr_str))

            # Run the code, if requested
            if args.run:
                cmd = "chmod +x AntibIoTic/bin/antibiotic_udoo_debug && echo '"+password+"' | sudo -S AntibIoTic/bin/antibiotic_udoo_debug "+gateway+" 2"
                print("[{}] > {}".format(host,cmd))
                stdin, stdout, stderr = client.exec_command(cmd) # Hangs here in the execution
                return_code = stdout.channel.recv_exit_status()
                stderr_str = stderr.read().decode('utf-8')
                if return_code!=0:
                    raise ValueError("Error executing command '[{}]> {}'.\t {}".format(host,cmd,stderr_str))
    except Exception as e:
            sys.exit(e)

    finally:
            if client:
                client.close()


if __name__ == '__main__':

    # Parse command line arguments
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-r", "--remote-ip", default="192.168.0.2", help="Remote IP address")
    parser.add_argument("-s", "--gateway-ip", default="192.169.0.2", help="AntibIoTic gateway IP address")
    parser.add_argument("-p", "--port", type=int, default=22, help="Remote SSH port")
    parser.add_argument("-u", "--username", default="pi", help="SSH username")
    parser.add_argument("-f", "--files", required=True, help="Path to the folder to upload (relative path from the cwd)")
    parser.add_argument("-pw", "--password", help="Password for SSH connection")
#    parser.add_argument("-c", "--compiler", default="gcc", help="Compiler to use on the remote system")
    parser.add_argument("-x", "--run", action="store_true", default=False, help="Should file be compiled and run after upload")
    args = parser.parse_args()

    # Check path
    if not os.path.exists(args.files):
        print("[Local] > Invalid path '{}'. Path does not exists.".format(args.files))
        sys.exit()
    
    if os.path.isfile(args.files):
        print("[Local] > Invalid path '{}'. A folder is expected.".format(args.files))
        sys.exit()
    
    folder_path = os.path.abspath(args.files)
    print("[Local] > Folder to upload: '{}'".format(folder_path))

    # Retrieve SSH password
    pasw = args.password
    if pasw is None:
        # Ask to insert password
        pasw = getpass.getpass("[Local] > Enter SSH password for user '{}':".format(args.username))

    # Upload binaries, compile, and execute
    exec_commands(args.remote_ip, args.port, args.username, pasw, folder_path, args.run, args.gateway_ip)
