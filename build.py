import os
import subprocess
import time

files = ["main.cpp"
        ]


compiler_path = "g++"
compiler_flags =   ["-O0", 
                    "-g3", 
                    "-Wall", 
                    "-std=c++20", 
                    "-iquote", 
                    "Lib"
                    ]

linker_path = "g++"
linker_flags = ["-ludev", 
                "-lpthread"
                ]

def compile(input_file:str, output_file:str) -> int:
    ''' returns 1 if error, 0 if ok '''
    print(". ", end='', flush=True)
    return subprocess.call(f'{compiler_path} {" ".join(compiler_flags)} {input_file} -o {output_file}',shell=True)


def compile_all():
    for item in files:
        f_name = item.split("/")[-1]
        f_type = item.split(".")[-1]
        if f_type == "cpp":
            if compile(item, f'temp_files/{f_name.split(".")[0]}.o') == 1:
                return 1
    return 0

def link_all():
    items = ''
    for item in files:
        f_name = item.split("/")[-1]
        items += f'temp_files/{f_name.split(".")[0]}.o '
    return subprocess.call(f'{compiler_path} {" ".join(linker_flags)} {items} -o temp_files/main.elf', shell=True)


# def convert_elf():
#     subprocess.call(f'{objcopy_path} -O ihex temp_files/main.elf temp_files/main.hex', shell=True)
#     subprocess.call(f'{objcopy_path} -O binary temp_files/main.elf firmware.bin', shell=True)


def main():
    print("Compilation: ", end='', flush=True)
    if not os.path.isdir('temp_files'):
        subprocess.call('mkdir temp_files', shell=True)
    if 0 == compile_all():
        print("complete!")
        if 0 == link_all():
            # convert_elf()
            # subprocess.call(f'{common_path}arm-none-eabi-objdump --disassemble temp_files/main.elf > temp_files/main.list', shell=True)
            subprocess.call('rm -r temp_files/*', shell=True)
    # subprocess.call(f'{stlink_path}/./pystlink flash:erase:verify:gd32f10x/build_BluePill/Blink_BluePill.bin', shell=True)
    # subprocess.call(f'{stlink_path}/./pystlink flash:erase:verify:firmware.bin', shell=True)
            # subprocess.call(f'{common_path}arm-none-eabi-size.exe main.elf', shell=True)


if __name__ == '__main__':
    main()