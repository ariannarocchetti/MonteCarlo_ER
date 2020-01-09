#!/usr/bin/python

import sys
import os
from datetime import date
#import tqdm
import glob
from os.path import normpath, basename
import subprocess
from pexpect import pxssh


DATE = "20191209*"
storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations/"
command = ("ls -d %s/raw_files/%s*"%(storage_path, DATE))
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")
print(len(out))

for i in range(0, len(out)):

    list_to_sort = glob.glob("%s/*10.root"%out[i])
    name_dir = basename(normpath(out[i]))
    #print(list_to_sort[0:2])
    #print(list_to_sort[-4:-2])
    print("In: ", name_dir, "root files: ", len(list_to_sort))
    if (len(list_to_sort)>997):
        print("extracted for", basename(normpath(list_to_sort[i])))
    print("----------------")
    """
    for j in list_to_sort:
        k = os.path.splitext("%s"%j)[0]
        os.system("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
        #print("./nSort -i  %s -d XENONnT 2 1 0 0 0"%k)
    print(len(list_to_sort))
    print("------------------")
    """
