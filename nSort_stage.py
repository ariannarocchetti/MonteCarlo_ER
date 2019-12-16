#!/usr/bin/python

import sys
import os
from datetime import date
#import tqdm
import glob

import subprocess
from pexpect import pxssh
import sys 

dir_ =sys.argv[1] #"20191202T162310-0600"
print(dir_)



#path =  "/dali/lgrandi/xenonnt/simulations/er_simulations/raw_files/"
dir_to_sort = dir_

list_to_sort = glob.glob("%s/*10.root"%dir_to_sort)
    #print(list_to_sort[0:2])
    #print(list_to_sort[-4:-2])
for j in list_to_sort:
    k = os.path.splitext("%s"%j)[0]
    os.system("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
        #print("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
#print(len(list_to_sort))
print("------------------")

"""

storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations"
command = ("ls -d %s/raw_files/*"%storage_path)
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")

print(out)

for i in range(0, len(out)-2):

    list_to_sort = glob.glob("%s/*10.root"%out[i])
    #print(list_to_sort[0:2])
    #print(list_to_sort[-4:-2])
    for j in list_to_sort:
        k = os.path.splitext("%s"%j)[0]
        os.system("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
        #print("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
    print(len(list_to_sort))
    print("------------------")
"""    

