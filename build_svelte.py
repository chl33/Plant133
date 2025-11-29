import os
import subprocess
from SCons.Script import Import

# Import the build environment from PlatformIO/SCons
Import("env")

def build_svelte():
    print("Building Svelte interface...")
    
    # Get the project root directory
    project_dir = env.subst("$PROJECT_DIR")
    script_path = os.path.join(project_dir, "build-svelte.sh")
    
    # Ensure the script is executable
    # This enables the OS to run it directly via the shebang (#!/bin/sh)
    os.chmod(script_path, 0o755)
    
    # Execute the shell script directly (shell=False)
    # We pass the full path as the executable.
    try:
        subprocess.run([script_path], cwd=project_dir, check=True)
    except subprocess.CalledProcessError:
        print("Error: Svelte build script returned non-zero exit code!")
        env.Exit(1)
    except OSError as e:
        print(f"Error: Could not execute build script at {script_path}")
        print(f"Details: {e}")
        env.Exit(1)

build_svelte()
