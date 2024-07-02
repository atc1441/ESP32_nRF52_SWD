from os import path
from secrets import token_hex 

store='secrets.h'
pwd_len = 8

if not path.isfile(store):
    with open(store,'w') as f:
        print("#define AP_PASSWORD \"{}\"".format(token_hex(pwd_len)), file=f)
        f.close()
        print('generated', store)

with open(store,'r')as f:
    pwd = f.read()
    print(store + ':')
    print(pwd)

