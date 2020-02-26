import subprocess
import os

def cmake(cwd, targets, options=[]):
    configcmd = ["cmake"]
    for option in options:
        configcmd.extend(["-D", option])
    configcmd.append(os.getcwd())
    subprocess.run(configcmd, cwd=cwd, check=True)

    buildcmd = ["cmake", "--build", ".", "--parallel"]
    for target in targets:
        buildcmd.extend(["--target", target])
    subprocess.run(buildcmd, cwd=cwd, check=True)
