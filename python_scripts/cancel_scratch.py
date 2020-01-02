from pexpect import pxssh

list_pcs= ['bcfr05', 'fsfr01', 'bcfr10', 'bcfr07', 'bcfr08', 'bcfr02', 'bcfr03', 'bcfr17', 'pcfr87' ]
date = '2019-10-16'
command = 'cd /scratch/arocchetti/'+ date +'/;rm *_GEN; rm *_DET; rm *_PHYS;  rm *.root; ls'

for pc in list_pcs:

    print("working on:", pc)
    s = pxssh.pxssh(encoding='utf-8') 

    if not s.login(pc, 'arocchetti', 'darkMatter47'):

        print ("SSH session failed on login.")
        print (str(s))
    
    else:
        
        print ("SSH session login successful ", pc) 
        #s.sendline('cd /scratch/arocchetti/2019-10-16/;rm *_GEN; rm *_.root; ls')
        s.sendline(command)
        s.prompt()
        print(s.before)
        s.logout()
        print("cleared scratch for :", pc)
