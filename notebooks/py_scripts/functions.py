def get_parameters(material):
    if material == "OuterCryostatShell":
        param = {
        'ids' : 'XENON1T',
        'mass' : 569.400, 
        'U238' : 2.4,
        'err_U238' : 0.7,
        'Ra226' : 0.64, 
        'err_Ra226' : 0,
        'Co60' : 9.7,
        'err_Co60' : 0.8,
        'K40' : 2.7, 
        'err_K40' : 0,
        'Cs137' : 0.64, 
        'err_Cs137' : 0, 
        'Th228' : 0.36, 
        'err_Th228' : 0, 
        'U235' : 0.11,
        'err_U235' : 0.03,
        'Th232' : 0.21,
        'err_Th232' : 0.06,   
        }
           
    if material == "OuterCryostat_Elongation":
        param = {
        'ids' : "SS_NiT_C5",
        'mass' : 61.500 ,
        'U238': 4,
        'err_U238' : 2,
        'Ra226' : 1.34 ,
        'err_Ra226' : 0.09,
        'Co60' : 0.61,
        'err_Co60' :0.05, 
        'K40' :1.4 ,
        'err_K40' :0.2,
        'Cs137' : 0.034,
        'err_Cs137' : 0.017,
        'Th228' : 0.57,
        'err_Th228' : 0.06,
        'U235' : 0.31,
        'err_U235' : 0, 
        'Th232' : 8.1,
        'err_Th232' : 0,    
        }
        
    if material == "OuterCryostat_flanges":
        param = {
        'mass':413.220,
        'ids':"XENON1T",
        'U238':1.4,
        'err_U238':0.4,
        'Ra226':0.4 ,
        'err_Ra226': 0,
        'Co60':37.3,
        'err_Co60': 0.9,
        'K40':5.6,
        'err_K40': 0,
        'Cs137':1.5,
        'err_Cs137':0,
        'Th228': 4.5,
        'err_Th228':0.6,
        'U235':0.06,
        'err_U235': 0.02,
        'Th232' : 0.21,
        'err_Th232' : 0.06,  
        }
    if material == "SS_InnerCryostatShell":
        param = {
         'mass' : 452.470 ,
         'ids' : "SS_NiT_C1",
         'U238' : 3.7,
         'err_U238' : 0.6,
         'Ra226' : 0.3 ,
         'err_Ra226' : 0.1,
         'Co60' : 2.36,
         'err_Co60' : 0.21,
         'K40' : 1.6,
         'err_K40': 0.6,
         'Cs137' : 0.21,
         'err_Cs137' : 0,
         'Th228' : 0.5,
         'err_Th228' : 0.1,
         'U235' : 0.7,
         'err_U235' : 0.3 ,
         'Th232' : 0.10 ,
         'err_Th232' : 0.08,
        }
    
    if material == "SS_InnerCryostatFlanges":
        param = { 
        'mass' : 227.500, #gr
        'ids' : "XENON1T",
        'U238' : 1.4,
        'err_U238' : 0.4,
        'Ra226' : 0.4 ,
        'err_Ra226' : 0,
        'Co60' : 37.3,
        'err_Co60' : 0.9,
        'K40' : 5.6,
        'err_K40' : 0,
        'Cs137' : 1.5,
        'err_Cs137' : 0,
        'Th228' : 4.5,
        'err_Th228' : 0.6,
        'U235' :  0.06,
        'err_U235' : 0.02,
        'Th232' : 0.21,
        'err_Th232' : 0.06,
       }
        
    if material == "SS_InnerCryostat_bottomDome":
        param = {
        'ids' : "SS_NiT_C5",
        'mass' :86.800, #g
        'U238': 4,
        'err_U238' : 2,
        'Ra226' : 1.34 ,
        'err_Ra226' : 0.09,
        'Co60' : 0.61,
        'err_Co60' :0.05, 
        'K40' :1.4 ,
        'err_K40' :0.2,
        'Cs137' : 0.034,
        'err_Cs137' : 0.017,
        'Th228' : 0.57,
        'err_Th228' : 0.06,
        'U235' : 0.31,
        'err_U235' : 0, #up
        'Th232' : 8.1,
        'err_Th232' : 0,
        }
            
    
    if material =="OuterCryostatReflector":
        param = {
        'ids' : "ePTFE-A",
        'mass' : 18.660, #gr from mc
        'U238' : 0.27,
        'err_U238' : 0.12,
        'Ra226' : 0 ,
        'err_Ra226' : 0,
        'Co60' :  0,
        'err_Co60' : 0,
        'K40' : 0,
        'err_K40' : 0,
        'Cs137' :  0,
        'err_Cs137' :  0,
        'Th228' : 0.12,
        'err_Th228' : 0.4,
        'U235' :  0.27*0.04,
        'err_U235' :0.12*0.04,
        'Th232' : 0.12,
        'err_Th232' : 0.4,
        }
        
    if material == "SS_BellPlate":
        param = {
         'mass' : 63.66881, #gr
         'ids' : "SS_NiT_C1",
         'U238' : 3.7,
         'err_U238' : 0.6,
         'Ra226' : 0.3 ,
         'err_Ra226' : 0.1,
         'Co60' : 2.36,
         'err_Co60' : 0.21,
         'K40' : 1.6,
         'err_K40': 0.6,
         'Cs137' : 0.21,
         'err_Cs137' : 0,
         'Th228' : 0.5,
         'err_Th228' : 0.1,
         'U235' : 0.7,
         'err_U235' : 0.3 ,
         'Th232' : 0.10 ,
         'err_Th232' : 0.08,
        }   
        
    if material == "SS_BellSideWall":
        param = {
        'ids' : "SS_NiT_C5",
        'mass' :33.35405, #gr
        'U238': 4,
        'err_U238' : 2,
        'Ra226' : 1.34 ,
        'err_Ra226' : 0.09,
        'Co60' : 0.61  ,
        'err_Co60' :0.05, 
        'K40' :1.4 ,
        'err_K40' :0.2,
        'Cs137' : 0.034,
        'err_Cs137' : 0.017,
        'Th228' : 0.57,
        'err_Th228' : 0.06,
        'U235' : 0.31,
        'err_U235' : 0, #up
        'Th232' : 8.1,
        'err_Th232' : 0,
        }
        
    if material == "SS_BellSideWallBottomLip":
        param = { 
         'mass' : 7.12738, #gr
         'ids' :"SS_Nit_E1",
         'U238' :2.5,
         'err_U238' : 0.3,
         'Ra226' : 0.6 ,
         'err_Ra226' : 0.1,
         'Co60' :0.4,
         'err_Co60' :0.1,
         'K40' :0.24,
         'err_K40' : 0,
         'Cs137' : 0.16,
         'err_Cs137' : 0,
         'Th228' : 0.4,
         'err_Th228' : 0.1,
         'U235' : 0.56,
         'err_U235' : 0,
         'Th232' :  0.4,
         'err_Th232' : 0.1,
        }
        
    if material == "PmtTpc":
        param = {
        'ids' : "average 1T/nT",
        'mass' :494, 
        'U238': 5*1000,
        'err_U238' : 1.6*1000,
        'Ra226' : 0.24*1000 ,
        'err_Ra226' : 0.08*1000,
        'Co60' : 0.47*1000  ,
        'err_Co60' :0.13*1000, 
        'K40' : 6.6*1000 ,
        'err_K40' :1.4*1000,
        'Cs137' : 0.08*1000,
        'err_Cs137' : 0.05*1000,
        'Th228' : 0.23*1000,
        'err_Th228' : 0.09*1000,
        'U235' : 5 *0.04*1000,
        'err_U235' : 1.6 *0.04*1000, #up
        'Th232':0.23*1000,
        'err_Th232' : 0.09*1000, 
        }
 
    if material =="Copper_TopRing":
        param = {
        'ids' : "[[xenon:xenonnt:screening:gamma_results:copper_nt_fs#activity_summary|Table]]",
        'mass' : 53.39539 + 84.97121, #gr 
        'U238': 0.33,
        'err_U238' : 0,
        'Ra226' : 0.17 ,
        'err_Ra226' : 0,
        'Co60' : 0.031  ,
        'err_Co60' :0, 
        'K40' : 0.45,
        'err_K40' :0.14,
        'Cs137' : 0.054,
        'err_Cs137' : 0,
        'Th228' : 0.18,
        'err_Th228' : 0.05,
        'U235' :0.33*0.04 ,
        'err_U235' : 0, #up
        'Th232':0.18,
        'err_Th232' : 0.05, 
        }
    
        
    if material == "Teflon_Pillar_":
        param = {
         'mass' : 28.95696+8.49816, #gr
         'ids' : "PTFE_Pillars",
         'U238' : 0.14,
         'err_U238' : 0.07,
         'Ra226' : 0.043,
         'err_Ra226' : 0.0011,
         'Co60' : 0,
         'err_Co60' : 0,
         'K40' : 0.42,
         'err_K40' : 0,
         'Cs137' : 0.013,
         'err_Cs137' : 0,
         'Th228' : 0.042,
         'err_Th228' : 0,
         'U235' : 0.049,
         'err_U235' : 0,
         'Th232': 0.105,
         'err_Th232' : 0.016, 
        }
        
        
    if material == "SS_AnodeRing":
        param = {
        'mass' : (1760+1914+1595+1508+1207)/1000, #gr
        'ids' :"SS_Nit_E1",
         'U238' :2.5,
         'err_U238' : 0.3,
         'Ra226' : 0.6 ,
         'err_Ra226' : 0.1,
         'Co60' :0.4,
         'err_Co60' :0.1,
         'K40' :0.24,
         'err_K40' : 0,
         'Cs137' : 0.16,
         'err_Cs137' : 0,
         'Th228' : 0.4,
         'err_Th228' : 0.1,
         'U235' : 0.56,
         'err_U235' : 0,
         'Th232' :  0.4,
         'err_Th232' : 0.1,
        } 
        
    if material == "Teflon_BottomTPC":
        param = {
         'mass': 0.22553,
         'ids': "worse between PTFE_Holders and PTFE_Pillars",
         'U238': 0.14,
         'err_U238':  0.07,
         'Ra226': 0.064,
         'err_Ra226': 0,
         'Co60': 0,
         'err_Co60': 0,
         'K40':  7,
         'err_K40':  1,
         'Cs137':  0.22,
         'err_Cs137': 0,
         'Th228': 0.042,
         'err_Th228':  0,
         'U235':  0.070,
         'err_U235': 0,
         'Th232': 0.26,
         'err_Th232' : 0.12, 
        }
        
    if material == "Teflon_TPC_Sliding":
        param = { 
        'mass' : 7.57118, #kg
         'ids' : "PTFE_SlidingReflector",
         'U238' : 0.15,
         'err_U238' : 0.07,
         'Ra226' : 0.150 ,
         'err_Ra226' : 0.030,
         'Co60' :  0,
         'err_Co60' : 0,
         'K40' : 0.080,
         'err_K40' : 0.030,
         'Cs137' : 0.046,
         'err_Cs137' :  0,
         'Th228' : 0.079,
         'err_Th228' : 0,
         'U235' :  0.065,
         'err_U235' : 0,
         'Th232': 0.028,
         'err_Th232' : 0.0162,
        }
        
    if material == "Teflon_TPC_Fixed":
        param = { 
        'mass' : 11.6234, #gr
         'ids' : "PTFE_Ref2",
         'U238' : 0.032,
         'err_U238' :  0,
         'Ra226' :  0.032 ,
         'err_Ra226' : 0,
         'Co60' : 0,
         'err_Co60' :  0,
         'K40' : 8,
         'err_K40' :  1,
         'Cs137' :  0.066,
         'err_Cs137' :  0,
         'Th228' : 0.089,
         'err_Th228' : 0,
         'U235' :  0.078,
         'err_U235' :  0,
         'Th232': 0.12,
         'err_Th232' : 0.03,
        }
        
    if material == "Copper_FieldGuard_": 
        param = {
        'ids' : "CU_FS01",
         'mass' : 170.35648, #gr
         'U238' : 0.03,
         'err_U238' : 0.01,
         'Ra226' :  0 ,
         'err_Ra226' : 0,
         'Co60' : 0,
         'err_Co60' :  0,
         'K40' :  0,
         'err_K40' : 0,
         'Cs137' : 0,
         'err_Cs137' : 0,
         'Th228' : 0.010,
         'err_Th228' : 0.004,
         'U235' : 0.03 * 0.04 ,
         'err_U235' : 0.01 * 0.04,
         'Th232': 0.010,
         'err_Th232' :  0.004,
        }
        
    if material == "Copper_FieldShaperRing_":
        param = {
        'ids' : "CU_FSW",
        'mass' :8.49816,#gr
        'U238' : 3.31,
        'err_U238' : 0,
        'Ra226' : 9.08e-02 ,
        'err_Ra226' : 0,
        'Co60': 4.32e-01,  
        'err_Co60' : 3.79e-02,
        'K40' : 5.58e-01,
        'err_K40' : 2.33e-01,
        'Cs137' :3.49e-02 ,
        'err_Cs137' : 0,
        'Th228' : 3.61e-02,
        'err_Th228' :0,
        'U235' : 0.17,
        'err_U235' : 0  ,
        'Th232': 3.61e-02,
        'err_Th232' : 0,
        }
        
    if material == "Copper_BottomPmtPlate":
        param = {
        'ids' : "1t MC paper",
        'mass' : 84.9712 + 53.39539,#gr
        'U238' : 1.2,
        'err_U238' : 0,
        'Ra226' : 0.033 ,
        'err_Ra226' : 0,
        'Co60': 0.1,
        'err_Co60' : 0.01,
        'K40' : 0.28,
        'err_K40' : 0,
        'Cs137' : 0.016,
        'err_Cs137' : 0,
        'Th228' : 0.034,
        'err_Th228' :0,
        'U235' : 0.55,
        'err_U235' : 0  , 
        'Th232': 0.043,
        'err_Th232' : 0,
        }
        
    return param
