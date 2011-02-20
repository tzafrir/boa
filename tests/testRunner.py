#!/usr/bin/python

import sys
from subprocess import *

boaExecutable = './boa'
assertsSuffix = '.asserts'

separator = '---'
redColor = '\033[0;31m'
noColor = '\033[0m'

errs = sys.stderr

def listContains(aList, member):
  if aList.count(member) > 0:
    return True
  return False

def dismantleListOfTuples(aList, n):
  """\
Receives a list of tuples, returns a list of all n'th elements in the list.
"""
  result = list()
  for aTuple in aList:
    result.append(aTuple[n])
  return result

def printFailure(failedLine, lineNumber):
  errs.write("FAILED(" + str(lineNumber) + "): " + failedLine + "\n")

def runTest(testName, testForBlame):
  """\
Runs BOA over testName.c, returns a list of tuples in the form
(bufferName, bufferLocation) 
"""
  results = list()
  blames = list()
  blameFlag = ""
  if (testForBlame):
    blameFlag = "-blame"
  p = Popen([boaExecutable, blameFlag, testName + '.c'], stdout=PIPE, stderr=PIPE, stdin=None)
  separatorCount = 0
  for lineWithBreak in p.stderr.readlines():
    line = lineWithBreak.split("\n")[0]
    if (separatorCount < 1) and (line != separator):
      continue
    if (separatorCount == 1 and line != separator):
      blames.append(line)
      continue
    if (separatorCount == 3 and line == separator):
      break
    if (line == separator):
      separatorCount += 1
      continue
    # We're in the relevant part of stderr
    values = line.split(" ")
    t = tuple((values[0], values[1]))
    results.append(t)
  return results, blames

def applyAssertions(testName, testOutput, blamesDict):
  """\
testOutput is a list of tuples in the form (bufferName, bufferLocation).

A file named tests/testcases/testName.asserts is expected to have assertions in the following form:

[HAS/NOT] [ByName/ByLocation/ByBoth] [buffer name / buffer location] [buffer location (when using both)]

Examples:

HAS ByName buffer
NOT ByLocation file.c:82
HAS ByBoth mychars_ chartest.c:15

This method returns True iff all assertions pass.
This method has a side effect of printing all failing assertions to stderr.

blames is a list of tuples in the form (bufName, line), where line is a blame line attached to the
buffer bufName.

"""
  assertFilename = testName + assertsSuffix
  try:
    a = open(assertFilename, 'r')
  except IOError:
    errs.write(redColor + "ERROR" + noColor + ": Unable to open " + assertFilename + "\n")
    return False
  # enum
  BYNAME, BYLOCATION, BYBOTH = range(3)
  lineNumber = 0
  result = True
  for lineWithBreak in a.readlines():
    line = lineWithBreak.split("\n")[0]
    lineNumber += 1

    values = line.split(" ")
    firstWord = values[0]
    if firstWord == "HAS":
      isHasAssertion = True
    elif firstWord == "NOT":
      isHasAssertion = False
    elif firstWord == "BLAME":
      bufName = values[1]
      # Remove first two words. Rebuild a string for the rest of the line.
      values.reverse()
      values.pop()
      values.pop()
      str = ""
      while (len(values) > 0):
        str += values.pop()
        if (len(values) > 0):
          str += " "

      # Assert that the output contains the line.
      foundMatch = False
      try:
        for output in blamesDict[bufName]:
          if output.__contains__(str):
            foundMatch |= True
        if not foundMatch:
          printFailure(line, lineNumber)
          result &= False
      except KeyError:
        errs.write("Blamed buffer missing in output: " + bufName)
      continue
    elif firstWord == "" or firstWord[0] == '#':
      # Comment line.
      continue
    else:
      errs.write("Invalid assertion in " + testName + assertsSuffix + ":" + str(lineNumber) + "\n")
      return False
    assertionTypeValue = values[1]
    if assertionTypeValue == "ByName":
      assertionType = BYNAME
    elif assertionTypeValue == "ByLocation":
      assertionType = BYLOCATION
    elif assertionTypeValue == "ByBoth":
      assertionType = BYBOTH
    else:
      errs.write("Invalid assertion type \"" + assertionTypeValue + "\" in line " +
          str(lineNumber) + "\n")
      exit(1)
    if assertionType == BYNAME:
      contains = listContains(dismantleListOfTuples(testOutput, 0), values[2])
    elif assertionType == BYLOCATION:
      contains = listContains(dismantleListOfTuples(testOutput, 1), values[2])
    else:
      contains = listContains(testOutput, (values[2], values[3]))
    if isHasAssertion:
      if not contains:
        printFailure(line, lineNumber)
        result &= False
    else:
      if contains:
        printFailure(line, lineNumber)
        result &= False
  return result

def parseBlames(blames):
  """
Backwards logic for parsing input of the form:
    bufname buflocation
       blame reason 1
       ...
       blame reason 2
    otherbuf otherloc
       other reasons

To the form:
{"bufname":["   blame reason 1", ..., "   blame reason 2"], "otherbuf":["   other reasons"]}
"""
  result = dict()
  for line in blames:
    firstWord = line.split(" ")[0]
    if (firstWord != ""):
      curBuf = firstWord
    else:
      try:
        result[curBuf].append(line)
      except KeyError:
        result[curBuf] = [ line ]
  return result

def main():
  if len(sys.argv) < 2:
    errs.write("Usage: %s [testName]*\n" % sys.argv[0])
    errs.write("    For each testcase, runs boa on testName.c and checks the assertions " +
                     "in testName.asserts\n")
    exit(1)
  for arg in sys.argv:
    if (arg == '-blame'):
      testForBlame = True
    else:
      testName = arg
  testOutput, blames = runTest(testName, testForBlame)
  blamesDict = parseBlames(blames)
  val = applyAssertions(testName, testOutput, blamesDict)
  return val
    

if __name__ == "__main__":
    if main():
      exit(0)
    else:
      exit(1)
