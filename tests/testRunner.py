#!/usr/bin/python

import sys
from subprocess import *

clangExecutable = '../llvm/Debug+Asserts/bin/clang'
boaPlugin = 'build/boa.so'
testsFolder = 'tests/testcases/'
assertsSuffix = '.asserts'

separator = '---'

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
  sys.stderr.write("FAILED(" + str(lineNumber) + "): " + failedLine + "\n")

def runTest(testName):
  """\
Runs BOA over testsFolder/testName.c, returns a list of tuples in the form
(bufferName, bufferLocation) 
"""
  results = list()
  p = Popen([clangExecutable, '-cc1', '-load', boaPlugin, '-plugin',
       'boa', testsFolder + testName + '.c'], stdout=PIPE, stderr=PIPE, stdin=None)
  separatorFound = False
  for lineWithBreak in p.stderr.readlines():
    line = lineWithBreak.split("\n")[0]
    if (not separatorFound) and (line != separator):
      continue
    if (separatorFound and line == separator):
      break
    if (line == separator):
      separatorFound = True
      continue
    # We're in the relevant part of stderr
    values = line.split(" ")
    t = tuple((values[0], values[1]))
    results.append(t)
  return results

def applyAssertions(testName, testOutput):
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
"""
  # TODO: Handle invalid assertion files.
  a = open(testsFolder + testName + assertsSuffix, 'r')
  # enum
  BYNAME, BYLOCATION, BYBOTH = range(3)
  lineNumber = 0
  result = True
  for lineWithBreak in a.readlines():
    line = lineWithBreak.split("\n")[0]
    lineNumber += 1
    if line == "":
      continue
    values = line.split(" ")
    if values[0] == "HAS":
      isHasAssertion = True
    else:
      isHasAssertion = False
    assertionTypeValue = values[1]
    if assertionTypeValue == "ByName":
      assertionType = BYNAME
    elif assertionTypeValue == "ByLocation":
      assertionType = BYLOCATION
    else:
      assertionType = BYBOTH
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

def main():
  if len(sys.argv) < 2:
    sys.stderr.write("Usage: %s [testName]*\n" % sys.argv[0])
    sys.stderr.write("    For each testcase, runs boa on testsFolder/testName.c and checks the assertions " +
                     "in testsFolder/testName.asserts\n")
  sys.argv.pop(0)
  for test in sys.argv:
    testOutput = runTest(test)
    val = applyAssertions(test, testOutput)
    return val
    

if __name__ == "__main__":
    if main():
      exit(0)
    else:
      exit(1)