def calculate_rate(mass, contamination, err_contamination, m_fv, E):
    time = n_gen / (mass * contamination)
    rate = n_events/(time * m_fv * E)
    alpha = mass / (m_fv * E * n_gen)
    error_rate = alpha * sqrt((n_events * err_contamination)**2 + (contamination*sqrt(n_events))**2)

def fv(data):
    data = data[data['R']<607.34]
    data = data[data.Z_uc < -106.0]
    data = data[data.Z_uc > -1315.0]
    data.head(2)
    return data

def single_scatter(data):
    data = data[data.ns==1]
    return data

def energy(data, min, max):
    data = data[(data.Ed<max) & (data.Ed>min)]
    return data

def calculate_events(data, isotope):
    ss = single_scatter(data)
    fv_ = fv(ss)
    energy_ = energy(fv_, 1, 12)
    #en = select_typepri(energy_, isotope)
    #events_df = pd.DataFrame()
    #events_df = pd.concat([events_df, en])
    events_passing = len(energy_)
    if events_passing ==0:
        en_enlarged = energy(fv_, 1, 100)
        events_passing = (len(en_enlarged)*11)/99
        if events_passing == 0:
            events_passing = 2.3
    return events_passing

def get_xyz(rootfile):
    dataframe = []
    for df in read_root(rootfile, "events/events", chunksize=1000000,
                             columns= ["xpri", "ypri", "zpri", "epri", "ns", "X", "Y", "Z", "Ed"],
                            where="ns==1"
                            ):#, unit = "chunks"):
        x_values=[x[0] for x in df.X]
        y_values=[y[0] for y in df.X]
        z_values=[z[0] for z in df.X]
        df["X"]=x_values
        df["Y"]=y_values
        df["Z"]=z_values
        dataframe.append(df)
    
    dataframe=pd.concat(dataframe)
    dataframe.columns = ['xp', 'yp', 'zp_uc', "epri", "ns", "X", "Y", "Z_uc", "Ed"] #rename 
    offset = 1488/2
    dataframe['rp'] = np.sqrt(dataframe.xp**2+ dataframe.yp**2)
    dataframe['r2p'] = dataframe.rp*dataframe.rp
    dataframe['R'] = np.sqrt(dataframe.X**2+ dataframe.Y**2)
    dataframe['R2'] = (dataframe.R*dataframe.R)
    dataframe['Z'] = dataframe.Z_uc+offset
    dataframe['zp'] = dataframe.zp_uc+ offset
    return dataframe

