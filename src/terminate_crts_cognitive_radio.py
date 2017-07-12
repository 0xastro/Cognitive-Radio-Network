#!/usr/bin/python
import commands
#find the pids of any process that contains CRTS. 
pids = commands.getoutput('ps -Ao \"%p, %a\" | grep -e \"[c]rts_cognitive_radio\" | cut -d\',\' -f1').split()
for pid in pids:
    cmd = 'kill -9 '
    cmd += pid
    #print cmd
    #kill the process
    commands.getoutput(cmd)

#tear down tunCRTS if it's still up    
#commands.getoutput('sudo ip tuntap del dev tunCRTS mode tun')
