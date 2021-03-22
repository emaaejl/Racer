import json
import os
import sys
#Assume
PATH_TO_RACER=os.environ["RACER_PATH"]
def compile_command_to_racer_script(compile_command_path, racer_runnerscript_outpath = ""):
    if not PATH_TO_RACER:
        exit("Error: Please set evironment variable 'RACER_PATH' to point to racer binary")
    #Parse compile-command (which is json)
    with open(racer_runnerscript_outpath, "w") as outfile:
        outfile.write(f"{PATH_TO_RACER} --cg -p={compile_command_path} \\\n")
        with open(compile_command_path, "r") as data:
            comp_commands = json.load(data)
            for c_com in comp_commands:
                outfile.write(f'{c_com["file"]} \\\n')

if __name__ == "__main__":
    if(not sys.argv[2]):
        exit(f"Usage: {sys.argv[0]} <compile_command_source_file> <runner_script_result>")
    compile_command_to_racer_script(sys.argv[1], sys.argv[2])