def get_data(isotope, component, N):
    df = pd.DataFrame()
    if component == "OuterCryostatShell":
        DATE = "XENONnT_20191202"
        material = "SS_OuterCryostat" 
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1, flange2, flange3, shell, elongation = divide_outercryo(data)
        fraction_shell, fraction_elong, fraction_flanges = outer_cryo_numbers(N)
        df = shell
        frac = fraction_shell
    elif component == "OuterCryostat_Elongation":
        DATE = "XENONnT_20191202"
        material = "SS_OuterCryostat"
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1, flange2, flange3, shell, elongation = divide_outercryo(data)
        df = elongation
        fraction_shell, fraction_elong, fraction_flanges = outer_cryo_numbers(N)
        frac = fraction_elong 
    elif component == "OuterCryostat_flanges":
        DATE = "XENONnT_20191202"
        material = "SS_OuterCryostat"
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1, flange2, flange3, shell, elongation = divide_outercryo(data)
        flanges = pd.concat([flange1, flange2, flange3])
        df = flanges
        fraction_shell, fraction_elong, fraction_flanges = outer_cryo_numbers(N)
        frac = fraction_flanges
    elif component == "SS_InnerCryostatShell":
        material = "SS_InnerCryostat" 
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1_in, flange2_in, flange3_in, shell_in, elongation_in = divide_innercryo(data)
        flanges_in = pd.concat([flange1_in, flange2_in, flange3_in])
        fraction_shell, fraction_elong, fraction_flanges = inner_cryo_numbers(N)
        df = shell_in
        frac = fraction_shell
    elif component == "SS_InnerCryostatFlanges":
        material = "SS_InnerCryostat" 
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1_in, flange2_in, flange3_in, shell_in, elongation_in = divide_innercryo(data)
        flanges_in = pd.concat([flange1_in, flange2_in, flange3_in])
        fraction_shell, fraction_elong, fraction_flanges = inner_cryo_numbers(N)
        df = flanges_in
        frac = fraction_flanges
    elif component == "SS_InnerCryostat_bottomDome":
        material = "SS_InnerCryostat" 
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        flange1_in, flange2_in, flange3_in, shell_in, elongation_in = divide_innercryo(data)
        flanges_in = pd.concat([flange1_in, flange2_in, flange3_in])
        fraction_shell, fraction_elong, fraction_flanges = inner_cryo_numbers(N)
        frac = fraction_elong
        df = elongation_in
    elif component == "SS_BellSideWall":
        material = 'SS_BellSideWall'
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        bottomLip, bellWall = divide_bellWall(data)
        fraction_side, fraction_lip = bell_wall_numbers(N)
        frac = fraction_side
        df = bellWall
    elif component == "SS_BellSideWallBottomLip":
        material = 'SS_BellSideWall'
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        bottomLip, bellWall = divide_bellWall(data)
        df = bottomLip 
        fraction_side, fraction_lip = bell_wall_numbers(N)
        frac = fraction_lip
    #elif component == "SS_Electrodes":
    #    material = 'SS_AnodeRing'
    #    rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
    #    print(rootfile)
    #    data = get_xyz(rootfile)
    #    frac = 1
    elif component == "Top_PTFE_Frame":
        material = 'SS_Teflon_TopElectrodesFrame'
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        data = get_xyz(rootfile)
        frac = 1
    elif component ==  "Teflon_TPC_Sliding":
        DATE = "XENONnT_20191202"
        material = "Teflon_TPC"
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        df = get_xyz(rootfile)
        frac = 1
    elif component ==  "Teflon_TPC_Fixed":
        DATE = "XENONnT_20191202"
        material = "Teflon_TPC"
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        df = get_xyz(rootfile)
        frac = 1
    else:
        material = component
        rootfile = '/dali/lgrandi/xenonnt/simulations/er_simulations/final_files' + '/'+ material + '/' + isotope + '/' + 'output_'+ material + '_' + isotope +'_FINAL_Sort' + '.root'
        df = get_xyz(rootfile)
        frac = 1
    return df, frac



