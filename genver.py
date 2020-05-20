import os, sys
import posixpath

# version definitions
major: int = 0
minor: int = 0

# file paths
buildnumber_path = posixpath.join(sys.argv[1], ".buildnumber")
versionh_path = posixpath.join(sys.argv[1], "main", "config.h")
num: int = 0

# check if file exists, if not create it
if not posixpath.exists(buildnumber_path):
	with open(buildnumber_path, "xt") as f:
		f.write("0")

# open file, increment build number
with open(buildnumber_path, "r+") as f:
	num_str = f.read()
	num = int(num_str)
	num += 1
	f.seek(0, 0)
	f.write(str(num))

# make version string from build number
version = "%d.%d.%d" % (major, minor, num)
	
# write header file
with open(versionh_path, "w+") as f:
	f.write("#ifndef CONFIG_H\n")
	f.write("#define CONFIG_H\n")
	f.write("#define BUILD_NUMBER %d\n" % num)
	f.write("#define VERSION_NUMBER \"%s\"\n" % version)
	f.write("#endif\n")