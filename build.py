import os
import subprocess
import time
import glob

additional_files = []

def get_files(path:str)->list:
    result = []
    for x in os.walk(path):
        for y in glob.glob(os.path.join(x[0], '*.cpp')):
            result.append(y[len(path)+1:])
    # print(',\n'.join(result))
    return(result)


compiler_path = "g++"
compiler_flags =   ["-O0", 
                    "-g3", 
                    "-Wall", 
                    "-std=c++17", 
                    "-iquote", 
                    "Lib",
                    ]

linker_path = "g++"
linker_flags = ["-ludev", 
                "-lpthread",
                "-lusb-1.0",    # !important!
                ]

def compile(input_file:str, output_file:str) -> int:
    ''' returns 1 if error, 0 if ok '''
    print(". ", end='', flush=True)
    c_str = f'{compiler_path} {" ".join(compiler_flags)} -c -o {output_file} {input_file}'
    # print(c_str)
    return subprocess.call(c_str,shell=True)


def compile_all(files:list):
    for item in files:
        f_name = item.split("/")[-1]
        f_type = item.split(".")[-1]
        if f_type == "cpp":
            if compile(item, f'temp_files/{f_name.split(".")[0]}.o') == 1:
                return 1
    return 0

def link_all(files:list):
    items = ''
    for item in files:
        f_name = item.split("/")[-1]
        items += f'temp_files/{f_name.split(".")[0]}.o '
    c_str = f'{compiler_path} {" ".join(compiler_flags)} {items} -o main {" ".join(linker_flags)}'
    # print(c_str)
    return subprocess.call(c_str, shell=True)

# g++ -O0 -g3 -Wall -std=c++17 -iquote Lib main.cpp -c -o temp_files/main.o
# g++ -O0 -g3 -Wall -std=c++17 -iquote Lib -ludev -lpthread temp_files/main.o  -o main


# def convert_elf():
#     subprocess.call(f'{objcopy_path} -O ihex temp_files/main.elf temp_files/main.hex', shell=True)
#     subprocess.call(f'{objcopy_path} -O binary temp_files/main.elf firmware.bin', shell=True)


def main(files:list):
    print("Compilation: ", end='', flush=True)
    if not os.path.isdir('temp_files'):
        subprocess.call('mkdir temp_files', shell=True)
    if 0 == compile_all(files):
        print("complete!")
        if 0 == link_all(files):
            # convert_elf()
            # subprocess.call(f'{common_path}arm-none-eabi-objdump --disassemble temp_files/main.elf > temp_files/main.list', shell=True)
            subprocess.call('rm -r temp_files/*', shell=True)
    # subprocess.call(f'{stlink_path}/./pystlink flash:erase:verify:gd32f10x/build_BluePill/Blink_BluePill.bin', shell=True)
    # subprocess.call(f'{stlink_path}/./pystlink flash:erase:verify:firmware.bin', shell=True)
            # subprocess.call(f'{common_path}arm-none-eabi-size.exe main.elf', shell=True)


if __name__ == '__main__':
    files = []
    for i in additional_files:
        files.append(i)
    for i in get_files(os.path.abspath(os.getcwd())):
        if not i in additional_files:
            files.append(i)

    # print(files)
    main(files)