def do(component, m_fv): 
   
    conv = 3.15*1e7
    rate_array = []
    error_array = []
    i = 0
    flag = 0
    total_str = ""  
    E = 11
    time_array = []
    
    for i in range(0, len(isotope_array)):
        
        isotope = isotope_array[i]
        data, frac = get_data(isotope, component, N)
        
        #n = select_typepri(data, isotope)-------> in case other type pri function 
        b = select_typepri(data, isotope) #later on with the good function
        n =  N*(frac/100)*(1/b) #n_gen * fraction in that component * chain_factor
        n_events = calculate_events(data, isotope)
        p = get_parameters(component)
        mass = p['mass']
        contamination = p[isotope] 
        err = "err_"+isotope
        err = str(err)
        error_cont = p[err]/1000
        
        if (contamination == 0):
            rate = 0
            error_rate = 0
            str_ = ( "| no contamination value ")
            alpha = 0
            time = 0

        else:
            contamination = p[isotope]/1000 
            time = n / (mass * contamination)
            rate = n_events/(time * m_fv ) #*E  #ev/s*t
            alpha = mass / (m_fv * n) #correct with mass*b/(m_fv*E*N_gen)  ##no energy  
            err = "err_"+isotope
            err = str(err) 
            error_rate = alpha * np.sqrt((n_events * error_cont)**2 + (contamination*np.sqrt(n_events))**2)
            if n_events == 2.3: 
                str_ = ("|<%4.4f" %(rate*conv))
                flag = flag +1
            else:
                str_ = ("|(%4.4f +- %4.4f)" %(rate*conv, error_rate*conv))
            
        rate_array = np.append(rate_array, rate)
        error_array = np.append(error_array, error_rate) 
        total_str = total_str  + str_ 
        
        if time == 0:
            time_array = time_array = np.append(time_array, time)  
        else:
            time_array = time_array = np.append(time_array, time/conv)
        
        i = i+1 
        tot_rate = 0
        tot_error_rate = 0
        if len(rate_array)!=0:
            for i in range(0, len(rate_array)):
                tot_rate = tot_rate + rate_array[i]
                tot_error_rate = tot_error_rate + error_array[i]
                if flag == 0:
                    total_rate_string = ("|(**%4.4f +- %4.4f)" %(tot_rate*conv, tot_error_rate*conv)+ "**|" )
                else:
                    total_rate_string = ("|**<%4.4f " %(tot_rate*conv)+ "**|") 
        else:
            total_rate_string = "|"        
    print("|" + component + total_str + total_rate_string)
    return tot_rate*conv, time_array
    

