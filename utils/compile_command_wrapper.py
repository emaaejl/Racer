import json
import os
import sys
import subprocess
#Require racer path to be defined
PATH_TO_RACER=os.environ["RACER_PATH"]
def compile_command_to_racer_script(compile_command_path, racer_runnerscript_outpath = ""):
    if not PATH_TO_RACER:
        exit("Error: Please set evironment variable 'RACER_PATH' to point to racer binary")
    filesToCheck = ""
    with open(compile_command_path, "r") as data:
        comp_commands = json.load(data)
        #Extract the files built
        comp_command_files = list(map(lambda e: f'"{e["file"]}"', comp_commands))
        filesToCheck = " \\\n".join([f for f in comp_command_files])
    if(not filesToCheck):
        exit("No files found in compilation database. Exiting ...")
    #Parse compile-command (which is json)
    with open(racer_runnerscript_outpath, "w") as outfile:
        #Run racer with timing
        outfile.write(f"/usr/bin/time -v {PATH_TO_RACER} --cg -p={os.path.split(compile_command_path)[0]} \\\n")
        outfile.write(f"{filesToCheck}\n")
    with open(os.path.splitext(racer_runnerscript_outpath)[0] + "_metrics.sh", "w") as metrics_out:
        #Run counting tool for LoC count, if such a tool exists
        if ( "LOC_COUNTER_PATH" in os.environ):      
            CODELINE_COUNTER = os.environ["LOC_COUNTER_PATH"]  
            metrics_out.write(f"\n{CODELINE_COUNTER} \\\n")
            metrics_out.write(f"{filesToCheck}\n")
        else:
            print("Note: LoC counter runner not generated.\nTo do so, set environment variable 'LOC_COUNTER_PATH' to preferred tool.")
        if("COMPLEXITY_TOOL" in os.environ):
            COMPLEXITY_TOOL = os.environ["COMPLEXITY_TOOL"]
            metrics_out.write(f"\n{COMPLEXITY_TOOL} \\\n")
            metrics_out.write(f"{filesToCheck}\n")
        else:
            print("Note: complexity metrics runner not generated.\nTo do so, set environment variable 'COMPLEXITY_TOOL' to preferred tool.")
    return racer_runnerscript_outpath

if __name__ == "__main__":
    if(not sys.argv[2]):
        exit(f"Usage: {sys.argv[0]} <compile_command_source_file> <runner_script_result>")
    script = compile_command_to_racer_script(sys.argv[1], sys.argv[2])