import time
import sys
if len(sys.argv) < 2:
    sys.exit(1)
    
try:
    seconds = int(sys.argv[1])
except ValueError:
    sys.exit(1)
    
time.sleep(seconds)