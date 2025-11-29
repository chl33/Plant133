try:
    import os
    import subprocess
    from SCons.Script import Import

    Import("env")

    def build_svelte():
        print("Building Svelte interface...")
        project_dir = env.subst("$PROJECT_DIR")
        script_path = os.path.join(project_dir, "build-svelte.sh")
        
        # Ensure the script is executable
        os.chmod(script_path, 0o755)
        
        # Execute the shell script
        result = subprocess.run([script_path], shell=True, cwd=project_dir)
        
        if result.returncode != 0:
            print("Error building Svelte interface!")
            env.Exit(1)

    # Execute immediately when the script is loaded.
    # This ensures the Svelte build runs and updates the header file BEFORE 
    # SCons calculates dependencies or starts compiling C++ source files.
    # This fixes the issue where the firmware build happened before the Svelte build.
    build_svelte()

except ImportError:
    pass
