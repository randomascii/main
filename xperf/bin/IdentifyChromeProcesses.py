import sys
import os
import re

if len(sys.argv) < 2:
  print "Usage: %s tracename" % sys.argv[0]
  sys.exit(0)

# Find the space-terminated word after 'type='
pidRe = re.compile(r".*\(([\d ]*)\),.*")
processTypeRe = re.compile(r".*.exe\" --type=([^ ]*) .*")

tracename = sys.argv[1]
command = 'xperf -i "%s" -tle -tti -a process -withcmdline' % tracename
# Note the importance of printing the '\r' so that the
# output will be compatible with Windows edit controls.
print "Chrome PIDs by process type:\r"
pidsByType = {}
for line in os.popen(command).readlines():
  if line.count("chrome.exe") > 0:
    match = processTypeRe.match(line)
    type = "browser"
    if match:
      type = match.groups()[0]
    pid = int(pidRe.match(line).groups()[0])
    pidList = pidsByType.get(type, [])
    pidList.append(pid)
    pidsByType[type] = pidList

keys = pidsByType.keys()
keys.sort()
for type in keys:
  print "%-10s:" % type,
  for pid in pidsByType[type]:
    print "%d" % pid,
  print "\r"
