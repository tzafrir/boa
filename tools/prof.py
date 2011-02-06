#!/usr/bin/python
import sys

profInfoByMethod = dict()

for line in sys.stdin.readlines():
  vals = line.split("\t")
  time = vals[1]
  method = vals[2]
  comment = vals[3].split("\n")[0]
  timeDict = { "time":time, "comment":comment }
  try:
    profInfoByMethod[method].append(timeDict)
  except KeyError:
    profInfoByMethod[method] = [ timeDict ]

for method in profInfoByMethod.keys():
  print method
  for timeDict in profInfoByMethod[method]:
    print timeDict["time"] + " " + timeDict["comment"]
