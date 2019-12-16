from random import randint
import subprocess
import os
import sys
from time import time

class Executable:
    def __init__(self, name):
        self.name = name
        self.passed = 0
        self.failed = 0
        self.totalTime = 0.0

    def execute(self, input):
        p = subprocess.Popen(['./'+self.name], stdout=subprocess.PIPE, stdin=subprocess.PIPE)        
        outs, _ = p.communicate(input)
        return outs

    def test(self, input):
        startTime = time()
        outs = self.execute(input)
        self.totalTime += time() - startTime
        try:
            return outs.decode("utf-8").split('\n')[-2].split(': ')[1].split(' , ')
        except:
            print("Unexpected Result:")
            print(outs.decode("utf-8"))
            print("Program terminated unexpectedly!")
            sys.exit(0)
    
    def time(self):
        return int(self.totalTime*1000)

def calcDistance(coord1, coord2):
    a = pow( pow(coord1[0]-coord2[0], 2) + pow(coord1[1] - coord2[1], 2), 1.0/2.0)
    #print("{0}:{1}, {2}:{3}, {4}".format(coord1[0],coord1[1],coord2[0], coord2[1], a))
    return a

def findClosestPairs(coords):
    closestPairs = []
    closestDistance = False
    for i in range(len(coords)):
        for j in range(i,len(coords)):
            if i != j:
                distance = calcDistance(coords[i], coords[j])
                if closestDistance is False or distance <= closestDistance:
                    if distance == closestDistance:
                        closestPairs.append([i, j])
                    else:
                        closestPairs = [[i,j]]
                    closestDistance = distance
    return closestPairs

def createTestSet(count = False):
    existings = []
    if count  == False:
        count = randint(30,300)
    coords = []
    for i in range(count):
        randNokta = [randint(1,80), randint(1,80)]
        while str(randNokta[0])+':'+str(randNokta[1]) in existings:
            randNokta = [randint(1,80), randint(1,80)]
        existings.append(str(randNokta[0])+':'+str(randNokta[1]))
        coords.append(randNokta)
    return coords

executables = []

if len(sys.argv) < 3:
    print("Provide name of source file as first parameter and names of source files as preeciding parameters!")
    sys.exit(0)
testCount = int(sys.argv[1])

for i in range(2, len(sys.argv)):
    name = sys.argv[i].strip('.c')
    p = subprocess.Popen(["gcc", name+'.c', "-o", name, "-lm"], stdout=subprocess.PIPE, stdin=subprocess.PIPE)
    p.wait()
    executables.append(Executable(name))

for i in range(testCount):
    testSet = createTestSet()
    closestPairs = findClosestPairs(testSet)

    input = b'1\n'+str(len(testSet)).encode()+b'\n'
    for i in range(len(testSet)):
        input += str(testSet[i][0]).encode() + b':' + str(testSet[i][1]).encode() + b'\n'

    for e in range(len(executables)):
        executable = executables[e]
        result = executable.test(input)
        match = False
        expecteds = ""
        for i in range(len(closestPairs)):
            closestPair = closestPairs[i]
            c1 = "{0}:{1}".format(testSet[closestPair[0]][0], testSet[closestPair[0]][1])
            c2 = "{0}:{1}".format(testSet[closestPair[1]][0], testSet[closestPair[1]][1])
            if expecteds != '':
                expecteds += ' or '
            expecteds += "{0} {1}".format(c1,c2)
            if c1 in result and c2 in result:
                match = True
        if match:
            executable.passed += 1
        else:
            executable.failed += 1
            print("Expected: ", expecteds, "; Got:", result[0], result[1], '({0})'.format(executable.name))

for e in range(len(executables)):
    executable = executables[e]
    failed = executable.failed
    passed = executable.passed
    time = executable.time()
    print(executable.name +" took {0}ms; total {1} test run, {2} passed, {3} failed.".format(time, failed+passed, passed, failed))