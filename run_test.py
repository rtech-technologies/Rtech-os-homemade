import subprocess
import time

p = subprocess.Popen(["./tests/test_rsl_compiler"])
time.sleep(1)
p.kill()
