import pandas as pd
import numpy as np
from pandas import HDFStore
from pandas import read_hdf
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import ROOT
import root_numpy
from matplotlib import colors
from matplotlib.colors import LogNorm
import matplotlib.patches as patches
import uproot
from matplotlib.patches import Rectangle
offset = 1488/2

def divide_outercryo(data):
    
    flange1 = data[(data.R>815) & (data.Z > 300+offset)]
    flange2 = data[(data.R>815) & (data.Z < 300+offset) & (data.Z > -520 + offset)] 
    flange3 = data[(data.R>815) & (data.Z < -520+offset) & (data.Z > -1000 + offset)]
    shell_1 = data[(data.R<815) & (data.Z > 764)]
    shell_2 = data[(data.R<815) &  (data.Z < 384)]
    shell = pd.concat([shell_1, shell_2]) 
    elongation = data[(data.R<815) & (data.Z > 384) & (data.Z < 764)]
    mass_tot = 1046.12 #kg
    geant_tot = len(data)
    mass_flange1 = (mass_tot * len(flange1)) / geant_tot
    #print("mass_flange1:", mass_flange1)
    mass_flange2 = (mass_tot * len(flange2))/ geant_tot
    #print("mass_flange2:",mass_flange2)
    mass_flange3 = (mass_tot *  len(flange3))/ geant_tot
    #print("mass_flange3:",mass_flange3)
    mass_shell = (mass_tot *len(shell))/ geant_tot
    #print("mass shell:",mass_shell)
    mass_elong = (mass_tot * len(elongation))/ geant_tot
    #print("mass elongation:",mass_elong)
    #print(mass_flange1+mass_flange2+mass_flange3)      
    return flange1, flange2, flange3, shell, elongation
    
def divide_innercryo(data):
    flange1_in = data[(data.R>735) & (data.Z > 125+offset)]
    flange2_in = data[(data.R>735) & (data.Z < 125+offset) & (data.Z > -500 + offset)] 
    flange3_in = data[(data.R>735) & (data.Z < -500+offset) & (data.Z > -1000 + offset)]
    shell_in = data[(data.R<735) & (data.Z > -910)]
    elongation_in = data[(data.R<735) & (data.Z < -910)]
    mass_tot =766.77
    geant_tot = len(data)
    
    mass_flange1_in = (mass_tot * len(flange1_in)) / geant_tot
    #print("mass_flange1_in:", mass_flange1_in)
    mass_flange2_in = (mass_tot * len(flange2_in))/ geant_tot
    #print("mass_flange2_in:",mass_flange2_in)
    mass_flange3_in = (mass_tot *  len(flange3_in))/ geant_tot
    #print("mass_flange3_in:",mass_flange3_in)
    mass_shell_in = (mass_tot *len(shell_in))/ geant_tot
    #print("mass shell_in:",mass_shell_in)
    mass_elong_in = (mass_tot * len(elongation_in))/ geant_tot
    #print("mass elongation:",mass_elong_in)
    #print(mass_flange1_in + mass_flange2_in + mass_flange3_in + mass_shell_in + mass_elong_in)   
   
    return flange1_in, flange2_in, flange3_in, shell_in, elongation_in

def divide_bellWall(data):
    bottomLip = data[data.Z < (719.29 + 50)]
    bellWall = data[data.Z > (719.29 + 50)]
    mass_bw =33354.05
    geant_bw = len(data)
    
    mass_bottomLip = (mass_bw * len( bottomLip)) / geant_bw
   
    #print("mass_bottomLip:", mass_bottomLip)
    return bottomLip, bellWall


